// E:\codelve\src\ui\file_browser.cpp
#include "file_browser.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <filesystem>
#include <algorithm>

// Link shlwapi library
#pragma comment(lib, "shlwapi.lib")

namespace fs = std::filesystem;

namespace codelve {
namespace ui {

// Control IDs
#define ID_PATH_LABEL    2001
#define ID_TREE_VIEW     2002
#define ID_OPEN_BTN      2003
#define ID_REFRESH_BTN   2004

// Image list indices
#define IDX_FOLDER       0
#define IDX_FILE         1
#define IDX_CODE_FILE    2

// Static member initialization
LRESULT CALLBACK FileBrowser::BrowserProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    FileBrowser* browser = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        // Store the FileBrowser instance when the window is being created
        CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        browser = reinterpret_cast<FileBrowser*>(create->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(browser));
    } else {
        // Retrieve the stored FileBrowser instance
        browser = reinterpret_cast<FileBrowser*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (browser) {
        switch (uMsg) {
            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                    case ID_OPEN_BTN:
                        if (HIWORD(wParam) == BN_CLICKED) {
                            browser->handleOpenButtonClick();
                            return 0;
                        }
                        break;
                        
                    case ID_REFRESH_BTN:
                        if (HIWORD(wParam) == BN_CLICKED) {
                            browser->refresh();
                            return 0;
                        }
                        break;
                }
                break;
                
            case WM_NOTIFY:
                {
                    NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
                    if (nmhdr->idFrom == ID_TREE_VIEW) {
                        if (nmhdr->code == TVN_SELCHANGED) {
                            NMTREEVIEW* nmTreeView = reinterpret_cast<NMTREEVIEW*>(lParam);
                            browser->handleItemSelect(nmTreeView->itemNew.hItem);
                            return 0;
                        } else if (nmhdr->code == NM_DBLCLK) {
                            browser->handleOpenButtonClick();
                            return 0;
                        }
                    }
                }
                break;
                
            case WM_SIZE:
                // Resize the child controls
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                
                // Position path label (top)
                SetWindowPos(browser->pathLabel_, NULL, 10, 10, 
                           width - 20, 20, 
                           SWP_NOZORDER);
                
                // Position tree view (middle)
                SetWindowPos(browser->treeView_, NULL, 10, 40, 
                           width - 20, height - 80, 
                           SWP_NOZORDER);
                
                // Position buttons (bottom)
                SetWindowPos(browser->openButton_, NULL, 10, height - 30, 
                           100, 25, 
                           SWP_NOZORDER);
                
                SetWindowPos(browser->refreshButton_, NULL, 120, height - 30, 
                           100, 25, 
                           SWP_NOZORDER);
                
                return 0;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

FileBrowser::FileBrowser(HWND parentWindow, std::shared_ptr<utils::Config> config)
    : parentWindow_(parentWindow),
      config_(config),
      browserContainer_(nullptr),
      pathLabel_(nullptr),
      treeView_(nullptr),
      openButton_(nullptr),
      refreshButton_(nullptr),
      imageList_(nullptr),
      folderIconIndex_(0),
      fileIconIndex_(1),
      codeFileIconIndex_(2) {
    utils::Logger::log(utils::LogLevel::INFO, "FileBrowser: Creating");
}

FileBrowser::~FileBrowser() {
    utils::Logger::log(utils::LogLevel::INFO, "FileBrowser: Destroying");
    
    // Clean up image list
    if (imageList_) {
        ImageList_Destroy(imageList_);
    }
    
    // The windows will be destroyed when the parent window is destroyed
}

bool FileBrowser::initialize() {
    // Register window class for the browser container
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = BrowserProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = browserClassName_;
    wcex.hIconSm = NULL;
    
    if (!RegisterClassEx(&wcex)) {
        utils::Logger::log(utils::LogLevel::ERROR, "FileBrowser: Failed to register browser class");
        return false;
    }
    
    // Create the browser container
    browserContainer_ = CreateWindowEx(
        0,
        browserClassName_,
        "File Browser",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 300, 200,
        parentWindow_,
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (!browserContainer_) {
        utils::Logger::log(utils::LogLevel::ERROR, "FileBrowser: Failed to create browser container");
        return false;
    }
    
    // Create the controls
    createControls();
    
    // Set root directory from config if available
    std::string defaultRoot = config_->getString("file_browser.default_directory", "");
    if (!defaultRoot.empty()) {
        setRootDirectory(defaultRoot);
    }
    
    utils::Logger::log(utils::LogLevel::INFO, "FileBrowser: Initialized");
    return true;
}

void FileBrowser::resize(int x, int y, int width, int height) {
    if (browserContainer_) {
        SetWindowPos(browserContainer_, NULL, x, y, width, height, SWP_NOZORDER);
    }
}

bool FileBrowser::setRootDirectory(const std::string& rootPath) {
    // Check if directory exists
    if (!fs::exists(rootPath) || !fs::is_directory(rootPath)) {
        utils::Logger::log(utils::LogLevel::ERROR, 
                         "FileBrowser: Invalid directory path: " + rootPath);
        return false;
    }
    
    rootDirectory_ = rootPath;
    
    // Update path label
    if (pathLabel_) {
        SetWindowText(pathLabel_, rootPath.c_str());
    }
    
    // Refresh the tree view
    populateTreeView();
    
    utils::Logger::log(utils::LogLevel::INFO, 
                     "FileBrowser: Set root directory to " + rootPath);
    
    return true;
}

std::string FileBrowser::getSelectedFile() const {
    if (!treeView_) {
        return "";
    }
    
    HTREEITEM selectedItem = TreeView_GetSelection(treeView_);
    if (selectedItem == NULL) {
        return "";
    }
    
    return getItemPath(selectedItem);
}

void FileBrowser::setFileSelectionCallback(FileSelectionCallback callback) {
    fileSelectionCallback_ = callback;
}

void FileBrowser::refresh() {
    if (!rootDirectory_.empty()) {
        populateTreeView();
    }
}

void FileBrowser::createControls() {
    // Create path label
    pathLabel_ = CreateWindowEx(
        0,
        "STATIC",
        "No directory selected",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 10, 280, 20,
        browserContainer_,
        (HMENU)ID_PATH_LABEL,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Create tree view
    treeView_ = CreateWindowEx(
        0,
        WC_TREEVIEW,
        "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | 
        TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        10, 40, 280, 100,
        browserContainer_,
        (HMENU)ID_TREE_VIEW,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Create image list for tree view
    imageList_ = ImageList_Create(16, 16, ILC_COLOR32, 3, 10);
    
    if (imageList_) {
        // Add standard system icons
        SHFILEINFO sfi;
        
        // Get folder icon
        SHGetFileInfo("C:\\", FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                     SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        folderIconIndex_ = ImageList_AddIcon(imageList_, sfi.hIcon);
        DestroyIcon(sfi.hIcon);
        
        // Get generic file icon
        SHGetFileInfo("dummy.txt", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
                     SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        fileIconIndex_ = ImageList_AddIcon(imageList_, sfi.hIcon);
        DestroyIcon(sfi.hIcon);
        
        // Get code file icon
        SHGetFileInfo("dummy.cpp", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
                     SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        codeFileIconIndex_ = ImageList_AddIcon(imageList_, sfi.hIcon);
        DestroyIcon(sfi.hIcon);
        
        // Assign image list to tree view
        TreeView_SetImageList(treeView_, imageList_, TVSIL_NORMAL);
    }
    
    // Create open button
    openButton_ = CreateWindowEx(
        0,
        "BUTTON",
        "Open",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 150, 100, 25,
        browserContainer_,
        (HMENU)ID_OPEN_BTN,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Create refresh button
    refreshButton_ = CreateWindowEx(
        0,
        "BUTTON",
        "Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        120, 150, 100, 25,
        browserContainer_,
        (HMENU)ID_REFRESH_BTN,
        GetModuleHandle(NULL),
        NULL
    );
}

void FileBrowser::populateTreeView() {
    if (!treeView_ || rootDirectory_.empty()) {
        return;
    }
    
    // Clear the tree view
    TreeView_DeleteAllItems(treeView_);
    
    // Add the root directory
    TV_INSERTSTRUCT tvis;
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvis.item.iImage = folderIconIndex_;
    tvis.item.iSelectedImage = folderIconIndex_;
    
    // Use the base name for display
    std::string baseName = fs::path(rootDirectory_).filename().string();
    if (baseName.empty()) {
        baseName = rootDirectory_;  // Use full path if base name is empty (e.g., for a drive)
    }
    
    tvis.item.pszText = const_cast<LPSTR>(baseName.c_str());
    tvis.item.lParam = reinterpret_cast<LPARAM>(new std::string(rootDirectory_));
    
    HTREEITEM rootItem = TreeView_InsertItem(treeView_, &tvis);
    
    // Populate the tree recursively (but only one level deep to start)
    addDirectoryToTree(rootItem, rootDirectory_);
    
    // Expand the root item
    TreeView_Expand(treeView_, rootItem, TVE_EXPAND);
}

void FileBrowser::addDirectoryToTree(HTREEITEM parentItem, const std::string& path) {
    try {
        // Get sorted entries
        std::vector<fs::directory_entry> dirEntries;
        std::vector<fs::directory_entry> fileEntries;
        
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                dirEntries.push_back(entry);
            } else if (entry.is_regular_file()) {
                fileEntries.push_back(entry);
            }
        }
        
        // Sort directories and files by name
        std::sort(dirEntries.begin(), dirEntries.end(), 
                 [](const fs::directory_entry& a, const fs::directory_entry& b) {
                     return a.path().filename() < b.path().filename();
                 });
        
        std::sort(fileEntries.begin(), fileEntries.end(), 
                 [](const fs::directory_entry& a, const fs::directory_entry& b) {
                     return a.path().filename() < b.path().filename();
                 });
        
        // First add directories
        for (const auto& entry : dirEntries) {
            // Skip hidden directories
            if (entry.path().filename().string()[0] == '.') {
                continue;
            }
            
            TV_INSERTSTRUCT tvis;
            tvis.hParent = parentItem;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
            tvis.item.iImage = folderIconIndex_;
            tvis.item.iSelectedImage = folderIconIndex_;
            tvis.item.cChildren = 1;  // Assume it has children for now
            
            std::string entryPath = entry.path().string();
            std::string displayName = entry.path().filename().string();
            
            tvis.item.pszText = const_cast<LPSTR>(displayName.c_str());
            tvis.item.lParam = reinterpret_cast<LPARAM>(new std::string(entryPath));
            
            TreeView_InsertItem(treeView_, &tvis);
        }
        
        // Then add files
        for (const auto& entry : fileEntries) {
            // Skip hidden files
            if (entry.path().filename().string()[0] == '.') {
                continue;
            }
            
            TV_INSERTSTRUCT tvis;
            tvis.hParent = parentItem;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
            
            // Determine icon based on file extension
            tvis.item.iImage = getIconIndexForFile(entry.path().string());
            tvis.item.iSelectedImage = tvis.item.iImage;
            
            std::string entryPath = entry.path().string();
            std::string displayName = entry.path().filename().string();
            
            tvis.item.pszText = const_cast<LPSTR>(displayName.c_str());
            tvis.item.lParam = reinterpret_cast<LPARAM>(new std::string(entryPath));
            
            TreeView_InsertItem(treeView_, &tvis);
        }
    } catch (const std::exception& e) {
        utils::Logger::log(utils::LogLevel::ERROR, 
                         "FileBrowser: Error populating tree: " + std::string(e.what()));
    }
}

int FileBrowser::getIconIndexForFile(const std::string& filePath) {
    // Get file extension
    std::string ext = fs::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Check if this is a code file
    static const std::vector<std::string> codeExtensions = {
        ".cpp", ".h", ".hpp", ".c", ".cs", ".java", ".py", ".js", ".ts",
        ".go", ".rs", ".php", ".rb", ".swift", ".kt", ".scala"
    };
    
    if (std::find(codeExtensions.begin(), codeExtensions.end(), ext) != codeExtensions.end()) {
        return codeFileIconIndex_;
    }
    
    return fileIconIndex_;
}

std::string FileBrowser::getItemPath(HTREEITEM item) const {
    if (!treeView_ || item == NULL) {
        return "";
    }
    
    TV_ITEM tvi;
    tvi.mask = TVIF_PARAM;
    tvi.hItem = item;
    
    if (TreeView_GetItem(treeView_, &tvi)) {
        if (tvi.lParam != 0) {
            std::string* path = reinterpret_cast<std::string*>(tvi.lParam);
            return *path;
        }
    }
    
    return "";
}

void FileBrowser::handleItemSelect(HTREEITEM item) {
    std::string path = getItemPath(item);
    
    // If this is a directory, we may want to expand it
    if (!path.empty() && fs::is_directory(path)) {
        // Check if we need to populate this directory
        TVITEM tvi;
        tvi.mask = TVIF_CHILDREN;
        tvi.hItem = item;
        
        if (TreeView_GetItem(treeView_, &tvi) && tvi.cChildren == 1) {
            // This directory has a dummy child item - expand it
            // Check if it has real children first
            HTREEITEM childItem = TreeView_GetChild(treeView_, item);
            if (childItem != NULL) {
                // Get the text of the child item
                char buffer[MAX_PATH];
                TVITEM ctvItem;
                ctvItem.mask = TVIF_TEXT | TVIF_PARAM;
                ctvItem.hItem = childItem;
                ctvItem.pszText = buffer;
                ctvItem.cchTextMax = MAX_PATH;
                
                if (TreeView_GetItem(treeView_, &ctvItem)) {
                    // If this is just a placeholder, delete it and add real children
                    if (ctvItem.lParam == 0) {
                        TreeView_DeleteItem(treeView_, childItem);
                        addDirectoryToTree(item, path);
                    }
                }
            }
        }
    }
    
    // Notify about selection
    if (fileSelectionCallback_ && !path.empty() && fs::is_regular_file(path)) {
        fileSelectionCallback_(path);
    }
}

void FileBrowser::handleOpenButtonClick() {
    std::string selectedPath = getSelectedFile();
    if (!selectedPath.empty()) {
        if (fs::is_directory(selectedPath)) {
            // Set as new root
            setRootDirectory(selectedPath);
        } else if (fs::is_regular_file(selectedPath)) {
            // Notify about file selection
            if (fileSelectionCallback_) {
                fileSelectionCallback_(selectedPath);
            }
        }
    }
}

}} // namespace codelve::ui