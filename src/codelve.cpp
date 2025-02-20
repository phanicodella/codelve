// File: codelve/src/codelve.cpp
#include "codelve.h"
#include "core/engine.h"
#include "ui/main_window.h"
#include "utils/config.h"
#include "utils/logger.h"

namespace codelve {

// Version information
constexpr const char* VERSION = "0.1.0";

CodeLve::CodeLve(std::shared_ptr<utils::Config> config)
    : config_(std::move(config)),
      engine_(nullptr),
      mainWindow_(nullptr) {
    LOG_INFO("CodeLve instance created");
}

CodeLve::~CodeLve() {
    LOG_INFO("CodeLve instance destroyed");
}

bool CodeLve::initialize() {
    LOG_INFO("Initializing CodeLve...");
    
    try {
        // Initialize core engine
        engine_ = std::make_unique<core::Engine>(config_);
        if (!engine_->initialize()) {
            LOG_ERROR("Failed to initialize core engine");
            return false;
        }
        
        // Initialize UI
        mainWindow_ = std::make_unique<ui::MainWindow>(config_, engine_.get());
        if (!mainWindow_->initialize()) {
            LOG_ERROR("Failed to initialize main window");
            return false;
        }
        
        LOG_INFO("CodeLve initialization complete");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception during initialization: {}", e.what());
        return false;
    }
}

int CodeLve::run() {
    LOG_INFO("Starting CodeLve main loop");
    
    try {
        // Run the main window event loop
        return mainWindow_->run();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception during main loop: {}", e.what());
        return 1;
    }
}

std::string CodeLve::getVersion() {
    return VERSION;
}

} // namespace codelve