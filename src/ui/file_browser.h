// E:\codelve\src\ui\file_browser.h
#pragma once
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <windows.h>
#include <commctrl.h>

namespace codelve {
namespace utils {
    class Config;
}
namespace ui {

/**
 * Callback for file selection
 */
using FileSelectionCallback = std::function<void(const std::string& filePath)>;

/**
 * Browser for navigating and selecting files in the codebase
 */
class FileBrowser {
public:
    /**
     * Constructor.
     * @param parentWindow Handle to parent window
     * @param config Shared configuration
     */
    FileBrowser(HWND parentWindow, std::shared_ptr<utils::Config> config);
    
    /**
     * Destructor.
     */
    ~FileBrowser();
    
    /**
     * Initialize the file browser.
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Resize and reposition the browser.
     * @param x Left position
     * @param y Top position
     * @param width Width
     * @param height Height
     */
    void resize(int x, int y, int width, int height);
    
    /**
     * Set the root directory for browsing.
     * @param rootPath Path to the root directory
     * @return true if the path was set successfully, false otherwise
     */
    bool setRootDirectory(const std::string& rootPath);
    
    /**
     * Get the currently selected file path.
     * @return Selected file path or empty string if none selected
     */
    std::string getSelectedFile() const;
    
    /**
     * Set file selection callback.
     * @param callback The callback function
     */
    void setFileSelectionCallback(FileSelectionCallback callback);
    
    /**
     * Refresh the file tree view.
     */
    void refresh();

private:
    // Parent window handle
    HWND parentWindow_;
    
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Control handles
    HWND browserContainer_;
    HWND pathLabel_;
    HWND treeView_;
    HWND openButton_;
    HWND refreshButton_;
    
    // Current directory
    std::string rootDirectory_;
    
    // File selection callback
    FileSelectionCallback fileSelectionCallback_;
    
    // Tree view image list
    HIMAGELIST imageList_;
    
    // Icon indices
    int folderIconIndex_;
    int fileIconIndex_;
    int codeFileIconIndex_;
    
    // Internal methods
    void createControls();
    void populateTreeView();
    void addDirectoryToTree(HTREEITEM parentItem, const std::string& path);
    int getIconIndexForFile(const std::string& filePath);
    std::string getItemPath(HTREEITEM item);
    void handleItemSelect(HTREEITEM item);
    void handleOpenButtonClick();
    
    // Window procedure for the browser container
    static LRESULT CALLBACK BrowserProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Window class name
    static constexpr const char* browserClassName_ = "CodeLveFileBrowser";
};

}} // namespace codelve::ui