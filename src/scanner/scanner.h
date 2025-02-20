// E:\codelve\src\scanner\scanner.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace codelve {
namespace utils {
    class Config;
}
namespace scanner {

/**
 * Stores information about a code symbol (function, class, variable, etc.)
 */
struct SymbolInfo {
    std::string name;
    std::string type;
    std::string filePath;
    int lineNumber;
    std::string signature;
    std::string documentation;
};

/**
 * Stores indexed code information for a codebase
 */
struct IndexedCode {
    // Map of file paths to file contents
    std::unordered_map<std::string, std::string> files;
    
    // Map of symbol names to files containing them
    std::unordered_map<std::string, std::vector<std::string>> symbols;
    
    // Detailed symbol information
    std::vector<SymbolInfo> symbolDetails;
    
    // Directory structure
    std::vector<std::string> directories;
    
    // File extensions present in the codebase
    std::vector<std::string> fileExtensions;
    
    // Total size of the codebase in bytes
    size_t totalSize;
    
    // Number of files
    size_t fileCount;
};

/**
 * Progress callback for the scanning process
 * @param stage The current scanning stage description
 * @param progress Value between 0 and 1 indicating progress
 * @param message Optional status message
 */
using ScanProgressCallback = std::function<void(const std::string& stage, 
                                              float progress, 
                                              const std::string& message)>;

/**
 * Scans and indexes codebase for analysis
 */
class Scanner {
public:
    /**
     * Constructor.
     * @param config Shared configuration
     */
    explicit Scanner(std::shared_ptr<utils::Config> config);
    
    /**
     * Destructor.
     */
    ~Scanner();
    
    /**
     * Set a progress callback for the scanning process.
     * @param callback The callback function
     */
    void setProgressCallback(ScanProgressCallback callback);
    
    /**
     * Scan a directory and build the indexed code structure.
     * @param directoryPath Path to the directory to scan
     * @return The indexed code structure
     */
    IndexedCode scanDirectory(const std::string& directoryPath);
    
    /**
     * Check if a file is relevant for code analysis.
     * @param filePath Path to the file
     * @return true if the file should be analyzed, false otherwise
     */
    bool isRelevantFile(const std::string& filePath) const;
    
    /**
     * Get the list of supported file extensions.
     * @return List of file extensions (e.g., ".cpp", ".py")
     */
    std::vector<std::string> getSupportedExtensions() const;
    
private:
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Callback for progress updates
    ScanProgressCallback progressCallback_;
    
    // Maximum file size to scan (in bytes)
    size_t maxFileSize_;
    
    // Maximum number of files to scan
size_t maxFileCount_;

// Maximum lines per file to scan
size_t maxLineCount_;
    
    // Supported file extensions
    std::vector<std::string> supportedExtensions_;
    
    // Directories to exclude from scanning
    std::vector<std::string> excludeDirectories_;
    
    // Methods for handling different file types
    void parseFile(const std::string& filePath, 
                  const std::string& content, 
                  IndexedCode& indexedCode);
    
    void parseCppFile(const std::string& filePath, 
                     const std::string& content, 
                     IndexedCode& indexedCode);
    
    void parsePythonFile(const std::string& filePath, 
                        const std::string& content, 
                        IndexedCode& indexedCode);
    
    void parseJavaScriptFile(const std::string& filePath, 
                           const std::string& content, 
                           IndexedCode& indexedCode);
    
    // Extract symbols from file content
    std::vector<SymbolInfo> extractSymbols(const std::string& filePath, 
                                         const std::string& content, 
                                         const std::string& fileType);
};

}} // namespace codelve::scanner