// E:\codelve\src\core\query_processor.h
#pragma once
#include <string>
#include <memory>
#include <vector>

namespace codelve {
namespace utils {
    class Config;
}
namespace core {

class ContextManager;

/**
 * Processes user queries and prepares them for LLM inference.
 * Handles formatting and structuring the query with appropriate context.
 */
class QueryProcessor {
public:
    /**
     * Constructor.
     * @param config Shared configuration
     * @param contextManager Shared context manager
     */
    QueryProcessor(std::shared_ptr<utils::Config> config, 
                   std::shared_ptr<ContextManager> contextManager);
    
    /**
     * Destructor.
     */
    ~QueryProcessor();
    
    /**
     * Process a raw user query and create a formatted query for the LLM.
     * @param rawQuery The raw user input
     * @return Formatted query with context
     */
    std::string processQuery(const std::string& rawQuery);
    
    /**
     * Get command or special instruction from query if any.
     * @param rawQuery The raw user input
     * @return Command string or empty if none
     */
    std::string extractCommand(const std::string& rawQuery);
    
    /**
     * Check if query is related to the codebase.
     * @param query The raw user input
     * @return True if query is about the codebase
     */
    bool isCodebaseQuery(const std::string& query) const;
    
    /**
     * Format code-specific instructions for the LLM.
     * @param query The processed query
     * @return Formatted instructions
     */
    std::string formatInstructions(const std::string& query) const;

private:
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Context manager for query-relevant context
    std::shared_ptr<ContextManager> contextManager_;
    
    // Keywords for detecting code-related queries
    std::vector<std::string> codeKeywords_;
    
    // Special commands that can be extracted from queries
    std::vector<std::string> specialCommands_;
    
    // Prompt template for code-related queries
    std::string codePromptTemplate_;
    
    // Prompt template for general queries
    std::string generalPromptTemplate_;
};

}} // namespace codelve::core