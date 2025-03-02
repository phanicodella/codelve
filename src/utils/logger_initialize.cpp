#include "logger.h"
#include <filesystem>
#include <direct.h>
#include <sstream>      // Add this for stringstream
#include <iomanip>      // Add this for put_time
#include <chrono>       // Add this for chrono functionality

namespace fs = std::filesystem;

namespace codelve {
    namespace utils {

        bool Logger::initialize(const std::string& logDir) {
            try {
                // Create log directory if it doesn't exist
                if (!fs::exists(logDir)) {
                    fs::create_directories(logDir);
                }

                // Generate log file name with timestamp
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                std::stringstream filenameSs;
                filenameSs << "codelve_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".log";

                std::string logPath = (fs::path(logDir) / filenameSs.str()).string();

                // Set the log file
                getInstance().setLogFile(logPath);
                getInstance().info("Logger initialized: " + logPath);
                return true;
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
                return false;
            }
        }

    }
} // namespace codelve::utils