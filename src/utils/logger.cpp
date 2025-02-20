// E:\codelve\src\utils\logger.cpp
#include "logger.h"
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace fs = std::filesystem;

namespace codelve {
namespace utils {

// Static member initialization
LogLevel Logger::minLevel_ = LogLevel::INFO;
std::string Logger::logDir_ = "./logs";
std::string Logger::logFilePath_ = "";
std::ofstream Logger::logFile_;
std::mutex Logger::mutex_;
bool Logger::initialized_ = false;

bool Logger::initialize(const std::string& logDir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return true;
    }
    
    logDir_ = logDir;
    
    try {
        // Create log directory if it doesn't exist
        if (!fs::exists(logDir_)) {
            fs::create_directories(logDir_);
        }
        
        // Generate log file name with timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream timestamp;
        timestamp << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S");
        
        std::string filename = "codelve_" + timestamp.str() + ".log";
        logFilePath_ = (fs::path(logDir_) / filename).string();
        
        // Open log file
        logFile_.open(logFilePath_, std::ios::out | std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file: " << logFilePath_ << std::endl;
            return false;
        }
        
        initialized_ = true;
        
        // Log initialization
        log(LogLevel::INFO, "Logger initialized. Log file: " + logFilePath_);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Logger initialization error: " << e.what() << std::endl;
        return false;
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < minLevel_) {
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Initialize logger if not already initialized
        if (!initialized_) {
            initialize(logDir_);
        }
        
        // Format log message
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream timestamp;
        timestamp << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        
        std::string levelStr = levelToString(level);
        std::string formattedMessage = timestamp.str() + " [" + levelStr + "] " + message;
        
        // Write to log file
        if (logFile_.is_open()) {
            logFile_ << formattedMessage << std::endl;
            logFile_.flush();
        }
        
        // Also write to console
        std::cout << formattedMessage << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Logging error: " << e.what() << std::endl;
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = level;
}

void Logger::setLogLevel(const std::string& level) {
    setLogLevel(stringToLevel(level));
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

LogLevel Logger::stringToLevel(const std::string& level) {
    if (level == "DEBUG" || level == "debug") {
        return LogLevel::DEBUG;
    } else if (level == "INFO" || level == "info") {
        return LogLevel::INFO;
    } else if (level == "WARNING" || level == "warning") {
        return LogLevel::WARNING;
    } else if (level == "ERROR" || level == "error") {
        return LogLevel::ERROR;
    } else if (level == "FATAL" || level == "fatal") {
        return LogLevel::FATAL;
    } else {
        return LogLevel::INFO;  // Default to INFO
    }
}

}} // namespace codelve::utils