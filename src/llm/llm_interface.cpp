// E:\CodeLve\src\llm\llm_interface.cpp
#ifndef USE_MOCK_LLM
#define USE_MOCK_LLM 1  // Set to 1 for mock mode, 0 for real llama.cpp
#endif
#include "llm_interface.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <regex>
#include <mutex>

// Include llama.cpp headers
#include "llama.h"

// Include mock implementation for testing
#include "mock_llm.h"

namespace codelve {
    namespace llm {

        class LlmInterface::Impl {
        public:
            Impl()
                : llamaModel(nullptr),
                llamaContext(nullptr),
                contextSize(0),
                memoryUsage(0) {
            }

            ~Impl() {
                unloadModel();
            }

            bool loadModel(const std::string& modelPath) {
                std::lock_guard<std::mutex> lock(mutex_);

                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::INFO, "LlmInterface: Attempting to load model from " + modelPath);

                if (!std::filesystem::exists(modelPath)) {
                    logger.log(utils::LogLevel::ERROR, "LlmInterface: Model file not found: " + modelPath);
                    return false;
                }

                logger.log(utils::LogLevel::INFO, "LlmInterface: Loading model, this may take some time...");

#if USE_MOCK_LLM
                // In mock mode, simulate model loading with delay
                std::this_thread::sleep_for(std::chrono::seconds(1));
                modelName = std::filesystem::path(modelPath).filename().string();
                contextSize = 8192;
                memoryUsage = static_cast<size_t>(4096) * 1024 * 1024;

                logger.log(utils::LogLevel::INFO, "LlmInterface: Mock model loaded successfully");
                return true;
#else
                // Initialize llama.cpp parameters
                llama_model_params modelParams = llama_model_default_params();

                // Load the model
                llamaModel = llama_load_model_from_file(modelPath.c_str(), modelParams);
                if (!llamaModel) {
                    logger.log(utils::LogLevel::ERROR, "LlmInterface: Failed to load model");
                    return false;
                }

                // Set up the context
                llama_context_params contextParams = llama_context_default_params();
                contextParams.n_ctx = 8192;  // Use larger context window
                contextParams.seed = -1;     // Use random seed

                llamaContext = llama_new_context_with_model(llamaModel, contextParams);
                if (!llamaContext) {
                    logger.log(utils::LogLevel::ERROR, "LlmInterface: Failed to create context");
                    llama_free_model(llamaModel);
                    llamaModel = nullptr;
                    return false;
                }

                // Store information about the model
                contextSize = llama_n_ctx(llamaContext);
                modelName = std::filesystem::path(modelPath).filename().string();

                // Estimate memory usage
                memoryUsage = llama_get_state_size(llamaContext);

                logger.log(utils::LogLevel::INFO, "LlmInterface: Model loaded successfully");
                logger.log(utils::LogLevel::INFO, "LlmInterface: Context size: " + std::to_string(contextSize) + " tokens");
                logger.log(utils::LogLevel::INFO, "LlmInterface: Memory usage: " + std::to_string(memoryUsage / (1024 * 1024)) + " MB");

                return true;
#endif
            }

            void unloadModel() {
                std::lock_guard<std::mutex> lock(mutex_);

#if !USE_MOCK_LLM
                if (llamaContext) {
                    llama_free(llamaContext);
                    llamaContext = nullptr;
                }

                if (llamaModel) {
                    llama_free_model(llamaModel);
                    llamaModel = nullptr;
                }
#endif

                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::INFO, "LlmInterface: Model unloaded");
            }

            std::string runInference(const std::string& prompt, const InferenceParams& params) {
                if (!isModelLoaded()) {
                    utils::Logger& logger = utils::Logger::getInstance();
                    logger.log(utils::LogLevel::ERROR, "LlmInterface: Cannot run inference, model not loaded");
                    return "ERROR: Model not loaded.";
                }

                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::INFO, "LlmInterface: Running inference");

                // For simplicity, we'll use the streaming implementation internally
                std::string fullResponse;
                auto callback = [&fullResponse](const std::string& token, bool isFinished) {
                    (void)isFinished; // Silence unused parameter warning
                    fullResponse += token;
                    };

                if (!runInferenceStreaming(prompt, callback, params)) {
                    return "ERROR: Inference failed.";
                }

                return fullResponse;
            }

            bool runInferenceStreaming(
                const std::string& prompt,
                ResponseCallback callback,
                const InferenceParams& params
            ) {
                if (!isModelLoaded()) {
                    utils::Logger& logger = utils::Logger::getInstance();
                    logger.log(utils::LogLevel::ERROR, "LlmInterface: Cannot run inference, model not loaded");
                    callback("ERROR: Model not loaded.", true);
                    return false;
                }

                utils::Logger& logger = utils::Logger::getInstance();
                logger.log(utils::LogLevel::INFO, "LlmInterface: Running streaming inference");

#if USE_MOCK_LLM
                // Use mock implementation for testing
                return MockLlm::generateResponse(prompt, callback);
#else
                try {
                    std::lock_guard<std::mutex> lock(mutex_);

                    // Reference params to silence warning
                    (void)params;

                    // Reset the context
                    llama_kv_cache_clear(llamaContext);

                    // Tokenize the prompt
                    std::vector<llama_token> tokens = llama_tokenize(llamaModel, prompt.c_str(), prompt.length(), true);
                    if (tokens.empty()) {
                        logger.log(utils::LogLevel::ERROR, "LlmInterface: Failed to tokenize prompt");
                        callback("ERROR: Failed to tokenize prompt.", true);
                        return false;
                    }

                    // Limit number of tokens to context size
                    if (tokens.size() >= contextSize) {
                        logger.log(utils::LogLevel::WARNING, "LlmInterface: Prompt too long, truncating");
                        tokens.resize(contextSize - 256);  // Leave space for generated tokens
                    }

                    // Evaluate the prompt
                    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size(), 0, 0);
                    if (llama_decode(llamaContext, batch) != 0) {
                        logger.log(utils::LogLevel::ERROR, "LlmInterface: Failed to decode prompt");
                        callback("ERROR: Failed to process prompt.", true);
                        return false;
                    }

                    // Generate tokens
                    llama_token id = 0;
                    std::string generatedText;
                    int numTokens = 0;

                    const int maxTokens = params.maxTokens > 0 ? params.maxTokens : 256;

                    // Prepare sampling parameters
                    llama_sampling_params sampling_params = llama_sampling_default_params();
                    sampling_params.temp = params.temperature;
                    sampling_params.top_p = params.topP;
                    sampling_params.penalty_repeat = 1.1f;

                    // Initialize sampler
                    llama_sampling_context* sampler = llama_sampling_init(sampling_params);

                    // Generate tokens one by one
                    while (numTokens < maxTokens) {
                        // Sample next token
                        id = llama_sampling_sample(sampler, llamaContext, NULL);

                        // Check for end of generation
                        if (id == llama_token_eos(llamaModel)) {
                            break;
                        }

                        // Get the token as text
                        const char* tokenText = llama_token_to_str(llamaModel, id);
                        if (!tokenText) {
                            break;
                        }

                        // Add token to generated text
                        std::string tokenString(tokenText);
                        generatedText += tokenString;

                        // Call the callback with the token
                        callback(tokenString, false);

                        // Add the token to the context
                        llama_batch nextBatch = llama_batch_get_one(&id, 1, tokens.size() + numTokens, 0);
                        if (llama_decode(llamaContext, nextBatch) != 0) {
                            break;
                        }

                        numTokens++;
                    }

                    // Clean up
                    llama_sampling_free(sampler);

                    // Signal completion
                    callback("", true);

                    logger.log(utils::LogLevel::INFO, "LlmInterface: Generated " + std::to_string(numTokens) + " tokens");
                    return true;
                }
                catch (const std::exception& e) {
                    logger.log(utils::LogLevel::ERROR, "LlmInterface: Exception during inference: " + std::string(e.what()));
                    callback("ERROR: Exception during inference.", true);
                    return false;
                }
#endif
            }

            int countTokens(const std::string& prompt) {
#if USE_MOCK_LLM
                return static_cast<int>(prompt.length() / 4);  // Rough estimate
#else
                if (!llamaModel) {
                    return static_cast<int>(prompt.length() / 4);  // Rough estimate
                }

                std::vector<llama_token> tokens = llama_tokenize(llamaModel, prompt.c_str(), prompt.length(), true);
                return static_cast<int>(tokens.size());
#endif
            }

            std::string getModelInfo() const {
                std::string info = "Model: " + modelName + "\n";
                info += "Context size: " + std::to_string(contextSize) + " tokens\n";
                info += "Memory usage: " + std::to_string(memoryUsage / (1024 * 1024)) + " MB\n";

#if USE_MOCK_LLM
                info += "Mode: Mock (Testing)\n";
#else
                info += "Mode: Real Model\n";
#endif

                return info;
            }

            bool isModelLoaded() const {
#if USE_MOCK_LLM
                return true;  // Mock is always "loaded"
#else
                return llamaModel != nullptr && llamaContext != nullptr;
#endif
            }

            // Model state
            llama_model* llamaModel;
            llama_context* llamaContext;
            std::string modelName;
            size_t contextSize;
            size_t memoryUsage;
            std::mutex mutex_;
        };

        // LlmInterface implementation

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