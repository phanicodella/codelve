// E:\CodeLve\src\core\engine_functions.cpp
#include "engine.h"
#include "context_manager.h"
#include "query_processor.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include "../scanner/scanner.h"
#include "../llm/llm_interface.h"
#include "../ui/main_window.h"

namespace codelve {
    namespace core {

        int Engine::run() {
            if (!mainWindow_) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "Engine: Cannot run, mainWindow not initialized");
                return 1;
            }

            return mainWindow_->run();
        }

        void Engine::processQuery(const std::string& query) {
            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Processing query: " + query);

            if (!queryProcessor_ || !llmInterface_) {
                logger.log(utils::LogLevel::ERROR, "Engine: Cannot process query, components not initialized");
                setStatus("Internal error: Components not initialized", true);
                return;
            }

            // Update status
            setStatus("Processing query...");

            // Format the query with context
            std::string formattedQuery = queryProcessor_->processQuery(query);

            // Run inference
            std::string response = llmInterface_->runInference(formattedQuery, llm::InferenceParams());

            // Display response
            displayResponse(response);

            // Add to conversation history
            if (contextManager_) {
                contextManager_->addToHistory(query, response);
            }

            // Update status
            setStatus("Ready");
        }

        std::string Engine::getStatus() const {
            return statusMessage_;
        }

        bool Engine::showFile(const std::string& filePath) {
            if (!mainWindow_) {
                return false;
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Showing file: " + filePath);

            // TODO: Implement file display in UI

            return true;
        }

        std::shared_ptr<utils::Config> Engine::getConfig() const {
            return config_;
        }

        void Engine::handleFileSelection(const std::string& filePath) {
            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: File selected: " + filePath);

            // Show the file in the UI
            showFile(filePath);
        }

        void Engine::displayResponse(const std::string& response) {
            if (mainWindow_) {
                mainWindow_->displayResponse(response);
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Response displayed");
        }

        void Engine::setStatus(const std::string& message, bool isError) {
            statusMessage_ = message;

            if (mainWindow_) {
                mainWindow_->setStatusMessage(message, isError);
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(isError ? utils::LogLevel::ERROR : utils::LogLevel::INFO,
                "Engine: Status - " + message);
        }

    }
} // namespace codelve::core