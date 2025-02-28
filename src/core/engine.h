#pragma once
#include <string>
#include <memory>
#include <functional>
#include <Windows.h>  // Include this for HWND type

namespace codelve {
    namespace utils {
        class Config;
    }
    namespace scanner {
        class Scanner;
        struct IndexedCode;
    }
    namespace llm {
        class LlmInterface;
    }
    namespace core {
        class ContextManager;
        class QueryProcessor;
    }
    namespace ui {
        class MainWindow;
        class ProgressDialog;  // Forward declaration of ProgressDialog
    }

    namespace core {

        /**
         * Callback for progress updates
         */
        using ProgressCallback = std::function<void(const std::string& stage,
            float progress,
            const std::string& message)>;

        /**
         * Core engine that coordinates all components
         */
        class Engine : public std::enable_shared_from_this<Engine> {
        public:
            /**
             * Constructor.
             * @param configPath Path to configuration file
             */
            explicit Engine(const std::string& configPath);

            /**
             * Destructor.
             */
            ~Engine();

            /**
             * Initialize the engine and all components.
             * @return true if initialization was successful, false otherwise
             */
            bool initialize();

            /**
             * Run the application.
             * @return Result code
             */
            int run();

            /**
             * Load and analyze a codebase.
             * @param directoryPath Path to the codebase directory
             * @param callback Callback for progress updates
             * @return true if loading was successful, false otherwise
             */
            bool loadCodebase(const std::string& directoryPath, ProgressCallback callback = nullptr);

            /**
             * Process a user query.
             * @param query The query string
             */
            void processQuery(const std::string& query);

            /**
             * Get the current status message.
             * @return Status message
             */
            std::string getStatus() const;

            /**
             * Show a file in the UI.
             * @param filePath Path to the file
             * @return true if the file was found and displayed, false otherwise
             */
            bool showFile(const std::string& filePath);

            /**
             * Get application config.
             * @return Shared config pointer
             */
            std::shared_ptr<utils::Config> getConfig() const;

        private:
            // Configuration
            std::string configPath_;
            std::shared_ptr<utils::Config> config_;

            // Status
            std::string statusMessage_;

            // Components
            std::shared_ptr<scanner::Scanner> scanner_;
            std::shared_ptr<core::ContextManager> contextManager_;
            std::shared_ptr<core::QueryProcessor> queryProcessor_;
            std::shared_ptr<llm::LlmInterface> llmInterface_;
            std::shared_ptr<ui::MainWindow> mainWindow_;

            // Indexed code data
            std::shared_ptr<scanner::IndexedCode> indexedCode_;

            // Progress dialog handle
            HWND progressDialogHandle;  // Declare progressDialogHandle with the correct type

            // Methods
            void setupComponents();
            void handleFileSelection(const std::string& filePath);
            void displayResponse(const std::string& response);
            void setStatus(const std::string& message, bool isError = false);
        };

    }
} // namespace codelve::core
