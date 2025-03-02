#include "main_window.h"
#include "file_browser.h"  
#include "chat_panel.h" 
#include "../utils/logger.h"
#include "../core/engine.h"

namespace codelve {
    namespace ui {

        MainWindow::MainWindow(std::shared_ptr<utils::Config> config, core::Engine* engine)
            : config_(config),
            mainWindow_(nullptr),
            statusBar_(nullptr) {

            // Convert raw pointer to shared_ptr using the enable_shared_from_this capability
            if (engine) {
                engine_ = engine->shared_from_this();
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "MainWindow: Created with raw engine pointer");
        }

    }
} // namespace