// E:\CodeLve\src\utils\logger.cpp
#include "logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace codelve {
    namespace utils {
        Logger::Logger() : minLevel_(LogLevel::INFO) {
        }

        Logger::~Logger() {
            if (logFile_.is_open()) {
                logFile_.close();
            }
        }

        Logger& Logger::getInstance() {
            static Logger instance;
            return instance;
        }

        void Logger::setLogFile(const std::string& logFile) {
            std::lock_guard<std::mutex> lock(logMutex_);
            if (logFile_.is_open()) {
                logFile_.close();
            }
            logFilePath_ = logFile;
            logFile_.open(logFile, std::ios::app);
            if (!logFile_.is_open()) {
                std::cerr << "Failed to open log file: " << logFile << std::endl;
            }
        }

        void Logger::setLogLevel(LogLevel minLevel) {
            minLevel_ = minLevel;
        }

        void Logger::setLogLevel(const std::string& level) {
            LogLevel logLevel = LogLevel::INFO;  // Default

            std::string levelLower = level;
            std::transform(levelLower.begin(), levelLower.end(), levelLower.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (levelLower == "debug") {
                logLevel = LogLevel::DEBUG;
            }
            else if (levelLower == "info") {
                logLevel = LogLevel::INFO;
            }
            else if (levelLower == "warning" || levelLower == "warn") {
                logLevel = LogLevel::WARNING;
            }
            else if (levelLower == "error") {
                logLevel = LogLevel::ERROR;
            }
            else if (levelLower == "fatal") {
                logLevel = LogLevel::FATAL;
            }

            getInstance().setLogLevel(logLevel);
        }

        void Logger::log(LogLevel level, const std::string& message) {
            if (level < minLevel_) {
                return;
            }
            std::lock_guard<std::mutex> lock(logMutex_);
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "]";
            ss << " [" << levelToString(level) << "] " << message;
            std::string logMessage = ss.str();
            std::cout << logMessage << std::endl;
            if (logFile_.is_open()) {
                logFile_ << logMessage << std::endl;
                logFile_.flush();
            }
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

        // Static method implementations
        void Logger::staticLog(LogLevel level, const std::string& message) {
            getInstance().log(level, message);
        }

        void Logger::staticDebug(const std::string& message) {
            getInstance().debug(message);
        }

        void Logger::staticInfo(const std::string& message) {
            getInstance().info(message);
        }

        void Logger::staticWarning(const std::string& message) {
            getInstance().warning(message);
        }

        void Logger::staticError(const std::string& message) {
            getInstance().error(message);
        }

        void Logger::staticFatal(const std::string& message) {
            getInstance().fatal(message);
        }
    } // namespace utils
} // namespace codelve
