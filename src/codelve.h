// File: codelve/src/codelve.h
#pragma once

#include <memory>
#include <string>

namespace codelve {

// Forward declarations
namespace utils {
    class Config;
    class Logger;
}

namespace core {
    class Engine;
}

namespace ui {
    class MainWindow;
}

/**
 * Main application class for CodeLve.
 * Coordinates between all components of the application.
 */
class CodeLve {
public:
    /**
     * Constructor.
     * @param config Shared pointer to configuration
     */
    explicit CodeLve(std::shared_ptr<utils::Config> config);
    
    /**
     * Destructor.
     */
    ~CodeLve();
    
    /**
     * Initialize the application.
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Run the application main loop.
     * @return Exit code
     */
    int run();
    
    /**
     * Get the application version.
     * @return Version string
     */
    static std::string getVersion();

private:
    // Prevent copying
    CodeLve(const CodeLve&) = delete;
    CodeLve& operator=(const CodeLve&) = delete;
    
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Core engine
    std::unique_ptr<core::Engine> engine_;
    
    // UI
    std::unique_ptr<ui::MainWindow> mainWindow_;
};

} // namespace codelve