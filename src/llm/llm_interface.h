// E:\codelve\src\llm\llm_interface.h
#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace codelve {
namespace utils {
    class Config;
}
namespace llm {

/**
 * Callback type for streaming LLM responses.
 * @param token The new token string
 * @param isFinished Whether this is the last token
 */
using ResponseCallback = std::function<void(const std::string& token, bool isFinished)>;

/**
 * Configuration parameters for an LLM inference request.
 */
struct InferenceParams {
    float temperature = 0.7f;
    int maxTokens = 2048;
    float topP = 0.95f;
    float presencePenalty = 0.0f;
    float frequencyPenalty = 0.0f;
    std::vector<std::string> stopSequences;
};

/**
 * Interface for communicating with the LLM.
 * Handles model loading, inference, and resource management.
 */
class LlmInterface {
public:
    /**
     * Constructor.
     * @param config Shared configuration
     */
    explicit LlmInterface(std::shared_ptr<utils::Config> config);
    
    /**
     * Destructor.
     */
    ~LlmInterface();
    
    /**
     * Initialize the LLM model.
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Check if the model is initialized.
     * @return true if model is initialized, false otherwise
     */
    bool isInitialized() const;
    
    /**
     * Run inference on the given prompt.
     * @param prompt The input prompt for the LLM
     * @param params Inference parameters
     * @return The model's response
     */
    std::string runInference(const std::string& prompt, const InferenceParams& params);
    
    /**
     * Run inference with streaming response.
     * @param prompt The input prompt for the LLM
     * @param callback The callback to receive tokens
     * @param params Inference parameters
     * @return true if inference completed successfully, false otherwise
     */
    bool runInferenceStreaming(const std::string& prompt, 
                               ResponseCallback callback,
                               const InferenceParams& params);
    
    /**
     * Get the number of tokens in a prompt.
     * @param prompt The prompt to tokenize
     * @return Estimated token count
     */
    int countTokens(const std::string& prompt);
    
    /**
     * Get model information.
     * @return Model information string
     */
    std::string getModelInfo() const;
    
    /**
     * Unload the model and free resources.
     */
    void unloadModel();

private:
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Model path
    std::string modelPath_;
    
    // Whether model is initialized
    bool isInitialized_;
    
    // Internal implementation pointer (PIMPL)
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Methods to handle model loading/unloading
    bool loadModel();
};

}} // namespace codelve::llm