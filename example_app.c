#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "webview_utils.h"

// Application state
typedef struct {
    WebView* main_window;
    WebView* about_window;
    int counter;
} AppState;

static AppState app_state = {0};

// JavaScript callback handlers
void on_button_click(WebView* webview, const char* name, const char* args, void* user_data) {
    printf("Button clicked with args: %s\n", args);
    
    app_state.counter++;
    
    // Update the counter display
    char js_code[256];
    snprintf(js_code, sizeof(js_code), 
             "document.getElementById('counter').textContent = 'Counter: %d';", 
             app_state.counter);
    
    webview_eval_js(webview, js_code, WEBVIEW_CONTEXT_MAIN);
}

void on_show_about(WebView* webview, const char* name, const char* args, void* user_data) {
    printf("Show about dialog requested\n");
    
    if (app_state.about_window) {
        // About window already exists, just show it
        webview_show(app_state.about_window);
        return;
    }
    
    // Create about window
    WebViewSettings* settings = webview_create_default_settings();
    if (!settings) return;
    
    settings->width = 400;
    settings->height = 300;
    settings->resizable = false;
    settings->maximizable = false;
    
    app_state.about_window = webview_create_child(app_state.main_window, "About Demo App", settings);
    if (!app_state.about_window) {
        webview_free_settings(settings);
        return;
    }
    
    const char* about_html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "    <title>About Demo App</title>"
        "    <style>"
        "        body { font-family: Arial, sans-serif; padding: 20px; text-align: center; }"
        "        .logo { font-size: 48px; margin-bottom: 20px; }"
        "        .info { margin: 10px 0; }"
        "        button { padding: 10px 20px; margin: 10px; cursor: pointer; }"
        "    </style>"
        "</head>"
        "<body>"
        "    <div class='logo'>🖥️</div>"
        "    <h2>Kuyil WebView Demo</h2>"
        "    <div class='info'>Version: 1.0.0</div>"
        "    <div class='info'>Built with: WebView Utils Library</div>"
        "    <div class='info'>A modern desktop app with web technologies</div>"
        "    <br>"
        "    <button onclick='closeAbout()'>Close</button>"
        "    <script>"
        "        function closeAbout() {"
        "            window.close();"
        "        }"
        "    </script>"
        "</body>"
        "</html>";
    
    webview_load_html(app_state.about_window, about_html);
}

void on_open_file(WebView* webview, const char* name, const char* args, void* user_data) {
    printf("Open file dialog requested\n");
    
    char* file_path = webview_show_open_dialog(webview, 
                                              "Select a file", 
                                              NULL, 
                                              "Text files (*.txt)|*.txt|All files (*.*)|*.*",
                                              false);
    
    if (file_path) {
        printf("Selected file: %s\n", file_path);
        
        // Update the UI with selected file path
        char js_code[512];
        snprintf(js_code, sizeof(js_code), 
                 "document.getElementById('file-path').textContent = 'Selected: %s';", 
                 file_path);
        
        webview_eval_js(webview, js_code, WEBVIEW_CONTEXT_MAIN);
        
        free(file_path);
    } else {
        printf("No file selected\n");
        webview_eval_js(webview, 
                       "document.getElementById('file-path').textContent = 'No file selected';", 
                       WEBVIEW_CONTEXT_MAIN);
    }
}

void on_show_message(WebView* webview, const char* name, const char* args, void* user_data) {
    printf("Show message box requested\n");
    
    char* result = webview_show_message_box(webview,
                                           "Demo Message",
                                           "This is a native message box!\n\nDo you like this demo?",
                                           "question",
                                           "yesno");
    
    if (result) {
        printf("Message box result: %s\n", result);
        
        char js_code[256];
        snprintf(js_code, sizeof(js_code), 
                 "document.getElementById('message-result').textContent = 'You clicked: %s';", 
                 result);
        
        webview_eval_js(webview, js_code, WEBVIEW_CONTEXT_MAIN);
        
        free(result);
    }
}

void on_console_message(WebView* webview, const char* message, const char* level, void* user_data) {
    printf("Console [%s]: %s\n", level, message);
}

void on_page_ready(WebView* webview, void* user_data) {
    printf("Page is ready!\n");
    
    // Bind native functions to JavaScript
    webview_bind_function(webview, "buttonClick", on_button_click, NULL);
    webview_bind_function(webview, "showAbout", on_show_about, NULL);
    webview_bind_function(webview, "openFile", on_open_file, NULL);
    webview_bind_function(webview, "showMessage", on_show_message, NULL);
    
    // Set up console logging
    webview_set_console_callback(webview, on_console_message, NULL);
    
    printf("Native functions bound to JavaScript\n");
}

const char* get_main_html() {
    return 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "    <title>Kuyil WebView Demo</title>"
        "    <meta charset='UTF-8'>"
        "    <style>"
        "        body {"
        "            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;"
        "            margin: 0;"
        "            padding: 20px;"
        "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);"
        "            color: white;"
        "            min-height: 100vh;"
        "            box-sizing: border-box;"
        "        }"
        "        .container {"
        "            max-width: 800px;"
        "            margin: 0 auto;"
        "            background: rgba(255, 255, 255, 0.1);"
        "            padding: 30px;"
        "            border-radius: 15px;"
        "            backdrop-filter: blur(10px);"
        "            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);"
        "        }"
        "        h1 {"
        "            text-align: center;"
        "            margin-bottom: 30px;"
        "            font-size: 2.5em;"
        "            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);"
        "        }"
        "        .section {"
        "            margin: 30px 0;"
        "            padding: 20px;"
        "            background: rgba(255, 255, 255, 0.1);"
        "            border-radius: 10px;"
        "        }"
        "        .section h2 {"
        "            margin-top: 0;"
        "            color: #fff;"
        "            border-bottom: 2px solid rgba(255, 255, 255, 0.3);"
        "            padding-bottom: 10px;"
        "        }"
        "        button {"
        "            background: linear-gradient(45deg, #ff6b6b, #ee5a52);"
        "            color: white;"
        "            border: none;"
        "            padding: 12px 24px;"
        "            margin: 8px 4px;"
        "            border-radius: 25px;"
        "            cursor: pointer;"
        "            font-size: 16px;"
        "            font-weight: 600;"
        "            transition: all 0.3s ease;"
        "            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);"
        "        }"
        "        button:hover {"
        "            transform: translateY(-2px);"
        "            box-shadow: 0 6px 20px rgba(0, 0, 0, 0.3);"
        "        }"
        "        button:active {"
        "            transform: translateY(0);"
        "        }"
        "        .counter {"
        "            font-size: 1.5em;"
        "            font-weight: bold;"
        "            margin: 20px 0;"
        "            text-align: center;"
        "            padding: 15px;"
        "            background: rgba(255, 255, 255, 0.2);"
        "            border-radius: 10px;"
        "        }"
        "        .info {"
        "            margin: 10px 0;"
        "            padding: 10px;"
        "            background: rgba(255, 255, 255, 0.1);"
        "            border-radius: 5px;"
        "            border-left: 4px solid #4ecdc4;"
        "        }"
        "        .footer {"
        "            text-align: center;"
        "            margin-top: 40px;"
        "            padding-top: 20px;"
        "            border-top: 1px solid rgba(255, 255, 255, 0.3);"
        "            opacity: 0.8;"
        "        }"
        "        .grid {"
        "            display: grid;"
        "            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));"
        "            gap: 20px;"
        "            margin: 20px 0;"
        "        }"
        "    </style>"
        "</head>"
        "<body>"
        "    <div class='container'>"
        "        <h1>🚀 Kuyil WebView Demo</h1>"
        "        <p style='text-align: center; font-size: 1.2em; opacity: 0.9;'>"
        "            A modern desktop application built with HTML, CSS, JavaScript, and C"
        "        </p>"
        ""
        "        <div class='section'>"
        "            <h2>📊 Interactive Counter</h2>"
        "            <div class='counter' id='counter'>Counter: 0</div>"
        "            <div style='text-align: center;'>"
        "                <button onclick='incrementCounter()'>Click Me!</button>"
        "                <button onclick='resetCounter()'>Reset</button>"
        "            </div>"
        "        </div>"
        ""
        "        <div class='grid'>"
        "            <div class='section'>"
        "                <h2>🪟 Window Management</h2>"
        "                <button onclick='showAbout()'>About Dialog</button>"
        "                <button onclick='toggleFullscreen()'>Toggle Fullscreen</button>"
        "            </div>"
        ""
        "            <div class='section'>"
        "                <h2>💬 Native Dialogs</h2>"
        "                <button onclick='openFile()'>Open File</button>"
        "                <button onclick='showMessage()'>Show Message</button>"
        "                <div class='info' id='file-path'>No file selected</div>"
        "                <div class='info' id='message-result'>No message shown</div>"
        "            </div>"
        "        </div>"
        ""
        "        <div class='section'>"
        "            <h2>🧪 JavaScript Features</h2>"
        "            <button onclick='testLocalStorage()'>Test LocalStorage</button>"
        "            <button onclick='testConsole()'>Test Console</button>"
        "            <button onclick='testFetch()'>Test Network</button>"
        "            <div class='info' id='feature-results'>Click buttons to test features</div>"
        "        </div>"
        ""
        "        <div class='section'>"
        "            <h2>ℹ️ System Information</h2>"
        "            <div class='info'>User Agent: <span id='user-agent'></span></div>"
        "            <div class='info'>Screen Size: <span id='screen-size'></span></div>"
        "            <div class='info'>Window Size: <span id='window-size'></span></div>"
        "        </div>"
        ""
        "        <div class='footer'>"
        "            <p>Built with ❤️ using Kuyil WebView Utils Library</p>"
        "            <p>Demonstrating the power of web technologies in native desktop apps</p>"
        "        </div>"
        "    </div>"
        ""
        "    <script>"
        "        let counter = 0;"
        ""
        "        function incrementCounter() {"
        "            counter++;"
        "            buttonClick('increment');"
        "            console.log('Counter incremented to:', counter);"
        "        }"
        ""
        "        function resetCounter() {"
        "            counter = 0;"
        "            document.getElementById('counter').textContent = 'Counter: 0';"
        "            console.log('Counter reset');"
        "        }"
        ""
        "        function toggleFullscreen() {"
        "            if (document.fullscreenElement) {"
        "                document.exitFullscreen();"
        "            } else {"
        "                document.documentElement.requestFullscreen();"
        "            }"
        "        }"
        ""
        "        function testLocalStorage() {"
        "            const testKey = 'webview_test';"
        "            const testValue = 'Hello WebView! ' + new Date().toLocaleTimeString();"
        "            localStorage.setItem(testKey, testValue);"
        "            const retrieved = localStorage.getItem(testKey);"
        "            document.getElementById('feature-results').textContent = 'LocalStorage: ' + retrieved;"
        "        }"
        ""
        "        function testConsole() {"
        "            console.log('This is a test log message');"
        "            console.warn('This is a test warning');"
        "            console.error('This is a test error (not real!)');"
        "            document.getElementById('feature-results').textContent = 'Console messages sent - check native output';"
        "        }"
        ""
        "        function testFetch() {"
        "            fetch('https://api.github.com/zen')"
        "                .then(response => response.text())"
        "                .then(data => {"
        "                    document.getElementById('feature-results').textContent = 'GitHub Zen: ' + data;"
        "                })"
        "                .catch(error => {"
        "                    document.getElementById('feature-results').textContent = 'Network test failed (expected in offline mode)';"
        "                });"
        "        }"
        ""
        "        // Initialize system info"
        "        document.addEventListener('DOMContentLoaded', function() {"
        "            document.getElementById('user-agent').textContent = navigator.userAgent.substring(0, 80) + '...';"
        "            document.getElementById('screen-size').textContent = screen.width + 'x' + screen.height;"
        "            document.getElementById('window-size').textContent = window.innerWidth + 'x' + window.innerHeight;"
        ""
        "            // Update window size on resize"
        "            window.addEventListener('resize', function() {"
        "                document.getElementById('window-size').textContent = window.innerWidth + 'x' + window.innerHeight;"
        "            });"
        ""
        "            console.log('Demo application loaded successfully!');"
        "        });"
        ""
        "        // Add some interactive effects"
        "        document.addEventListener('click', function(e) {"
        "            if (e.target.tagName === 'BUTTON') {"
        "                // Add ripple effect"
        "                const ripple = document.createElement('div');"
        "                ripple.style.cssText = '"
        "                    position: absolute;"
        "                    border-radius: 50%;"
        "                    background: rgba(255, 255, 255, 0.6);"
        "                    transform: scale(0);"
        "                    animation: ripple 600ms linear;"
        "                    pointer-events: none;"
        "                ';"
        "                e.target.style.position = 'relative';"
        "                e.target.style.overflow = 'hidden';"
        "                e.target.appendChild(ripple);"
        ""
        "                setTimeout(() => ripple.remove(), 600);"
        "            }"
        "        });"
        ""
        "        // Add CSS animation for ripple"
        "        const style = document.createElement('style');"
        "        style.textContent = '"
        "            @keyframes ripple {"
        "                to {"
        "                    transform: scale(4);"
        "                    opacity: 0;"
        "                }"
        "            }"
        "        ';"
        "        document.head.appendChild(style);"
        "    </script>"
        "</body>"
        "</html>";
}

int main(int argc, char* argv[]) {
    printf("Kuyil WebView Demo Application\n");
    printf("=============================\n");
    
    // Initialize webview system
    if (!webview_init()) {
        printf("Failed to initialize webview system\n");
        return 1;
    }
    
    // Create application settings
    WebViewSettings* settings = webview_create_default_settings();
    if (!settings) {
        printf("Failed to create webview settings\n");
        webview_cleanup();
        return 1;
    }
    
    // Customize settings for demo
    settings->width = 1000;
    settings->height = 700;
    settings->debug = true;
    settings->dev_tools = true;
    settings->debug_level = WEBVIEW_DEBUG_INFO;
    
    // Update app info
    if (settings->app_name) free(settings->app_name);
    settings->app_name = strdup("Kuyil WebView Demo");
    
    if (settings->app_version) free(settings->app_version);
    settings->app_version = strdup("1.0.0");
    
    // Set callbacks
    settings->ready_callback = on_page_ready;
    
    // Create main window
    app_state.main_window = webview_create("Kuyil WebView Demo", settings);
    if (!app_state.main_window) {
        printf("Failed to create webview window\n");
        webview_free_settings(settings);
        webview_cleanup();
        return 1;
    }
    
    printf("WebView window created successfully\n");
    printf("Window features:\n");
    printf("  - Size: %dx%d\n", settings->width, settings->height);
    printf("  - Debug mode: %s\n", settings->debug ? "enabled" : "disabled");
    printf("  - Developer tools: %s\n", settings->dev_tools ? "available" : "disabled");
    
    // Center the window
    webview_center(app_state.main_window);
    
    // Load the main HTML content
    webview_load_html(app_state.main_window, get_main_html());
    
    printf("\nStarting event loop...\n");
    printf("The demo application should now be visible.\n");
    printf("Try the interactive features:\n");
    printf("  - Click the counter button\n");
    printf("  - Open the About dialog\n");
    printf("  - Test file dialogs and message boxes\n");
    printf("  - Check the console output for JavaScript messages\n");
    printf("\nPress Ctrl+C or close the window to exit.\n\n");
    
    // Run the event loop
    int exit_code = webview_run(app_state.main_window);
    
    printf("Application exiting with code: %d\n", exit_code);
    
    // Cleanup
    if (app_state.about_window) {
        webview_destroy(app_state.about_window);
    }
    
    webview_destroy(app_state.main_window);
    webview_cleanup();
    
    printf("WebView demo application finished.\n");
    return exit_code;
}