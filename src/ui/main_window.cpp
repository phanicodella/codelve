// E:\codelve\src\ui\main_window.cpp
#include "main_window.h"
#include "chat_panel.h"
#include "file_browser.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include "../core/engine.h"
#include <commctrl.h>
#include <windowsx.h>
#include <sstream>

// Link CommCtrl library
#pragma comment(lib, "comctl32.lib")

namespace codelve {
namespace ui {

// Static member initialization
LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* window = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        // Store the MainWindow instance when the window is being created
        CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<MainWindow*>(create->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        // Retrieve the stored MainWindow instance
        window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (window) {
        return window->handleMessage(hwnd, uMsg, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

MainWindow::MainWindow(std::shared_ptr<utils::Config> config, std::shared_ptr<core::Engine> engine)
    : config_(config),
      engine_(engine),
      mainWindow_(nullptr),
      statusBar_(nullptr) {
    utils::Logger::log(utils::LogLevel::INFO, "MainWindow: Creating");
}

MainWindow::~MainWindow() {
    utils::Logger::log(utils::LogLevel::INFO, "MainWindow: Destroying");
    
    if (mainWindow_) {
        DestroyWindow(mainWindow_);
    }
}

bool MainWindow::initialize() {
    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);
    
    // Register window class
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = windowClassName_;
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wcex)) {
        utils::Logger::log(utils::LogLevel::ERROR, "MainWindow: Failed to register window class");
        return false;
    }
    
    // Create main window
    createMainWindow();
    
    if (!mainWindow_) {
        utils::Logger::log(utils::LogLevel::ERROR, "MainWindow: Failed to create main window");
        return false;
    }
    
    // Create status bar
    createStatusBar();
    
    // Create UI components
    createUIComponents();
    
    utils::Logger::log(utils::LogLevel::INFO, "MainWindow: Initialized");
    return true;
}

int MainWindow::run() {
    // Show the window
    ShowWindow(mainWindow_, SW_SHOW);
    UpdateWindow(mainWindow_);
    
    // Run message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return static_cast<int>(msg.wParam);
}

void MainWindow::setStatusMessage(const std::string& message, bool isError) {
    if (statusBar_) {
        // Set status bar text
        SendMessage(statusBar_, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(message.c_str()));
        
        // Set color for error messages
        if (isError) {
            SendMessage(statusBar_, SB_SETBKCOLOR, 0, RGB(255, 200, 200));
        } else {
            SendMessage(statusBar_, SB_SETBKCOLOR, 0, GetSysColor(COLOR_BTNFACE));
        }
    }
    
    utils::Logger::log(isError ? utils::LogLevel::ERROR : utils::LogLevel::INFO, 
                     "MainWindow: Status - " + message);
}

void MainWindow::displayResponse(const std::string& response) {
    if (chatPanel_) {
        chatPanel_->addResponse(response);
    }
}

void MainWindow::setQueryCallback(QueryCallback callback) {
    queryCallback_ = callback;
    
    // Pass the callback to the chat panel
    if (chatPanel_) {
        chatPanel_->setQueryCallback(callback);
    }
}

void MainWindow::setFileSelectionCallback(FileSelectionCallback callback) {
    fileSelectionCallback_ = callback;
    
    // Pass the callback to the file browser
    if (fileBrowser_) {
        fileBrowser_->setFileSelectionCallback(callback);
    }
}

HWND MainWindow::showProgressDialog(const std::string& title, const std::string& message) {
    // Create a modeless dialog for progress
    // In a real implementation, this would be a proper dialog with a progress bar
    // For now, we'll just create a simple window
    
    HWND progressDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        "STATIC",
        title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 150,
        mainWindow_,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (progressDialog) {
        // Create a static text control for the message
        HWND textControl = CreateWindowEx(
            0,
            "STATIC",
            message.c_str(),
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 10,
            380, 50,
            progressDialog,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        // Create a progress bar
        HWND progressBar = CreateWindowEx(
            0,
            PROGRESS_CLASS,
            NULL,
            WS_CHILD | WS_VISIBLE,
            10, 70,
            380, 25,
            progressDialog,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        // Set progress bar range (0-100)
        SendMessage(progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        
        // Store handles in window properties for later use
        SetProp(progressDialog, "TextControl", textControl);
        SetProp(progressDialog, "ProgressBar", progressBar);
        
        // Show the dialog
        ShowWindow(progressDialog, SW_SHOW);
        UpdateWindow(progressDialog);
    }
    
    return progressDialog;
}

void MainWindow::updateProgressDialog(HWND dialogHandle, float progress, const std::string& message) {
    if (dialogHandle && IsWindow(dialogHandle)) {
        // Get control handles
        HWND textControl = (HWND)GetProp(dialogHandle, "TextControl");
        HWND progressBar = (HWND)GetProp(dialogHandle, "ProgressBar");
        
        // Update message
        if (textControl) {
            SetWindowText(textControl, message.c_str());
        }
        
        // Update progress bar
        if (progressBar) {
            int progressInt = static_cast<int>(progress * 100.0f);
            SendMessage(progressBar, PBM_SETPOS, progressInt, 0);
        }
    }
}

void MainWindow::closeProgressDialog(HWND dialogHandle) {
    if (dialogHandle && IsWindow(dialogHandle)) {
        // Remove properties
        RemoveProp(dialogHandle, "TextControl");
        RemoveProp(dialogHandle, "ProgressBar");
        
        // Destroy the dialog
        DestroyWindow(dialogHandle);
    }
}

HWND MainWindow::getHandle() const {
    return mainWindow_;
}

void MainWindow::createMainWindow() {
    // Get window size from config
    int width = config_->getInt("ui.window_width", 1024);
    int height = config_->getInt("ui.window_height", 768);
    
    // Get window title from config
    std::string windowTitle = config_->getString("ui.window_title", "CodeLve - Offline Code Analysis");
    
    // Create the main window
    mainWindow_ = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        windowClassName_,
        windowTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (mainWindow_) {
        utils::Logger::log(utils::LogLevel::INFO, "MainWindow: Created main window");
    } else {
        utils::Logger::log(utils::LogLevel::ERROR, "MainWindow: Failed to create main window");
    }
}

void MainWindow::createStatusBar() {
    // Create status bar
    statusBar_ = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        mainWindow_,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (statusBar_) {
        // Set default status text
        setStatusMessage("Ready");
    } else {
        utils::Logger::log(utils::LogLevel::ERROR, "MainWindow: Failed to create status bar");
    }
}

void MainWindow::createUIComponents() {
    // Get client rect
    RECT clientRect;
    GetClientRect(mainWindow_, &clientRect);
    
    // Calculate dimensions
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    
    // Create file browser component
    fileBrowser_ = std::make_unique<FileBrowser>(mainWindow_, config_);
    if (fileBrowser_->initialize()) {
        utils::Logger::log(utils::LogLevel::INFO, "MainWindow: Created file browser");
    } else {
        utils::Logger::log(utils::LogLevel::ERROR, "MainWindow: Failed to create file browser");
    }
    
    // Create chat panel component
    chatPanel_ = std::make_unique<ChatPanel>(mainWindow_, config_);
    if (chatPanel_->initialize()) {
        utils::Logger::log(utils::LogLevel::INFO, "MainWindow: Created chat panel");
    } else {
        utils::Logger::log(utils::LogLevel::ERROR, "MainWindow: Failed to create chat panel");
    }
    
    // Position the components
    resizeComponents(width, height);
}

void MainWindow::resizeComponents(int width, int height) {
    // Get status bar height
    RECT statusRect;
    GetWindowRect(statusBar_, &statusRect);
    int statusHeight = statusRect.bottom - statusRect.top;
    
    // Calculate working area height
    int workingHeight = height - statusHeight;
    
    // Set file browser size and position (left side, 30% width)
    int fileBrowserWidth = width * 0.3;
    if (fileBrowser_) {
        fileBrowser_->resize(0, 0, fileBrowserWidth, workingHeight);
    }
    
    // Set chat panel size and position (right side, 70% width)
    if (chatPanel_) {
        chatPanel_->resize(fileBrowserWidth, 0, width - fileBrowserWidth, workingHeight);
    }
    
    // Resize status bar
    SendMessage(statusBar_, WM_SIZE, 0, 0);
}

LRESULT MainWindow::handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            return 0;
            
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                resizeComponents(width, height);
            }
            return 0;
            
        case WM_COMMAND:
            // Handle commands from child controls
            // We'll implement specific handling as needed
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

}} // namespace codelve::ui