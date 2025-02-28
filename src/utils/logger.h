// E:\CodeLve\src\utils\logger.h
#pragma once

// Undef Windows ERROR macro if it's defined
#ifdef ERROR
#undef ERROR
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <algorithm>

namespace codelve {
    namespace utils {
        // Define log levels
        enum class LogLevel {
            DEBUG,
            INFO,
            WARNING,
            ERROR,
            FATAL
        };

        class Logger {
        public:
            static Logger& getInstance();

            // Core logging methods
            void log(LogLevel level, const std::string& message);
            void setLogFile(const std::string& logFile);
            void setLogLevel(LogLevel minLevel);

            // Static versions
            static void setLogLevel(const std::string& level);
            static bool initialize(const std::string& logDir);

            // Instance convenience methods
            void debug(const std::string& message);
            void info(const std::string& message);
            void warning(const std::string& message);
            void error(const std::string& message);
            void fatal(const std::string& message);

            // Static convenience methods
            static void staticLog(LogLevel level, const std::string& message);
            static void staticDebug(const std::string& message);
            static void staticInfo(const std::string& message);
            static void staticWarning(const std::string& message);
            static void staticError(const std::string& message);
            static void staticFatal(const std::string& message);

        private:
            // Private constructor and destructor for singleton
            Logger();
            ~Logger();

            // Member variables
            std::mutex logMutex_;
            std::ofstream logFile_;
            std::string logFilePath_;
            LogLevel minLevel_;

            // Helper methods
            std::string levelToString(LogLevel level);
        };

        // Macro helpers for logging
#define LOG_DEBUG(msg) codelve::utils::Logger::staticDebug(msg)
#define LOG_INFO(msg) codelve::utils::Logger::staticInfo(msg)
#define LOG_WARNING(msg) codelve::utils::Logger::staticWarning(msg)
#define LOG_ERROR(msg) codelve::utils::Logger::staticError(msg)
#define LOG_FATAL(msg) codelve::utils::Logger::staticFatal(msg)
    } // namespace utils
} // namespace codelve
