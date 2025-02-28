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
            if (mainWindow_) {
                progressDialogHandle = mainWindow_->showProgressDialog("Loading Codebase", "Scanning files...");
            }

            auto scanCallback = [this, callback, progressDialogHandle](const std::string& stage, float progress, const std::string& message) {
                if (callback) {
                    callback(stage, progress, message);
                }
                if (mainWindow_ && progressDialogHandle) {
                    mainWindow_->updateProgressDialog(progressDialogHandle, progress, stage + ": " + message);
                }
                setStatus(stage + ": " + message);
                };

            scanner_->setProgressCallback(scanCallback);

            std::thread([this, directoryPath, progressDialogHandle]() {
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

    }
} // namespace codelve::core
