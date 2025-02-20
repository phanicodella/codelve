// File: codelve/src/core/context_manager.h
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace codelve {

// Forward declarations
namespace utils {
    class Config;
}

namespace scanner {
    struct IndexedCode;
}

namespace core {

/**
 * Manages context for the LLM, including keeping track of 
 * relevant code files and snippets for queries.
 */
class ContextManager {
public:
    /**
     * Constructor.
     * @param config Shared pointer to configuration
     */
    explicit ContextManager(std::shared_ptr<utils::Config> config);
    
    /**
     * Destructor.
     */
    ~ContextManager();
    
    /**
     * Initialize with indexed code.
     * @param indexedCode The indexed code from the codebase
     * @return true if initialization was successful, false otherwise
     */
    bool initialize(const scanner::IndexedCode& indexedCode);
    
    /**
     * Build context for a specific query.
     * @param query The natural language query
     * @return Context string to provide to the LLM
     */
    std::string buildContext(const std::string& query);
    
    /**
     * Get a specific file from the codebase.
     * @param filePath Path to the file
     * @return File content or empty string if not found
     */
    std::string getFile(const std::string& filePath) const;
    
    /**
     * Get relevant files for a query.
     * @param query The natural language query
     * @param maxFiles Maximum number of files to return
     * @return Vector of file paths
     */
    std::vector<std::string> getRelevantFiles(const std::string& query, int maxFiles = 5) const;
    
    /**
     * Get the current conversation history.
     * @return Conversation history as string
     */
    std::string getConversationHistory() const;
    
    /**
     * Add an entry to the conversation history.
     * @param query User query
     * @param response System response
     */
    void addToHistory(const std::string& query, const std::string& response);
    
    /**
     * Clear the conversation history.
     */
    void clearHistory();

private:
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Indexed code data
    std::unordered_map<std::string, std::string> files_;
    std::unordered_map<std::string, std::vector<std::string>> symbols_;
    
    // Conversation history
    std::vector<std::pair<std::string, std::string>> history_;
    
    // Maximum context window size
    int maxContextSize_;
    
    // Maximum history entries
    int maxHistoryEntries_;
    
    // Methods to find relevant information
    std::vector<std::string> findRelevantSymbols(const std::string& query) const;
    std::string findFileContainingSymbol(const std::string& symbol) const;
    std::string formatCodeSnippet(const std::string& filePath, int startLine, int endLine) const;
};

}} // namespace codelve::core