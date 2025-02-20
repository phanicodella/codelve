// E:\codelve\src\core\engine.cpp
#include "engine.h"
#include "context_manager.h"
#include "query_processor.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include "../scanner/scanner.h"
#include "../llm/llm_interface.h"
#include "../ui/main_window.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace codelve {
namespace core {

Engine::Engine(const std::string& configPath)
    : configPath_(configPath) {
    
    // Create configuration
    config_ = std::make_shared<utils::Config>();
    
    // Set initial status
    statusMessage_ = "Initializing...";
    
    utils::Logger::log(utils::LogLevel::INFO, "Engine: Created with config path: " + configPath);
}

Engine::~Engine() {
    utils::Logger::log(utils::LogLevel::INFO, "Engine: Destroyed");
}

bool Engine::initialize() {
    try {
        // Load configuration
        if (!configPath_.empty() && fs::exists(configPath_)) {
            if (!config_->loadFromFile(configPath_)) {
                utils::Logger::log(utils::LogLevel::ERROR, "Engine: Failed to load config from: " + configPath_);
                // Continue with default config
            }
        }
        
        // Initialize logger
        std::string logLevel = config_->getString("log_level", "info");
        utils::Logger::setLogLevel(logLevel);
        
        // Setup components
        setupComponents();
        
        // Initialize UI
        if (mainWindow_ && !mainWindow_->initialize()) {
            utils::Logger::log(utils::LogLevel::ERROR, "Engine: Failed to initialize UI");
            return false;
        }
        
        // Initialize LLM interface if needed
        bool preloadModel = config_->getBool("llm.preload_model", false);
        if (preloadModel && llmInterface_) {
            utils::Logger::log(utils::LogLevel::INFO, "Engine: Preloading LLM model...");
            if (!llmInterface_->initialize()) {
                utils::Logger::log(utils::LogLevel::ERROR, "Engine: Failed to preload LLM model");
                setStatus("Failed to load language model", true);
                // Continue anyway, we'll try again when needed
            } else {
                utils::Logger::log(utils::LogLevel::INFO, "Engine: LLM model preloaded successfully");
                setStatus("Language model loaded successfully");
            }
        }
        
        setStatus("Ready");
        utils::Logger::log(utils::LogLevel::INFO, "Engine: Initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        utils::Logger::log(utils::LogLevel::ERROR, "Engine: Initialization error: " + std::string(e.what()));
        setStatus("Initialization error: " + std::string(e.what()), true);
        return false;
    }
}

int Engine::run() {
    if (!mainWindow_) {
        utils::Logger::log(utils::LogLevel::ERROR, "Engine: Cannot run, UI not initialized");
        return 1;
    }
    
    utils::Logger::log(utils::LogLevel::INFO, "Engine: Starting application");
    return mainWindow_->run();
}

bool Engine::loadCodebase(const std::string& directoryPath, ProgressCallback callback) {
    if (!scanner_) {
        utils::Logger::log(utils::LogLevel::ERROR, "Engine: Cannot load codebase, scanner not initialized");
        setStatus("Internal error: Scanner not initialized", true);
        return false;
    }
    
    if (directoryPath.empty() || !fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        utils::Logger::log(utils::LogLevel::ERROR, "Engine: Invalid directory path: " + directoryPath);
        setStatus("Invalid directory path: " + directoryPath, true);
        return false;
    }
    
    utils::Logger::log(utils::LogLevel::INFO, "Engine: Loading codebase from: " + directoryPath);
    setStatus("Loading codebase from: " + directoryPath);
    
    // Show progress dialog in UI
    HWND progressDialog = nullptr;
    if (mainWindow_) {
        progressDialog = mainWindow_->showProgressDialog("Loading Codebase", "Scanning files...");
    }
    
    // Set up progress callback for scanner
    scanner::Scanner::ScanProgressCallback scanCallback = 
        [this, callback, progressDialog](const std::string& stage, float progress, const std::string& message) {
            if (callback) {
                callback(stage, progress, message);
            }
            
            if (mainWindow_ && progressDialog) {
                mainWindow_->updateProgressDialog(progressDialog, progress, stage + ": " + message);
            }
            
            setStatus(stage + ": " + message);
        };
    
    scanner_->setProgressCallback(scanCallback);
    
    // Scan the directory in a new thread
    std::thread scanThread([this, directoryPath, progressDialog]() {
        try {
            // Scan the directory
            auto scannedCode = scanner_->scanDirectory(directoryPath);
            
            // Store the indexed code
            indexedCode_ = std::make_shared<scanner::IndexedCode>(scannedCode);
            
            // Initialize context manager with the indexed code
            if (contextManager_) {
                if (!contextManager_->initialize(scannedCode)) {
                    utils::Logger::log(utils::LogLevel::ERROR, "Engine: Failed to initialize context manager with indexed code");
                    setStatus("Failed to process indexed code", true);
                }
            }
            
            // Close progress dialog
            if (mainWindow_ && progressDialog) {
                mainWindow_->closeProgressDialog(progressDialog);
            }
            
            utils::Logger::log(utils::LogLevel::INFO, "Engine: Codebase loaded successfully");
            setStatus("Codebase loaded: " + std::to_string(scannedCode.fileCount) + " files");
            
        } catch (const std::exception& e) {
            utils::Logger::log(utils::LogLevel::ERROR, "Engine: Error loading codebase: " + std::string(e.what()));
            setStatus("Error loading codebase: " + std::string(e.what()), true);
            
            // Close progress dialog
            if (mainWindow_ && progressDialog) {
                mainWindow_->closeProgressDialog(progressDialog);
            }
        }
    });
    
    // Detach thread so it runs independently
    scanThread.detach();
    
    return true;
}

void Engine::processQuery(const std::string& query) {
    if (query.empty()) {
        return;
    }
    
    utils::Logger::log(utils::LogLevel::INFO, "Engine: Processing query: " + query);
    
    // Check for special commands
    if (query == "/exit" || query == "/quit") {
        if (mainWindow_) {
            PostMessage(mainWindow_->getHandle(), WM_CLOSE, 0, 0);
        }
        return;
    } else if (query == "/help") {
        displayResponse("# CodeLve Help\n\n"
                      "- Type any question about the codebase\n"
                      "- Use /clear to clear the chat history\n"
                      "- Use /exit or /quit to exit the application\n"
                      "- Use /help to show this help message");
        return;
    } else if (query == "/clear") {
        if (mainWindow_) {
            // This triggers the UI to clear the chat history
            displayResponse("!CLEAR!");
        }
        return;
    }
    
    // Ensure LLM is initialized
    if (llmInterface_ && !llmInterface_->isInitialized()) {
        setStatus("Initializing language model...");
        if (!llmInterface_->initialize()) {
            utils::Logger::log(utils::LogLevel::ERROR, "Engine: Failed to initialize LLM model for query");
            setStatus("Failed to load language model", true);
            displayResponse("Sorry, I couldn't load the language model. Please check the logs for details.");
            return;
        }
    }
    
    // Show typing indicator
    displayResponse("!TYPING!");
    
    // Process the query in a separate thread
    std::thread queryThread([this, query]() {
        try {
            // Process the query
            std::string formattedQuery;
            if (queryProcessor_) {
                formattedQuery = queryProcessor_->processQuery(query);
            } else {
                formattedQuery = query;
            }
            
            // Run inference
            std::string response;
            if (llmInterface_) {
                llm::InferenceParams params;
                params.temperature = config_->getFloat("llm.temperature", 0.7f);
                params.maxTokens = config_->getInt("llm.max_tokens", 1024);
                params.topP = config_->getFloat("llm.top_p", 0.95f);
                
                response = llmInterface_->runInference(formattedQuery, params);
            } else {
                response = "Language model not available.";
            }
            
            // Add to conversation history
            if (contextManager_) {
                contextManager_->addToHistory(query, response);
            }
            
            // Display response
            displayResponse(response);
            
            utils::Logger::log(utils::LogLevel::INFO, "Engine: Query processed successfully");
            
        } catch (const std::exception& e) {
            utils::Logger::log(utils::LogLevel::ERROR, "Engine: Error processing query: " + std::string(e.what()));
            displayResponse("Sorry, an error occurred while processing your query: " + std::string(e.what()));
        }
    });
    
    // Detach thread so it runs independently
    queryThread.detach();
}

std::string Engine::getStatus() const {
    return statusMessage_;
}

bool Engine::showFile(const std::string& filePath) {
    if (!indexedCode_ || !mainWindow_) {
        return false;
    }
    
    auto it = indexedCode_->files.find(filePath);
    if (it == indexedCode_->files.end()) {
        utils::Logger::log(utils::LogLevel::ERROR, "Engine: File not found in indexed code: " + filePath);
        setStatus("File not found: " + filePath, true);
        return false;
    }
    
    // Display the file content (assuming a method in MainWindow to display code)
    // This would typically involve showing the file in a code editor view
    
    utils::Logger::log(utils::LogLevel::INFO, "Engine: Showing file: " + filePath);
    setStatus("Viewing file: " + filePath);
    return true;
}

std::shared_ptr<utils::Config> Engine::getConfig() const {
    return config_;
}

void Engine::setupComponents() {
    // Create scanner
    scanner_ = std::make_shared<scanner::Scanner>(config_);
    
    // Create context manager
    contextManager_ = std::make_shared<ContextManager>(config_);
    
    // Create query processor
    queryProcessor_ = std::make_shared<QueryProcessor>(config_, contextManager_);
    
    // Create LLM interface
    llmInterface_ = std::make_shared<llm::LlmInterface>(config_);
    
    // Create main window
    mainWindow_ = std::make_shared<ui::MainWindow>(config_, shared_from_this());
    
    // Set up callbacks
    if (mainWindow_) {
        // Set query callback
        mainWindow_->setQueryCallback([this](const std::string& query) {
            processQuery(query);
        });
        
        // Set file selection callback
        mainWindow_->setFileSelectionCallback([this](const std::string& filePath) {
            handleFileSelection(filePath);
        });
    }
    
    utils::Logger::log(utils::LogLevel::INFO, "Engine: Components set up");
}

void Engine::handleFileSelection(const std::string& filePath) {
    if (filePath.empty()) {
        return;
    }
    
    if (fs::is_directory(filePath)) {
        // Load as codebase
        loadCodebase(filePath);
    } else if (fs::is_regular_file(filePath)) {
        // Show file
        showFile(filePath);
    }
}

void Engine::displayResponse(const std::string& response) {
    if (mainWindow_) {
        mainWindow_->displayResponse(response);
    }
}

void Engine::setStatus(const std::string& message, bool isError) {
    statusMessage_ = message;
    
    utils::Logger::log(isError ? utils::LogLevel::ERROR : utils::LogLevel::INFO, 
                     "Engine: Status - " + message);
    
    if (mainWindow_) {
        mainWindow_->setStatusMessage(message, isError);
    }
}

}} // namespace codelve::core