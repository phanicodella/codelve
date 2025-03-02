#include "engine.h"
#include "../utils/logger.h"

namespace codelve {
    namespace core {

        Engine::Engine(std::shared_ptr<utils::Config> config)
            : configPath_(""),
            config_(config),
            scanner_(nullptr),
            contextManager_(nullptr),
            queryProcessor_(nullptr),
            llmInterface_(nullptr),
            mainWindow_(nullptr),
            progressDialogHandle(nullptr) {

            // Set initial status
            statusMessage_ = "Initializing...";

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "Engine: Created with config object");
        }

    }
} // namespace