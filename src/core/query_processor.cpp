// E:\codelve\src\core\query_processor.cpp
#include "query_processor.h"
#include "context_manager.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include <algorithm>
#include <sstream>

namespace codelve {
namespace core {

QueryProcessor::QueryProcessor(std::shared_ptr<utils::Config> config, 
                               std::shared_ptr<ContextManager> contextManager)
    : config_(config),
      contextManager_(contextManager) {
    
    // Initialize code-related keywords
    codeKeywords_ = {
        "code", "function", "class", "method", "variable", "implement", 
        "debug", "bug", "error", "refactor", "optimize", "documentation",
        "api", "module", "library", "interface", "test", "unit test"
    };
    
    // Initialize special commands
    specialCommands_ = {
        "/help", "/clear", "/reset", "/exit", "/info", "/settings"
    };
    
    // Load prompt templates from config
    codePromptTemplate_ = config_->getString("prompts.code_template", 
        "You are CodeLve, an AI assistant for code analysis.\n"
        "Analyze the following code and answer the user's question:\n\n"
        "{context}\n\n"
        "User query: {query}\n"
        "Provide a detailed and accurate response focusing on the code.");
    
    generalPromptTemplate_ = config_->getString("prompts.general_template", 
        "You are CodeLve, an AI assistant for developers.\n"
        "Answer the following question based on your knowledge:\n\n"
        "User query: {query}\n"
        "Provide a concise and helpful response.");
    
    utils::Logger::log(utils::LogLevel::INFO, "QueryProcessor: Initialized");
}

QueryProcessor::~QueryProcessor() {
    utils::Logger::log(utils::LogLevel::INFO, "QueryProcessor: Destroyed");
}

std::string QueryProcessor::processQuery(const std::string& rawQuery) {
    // Check if this is a special command
    std::string command = extractCommand(rawQuery);
    if (!command.empty()) {
        return command;
    }
    
    // Determine if this is a code-related query
    bool isCode = isCodebaseQuery(rawQuery);
    
    std::string formattedQuery;
    if (isCode) {
        // Build context-aware query for code questions
        std::string context = contextManager_->buildContext(rawQuery);
        
        // Apply the code prompt template
        formattedQuery = codePromptTemplate_;
        size_t contextPos = formattedQuery.find("{context}");
        if (contextPos != std::string::npos) {
            formattedQuery.replace(contextPos, 9, context);
        }
        
        size_t queryPos = formattedQuery.find("{query}");
        if (queryPos != std::string::npos) {
            formattedQuery.replace(queryPos, 7, rawQuery);
        }
        
        // Add specific instructions based on query type
        formattedQuery += "\n" + formatInstructions(rawQuery);
        
    } else {
        // Apply the general prompt template for non-code questions
        formattedQuery = generalPromptTemplate_;
        size_t queryPos = formattedQuery.find("{query}");
        if (queryPos != std::string::npos) {
            formattedQuery.replace(queryPos, 7, rawQuery);
        }
    }
    
    utils::Logger::log(utils::LogLevel::INFO, 
        "QueryProcessor: Processed query, detected as " + 
        std::string(isCode ? "code-related" : "general"));
    
    return formattedQuery;
}

std::string QueryProcessor::extractCommand(const std::string& rawQuery) {
    // Trim whitespace
    std::string trimmedQuery = rawQuery;
    trimmedQuery.erase(0, trimmedQuery.find_first_not_of(" \t\n\r\f\v"));
    trimmedQuery.erase(trimmedQuery.find_last_not_of(" \t\n\r\f\v") + 1);
    
    // Check if this is a special command
    for (const auto& cmd : specialCommands_) {
        if (trimmedQuery == cmd || trimmedQuery.find(cmd + " ") == 0) {
            utils::Logger::log(utils::LogLevel::INFO, 
                "QueryProcessor: Extracted command: " + cmd);
            return cmd;
        }
    }
    
    return "";
}

bool QueryProcessor::isCodebaseQuery(const std::string& query) const {
    // Convert query to lowercase for case-insensitive matching
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    // Check for code-related keywords
    for (const auto& keyword : codeKeywords_) {
        if (lowerQuery.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    // Check if query mentions specific files in the codebase
    // This is a simple implementation that will be improved later
    std::vector<std::string> relevantFiles = contextManager_->getRelevantFiles(query, 1);
    return !relevantFiles.empty();
}

std::string QueryProcessor::formatInstructions(const std::string& query) const {
    std::stringstream instructions;
    
    // Detect query intent and provide specific instructions
    
    // For code explanation
    if (query.find("explain") != std::string::npos || 
        query.find("understand") != std::string::npos || 
        query.find("what does") != std::string::npos) {
        
        instructions << "Focus on explaining the code's purpose, functionality, and structure. "
                    << "Break down complex parts and explain the logic step by step.";
    }
    // For debugging help
    else if (query.find("bug") != std::string::npos || 
             query.find("error") != std::string::npos || 
             query.find("fix") != std::string::npos || 
             query.find("issue") != std::string::npos) {
        
        instructions << "Identify potential bugs or issues in the code. "
                    << "Suggest specific fixes and explain why they would solve the problem.";
    }
    // For optimization
    else if (query.find("optimize") != std::string::npos || 
             query.find("performance") != std::string::npos || 
             query.find("faster") != std::string::npos || 
             query.find("efficient") != std::string::npos) {
        
        instructions << "Analyze the code for performance bottlenecks. "
                    << "Suggest optimizations and explain the expected improvements.";
    }
    // For implementation help
    else if (query.find("implement") != std::string::npos || 
             query.find("create") != std::string::npos || 
             query.find("write") != std::string::npos || 
             query.find("add") != std::string::npos) {
        
        instructions << "Provide a complete implementation that follows best practices. "
                    << "Ensure the code is well-documented and fits with the existing codebase style.";
    }
    // For documentation
    else if (query.find("document") != std::string::npos || 
             query.find("comments") != std::string::npos || 
             query.find("readme") != std::string::npos) {
        
        instructions << "Generate comprehensive documentation for the code. "
                    << "Include function descriptions, parameter details, return values, and usage examples.";
    }
    // Default instructions
    else {
        instructions << "Provide a detailed analysis relevant to the user's query. "
                    << "Include code examples where appropriate and explain any technical concepts.";
    }
    
    return instructions.str();
}

}} // namespace codelve::core