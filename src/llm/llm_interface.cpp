#include "llm_interface.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <regex>

namespace llama_cpp {
    struct llama_context;
    struct llama_model;
}

namespace codelve {
    namespace llm {

        class LlmInterface::Impl {
        public:
            Impl();
            ~Impl();

            bool loadModel(const std::string& modelPath);
            void unloadModel();
            std::string runInference(const std::string& prompt, const InferenceParams& params);
            bool runInferenceStreaming(const std::string& prompt, ResponseCallback callback, const InferenceParams& params);
            int countTokens(const std::string& prompt);
            std::string getModelInfo() const;

            void* llamaModel;
            void* llamaContext;
            std::string modelName;
            size_t contextSize;
            size_t memoryUsage;
        };

        LlmInterface::Impl::Impl()
            : llamaModel(nullptr),
            llamaContext(nullptr),
            contextSize(0),
            memoryUsage(0) {
        }

        LlmInterface::Impl::~Impl() {
            unloadModel();
        }

        bool LlmInterface::Impl::loadModel(const std::string& modelPath) {
            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "LlmInterface: Attempting to load model from " + modelPath);

            if (!std::filesystem::exists(modelPath)) {
                logger.log(utils::LogLevel::ERROR, "LlmInterface: Model file not found: " + modelPath);
                return false;
            }

            logger.log(utils::LogLevel::INFO, "LlmInterface: Loading model, this may take some time...");

            llamaModel = new int(1);
            llamaContext = new int(1);

            modelName = std::filesystem::path(modelPath).filename().string();
            contextSize = 8192;
            memoryUsage = 4096 * 1024 * 1024;

            logger.log(utils::LogLevel::INFO, "LlmInterface: Model loaded successfully");
            return true;
        }

        void LlmInterface::Impl::unloadModel() {
            if (llamaContext) {
                delete static_cast<int*>(llamaContext);
                llamaContext = nullptr;
            }

            if (llamaModel) {
                delete static_cast<int*>(llamaModel);
                llamaModel = nullptr;
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "LlmInterface: Model unloaded");
        }

        std::string LlmInterface::Impl::runInference(const std::string& prompt, const InferenceParams& params) {
            if (!llamaModel || !llamaContext) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "LlmInterface: Cannot run inference, model not loaded");
                return "ERROR: Model not loaded.";
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "LlmInterface: Running inference");

            std::string response = "This is a simulated response from the LLM interface.\n";
            response += "In the actual implementation, this would use llama.cpp to generate a real response.\n";
            response += "The prompt was: " + prompt.substr(0, 50) + "...\n";

            logger.log(utils::LogLevel::INFO, "LlmInterface: Inference completed");

            return response;
        }

        bool LlmInterface::Impl::runInferenceStreaming(
            const std::string& prompt,
            ResponseCallback callback,
            const InferenceParams& params
        ) {
            if (!llamaModel || !llamaContext) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "LlmInterface: Cannot run inference, model not loaded");
                callback("ERROR: Model not loaded.", true);
                return false;
            }

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "LlmInterface: Running streaming inference");

            std::vector<std::string> simulatedTokens = {
                "This ", "is ", "a ", "simulated ", "streaming ", "response ",
                "from ", "the ", "LLM ", "interface.\n",
                "In ", "the ", "actual ", "implementation, ", "this ", "would ",
                "use ", "llama.cpp ", "to ", "generate ", "real ", "tokens ", "one ", "by ", "one.\n"
            };

            for (size_t i = 0; i < simulatedTokens.size(); i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                bool isLast = (i == simulatedTokens.size() - 1);
                callback(simulatedTokens[i], isLast);
            }

            logger.log(utils::LogLevel::INFO, "LlmInterface: Streaming inference completed");

            return true;
        }

        int LlmInterface::Impl::countTokens(const std::string& prompt) {
            return static_cast<int>(prompt.length() / 4);
        }

        std::string LlmInterface::Impl::getModelInfo() const {
            std::string info = "Model: " + modelName + "\n";
            info += "Context size: " + std::to_string(contextSize) + " tokens\n";
            info += "Memory usage: " + std::to_string(memoryUsage / (1024 * 1024)) + " MB\n";
            return info;
        }

        LlmInterface::LlmInterface(std::shared_ptr<utils::Config> config)
            : config_(config),
            isInitialized_(false),
            impl_(std::make_unique<Impl>()) {

            modelPath_ = config_->getString("llm.model_path", "");

            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "LlmInterface: Created with model path: " + modelPath_);
        }

        LlmInterface::~LlmInterface() {
            unloadModel();
            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "LlmInterface: Destroyed");
        }

        bool LlmInterface::initialize() {
            if (isInitialized_) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::INFO, "LlmInterface: Already initialized");
                return true;
            }

            if (modelPath_.empty()) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "LlmInterface: Model path not specified in configuration");
                return false;
            }

            bool success = loadModel();
            if (success) {
                isInitialized_ = true;
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::INFO, "LlmInterface: Initialized successfully");
            }

            return isInitialized_;
        }

        bool LlmInterface::isInitialized() const {
            return isInitialized_;
        }

        std::string LlmInterface::runInference(const std::string& prompt, const InferenceParams& params) {
            if (!isInitialized_) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "LlmInterface: Cannot run inference, not initialized");
                return "ERROR: LLM interface not initialized.";
            }

            return impl_->runInference(prompt, params);
        }

        bool LlmInterface::runInferenceStreaming(
            const std::string& prompt,
            ResponseCallback callback,
            const InferenceParams& params
        ) {
            if (!isInitialized_) {
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::ERROR, "LlmInterface: Cannot run streaming inference, not initialized");
                callback("ERROR: LLM interface not initialized.", true);
                return false;
            }

            return impl_->runInferenceStreaming(prompt, callback, params);
        }

        int LlmInterface::countTokens(const std::string& prompt) {
            return impl_->countTokens(prompt);
        }

        std::string LlmInterface::getModelInfo() const {
            if (!isInitialized_) {
                return "Model not loaded.";
            }

            return impl_->getModelInfo();
        }

        void LlmInterface::unloadModel() {
            if (isInitialized_) {
                impl_->unloadModel();
                isInitialized_ = false;
                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::INFO, "LlmInterface: Model unloaded");
            }
        }

        bool LlmInterface::loadModel() {
            utils::Logger& logger = utils::Logger::getInstance();
            logger.log(utils::LogLevel::INFO, "LlmInterface: Loading model from " + modelPath_);

            bool success = impl_->loadModel(modelPath_);

            if (success) {
                logger.log(utils::LogLevel::INFO, "LlmInterface: Model loaded successfully");
            }
            else {
                logger.log(utils::LogLevel::ERROR, "LlmInterface: Failed to load model");
            }

            return success;
        }

    }
} // namespace codelve::llm
