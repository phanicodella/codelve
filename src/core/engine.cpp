// E:\CodeLve\src\core\engine.cpp
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
            : configPath_(configPath),
            scanner_(nullptr),
            contextManager_(nullptr),
            queryProcessor_(nullptr),
            llmInterface_(nullptr),
            mainWindow_(nullptr),
            progressDialogHandle(nullptr) {  // Ensure progressDialogHandle is initialized

            // Create configuration
            config_ = std::make_shared<utils::Config>();

            // Set initial status
            statusMessage_ = "Initializing...";

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Created with config path: " + configPath_);
        }

        Engine::~Engine() {
            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Destroyed");
        }

        bool Engine::initialize() {
            try {
                if (!configPath_.empty() && fs::exists(configPath_)) {
                    if (!config_->loadFromFile(configPath_)) {
                        utils::Logger& logger = utils::Logger::getInstance();
                        logger.log(utils::LogLevel::ERROR, "Engine: Failed to load config from: " + configPath_);
                    }
                }

                std::string logLevel = config_->getString("log_level", "info");
                utils::Logger::setLogLevel(logLevel);

                setupComponents();  // Ensure all components are initialized

                if (mainWindow_ && !mainWindow_->initialize()) {
                    utils::Logger& logger = utils::Logger::getInstance();
                    logger.log(utils::LogLevel::ERROR, "Engine: Failed to initialize UI");
                    return false;
                }

                bool preloadModel = config_->getBool("llm.preload_model", false);
                if (preloadModel && llmInterface_) {
                    utils::Logger& logger = utils::Logger::getInstance();
                    logger.log(utils::LogLevel::INFO, "Engine: Preloading LLM model...");
                    if (!llmInterface_->initialize()) {
                        logger.log(utils::LogLevel::ERROR, "Engine: Failed to preload LLM model");
                        setStatus("Failed to load language model", true);
                    }
                    else {
                        logger.log(utils::LogLevel::INFO, "Engine: LLM model preloaded successfully");
                        setStatus("Language model loaded successfully");
                    }
                }

                setStatus("Ready");
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::INFO, "Engine: Initialized successfully");
                return true;
            }
            catch (const std::exception& e) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "Engine: Initialization error: " + std::string(e.what()));
                setStatus("Initialization error: " + std::string(e.what()), true);
                return false;
            }
        }

        int Engine::run() {
            if (!mainWindow_) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "Engine: No UI available to run");
                return 1;
            }

            try {
                return mainWindow_->run();
            }
            catch (const std::exception& e) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "Engine: Error during execution: " + std::string(e.what()));
                setStatus("Runtime error: " + std::string(e.what()), true);
                return 1;
            }
        }

        bool Engine::loadCodebase(const std::string& directoryPath, ProgressCallback callback) {
            if (!scanner_) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "Engine: Cannot load codebase, scanner not initialized");
                setStatus("Internal error: Scanner not initialized", true);
                return false;
            }

            if (directoryPath.empty() || !fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "Engine: Invalid directory path: " + directoryPath);
                setStatus("Invalid directory path: " + directoryPath, true);
                return false;
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Loading codebase from: " + directoryPath);
            setStatus("Loading codebase from: " + directoryPath);

            // Show progress dialog in UI
            // FIX: Remove the local variable that shadows the class member
            if (mainWindow_) {
                progressDialogHandle = mainWindow_->showProgressDialog("Loading Codebase", "Scanning files...");
            }

            auto scanCallback = [this, callback](const std::string& stage, float progress, const std::string& message) {
                if (callback) {
                    callback(stage, progress, message);
                }
                if (mainWindow_ && progressDialogHandle) {
                    mainWindow_->updateProgressDialog(progressDialogHandle, progress, stage + ": " + message);
                }
                setStatus(stage + ": " + message);
                };

            scanner_->setProgressCallback(scanCallback);

            // FIX: Use correct capture for the query variable
            std::thread([this, directoryPath]() {
                try {
                    auto scannedCode = scanner_->scanDirectory(directoryPath);

                    indexedCode_ = std::make_shared<scanner::IndexedCode>(scannedCode);

                    if (contextManager_) {
                        if (!contextManager_->initialize(scannedCode)) {
                            utils::Logger& logger = utils::Logger::getInstance();
                            logger.log(utils::LogLevel::ERROR, "Engine: Failed to initialize context manager with indexed code");
                            setStatus("Failed to process indexed code", true);
                        }
                    }

                    if (mainWindow_ && progressDialogHandle) {
                        mainWindow_->closeProgressDialog(progressDialogHandle);
                    }

                    utils::Logger& logger = utils::Logger::getInstance();
                    logger.log(utils::LogLevel::INFO, "Engine: Codebase loaded successfully");
                    setStatus("Codebase loaded successfully");

                }
                catch (const std::exception& e) {
                    utils::Logger& logger = utils::Logger::getInstance();
                    logger.log(utils::LogLevel::ERROR, "Engine: Error loading codebase: " + std::string(e.what()));
                    setStatus("Error loading codebase: " + std::string(e.what()), true);

                    if (mainWindow_ && progressDialogHandle) {
                        mainWindow_->closeProgressDialog(progressDialogHandle);
                    }
                }
                }).detach();

            return true;
        }

        void Engine::setupComponents() {
            scanner_ = std::make_shared<scanner::Scanner>(config_);
            contextManager_ = std::make_shared<ContextManager>(config_);
            queryProcessor_ = std::make_shared<QueryProcessor>(config_, contextManager_);
            llmInterface_ = std::make_shared<llm::LlmInterface>(config_);
            mainWindow_ = std::make_shared<ui::MainWindow>(config_, shared_from_this());

            if (mainWindow_) {
                mainWindow_->setQueryCallback([this](const std::string& query) { processQuery(query); });
                mainWindow_->setFileSelectionCallback([this](const std::string& filePath) { handleFileSelection(filePath); });
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Components set up");
        }

        void Engine::processQuery(const std::string& query) {
            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Processing query: " + query);

            if (query.empty()) {
                return;
            }

            setStatus("Processing query...");

            // Process the query with the QueryProcessor
            std::string formattedQuery = queryProcessor_->processQuery(query);

            // Run inference with the LLM
            if (!llmInterface_->isInitialized()) {
                logger.log(utils::LogLevel::INFO, "Engine: Initializing LLM on demand");
                if (!llmInterface_->initialize()) {
                    logger.log(utils::LogLevel::ERROR, "Engine: Failed to initialize LLM");
                    if (mainWindow_) {
                        mainWindow_->displayResponse("Error: Failed to initialize language model. Please check logs.");
                    }
                    setStatus("Failed to initialize language model", true);
                    return;
                }
            }

            // Create streaming response callback
            // FIX: Use explicit capture for the query variable
            auto streamingCallback = [this, query](const std::string& token, bool isFinished) {
                static std::string fullResponse;
                fullResponse += token;

                if (mainWindow_) {
                    mainWindow_->displayResponse(fullResponse);
                }

                if (isFinished) {
                    // Store in conversation history
                    if (contextManager_) {
                        contextManager_->addToHistory(query, fullResponse);
                    }

                    setStatus("Ready");
                    fullResponse.clear();
                }
                };

            // Set inference parameters
            llm::InferenceParams params;
            // FIX: Use an explicit cast to avoid the double to float conversion warning
            params.temperature = static_cast<float>(config_->getFloat("llm.temperature", 0.7f));
            params.maxTokens = config_->getInt("llm.max_tokens", 2048);

            // Run inference with streaming
            // FIX: Pass the callback correctly
            if (!llmInterface_->runInferenceStreaming(formattedQuery, streamingCallback, params)) {
                logger.log(utils::LogLevel::ERROR, "Engine: Inference failed");
                if (mainWindow_) {
                    mainWindow_->displayResponse("Error: Failed to process query. Please try again.");
                }
                setStatus("Inference failed", true);
            }
        }

        void Engine::handleFileSelection(const std::string& filePath) {
            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: File selected: " + filePath);

            // Do something with the selected file
            // For example, display it in the UI or analyze it
            showFile(filePath);
        }

        bool Engine::showFile(const std::string& filePath) {
            if (filePath.empty() || !fs::exists(filePath)) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "Engine: Invalid file path: " + filePath);
                return false;
            }

            try {
                // Read file content
                std::ifstream file(filePath);
                if (!file.is_open()) {
                    utils::Logger& logger = utils::Logger::getInstance();
                    logger.log(utils::LogLevel::ERROR, "Engine: Failed to open file: " + filePath);
                    return false;
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string content = buffer.str();

                // Display file in UI
                if (mainWindow_) {
                    // This part depends on how you want to display files
                    // For now, we'll just show it in the chat panel
                    std::string fileInfo = "File: " + filePath + "\n\n";
                    mainWindow_->displayResponse(fileInfo + content);
                }

                return true;
            }
            catch (const std::exception& e) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "Engine: Error showing file: " + std::string(e.what()));
                return false;
            }
        }

        std::string Engine::getStatus() const {
            return statusMessage_;
        }

        void Engine::setStatus(const std::string& message, bool isError) {
            statusMessage_ = message;

            if (mainWindow_) {
                mainWindow_->setStatusMessage(message, isError);
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(isError ? utils::LogLevel::ERROR : utils::LogLevel::INFO,
                "Engine: Status set to: " + message);
        }

        std::shared_ptr<utils::Config> Engine::getConfig() const {
            return config_;
        }

    }
} // namespace codelve::core