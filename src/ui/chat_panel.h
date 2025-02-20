// E:\codelve\src\ui\chat_panel.h
#pragma once
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <windows.h>
#include <richedit.h>

namespace codelve {
namespace utils {
    class Config;
}
namespace ui {

/**
 * Callback for query submission
 */
using QueryCallback = std::function<void(const std::string& query)>;

/**
 * Chat interface panel for interacting with the LLM
 */
class ChatPanel {
public:
    /**
     * Constructor.
     * @param parentWindow Handle to parent window
     * @param config Shared configuration
     */
    ChatPanel(HWND parentWindow, std::shared_ptr<utils::Config> config);
    
    /**
     * Destructor.
     */
    ~ChatPanel();
    
    /**
     * Initialize the chat panel.
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * Resize and reposition the panel.
     * @param x Left position
     * @param y Top position
     * @param width Width
     * @param height Height
     */
    void resize(int x, int y, int width, int height);
    
    /**
     * Add a user query to the chat history.
     * @param query The user's query
     */
    void addQuery(const std::string& query);
    
    /**
     * Add an LLM response to the chat history.
     * @param response The LLM's response
     */
    void addResponse(const std::string& response);
    
    /**
     * Clear the chat history.
     */
    void clearChat();
    
    /**
     * Set query submission callback.
     * @param callback The callback function
     */
    void setQueryCallback(QueryCallback callback);
    
    /**
     * Get the current query text.
     * @return Query text
     */
    std::string getQueryText();
    
    /**
     * Set the query text.
     * @param text New query text
     */
    void setQueryText(const std::string& text);

private:
    // Parent window handle
    HWND parentWindow_;
    
    // Configuration
    std::shared_ptr<utils::Config> config_;
    
    // Control handles
    HWND panelContainer_;
    HWND chatHistory_;
    HWND queryInput_;
    HWND submitButton_;
    HWND clearButton_;
    
    // Query callback
    QueryCallback queryCallback_;
    
    // Chat history entries
    struct ChatEntry {
        bool isUser;
        std::string text;
    };
    std::vector<ChatEntry> chatEntries_;
    
    // Internal methods
    void createControls();
    void updateChatHistory();
    void handleSubmitQuery();
    
    // Window procedure for the panel container
    static LRESULT CALLBACK PanelProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Window class name
    static constexpr const char* panelClassName_ = "CodeLveChatPanel";
};

}} // namespace codelve::ui