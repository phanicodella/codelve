// E:\codelve\src\utils\logger.h
#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <mutex>

namespace codelve {
namespace utils {

// Log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// Simple logging utility
class Logger {
public:
    // Initialize the logger
    static bool initialize(const std::string& logDir);
    
    // Log a message
    static void log(LogLevel level, const std::string& message);
    
    // Set the minimum log level
    static void setLogLevel(LogLevel level);
    
    // Set the minimum log level from string
    static void setLogLevel(const std::string& level);
    
    // Convenience logging methods
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void fatal(const std::string& message);

private:
    static LogLevel minLevel_;
    static std::string logDir_;
    static std::string logFilePath_;
    static std::ofstream logFile_;
    static std::mutex mutex_;
    static bool initialized_;
    
    // Convert log level to string
    static std::string levelToString(LogLevel level);
    
    // Convert string to log level
    static LogLevel stringToLevel(const std::string& level);
};

// Convenience macros for logging
#define LOG_DEBUG(message) codelve::utils::Logger::debug(message)
#define LOG_INFO(message) codelve::utils::Logger::info(message)
#define LOG_WARNING(message) codelve::utils::Logger::warning(message)
#define LOG_ERROR(message) codelve::utils::Logger::error(message)
#define LOG_FATAL(message) codelve::utils::Logger::fatal(message)

}} // namespace codelve::utils