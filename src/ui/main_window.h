// E:\codelve\src\ui\main_window.h
#pragma once
#include <string>
#include <memory>
#include <functional>
#include <windows.h>

namespace codelve {
namespace utils {
    class Config;
}
namespace core {
    class Engine;
}
namespace ui {

// Forward declarations
class ChatPanel;
class FileBrowser;

/**
 * Callback for query submission from UI
 */
using QueryCallback = std::function<void(const std::string& query)>;

/**
 * Callback for file selection
 */
using FileSelectionCallback = std::function<void(const std::string& filePath)>;

/**
 * Main application window that contains all UI components
 */
class MainWindow {
public:
    /**
     * Constructor.
     * @param config Shared configuration
     * @param engine Shared engine instance
     */
    MainWindow(std::shared_ptr<utils::Config> config, std::shared_ptr<core::Engine> engine);
    
    /**
     * Destructor.
     */
    ~MainWindow();
    
    /**
     * Initialize the main window and UI components.
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Start the UI message loop.
     * @return Result code from message loop
     */
    int run();
    
    /**
     * Display a status message.
     * @param message The message to display
     * @param isError Whether this is an error message
     */
    void setStatusMessage(const std::string& message, bool isError = false);
    
    /**
     * Display response from LLM.
     * @param response The response text
     */
    void displayResponse(const std::string& response);
    
    /**
     * Set query submission callback.
     * @param callback The callback function
     */
    void setQueryCallback(QueryCallback callback);
    
    /**
     * Set file selection callback.
     * @param callback The callback function
     */
    void setFileSelectionCallback(FileSelectionCallback callback);
    
    /**
     * Show a progress dialog.
     * @param title Dialog title
     * @param message Initial message
     * @return Dialog handle
     */
    HWND showProgressDialog(const std::string& title, const std::string& message);
    
    /**
     * Update progress dialog.
     * @param dialogHandle Dialog handle
     * @param progress Progress value (0.0 - 1.0)
     * @param message New message
     */
    void updateProgressDialog(HWND dialogHandle, float progress, const std::string& message);
    
    /**
     * Close progress dialog.
     * @param dialogHandle Dialog handle
     */
    void closeProgressDialog(HWND dialogHandle);

    /**
     * Get the window handle.
     * @return Window handle
     */
    HWND getHandle() const;

private:
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Engine instance
    std::shared_ptr<core::Engine> engine_;
    
    // Windows handle
    HWND mainWindow_;
    HWND statusBar_;
    
    // UI components
    std::unique_ptr<ChatPanel> chatPanel_;
    std::unique_ptr<FileBrowser> fileBrowser_;
    
    // Callbacks
    QueryCallback queryCallback_;
    FileSelectionCallback fileSelectionCallback_;
    
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Helper methods
    void createMainWindow();
    void createStatusBar();
    void createUIComponents();
    void resizeComponents(int width, int height);
    
    // Internal message handler
    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Windows class name
    static constexpr const char* windowClassName_ = "CodeLveMainWindow";
};

}} // namespace codelve::ui