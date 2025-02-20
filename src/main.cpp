// E:\codelve\src\main.cpp
#include "core/engine.h"
#include "utils/logger.h"
#include <windows.h>
#include <string>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

// Application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Unused parameters
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    
    try {
        // Initialize COM for shell operations
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        
        // Get application directory
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        fs::path appDir = fs::path(exePath).parent_path();
        
        // Set config path
        fs::path configPath = appDir / "config" / "codelve.cfg";
        
        // Initialize logger
        codelve::utils::Logger::initialize((appDir / "logs").string());
        codelve::utils::Logger::log(codelve::utils::LogLevel::INFO, "Application starting");
        
        // Create and initialize engine
        codelve::core::Engine engine(configPath.string());
        if (!engine.initialize()) {
            codelve::utils::Logger::log(codelve::utils::LogLevel::ERROR, "Failed to initialize engine");
            MessageBoxA(NULL, "Failed to initialize application. Please check the logs.", "Error", MB_OK | MB_ICONERROR);
            return 1;
        }
        
        // Run the application
        int result = engine.run();
        
        // Clean up
        CoUninitialize();
        
        codelve::utils::Logger::log(codelve::utils::LogLevel::INFO, "Application exiting with code: " + std::to_string(result));
        return result;
        
    } catch (const std::exception& e) {
        std::string errorMsg = "Unhandled exception: " + std::string(e.what());
        codelve::utils::Logger::log(codelve::utils::LogLevel::FATAL, errorMsg);
        MessageBoxA(NULL, errorMsg.c_str(), "Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    } catch (...) {
        std::string errorMsg = "Unknown unhandled exception";
        codelve::utils::Logger::log(codelve::utils::LogLevel::FATAL, errorMsg);
        MessageBoxA(NULL, errorMsg.c_str(), "Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}