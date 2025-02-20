// E:\codelve\src\scanner\scanner.cpp
#include "scanner.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace fs = std::filesystem;

namespace codelve {
namespace scanner {

Scanner::Scanner(std::shared_ptr<utils::Config> config)
    : config_(config),
      maxFileSize_(10 * 1024 * 1024),  // 10MB
      maxFileCount_(10000), 
	    maxLineCount_(10000) {  // Default to 10,000 lines
    
 
    
    // Initialize supported file extensions
    supportedExtensions_ = {
        ".cpp", ".h", ".hpp", ".c", ".cs", ".java", ".py", ".js", ".ts",
        ".go", ".rs", ".php", ".rb", ".swift", ".kt", ".scala"
    };
    
    // Load excluded directories
    excludeDirectories_ = {
        "node_modules", "build", "dist", "target", "bin", "obj",
        ".git", ".svn", ".hg", ".vs", ".idea", "venv", "__pycache__"
    };
    
      // Load configuration values
    maxFileSize_ = config_->getInt("scanner.max_file_size_bytes", 10 * 1024 * 1024);
    maxFileCount_ = config_->getInt("scanner.max_file_count", 10000);
    maxLineCount_ = config_->getInt("scanner.max_line_count", 10000);
    
    // Load custom supported extensions if configured
    std::string customExtensions = config_->getString("scanner.supported_extensions", "");
    if (!customExtensions.empty()) {
        supportedExtensions_.clear();
        std::istringstream iss(customExtensions);
        std::string ext;
        while (std::getline(iss, ext, ',')) {
            // Trim whitespace
            ext.erase(0, ext.find_first_not_of(" \t\n\r\f\v"));
            ext.erase(ext.find_last_not_of(" \t\n\r\f\v") + 1);
            // Add dot if missing
            if (!ext.empty() && ext[0] != '.') {
                ext = "." + ext;
            }
            if (!ext.empty()) {
                supportedExtensions_.push_back(ext);
            }
        }
    }
    
    // Load custom excluded directories if configured
    std::string customExcludes = config_->getString("scanner.exclude_directories", "");
    if (!customExcludes.empty()) {
        excludeDirectories_.clear();
        std::istringstream iss(customExcludes);
        std::string dir;
        while (std::getline(iss, dir, ',')) {
            // Trim whitespace
            dir.erase(0, dir.find_first_not_of(" \t\n\r\f\v"));
            dir.erase(dir.find_last_not_of(" \t\n\r\f\v") + 1);
            if (!dir.empty()) {
                excludeDirectories_.push_back(dir);
            }
        }
    }
    
    utils::Logger::log(utils::LogLevel::INFO, "Scanner: Initialized with " + 
                                               std::to_string(supportedExtensions_.size()) + 
                                               " supported extensions");
}

Scanner::~Scanner() {
    utils::Logger::log(utils::LogLevel::INFO, "Scanner: Destroyed");
}

void Scanner::setProgressCallback(ScanProgressCallback callback) {
    progressCallback_ = callback;
}

IndexedCode Scanner::scanDirectory(const std::string& directoryPath) {
    IndexedCode indexedCode;
    indexedCode.totalSize = 0;
    indexedCode.fileCount = 0;
    
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        utils::Logger::log(utils::LogLevel::ERROR, "Scanner: Invalid directory path: " + directoryPath);
        return indexedCode;
    }
    
    utils::Logger::log(utils::LogLevel::INFO, "Scanner: Starting scan of directory: " + directoryPath);
    
    try {
        // First pass: count files to scan
        size_t totalFiles = 0;
        for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
            if (entry.is_regular_file() && isRelevantFile(entry.path().string())) {
                totalFiles++;
            }
            
            // Limit scan to max file count
            if (totalFiles > maxFileCount_) {
                break;
            }
            
            // Check if the current directory should be excluded
            fs::path currentPath = entry.path();
            std::string dirName = currentPath.filename().string();
            if (entry.is_directory() && 
                std::find(excludeDirectories_.begin(), excludeDirectories_.end(), dirName) != excludeDirectories_.end()) {
                utils::Logger::log(utils::LogLevel::DEBUG, "Scanner: Skipping excluded directory: " + currentPath.string());
                continue;
            }
        }
        
        // Report initial progress
        if (progressCallback_) {
            progressCallback_("Counting files", 0.0f, "Found " + std::to_string(totalFiles) + " relevant files");
        }
        
        // Second pass: scan files
        size_t processedFiles = 0;
        for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
            // Check if the current directory should be excluded
            fs::path currentPath = entry.path();
            std::string dirName = currentPath.filename().string();
            if (entry.is_directory()) {
                // Add to directory list
                indexedCode.directories.push_back(currentPath.string());
                
                // Skip excluded directories
                if (std::find(excludeDirectories_.begin(), excludeDirectories_.end(), dirName) != excludeDirectories_.end()) {
                    utils::Logger::log(utils::LogLevel::DEBUG, "Scanner: Skipping excluded directory: " + currentPath.string());
                    continue;
                }
                continue;
            }
            
            if (!entry.is_regular_file() || !isRelevantFile(entry.path().string())) {
                continue;
            }
            
            // Check file size
            auto fileSize = entry.file_size();
            if (fileSize > maxFileSize_) {
                utils::Logger::log(utils::LogLevel::DEBUG, "Scanner: Skipping large file: " + entry.path().string());
                continue;
            }
            
            // Read file content
            std::ifstream file(entry.path(), std::ios::in);
            if (!file) {
                utils::Logger::log(utils::LogLevel::WARNING, "Scanner: Failed to open file: " + entry.path().string());
                continue;
            }
            
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
			// Count the number of lines in the file
size_t lineCount = std::count(content.begin(), content.end(), '\n') + 1;
if (lineCount > maxLineCount_) {
    utils::Logger::log(utils::LogLevel::DEBUG, 
        "Scanner: Skipping file with excessive line count (" + 
        std::to_string(lineCount) + " lines): " + entry.path().string());
    continue;
}
            
            // Add file to indexed code
            std::string filePath = entry.path().string();
            indexedCode.files[filePath] = content;
            
            // Add file extension to list if not already present
            std::string ext = fs::path(filePath).extension().string();
            if (!ext.empty() && 
                std::find(indexedCode.fileExtensions.begin(), indexedCode.fileExtensions.end(), ext) == indexedCode.fileExtensions.end()) {
                indexedCode.fileExtensions.push_back(ext);
            }
            
            // Parse file to extract symbols
            parseFile(filePath, content, indexedCode);
            
            // Update statistics
            indexedCode.totalSize += fileSize;
            indexedCode.fileCount++;
            processedFiles++;
            
            // Report progress
            if (progressCallback_) {
                float progress = static_cast<float>(processedFiles) / static_cast<float>(totalFiles);
                progressCallback_("Scanning files", progress, "Processed " + std::to_string(processedFiles) + " of " + std::to_string(totalFiles) + " files");
            }
            
            // Limit scan to max file count
            if (processedFiles >= maxFileCount_) {
                utils::Logger::log(utils::LogLevel::WARNING, "Scanner: Reached maximum file count limit");
                break;
            }
        }
        
        // Report completion
        if (progressCallback_) {
            progressCallback_("Scan complete", 1.0f, "Scanned " + std::to_string(processedFiles) + " files");
        }
        
        utils::Logger::log(utils::LogLevel::INFO, "Scanner: Completed scan of directory: " + directoryPath);
        utils::Logger::log(utils::LogLevel::INFO, "Scanner: Found " + std::to_string(indexedCode.fileCount) + " files with total size " + std::to_string(indexedCode.totalSize) + " bytes");
        
    } catch (const std::exception& e) {
        utils::Logger::log(utils::LogLevel::ERROR, "Scanner: Error scanning directory: " + std::string(e.what()));
    }
    
    return indexedCode;
}

bool Scanner::isRelevantFile(const std::string& filePath) const {
    // Get file extension
    std::string ext = fs::path(filePath).extension().string();
    
    // Convert to lowercase for case-insensitive comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Check if extension is in supported list
    return std::find(supportedExtensions_.begin(), supportedExtensions_.end(), ext) != supportedExtensions_.end();
}

std::vector<std::string> Scanner::getSupportedExtensions() const {
    return supportedExtensions_;
}

void Scanner::parseFile(const std::string& filePath, 
                      const std::string& content, 
                      IndexedCode& indexedCode) {
    // Determine file type from extension
    std::string ext = fs::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Extract symbols based on file type
    if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c") {
        parseCppFile(filePath, content, indexedCode);
    } else if (ext == ".py") {
        parsePythonFile(filePath, content, indexedCode);
    } else if (ext == ".js" || ext == ".ts") {
        parseJavaScriptFile(filePath, content, indexedCode);
    }
    // Add more file type parsers as needed
    
    // Extract general symbols
    auto symbols = extractSymbols(filePath, content, ext);
    
    // Add symbols to indexed code
    for (const auto& symbol : symbols) {
        // Add symbol to the file -> symbols mapping
        if (indexedCode.symbols.find(symbol.name) == indexedCode.symbols.end()) {
            indexedCode.symbols[symbol.name] = std::vector<std::string>();
        }
        
        // Add file to the symbol's file list if not already present
        auto& symbolFiles = indexedCode.symbols[symbol.name];
        if (std::find(symbolFiles.begin(), symbolFiles.end(), filePath) == symbolFiles.end()) {
            symbolFiles.push_back(filePath);
        }
        
        // Add detailed symbol info
        indexedCode.symbolDetails.push_back(symbol);
    }
}

void Scanner::parseCppFile(const std::string& filePath, 
                         const std::string& content, 
                         IndexedCode& indexedCode) {
    // This is a simplified implementation
    // A more robust implementation would use a proper C++ parser
    
    // Extract includes
    std::regex includeRegex(R"(#include\s*[<"]([^>"]+)[>"])");
    auto includeBegin = std::sregex_iterator(content.begin(), content.end(), includeRegex);
    auto includeEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = includeBegin; i != includeEnd; ++i) {
        std::smatch match = *i;
        std::string includePath = match[1].str();
        
        // Handle include as a special symbol type
        SymbolInfo symbol;
        symbol.name = includePath;
        symbol.type = "include";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        indexedCode.symbolDetails.push_back(symbol);
    }
    
    // Extract class definitions
    std::regex classRegex(R"(class\s+(\w+)(\s*:\s*\w+\s+\w+)?\s*\{)");
    auto classBegin = std::sregex_iterator(content.begin(), content.end(), classRegex);
    auto classEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = classBegin; i != classEnd; ++i) {
        std::smatch match = *i;
        std::string className = match[1].str();
        
        SymbolInfo symbol;
        symbol.name = className;
        symbol.type = "class";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        indexedCode.symbolDetails.push_back(symbol);
    }
    
    // Extract function definitions
    std::regex functionRegex(R"((\w+)\s+(\w+)\s*\([^)]*\)\s*(\{|;))");
    auto functionBegin = std::sregex_iterator(content.begin(), content.end(), functionRegex);
    auto functionEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = functionBegin; i != functionEnd; ++i) {
        std::smatch match = *i;
        std::string returnType = match[1].str();
        std::string functionName = match[2].str();
        
        // Skip if this is likely a variable declaration or keyword
        if (returnType == "if" || returnType == "for" || returnType == "while" || returnType == "switch") {
            continue;
        }
        
        SymbolInfo symbol;
        symbol.name = functionName;
        symbol.type = "function";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        indexedCode.symbolDetails.push_back(symbol);
    }
}

void Scanner::parsePythonFile(const std::string& filePath, 
                            const std::string& content, 
                            IndexedCode& indexedCode) {
    // This is a simplified implementation
    // A more robust implementation would use a proper Python parser
    
    // Extract imports
    std::regex importRegex(R"(import\s+(\w+)|from\s+(\w+)\s+import)");
    auto importBegin = std::sregex_iterator(content.begin(), content.end(), importRegex);
    auto importEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = importBegin; i != importEnd; ++i) {
        std::smatch match = *i;
        std::string importName = match[1].str();
        if (importName.empty()) {
            importName = match[2].str();
        }
        
        SymbolInfo symbol;
        symbol.name = importName;
        symbol.type = "import";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        indexedCode.symbolDetails.push_back(symbol);
    }
    
    // Extract class definitions
    std::regex classRegex(R"(class\s+(\w+)(\([^)]*\))?\s*:)");
    auto classBegin = std::sregex_iterator(content.begin(), content.end(), classRegex);
    auto classEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = classBegin; i != classEnd; ++i) {
        std::smatch match = *i;
        std::string className = match[1].str();
        
        SymbolInfo symbol;
        symbol.name = className;
        symbol.type = "class";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        indexedCode.symbolDetails.push_back(symbol);
    }
    
    // Extract function definitions
    std::regex functionRegex(R"(def\s+(\w+)\s*\([^)]*\)\s*:)");
    auto functionBegin = std::sregex_iterator(content.begin(), content.end(), functionRegex);
    auto functionEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = functionBegin; i != functionEnd; ++i) {
        std::smatch match = *i;
        std::string functionName = match[1].str();
        
        SymbolInfo symbol;
        symbol.name = functionName;
        symbol.type = "function";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        indexedCode.symbolDetails.push_back(symbol);
    }
}

void Scanner::parseJavaScriptFile(const std::string& filePath, 
                               const std::string& content, 
                               IndexedCode& indexedCode) {
    // This is a simplified implementation
    // A more robust implementation would use a proper JavaScript parser
    
    // Extract imports/requires
    std::regex importRegex(R"(import\s+.*?from\s+['"]([^'"]+)['"]|require\s*\(\s*['"]([^'"]+)['"]\s*\))");
    auto importBegin = std::sregex_iterator(content.begin(), content.end(), importRegex);
    auto importEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = importBegin; i != importEnd; ++i) {
        std::smatch match = *i;
        std::string importPath = match[1].str();
        if (importPath.empty()) {
            importPath = match[2].str();
        }
        
        SymbolInfo symbol;
        symbol.name = importPath;
        symbol.type = "import";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        indexedCode.symbolDetails.push_back(symbol);
    }
    
    // Extract class definitions
    std::regex classRegex(R"(class\s+(\w+)(\s+extends\s+\w+)?\s*\{)");
    auto classBegin = std::sregex_iterator(content.begin(), content.end(), classRegex);
    auto classEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = classBegin; i != classEnd; ++i) {
        std::smatch match = *i;
        std::string className = match[1].str();
        
        SymbolInfo symbol;
        symbol.name = className;
        symbol.type = "class";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        indexedCode.symbolDetails.push_back(symbol);
    }
    
    // Extract function definitions
    std::regex functionRegex(R"(function\s+(\w+)\s*\([^)]*\)|(\w+)\s*:\s*function\s*\([^)]*\)|(\w+)\s*=\s*function\s*\([^)]*\))");
    auto functionBegin = std::sregex_iterator(content.begin(), content.end(), functionRegex);
    auto functionEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = functionBegin; i != functionEnd; ++i) {
        std::smatch match = *i;
        std::string functionName;
        if (!match[1].str().empty()) {
            functionName = match[1].str();
        } else if (!match[2].str().empty()) {
            functionName = match[2].str();
        } else if (!match[3].str().empty()) {
            functionName = match[3].str();
        }
        
        if (!functionName.empty()) {
            SymbolInfo symbol;
            symbol.name = functionName;
            symbol.type = "function";
            symbol.filePath = filePath;
            symbol.lineNumber = 0;  // We'd need to calculate line number properly
            symbol.signature = match[0].str();
            
            indexedCode.symbolDetails.push_back(symbol);
        }
    }
    
    // Extract arrow functions
    std::regex arrowRegex(R"(const\s+(\w+)\s*=\s*\([^)]*\)\s*=>|let\s+(\w+)\s*=\s*\([^)]*\)\s*=>|var\s+(\w+)\s*=\s*\([^)]*\)\s*=>)");
    auto arrowBegin = std::sregex_iterator(content.begin(), content.end(), arrowRegex);
    auto arrowEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = arrowBegin; i != arrowEnd; ++i) {
        std::smatch match = *i;
        std::string functionName;
        if (!match[1].str().empty()) {
            functionName = match[1].str();
        } else if (!match[2].str().empty()) {
            functionName = match[2].str();
        } else if (!match[3].str().empty()) {
            functionName = match[3].str();
        }
        
        if (!functionName.empty()) {
            SymbolInfo symbol;
            symbol.name = functionName;
            symbol.type = "function";
            symbol.filePath = filePath;
            symbol.lineNumber = 0;  // We'd need to calculate line number properly
            symbol.signature = match[0].str();
            
            indexedCode.symbolDetails.push_back(symbol);
        }
    }
}

std::vector<SymbolInfo> Scanner::extractSymbols(const std::string& filePath, 
                                             const std::string& content, 
                                             const std::string& fileType) {
    // This is a simplified implementation that extracts some common symbols
    // regardless of language-specific parsing
    std::vector<SymbolInfo> symbols;
    
    // Extract TODOs
    std::regex todoRegex(R"(TODO\s*:?\s*(.*))");
    auto todoBegin = std::sregex_iterator(content.begin(), content.end(), todoRegex);
    auto todoEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = todoBegin; i != todoEnd; ++i) {
        std::smatch match = *i;
        std::string todoText = match[1].str();
        
        SymbolInfo symbol;
        symbol.name = "TODO";
        symbol.type = "comment";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        symbol.documentation = todoText;
        
        symbols.push_back(symbol);
    }
    
    // Extract constants/uppercase variables
    std::regex constRegex(R"(const\s+([A-Z][A-Z0-9_]*)\s*=|#define\s+([A-Z][A-Z0-9_]*))");
    auto constBegin = std::sregex_iterator(content.begin(), content.end(), constRegex);
    auto constEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = constBegin; i != constEnd; ++i) {
        std::smatch match = *i;
        std::string constName = match[1].str();
        if (constName.empty()) {
            constName = match[2].str();
        }
        
        SymbolInfo symbol;
        symbol.name = constName;
        symbol.type = "constant";
        symbol.filePath = filePath;
        symbol.lineNumber = 0;  // We'd need to calculate line number properly
        symbol.signature = match[0].str();
        
        symbols.push_back(symbol);
    }
    
    return symbols;
}

}} // namespace codelve::scanner