// E:\codelve\src\core\context_manager.cpp
#include "context_manager.h"
#include "../utils/config.h"
#include "../scanner/scanner.h"
#include "../utils/logger.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace codelve {
namespace core {

ContextManager::ContextManager(std::shared_ptr<utils::Config> config)
    : config_(config),
      maxContextSize_(8192),
      maxHistoryEntries_(10) {
    // Load configuration values
    maxContextSize_ = config_->getInt("llm.max_context_size", 8192);
    maxHistoryEntries_ = config_->getInt("context.max_history", 10);
    
    utils::Logger::log(utils::LogLevel::INFO, "ContextManager: Initialized with max context size: " + 
                                               std::to_string(maxContextSize_));
}

ContextManager::~ContextManager() {
    utils::Logger::log(utils::LogLevel::INFO, "ContextManager: Destroyed");
}

bool ContextManager::initialize(const scanner::IndexedCode& indexedCode) {
    try {
        // Store indexed files
        files_ = indexedCode.files;
        
        // Store symbol information
        symbols_ = indexedCode.symbols;
        
        utils::Logger::log(utils::LogLevel::INFO, 
            "ContextManager: Initialized with " + 
            std::to_string(files_.size()) + " files and " + 
            std::to_string(symbols_.size()) + " symbols");
        
        return true;
    } catch (const std::exception& e) {
        utils::Logger::log(utils::LogLevel::ERROR, 
            "ContextManager: Failed to initialize with indexed code: " + 
            std::string(e.what()));
        return false;
    }
}

std::string ContextManager::buildContext(const std::string& query) {
    std::stringstream context;
    
    // Add conversation history
    context << "### Conversation History ###\n";
    std::string historyText = getConversationHistory();
    if (!historyText.empty()) {
        context << historyText << "\n\n";
    }
    
    // Add current query
    context << "### Current Query ###\n";
    context << query << "\n\n";
    
    // Find relevant symbols for the query
    std::vector<std::string> relevantSymbols = findRelevantSymbols(query);
    
    // Get relevant files based on symbols
    std::vector<std::string> relevantFiles = getRelevantFiles(query);
    
    // Add relevant code files to context
    context << "### Relevant Code ###\n";
    for (const auto& filePath : relevantFiles) {
        if (files_.find(filePath) != files_.end()) {
            context << "File: " << filePath << "\n";
            context << "```\n" << files_[filePath] << "\n```\n\n";
        }
    }
    
    // Ensure context fits within token limits (rough approximation)
    std::string result = context.str();
    if (result.length() > maxContextSize_ * 4) {  // Approximate character to token ratio
        result = result.substr(0, maxContextSize_ * 4);
        utils::Logger::log(utils::LogLevel::WARNING, 
            "ContextManager: Context truncated to fit token limit");
    }
    
    utils::Logger::log(utils::LogLevel::INFO, 
        "ContextManager: Built context with " + std::to_string(result.length()) + " characters");
    
    return result;
}

std::string ContextManager::getFile(const std::string& filePath) const {
    auto it = files_.find(filePath);
    if (it != files_.end()) {
        return it->second;
    }
    return "";
}

std::vector<std::string> ContextManager::getRelevantFiles(const std::string& query, int maxFiles) const {
    // This is a simple implementation that will be improved later with better relevance scoring
    std::vector<std::string> relevantFiles;
    
    // First get files containing symbols relevant to the query
    std::vector<std::string> relevantSymbols = findRelevantSymbols(query);
    
    for (const auto& symbol : relevantSymbols) {
        std::string filePath = findFileContainingSymbol(symbol);
        if (!filePath.empty() && 
            std::find(relevantFiles.begin(), relevantFiles.end(), filePath) == relevantFiles.end()) {
            relevantFiles.push_back(filePath);
            if (relevantFiles.size() >= static_cast<size_t>(maxFiles)) {
                break;
            }
        }
    }
    
    // If we still need more files, add some based on filename matching with query terms
    if (relevantFiles.size() < static_cast<size_t>(maxFiles)) {
        // Split query into terms
        std::istringstream iss(query);
        std::vector<std::string> queryTerms;
        std::string term;
        while (iss >> term) {
            if (term.length() > 3) {  // Only consider meaningful terms
                std::transform(term.begin(), term.end(), term.begin(), ::tolower);
                queryTerms.push_back(term);
            }
        }
        
        // Check files for relevance
        for (const auto& filePair : files_) {
            // Skip files already included
            if (std::find(relevantFiles.begin(), relevantFiles.end(), filePair.first) != relevantFiles.end()) {
                continue;
            }
            
            // Check if filename contains any query terms
            std::string filename = fs::path(filePair.first).filename().string();
            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
            
            for (const auto& term : queryTerms) {
                if (filename.find(term) != std::string::npos) {
                    relevantFiles.push_back(filePair.first);
                    break;
                }
            }
            
            if (relevantFiles.size() >= static_cast<size_t>(maxFiles)) {
                break;
            }
        }
    }
    
    return relevantFiles;
}

std::string ContextManager::getConversationHistory() const {
    std::stringstream history;
    
    for (const auto& entry : history_) {
        history << "User: " << entry.first << "\n";
        history << "CodeLve: " << entry.second << "\n\n";
    }
    
    return history.str();
}

void ContextManager::addToHistory(const std::string& query, const std::string& response) {
    history_.emplace_back(query, response);
    
    // Limit history size
    if (history_.size() > static_cast<size_t>(maxHistoryEntries_)) {
        history_.erase(history_.begin());
    }
}

void ContextManager::clearHistory() {
    history_.clear();
    utils::Logger::log(utils::LogLevel::INFO, "ContextManager: Conversation history cleared");
}

std::vector<std::string> ContextManager::findRelevantSymbols(const std::string& query) const {
    std::vector<std::string> relevantSymbols;
    
    // Split query into terms
    std::istringstream iss(query);
    std::vector<std::string> queryTerms;
    std::string term;
    while (iss >> term) {
        if (term.length() > 3) {  // Only consider meaningful terms
            std::transform(term.begin(), term.end(), term.begin(), ::tolower);
            queryTerms.push_back(term);
        }
    }
    
    // Find symbols that match query terms
    for (const auto& symbolPair : symbols_) {
        std::string symbolLower = symbolPair.first;
        std::transform(symbolLower.begin(), symbolLower.end(), symbolLower.begin(), ::tolower);
        
        for (const auto& term : queryTerms) {
            if (symbolLower.find(term) != std::string::npos) {
                relevantSymbols.push_back(symbolPair.first);
                break;
            }
        }
    }
    
    return relevantSymbols;
}

std::string ContextManager::findFileContainingSymbol(const std::string& symbol) const {
    auto it = symbols_.find(symbol);
    if (it != symbols_.end() && !it->second.empty()) {
        return it->second[0];  // Return the first file containing this symbol
    }
    return "";
}

std::string ContextManager::formatCodeSnippet(const std::string& filePath, int startLine, int endLine) const {
    std::string fileContent = getFile(filePath);
    if (fileContent.empty()) {
        return "";
    }
    
    // Split into lines
    std::istringstream iss(fileContent);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    // Validate line numbers
    if (startLine < 0) startLine = 0;
    if (endLine >= static_cast<int>(lines.size())) endLine = lines.size() - 1;
    if (startLine > endLine) return "";
    
    // Extract the snippet
    std::stringstream snippet;
    for (int i = startLine; i <= endLine; i++) {
        snippet << lines[i] << "\n";
    }
    
    return snippet.str();
}

}} // namespace codelve::core