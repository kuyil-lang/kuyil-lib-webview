#ifndef WEBVIEW_UTILS_H
#define WEBVIEW_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>


/**
 * Register a callback to process VM task queue during GTK main loop
 * @param processor Function pointer that processes pending tasks
 */
void webview_set_task_processor(int (*processor)(int max_tasks));

#ifdef __cplusplus
extern "C" {
#endif

// Version information
#define WEBVIEW_UTILS_VERSION_MAJOR 1
#define WEBVIEW_UTILS_VERSION_MINOR 0
#define WEBVIEW_UTILS_VERSION_PATCH 0
#define WEBVIEW_UTILS_VERSION_STRING "1.0.0"

// Maximum limits
#define MAX_TITLE_LENGTH 256
#define MAX_URL_LENGTH 2048
#define MAX_HTML_LENGTH 65536
#define MAX_JS_CODE_LENGTH 8192
#define MAX_ERROR_LENGTH 512
#define MAX_CALLBACK_NAME_LENGTH 64

// Forward declarations
typedef struct WebView WebView;
typedef struct WebViewWindow WebViewWindow;
typedef struct WebViewSettings WebViewSettings;

// Window size modes
typedef enum {
    WEBVIEW_SIZE_FIXED = 0,      // Fixed size, not resizable
    WEBVIEW_SIZE_MIN,            // Minimum size constraint
    WEBVIEW_SIZE_MAX,            // Maximum size constraint  
    WEBVIEW_SIZE_RESIZABLE       // Freely resizable
} WebViewSizeMode;

// Window position modes
typedef enum {
    WEBVIEW_POS_CENTER = 0,      // Center on screen
    WEBVIEW_POS_CUSTOM,          // Custom x,y position
    WEBVIEW_POS_MOUSE,           // Position at mouse cursor
    WEBVIEW_POS_DEFAULT          // OS default position
} WebViewPosition;

// Debug levels
typedef enum {
    WEBVIEW_DEBUG_NONE = 0,      // No debug output
    WEBVIEW_DEBUG_ERROR,         // Error messages only
    WEBVIEW_DEBUG_WARNING,       // Warnings and errors
    WEBVIEW_DEBUG_INFO,          // Info, warnings, and errors
    WEBVIEW_DEBUG_VERBOSE        // All debug messages
} WebViewDebugLevel;

// Window states
typedef enum {
    WEBVIEW_STATE_NORMAL = 0,    // Normal window
    WEBVIEW_STATE_MINIMIZED,     // Minimized window
    WEBVIEW_STATE_MAXIMIZED,     // Maximized window
    WEBVIEW_STATE_FULLSCREEN,    // Fullscreen mode
    WEBVIEW_STATE_HIDDEN         // Hidden window
} WebViewWindowState;

// JavaScript execution contexts
typedef enum {
    WEBVIEW_CONTEXT_MAIN = 0,    // Main world context
    WEBVIEW_CONTEXT_ISOLATED     // Isolated world context
} WebViewContext;

// Callback function types
typedef void (*WebViewReadyCallback)(WebView* webview, void* user_data);
typedef void (*WebViewNavigationCallback)(WebView* webview, const char* url, void* user_data);
typedef void (*WebViewCloseCallback)(WebView* webview, void* user_data);
typedef void (*WebViewErrorCallback)(WebView* webview, const char* error, void* user_data);
typedef void (*WebViewJSCallback)(WebView* webview, const char* name, const char* args, void* user_data);
typedef void (*WebViewConsoleCallback)(WebView* webview, const char* message, const char* level, void* user_data);

// WebView settings structure
struct WebViewSettings {
    // Window properties
    int width;                           // Window width in pixels
    int height;                          // Window height in pixels
    int x;                              // Window x position
    int y;                              // Window y position
    WebViewSizeMode size_mode;          // Size constraint mode
    WebViewPosition position;           // Window positioning mode
    bool resizable;                     // Whether window is resizable
    bool minimizable;                   // Whether window can be minimized
    bool maximizable;                   // Whether window can be maximized
    bool closable;                      // Whether window can be closed
    bool always_on_top;                 // Keep window always on top
    bool skip_taskbar;                  // Hide from taskbar
    bool transparent;                   // Transparent window background
    
    // WebView properties
    bool debug;                         // Enable debug mode
    WebViewDebugLevel debug_level;      // Debug output level
    bool dev_tools;                     // Enable developer tools
    bool context_menu;                  // Enable right-click context menu
    bool zoom_control;                  // Enable zoom controls
    bool text_selection;                // Enable text selection
    bool drag_drop;                     // Enable drag and drop
    bool file_access;                   // Allow file:// URL access
    bool local_storage;                 // Enable localStorage
    bool session_storage;               // Enable sessionStorage
    bool cache;                         // Enable caching
    bool images;                        // Load images
    bool javascript;                    // Enable JavaScript execution
    
    // Security settings
    bool allow_unsafe_inline;           // Allow unsafe inline scripts
    bool allow_unsafe_eval;             // Allow unsafe eval()
    bool allow_external_navigation;     // Allow navigation to external URLs
    bool allow_downloads;               // Allow file downloads
    bool allow_popups;                  // Allow popup windows
    
    // User agent and other
    char* user_agent;                   // Custom user agent string
    char* app_name;                     // Application name
    char* app_version;                  // Application version
    int memory_limit;                   // Memory limit in MB (0 = unlimited)
    
    // Callbacks
    WebViewReadyCallback ready_callback;
    WebViewNavigationCallback navigation_callback;
    WebViewCloseCallback close_callback;
    WebViewErrorCallback error_callback;
    WebViewConsoleCallback console_callback;
    void* user_data;                    // User data passed to callbacks
};

// ============================================================================
// WebView Creation and Management
// ============================================================================

/**
 * Initialize webview system
 * @return true on success, false on error
 */
bool webview_init(void);

/**
 * Cleanup webview system
 */
void webview_cleanup(void);

/**
 * Create default webview settings
 * @return Default settings structure (must be freed with webview_free_settings)
 */
WebViewSettings* webview_create_default_settings(void);

/**
 * Free webview settings structure
 * @param settings Settings to free
 */
void webview_free_settings(WebViewSettings* settings);

/**
 * Create a new webview window
 * @param title Window title
 * @param settings WebView settings (or NULL for defaults)
 * @return WebView handle or NULL on error
 */
WebView* webview_create(const char* title, WebViewSettings* settings);

/**
 * Destroy webview window
 * @param webview WebView handle
 */
void webview_destroy(WebView* webview);

/**
 * Get webview window handle
 * @param webview WebView handle
 * @return Native window handle or NULL
 */
void* webview_get_window_handle(WebView* webview);

/**
 * Check if webview is valid and running
 * @param webview WebView handle
 * @return true if valid, false otherwise
 */
bool webview_is_valid(WebView* webview);

// ============================================================================
// Content Loading and Navigation
// ============================================================================

/**
 * Load URL in webview
 * @param webview WebView handle
 * @param url URL to load
 * @return true on success, false on error
 */
bool webview_navigate(WebView* webview, const char* url);

/**
 * Load HTML content directly
 * @param webview WebView handle
 * @param html HTML content
 * @return true on success, false on error
 */
bool webview_load_html(WebView* webview, const char* html);

/**
 * Load HTML from file
 * @param webview WebView handle
 * @param file_path Path to HTML file
 * @return true on success, false on error
 */
bool webview_load_file(WebView* webview, const char* file_path);

/**
 * Reload current page
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_reload(WebView* webview);

/**
 * Stop loading current page
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_stop(WebView* webview);

/**
 * Navigate back in history
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_go_back(WebView* webview);

/**
 * Navigate forward in history
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_go_forward(WebView* webview);

/**
 * Check if can navigate back
 * @param webview WebView handle
 * @return true if can go back, false otherwise
 */
bool webview_can_go_back(WebView* webview);

/**
 * Check if can navigate forward
 * @param webview WebView handle
 * @return true if can go forward, false otherwise
 */
bool webview_can_go_forward(WebView* webview);

/**
 * Get current URL
 * @param webview WebView handle
 * @return Current URL or NULL (must be freed)
 */
char* webview_get_url(WebView* webview);

/**
 * Get page title
 * @param webview WebView handle
 * @return Page title or NULL (must be freed)
 */
char* webview_get_title(WebView* webview);

// ============================================================================
// JavaScript Integration
// ============================================================================

/**
 * Execute JavaScript code
 * @param webview WebView handle
 * @param js_code JavaScript code to execute
 * @param context Execution context
 * @return true on success, false on error
 */
bool webview_eval_js(WebView* webview, const char* js_code, WebViewContext context);

/**
 * Execute JavaScript and get result
 * @param webview WebView handle
 * @param js_code JavaScript code to execute
 * @param context Execution context
 * @return Result as string or NULL (must be freed)
 */
char* webview_eval_js_with_result(WebView* webview, const char* js_code, WebViewContext context);

/**
 * Bind native function to JavaScript
 * @param webview WebView handle
 * @param name Function name in JavaScript
 * @param callback Callback function to handle calls
 * @param user_data User data passed to callback
 * @return true on success, false on error
 */
bool webview_bind_function(WebView* webview, const char* name, WebViewJSCallback callback, void* user_data);

/**
 * Unbind function from JavaScript
 * @param webview WebView handle
 * @param name Function name to unbind
 * @return true on success, false on error
 */
bool webview_unbind_function(WebView* webview, const char* name);

/**
 * Call JavaScript function from native code
 * @param webview WebView handle
 * @param function_name JavaScript function name
 * @param args JSON array of arguments
 * @return Result as string or NULL (must be freed)
 */
char* webview_call_js_function(WebView* webview, const char* function_name, const char* args);

/**
 * Add JavaScript code to be executed on page load
 * @param webview WebView handle
 * @param js_code JavaScript code
 * @param context Execution context
 * @return true on success, false on error
 */
bool webview_add_script(WebView* webview, const char* js_code, WebViewContext context);

/**
 * Remove all added scripts
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_clear_scripts(WebView* webview);

// ============================================================================
// Window Management
// ============================================================================

/**
 * Show webview window
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_show(WebView* webview);

/**
 * Hide webview window
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_hide(WebView* webview);

/**
 * Set window title
 * @param webview WebView handle
 * @param title New window title
 * @return true on success, false on error
 */
bool webview_set_title(WebView* webview, const char* title);

/**
 * Get window title
 * @param webview WebView handle
 * @return Window title or NULL (must be freed)
 */
char* webview_get_window_title(WebView* webview);

/**
 * Set window size
 * @param webview WebView handle
 * @param width New width in pixels
 * @param height New height in pixels
 * @return true on success, false on error
 */
bool webview_set_size(WebView* webview, int width, int height);

/**
 * Get window size
 * @param webview WebView handle
 * @param width Output parameter for width
 * @param height Output parameter for height
 * @return true on success, false on error
 */
bool webview_get_size(WebView* webview, int* width, int* height);

/**
 * Set window position
 * @param webview WebView handle
 * @param x X position in pixels
 * @param y Y position in pixels
 * @return true on success, false on error
 */
bool webview_set_position(WebView* webview, int x, int y);

/**
 * Get window position
 * @param webview WebView handle
 * @param x Output parameter for x position
 * @param y Output parameter for y position
 * @return true on success, false on error
 */
bool webview_get_position(WebView* webview, int* x, int* y);

/**
 * Center window on screen
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_center(WebView* webview);

/**
 * Set window state
 * @param webview WebView handle
 * @param state New window state
 * @return true on success, false on error
 */
bool webview_set_state(WebView* webview, WebViewWindowState state);

/**
 * Get window state
 * @param webview WebView handle
 * @return Current window state
 */
WebViewWindowState webview_get_state(WebView* webview);

/**
 * Set window always on top
 * @param webview WebView handle
 * @param on_top true to keep on top, false otherwise
 * @return true on success, false on error
 */
bool webview_set_always_on_top(WebView* webview, bool on_top);

/**
 * Set window transparency
 * @param webview WebView handle
 * @param transparent true for transparent, false for opaque
 * @return true on success, false on error
 */
bool webview_set_transparent(WebView* webview, bool transparent);

// ============================================================================
// Event Loop and Messaging
// ============================================================================

/**
 * Run main event loop (blocking)
 * @param webview WebView handle
 * @return Exit code
 */
int webview_run(WebView* webview);

/**
 * Run single event loop iteration (non-blocking)
 * @param webview WebView handle
 * @return true to continue, false to exit
 */
bool webview_step(WebView* webview);

/**
 * Terminate event loop
 * @param webview WebView handle
 * @param exit_code Exit code to return from run()
 */
void webview_terminate(WebView* webview, int exit_code);

/**
 * Dispatch function call to main thread
 * @param webview WebView handle
 * @param function Function to call
 * @param data Data to pass to function
 * @return true on success, false on error
 */
bool webview_dispatch(WebView* webview, void (*function)(WebView*, void*), void* data);

/**
 * Post message to JavaScript
 * @param webview WebView handle
 * @param message JSON message to post
 * @return true on success, false on error
 */
bool webview_post_message(WebView* webview, const char* message);

/**
 * Set message handler for messages from JavaScript
 * @param webview WebView handle
 * @param callback Callback function
 * @param user_data User data passed to callback
 * @return true on success, false on error
 */
bool webview_set_message_handler(WebView* webview, void (*callback)(WebView*, const char*, void*), void* user_data);

// ============================================================================
// Developer Tools and Debugging
// ============================================================================

/**
 * Open developer tools
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_open_dev_tools(WebView* webview);

/**
 * Close developer tools
 * @param webview WebView handle
 * @return true on success, false on error
 */
bool webview_close_dev_tools(WebView* webview);

/**
 * Check if developer tools are open
 * @param webview WebView handle
 * @return true if open, false otherwise
 */
bool webview_are_dev_tools_open(WebView* webview);

/**
 * Set console message callback
 * @param webview WebView handle
 * @param callback Callback for console messages
 * @param user_data User data passed to callback
 * @return true on success, false on error
 */
bool webview_set_console_callback(WebView* webview, WebViewConsoleCallback callback, void* user_data);

/**
 * Enable or disable debug mode
 * @param webview WebView handle
 * @param debug true to enable, false to disable
 * @param level Debug level
 * @return true on success, false on error
 */
bool webview_set_debug_mode(WebView* webview, bool debug, WebViewDebugLevel level);

/**
 * Print debug message
 * @param webview WebView handle
 * @param level Message level
 * @param message Debug message
 */
void webview_debug_print(WebView* webview, WebViewDebugLevel level, const char* message);

// ============================================================================
// File and Resource Management
// ============================================================================

/**
 * Set custom protocol handler
 * @param webview WebView handle
 * @param protocol Protocol name (e.g., "app")
 * @param handler Handler function
 * @param user_data User data passed to handler
 * @return true on success, false on error
 */
bool webview_set_protocol_handler(WebView* webview, const char* protocol, 
                                  void (*handler)(WebView*, const char*, void*), void* user_data);

/**
 * Remove protocol handler
 * @param webview WebView handle
 * @param protocol Protocol name to remove
 * @return true on success, false on error
 */
bool webview_remove_protocol_handler(WebView* webview, const char* protocol);

/**
 * Serve file from local filesystem
 * @param webview WebView handle
 * @param virtual_path Virtual path in webview
 * @param file_path Actual file path on filesystem
 * @return true on success, false on error
 */
bool webview_serve_file(WebView* webview, const char* virtual_path, const char* file_path);

/**
 * Serve directory from local filesystem
 * @param webview WebView handle
 * @param virtual_path Virtual path prefix
 * @param directory_path Directory path on filesystem
 * @return true on success, false on error
 */
bool webview_serve_directory(WebView* webview, const char* virtual_path, const char* directory_path);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get screen size
 * @param width Output parameter for screen width
 * @param height Output parameter for screen height
 * @return true on success, false on error
 */
bool webview_get_screen_size(int* width, int* height);

/**
 * Get available screen work area (excluding taskbars, etc.)
 * @param x Output parameter for work area x
 * @param y Output parameter for work area y
 * @param width Output parameter for work area width
 * @param height Output parameter for work area height
 * @return true on success, false on error
 */
bool webview_get_work_area(int* x, int* y, int* width, int* height);

/**
 * Show native message box
 * @param webview WebView handle (or NULL for standalone)
 * @param title Message box title
 * @param message Message text
 * @param type Message box type ("info", "warning", "error", "question")
 * @param buttons Button configuration ("ok", "okcancel", "yesno", etc.)
 * @return Button clicked ("ok", "cancel", "yes", "no")
 */
char* webview_show_message_box(WebView* webview, const char* title, const char* message, 
                               const char* type, const char* buttons);

/**
 * Show file open dialog
 * @param webview WebView handle (or NULL for standalone)
 * @param title Dialog title
 * @param default_path Default directory path
 * @param filter File filter (e.g., "Text files (*.txt)|*.txt")
 * @param multiple Allow multiple file selection
 * @return Selected file path(s) or NULL (must be freed)
 */
char* webview_show_open_dialog(WebView* webview, const char* title, const char* default_path,
                               const char* filter, bool multiple);

/**
 * Show file save dialog
 * @param webview WebView handle (or NULL for standalone)
 * @param title Dialog title
 * @param default_path Default file path
 * @param filter File filter
 * @return Selected file path or NULL (must be freed)
 */
char* webview_show_save_dialog(WebView* webview, const char* title, const char* default_path,
                               const char* filter);

/**
 * Show directory selection dialog
 * @param webview WebView handle (or NULL for standalone)
 * @param title Dialog title
 * @param default_path Default directory path
 * @return Selected directory path or NULL (must be freed)
 */
char* webview_show_directory_dialog(WebView* webview, const char* title, const char* default_path);

// ============================================================================
// Error Handling and Information
// ============================================================================

/**
 * Get last error message
 * @return Error message string
 */
const char* webview_get_last_error(void);

/**
 * Clear last error message
 */
void webview_clear_error(void);

/**
 * Get webview utils library version
 * @return Version string
 */
const char* webview_utils_get_version(void);

/**
 * Get underlying webview engine information
 * @return Engine info string (must be freed)
 */
char* webview_get_engine_info(void);

/**
 * Check if feature is supported
 * @param feature Feature name to check
 * @return true if supported, false otherwise
 */
bool webview_is_feature_supported(const char* feature);

/**
 * Get list of supported features
 * @param count Output parameter for feature count
 * @return Array of feature names (must be freed)
 */
char** webview_get_supported_features(int* count);

// ============================================================================
// Multi-Window Management
// ============================================================================

/**
 * Create child window
 * @param parent Parent webview
 * @param title Child window title
 * @param settings Child window settings
 * @return Child WebView handle or NULL
 */
WebView* webview_create_child(WebView* parent, const char* title, WebViewSettings* settings);

/**
 * Get parent window
 * @param webview WebView handle
 * @return Parent WebView handle or NULL
 */
WebView* webview_get_parent(WebView* webview);

/**
 * Get list of child windows
 * @param webview WebView handle
 * @param count Output parameter for child count
 * @return Array of child WebView handles (must be freed)
 */
WebView** webview_get_children(WebView* webview, int* count);

/**
 * Close all child windows
 * @param webview WebView handle
 * @return Number of children closed
 */
int webview_close_all_children(WebView* webview);

// ============================================================================
// Developer Tools
// ============================================================================

/**
 * Enable or disable WebKit developer tools (inspector)
 * @param webview WebView handle
 * @param enable true to enable, false to disable
 * @return true on success, false on error
 */
bool webview_enable_dev_tools(WebView* webview, bool enable);

/**
 * Register a callback to process VM task queue during GTK main loop
 * @param processor Function pointer that processes pending tasks
 */
void webview_set_task_processor(int (*processor)(int max_tasks));

#ifdef __cplusplus
}
#endif

#endif // WEBVIEW_UTILS_H