// E:\codelve\src\ui\chat_panel.cpp
#include "chat_panel.h"
#include "../utils/config.h"
#include "../utils/logger.h"
#include <windowsx.h>
#include <richedit.h>
#include <sstream>
#include <regex>

// Link richedit library
#pragma comment(lib, "riched20.lib")

namespace codelve {
namespace ui {

// Control IDs
#define ID_CHAT_HISTORY 1001
#define ID_QUERY_INPUT  1002
#define ID_SUBMIT_BTN   1003
#define ID_CLEAR_BTN    1004

// Static member initialization
LRESULT CALLBACK ChatPanel::PanelProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ChatPanel* panel = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        // Store the ChatPanel instance when the window is being created
        CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        panel = reinterpret_cast<ChatPanel*>(create->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
    } else {
        // Retrieve the stored ChatPanel instance
        panel = reinterpret_cast<ChatPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (panel) {
        switch (uMsg) {
            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                    case ID_SUBMIT_BTN:
                        if (HIWORD(wParam) == BN_CLICKED) {
                            panel->handleSubmitQuery();
                            return 0;
                        }
                        break;
                        
                    case ID_CLEAR_BTN:
                        if (HIWORD(wParam) == BN_CLICKED) {
                            panel->clearChat();
                            return 0;
                        }
                        break;
                }
                break;
                
            case WM_SIZE:
                // Resize the child controls
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                
                // Position chat history (top area, 70% height)
                int chatHistoryHeight = static_cast<int>(height * 0.7);
                SetWindowPos(panel->chatHistory_, NULL, 0, 0, 
                            width, chatHistoryHeight, 
                            SWP_NOZORDER);
                
                // Position query input (bottom area, 20% height)
                int queryInputHeight = static_cast<int>(height * 0.2);
                int queryInputTop = chatHistoryHeight + 5;
                SetWindowPos(panel->queryInput_, NULL, 0, queryInputTop, 
                            width - 180, queryInputHeight, 
                            SWP_NOZORDER);
                
                // Position submit button (bottom right)
                SetWindowPos(panel->submitButton_, NULL, width - 170, queryInputTop, 
                            80, 30, 
                            SWP_NOZORDER);
                
                // Position clear button (bottom right, next to submit)
                SetWindowPos(panel->clearButton_, NULL, width - 80, queryInputTop, 
                            80, 30, 
                            SWP_NOZORDER);
                
                return 0;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

ChatPanel::ChatPanel(HWND parentWindow, std::shared_ptr<utils::Config> config)
    : parentWindow_(parentWindow),
      config_(config),
      panelContainer_(nullptr),
      chatHistory_(nullptr),
      queryInput_(nullptr),
      submitButton_(nullptr),
      clearButton_(nullptr) {
    utils::Logger::log(utils::LogLevel::INFO, "ChatPanel: Creating");
}

ChatPanel::~ChatPanel() {
    utils::Logger::log(utils::LogLevel::INFO, "ChatPanel: Destroying");
    
    // The windows will be destroyed when the parent window is destroyed
}

bool ChatPanel::initialize() {
    // Register window class for the panel container
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PanelProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = panelClassName_;
    wcex.hIconSm = NULL;
    
    if (!RegisterClassEx(&wcex)) {
        utils::Logger::log(utils::LogLevel::ERROR, "ChatPanel: Failed to register panel class");
        return false;
    }
    
    // Create the panel container
    panelContainer_ = CreateWindowEx(
        0,
        panelClassName_,
        "Chat Panel",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 300, 200,
        parentWindow_,
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (!panelContainer_) {
        utils::Logger::log(utils::LogLevel::ERROR, "ChatPanel: Failed to create panel container");
        return false;
    }
    
    // Create the controls
    createControls();
    
    utils::Logger::log(utils::LogLevel::INFO, "ChatPanel: Initialized");
    return true;
}

void ChatPanel::resize(int x, int y, int width, int height) {
    if (panelContainer_) {
        SetWindowPos(panelContainer_, NULL, x, y, width, height, SWP_NOZORDER);
    }
}

void ChatPanel::addQuery(const std::string& query) {
    ChatEntry entry;
    entry.isUser = true;
    entry.text = query;
    chatEntries_.push_back(entry);
    
    updateChatHistory();
}

void ChatPanel::addResponse(const std::string& response) {
    ChatEntry entry;
    entry.isUser = false;
    entry.text = response;
    chatEntries_.push_back(entry);
    
    updateChatHistory();
}

void ChatPanel::clearChat() {
    chatEntries_.clear();
    updateChatHistory();
    setQueryText("");
}

void ChatPanel::setQueryCallback(QueryCallback callback) {
    queryCallback_ = callback;
}

std::string ChatPanel::getQueryText() {
    if (!queryInput_) {
        return "";
    }
    
    int length = GetWindowTextLength(queryInput_);
    if (length == 0) {
        return "";
    }
    
    std::vector<char> buffer(length + 1);
    GetWindowText(queryInput_, buffer.data(), length + 1);
    
    return std::string(buffer.data());
}

void ChatPanel::setQueryText(const std::string& text) {
    if (queryInput_) {
        SetWindowText(queryInput_, text.c_str());
    }
}

void ChatPanel::createControls() {
    // Load the rich edit control
    LoadLibrary("Riched20.dll");
    
    // Create chat history rich edit control
    chatHistory_ = CreateWindowEx(
        0,
        RICHEDIT_CLASS,
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | 
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0, 0, 100, 100,
        panelContainer_,
        (HMENU)ID_CHAT_HISTORY,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Create query input edit control
    queryInput_ = CreateWindowEx(
        0,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | 
        ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        0, 0, 100, 30,
        panelContainer_,
        (HMENU)ID_QUERY_INPUT,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Create submit button
    submitButton_ = CreateWindowEx(
        0,
        "BUTTON",
        "Submit",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 80, 30,
        panelContainer_,
        (HMENU)ID_SUBMIT_BTN,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Create clear button
    clearButton_ = CreateWindowEx(
        0,
        "BUTTON",
        "Clear",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 80, 30,
        panelContainer_,
        (HMENU)ID_CLEAR_BTN,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Set up rich edit formatting
    if (chatHistory_) {
        // Set default text format (font, size, etc.)
        CHARFORMAT2 cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_FACE | CFM_SIZE | CFM_EFFECTS;
        cf.dwEffects = 0;
        cf.yHeight = 220;  // 11pt (in twips)
        strcpy(cf.szFaceName, "Segoe UI");
        
        SendMessage(chatHistory_, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
        
        // Set background color
        SendMessage(chatHistory_, EM_SETBKGNDCOLOR, 0, RGB(250, 250, 250));
        
        // Set event mask for notifications
        SendMessage(chatHistory_, EM_SETEVENTMASK, 0, ENM_LINK);
    }
}

void ChatPanel::updateChatHistory() {
    if (!chatHistory_) {
        return;
    }
    
    // Clear current content
    SetWindowText(chatHistory_, "");
    
    // Use RichEdit to format the chat
    for (const auto& entry : chatEntries_) {
        // Format the text based on user or assistant
        CHARFORMAT2 cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_FACE | CFM_SIZE | CFM_EFFECTS | CFM_COLOR;
        cf.dwEffects = 0;
        cf.yHeight = 220;  // 11pt (in twips)
        
        if (entry.isUser) {
            // User formatting
            cf.crTextColor = RGB(0, 0, 150);  // Dark blue
            strcpy(cf.szFaceName, "Segoe UI");
            cf.dwEffects |= CFE_BOLD;  // Bold text for user
        } else {
            // Assistant formatting
            cf.crTextColor = RGB(0, 100, 0);  // Dark green
            strcpy(cf.szFaceName, "Segoe UI");
        }
        
        // Get current text length for insertion point
        GETTEXTLENGTHEX gtl;
        gtl.flags = GTL_NUMCHARS;
        gtl.codepage = 1200;
        int textLen = SendMessage(chatHistory_, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
        
        // Set selection at the end of the text
        SendMessage(chatHistory_, EM_SETSEL, textLen, textLen);
        
        // Set character format
        SendMessage(chatHistory_, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        
        // Insert the role header
        std::string header = entry.isUser ? "You: " : "CodeLve: ";
        SendMessage(chatHistory_, EM_REPLACESEL, FALSE, (LPARAM)header.c_str());
        
        // Reset formatting for the content
        cf.dwEffects = 0;
        SendMessage(chatHistory_, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        
        // Format code blocks in the text if present
        std::string formattedText = entry.text;
        
        // Simple regex-based code block formatting (```...```)
        std::regex codeBlockRegex(R"(```(?:[a-zA-Z]*\n)?(.*?)```)");
        formattedText = std::regex_replace(formattedText, codeBlockRegex, 
                                          "[CODE]\n$1\n[/CODE]");
        
        // Get updated text length
        textLen = SendMessage(chatHistory_, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
        
        // Set selection at the end of the text
        SendMessage(chatHistory_, EM_SETSEL, textLen, textLen);
        
        // Add the message content
        SendMessage(chatHistory_, EM_REPLACESEL, FALSE, (LPARAM)formattedText.c_str());
        
        // Add a newline after each entry
        SendMessage(chatHistory_, EM_REPLACESEL, FALSE, (LPARAM)"\r\n\r\n");
    }
    
    // Scroll to the bottom
    SendMessage(chatHistory_, WM_VSCROLL, SB_BOTTOM, 0);
}

void ChatPanel::handleSubmitQuery() {
    std::string query = getQueryText();
    if (query.empty()) {
        return;
    }
    
    // Add the query to the chat history
    addQuery(query);
    
    // Clear the query input
    setQueryText("");
    
    // Call the query callback if set
    if (queryCallback_) {
        queryCallback_(query);
    }
}

}} // namespace codelve::ui