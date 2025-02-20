// File: codelve/src/core/engine.h
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace codelve {

// Forward declarations
namespace utils {
    class Config;
}

namespace model {
    class LlamaInterface;
    class ModelConfig;
}

namespace scanner {
    class FileSystem;
    class CodeParser;
    class Indexer;
}

namespace core {

class QueryProcessor;
class ContextManager;

/**
 * The core engine that coordinates all components of CodeLve.
 */
class Engine {
public:
    /**
     * Constructor.
     * @param config Shared pointer to configuration
     */
    explicit Engine(std::shared_ptr<utils::Config> config);
    
    /**
     * Destructor.
     */
    ~Engine();
    
    /**
     * Initialize the engine.
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Process a query about the codebase.
     * @param query The natural language query
     * @param callback Callback function to handle query response
     * @return true if query processing started successfully, false otherwise
     */
    bool processQuery(const std::string& query, 
                     std::function<void(const std::string&, bool)> callback);
    
    /**
     * Load a codebase for analysis.
     * @param path Path to the codebase directory
     * @return true if loading started successfully, false otherwise
     */
    bool loadCodebase(const std::string& path);
    
    /**
     * Get information about the current codebase.
     * @return Information string about the loaded codebase
     */
    std::string getCodebaseInfo() const;
    
    /**
     * Check if a codebase is currently loaded.
     * @return true if a codebase is loaded, false otherwise
     */
    bool isCodebaseLoaded() const;
    
    /**
     * Get the current memory usage.
     * @return Memory usage report string
     */
    std::string getMemoryUsage() const;

private:
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Model components
    std::unique_ptr<model::LlamaInterface> llamaInterface_;
    std::shared_ptr<model::ModelConfig> modelConfig_;
    
    // Scanner components
    std::unique_ptr<scanner::FileSystem> fileSystem_;
    std::unique_ptr<scanner::CodeParser> codeParser_;
    std::unique_ptr<scanner::Indexer> indexer_;
    
    // Core components
    std::unique_ptr<QueryProcessor> queryProcessor_;
    std::unique_ptr<ContextManager> contextManager_;
    
    // Codebase state
    bool codebaseLoaded_;
    std::string codebasePath_;
    int totalFiles_;
    int parsedFiles_;
};

}} // namespace codelve::core