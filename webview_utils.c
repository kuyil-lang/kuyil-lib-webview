#define _GNU_SOURCE  // For strdup
#include "webview_utils.h"
#include "../../src/ast.h"  // For Value type
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __linux__
    #include <gtk/gtk.h>
    #include <webkit2/webkit2.h>
    #define WEBVIEW_GTK 1
#else
    // For non-Linux platforms, provide stub implementation
    #define WEBVIEW_STUB 1
#endif

// External VM functions for avatar completion processing
extern int vm_process_avatar_completions(int max_avatars);
extern size_t vm_avatar_pending_count(void);

// External async request queue processing
extern int vm_process_async_requests(int max_requests);

// Simplified WebView structure
struct WebView {
#ifdef WEBVIEW_GTK
    GtkWidget* window;
    GtkWidget* webview;
#endif
    WebViewSettings* settings;
    char* title;
    bool is_valid;
    int exit_code;
    
    // Callback storage
    WebViewReadyCallback ready_callback;
    void* ready_callback_data;
    
    WebViewConsoleCallback console_callback;
    void* console_callback_data;
};

// Global state
static bool g_webview_initialized = false;
static char g_webview_error[512] = {0};

// Task queue processor callback (called from external VM)
static int (*g_task_processor)(int max_tasks) = NULL;

// Helper functions
static void set_error(const char* message) {
    snprintf(g_webview_error, sizeof(g_webview_error), "%s", message);
}

#ifdef WEBVIEW_GTK
// GTK/WebKit implementation

// Forward declarations for heapfs and file access
typedef struct {
    const char* path;
    const char* content;
    size_t size;
    const char* mime_type;
} HeapFSFile;

extern HeapFSFile* heapfs_get_c_file_info(const char* path);
extern size_t heapfs_c_get_file_size(HeapFSFile* file);
extern const char* heapfs_c_get_file_data(HeapFSFile* file);
extern const char* heapfs_c_get_file_mime(HeapFSFile* file);

// Forward declaration for file I/O
extern Value file_read_text_value(const char* path);

// URI scheme handler for app-heapfs://
static void on_uri_scheme_request_heapfs(WebKitURISchemeRequest* request, gpointer user_data) {
    (void)user_data;
    
    const char* uri = webkit_uri_scheme_request_get_uri(request);
    const char* path = uri + strlen("app-heapfs://");  // Skip scheme prefix
    
    // Get file from heapfs
    HeapFSFile* file_info = heapfs_get_c_file_info(path);
    if (!file_info) {
        GError* error = g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "File not found in heapfs: %s", path);
        webkit_uri_scheme_request_finish_error(request, error);
        g_error_free(error);
        return;
    }
    
    // Get file data and metadata
    const char* data = heapfs_c_get_file_data(file_info);
    size_t size = heapfs_c_get_file_size(file_info);
    const char* mime_type = heapfs_c_get_file_mime(file_info);
    
    // Create GInputStream from the data
    GInputStream* stream = g_memory_input_stream_new_from_data(
        g_memdup(data, size), size, g_free);
    
    // Finish the request with the stream
    webkit_uri_scheme_request_finish(request, stream, size, mime_type);
    g_object_unref(stream);
}

// URI scheme handler for app-localfs://
static void on_uri_scheme_request_localfs(WebKitURISchemeRequest* request, gpointer user_data) {
    (void)user_data;
    
    const char* uri = webkit_uri_scheme_request_get_uri(request);
    const char* path = uri + strlen("app-localfs://");  // Skip scheme prefix
    
    // Read file from local filesystem using Kuyil's file I/O
    Value result = file_read_text_value(path);
    if (result.type != 2) {  // VALUE_STRING = 2
        GError* error = g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "File not found on disk: %s", path);
        webkit_uri_scheme_request_finish_error(request, error);
        g_error_free(error);
        return;
    }
    
    const char* content = result.as.string;
    
    // Determine MIME type from file extension
    const char* mime_type = "application/octet-stream";
    if (strstr(path, ".html") || strstr(path, ".htm")) mime_type = "text/html";
    else if (strstr(path, ".css")) mime_type = "text/css";
    else if (strstr(path, ".js")) mime_type = "application/javascript";
    else if (strstr(path, ".json")) mime_type = "application/json";
    else if (strstr(path, ".png")) mime_type = "image/png";
    else if (strstr(path, ".jpg") || strstr(path, ".jpeg")) mime_type = "image/jpeg";
    else if (strstr(path, ".svg")) mime_type = "image/svg+xml";
    
    size_t size = strlen(content);
    
    // Create GInputStream from the content (duplicate for ownership)
    char* content_copy = strdup(content);
    GInputStream* stream = g_memory_input_stream_new_from_data(
        content_copy, size, g_free);  // g_free will be called when stream is done
    
    // Finish the request with the stream
    webkit_uri_scheme_request_finish(request, stream, size, mime_type);
    g_object_unref(stream);
}

// GTK timeout callback to process task queue
static gboolean on_process_tasks(gpointer data) {
    (void)data;  // Unused
    if (g_task_processor) {
        g_task_processor(100);  // Process up to 100 tasks per timer tick
    }
    return TRUE;  // Continue calling this timeout
}

static gboolean on_window_delete(GtkWidget* widget, GdkEvent* event, gpointer data) {
    (void)widget; (void)event;  // Suppress unused parameter warnings
    WebView* webview = (WebView*)data;
    if (webview) {
        webview->exit_code = 0;
        webview->is_valid = false;  // Mark as invalid so step() will return false
        // Only quit main loop if it's actually running
        if (gtk_main_level() > 0) {
            gtk_main_quit();
        }
    }
    return FALSE;
}

static void on_load_changed(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer data) {
    (void)web_view;  // Suppress unused parameter warning
    WebView* webview = (WebView*)data;
    
    if (load_event == WEBKIT_LOAD_FINISHED && webview && webview->ready_callback) {
        webview->ready_callback(webview, webview->ready_callback_data);
    }
}

bool webview_init(void) {
    if (g_webview_initialized) {
        return true;
    }
    
    if (!gtk_init_check(0, NULL)) {
        set_error("Failed to initialize GTK");
        return false;
    }
    
    // Register custom URI schemes
    WebKitWebContext* context = webkit_web_context_get_default();
    
    // Register app-heapfs:// for embedded resources
    webkit_web_context_register_uri_scheme(context, "app-heapfs",
        on_uri_scheme_request_heapfs, NULL, NULL);
    
    // Register app-localfs:// for local filesystem access
    webkit_web_context_register_uri_scheme(context, "app-localfs",
        on_uri_scheme_request_localfs, NULL, NULL);
    
    g_webview_initialized = true;
    return true;
}

void webview_cleanup(void) {
    g_webview_initialized = false;
}

WebView* webview_create(const char* title, WebViewSettings* settings) {
    if (!g_webview_initialized) {
        set_error("WebView not initialized");
        return NULL;
    }
    
    if (!title || !settings) {
        set_error("Invalid parameters");
        return NULL;
    }
    
    WebView* webview = calloc(1, sizeof(WebView));
    if (!webview) {
        set_error("Memory allocation failed");
        return NULL;
    }
    
    // Create GTK window
    webview->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!webview->window) {
        set_error("Failed to create window");
        free(webview);
        return NULL;
    }
    
    // Set window properties
    gtk_window_set_title(GTK_WINDOW(webview->window), title);
    gtk_window_set_default_size(GTK_WINDOW(webview->window), settings->width, settings->height);
    
    if (!settings->resizable) {
        gtk_window_set_resizable(GTK_WINDOW(webview->window), FALSE);
    }
    
    // Create WebKit web view
    webview->webview = webkit_web_view_new();
    if (!webview->webview) {
        set_error("Failed to create WebView");
        gtk_widget_destroy(webview->window);
        free(webview);
        return NULL;
    }
    
    // Add WebView to window
    gtk_container_add(GTK_CONTAINER(webview->window), webview->webview);
    
    // Always enable developer tools for debugging
    WebKitSettings* webkit_settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview->webview));
    if (webkit_settings) {
        webkit_settings_set_enable_developer_extras(webkit_settings, TRUE);
    }
    
    // Set up callbacks
    g_signal_connect(webview->window, "delete-event", G_CALLBACK(on_window_delete), webview);
    g_signal_connect(webview->webview, "load-changed", G_CALLBACK(on_load_changed), webview);
    
    // Copy settings
    webview->settings = webview_create_default_settings();
    memcpy(webview->settings, settings, sizeof(WebViewSettings));
    
    webview->title = strdup(title);
    webview->is_valid = true;
    webview->exit_code = 0;
    
    // Set ready callback if provided
    if (settings->ready_callback) {
        webview->ready_callback = settings->ready_callback;
        webview->ready_callback_data = settings->user_data;
    }
    
    return webview;
}

bool webview_load_html(WebView* webview, const char* html) {
    if (!webview || !webview->is_valid || !html) {
        set_error("Invalid parameters");
        return false;
    }
    
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(webview->webview), html, NULL);
    return true;
}

bool webview_load_url(WebView* webview, const char* url) {
    if (!webview || !webview->is_valid || !url) {
        set_error("Invalid parameters");
        return false;
    }
    
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview->webview), url);
    return true;
}

bool webview_show(WebView* webview) {
    if (!webview || !webview->is_valid) {
        set_error("Invalid webview");
        return false;
    }
    
    gtk_widget_show_all(webview->window);
    return true;
}

bool webview_hide(WebView* webview) {
    if (!webview || !webview->is_valid) {
        set_error("Invalid webview");
        return false;
    }
    
    gtk_widget_hide(webview->window);
    return true;
}

// Idle callback to process avatar completions and async requests
static gboolean on_process_avatars(gpointer user_data) {
    (void)user_data;  // Unused
    
    // Process up to 10 avatar completions per idle callback
    int avatars_processed = vm_process_avatar_completions(10);
    
    // Process up to 10 async requests per idle callback
    int requests_processed = vm_process_async_requests(10);
    
    // Continue calling this callback (return TRUE to keep it active)
    return TRUE;
}

int webview_run(WebView* webview) {
    if (!webview || !webview->is_valid) {
        set_error("Invalid webview");
        return -1;
    }
    
    gtk_widget_show_all(webview->window);
    
    // Add idle callback to process avatar completions
    // This runs whenever GTK is idle, ensuring avatars are processed
    g_idle_add(on_process_avatars, NULL);
    
    gtk_main();
    
    return webview->exit_code;
}

bool webview_step(WebView* webview) {
    if (!webview || !webview->is_valid) {
        return false;
    }
    
    if (!webview->window) {
        webview->is_valid = false;
        return false;
    }
    
    // Ensure window is visible (safe to call multiple times)
    if (!gtk_widget_get_visible(webview->window)) {
        gtk_widget_show_all(webview->window);
    }
    
    // Process pending GTK events (non-blocking)
    while (gtk_events_pending()) {
        gtk_main_iteration_do(FALSE);
    }
    
    // Return false if window was closed/destroyed
    return webview->is_valid;
}

bool webview_is_valid(WebView* webview) {
    if (!webview) {
        return false;
    }
    return webview->is_valid;
}

bool webview_eval_js(WebView* webview, const char* js, WebViewContext context) {
    if (!webview || !webview->is_valid || !js) {
        set_error("Invalid parameters");
        return false;
    }
    
    (void)context;  // Suppress unused parameter warning
    webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(webview->webview), js, NULL, NULL, NULL);
    return true;
}

void webview_destroy(WebView* webview) {
    if (!webview) return;
    
    if (webview->window && GTK_IS_WIDGET(webview->window)) {
        gtk_widget_destroy(webview->window);
    }
    
    if (webview->title) {
        free(webview->title);
    }
    
    if (webview->settings) {
        webview_free_settings(webview->settings);
    }
    
    webview->is_valid = false;
    free(webview);
}

#else
// Stub implementation for non-GTK platforms

bool webview_init(void) {
    set_error("WebView not supported on this platform");
    return false;
}

void webview_cleanup(void) {}

WebView* webview_create(const char* title, WebViewSettings* settings) {
    set_error("WebView not supported on this platform");
    return NULL;
}

bool webview_load_html(WebView* webview, const char* html) {
    set_error("WebView not supported on this platform");
    return false;
}

bool webview_load_url(WebView* webview, const char* url) {
    set_error("WebView not supported on this platform");
    return false;
}

bool webview_show(WebView* webview) {
    set_error("WebView not supported on this platform");
    return false;
}

bool webview_hide(WebView* webview) {
    set_error("WebView not supported on this platform");
    return false;
}

int webview_run(WebView* webview) {
    set_error("WebView not supported on this platform");
    return -1;
}

bool webview_eval_js(WebView* webview, const char* js, WebViewContext context) {
    (void)webview; (void)js; (void)context;  // Suppress unused parameter warnings
    set_error("WebView not supported on this platform");
    return false;
}

void webview_destroy(WebView* webview) {
    if (webview) {
        free(webview);
    }
}

#endif

// Common implementation for settings and utility functions

WebViewSettings* webview_create_default_settings(void) {
    WebViewSettings* settings = calloc(1, sizeof(WebViewSettings));
    if (!settings) {
        set_error("Memory allocation failed");
        return NULL;
    }
    
    settings->width = 800;
    settings->height = 600;
    settings->resizable = true;
    settings->maximizable = true;
    settings->minimizable = true;
    settings->debug = false;
    settings->dev_tools = false;
    settings->debug_level = WEBVIEW_DEBUG_NONE;
    settings->app_name = strdup("WebView Application");
    settings->app_version = strdup("1.0.0");
    settings->ready_callback = NULL;
    settings->user_data = NULL;
    
    return settings;
}

void webview_free_settings(WebViewSettings* settings) {
    if (!settings) return;
    
    if (settings->app_name) {
        free(settings->app_name);
    }
    
    if (settings->app_version) {
        free(settings->app_version);
    }
    
    free(settings);
}

const char* webview_get_last_error(void) {
    return g_webview_error;
}

bool webview_center(WebView* webview) {
    if (!webview || !webview->is_valid) {
        set_error("Invalid webview");
        return false;
    }
    
#ifdef WEBVIEW_GTK
    gtk_window_set_position(GTK_WINDOW(webview->window), GTK_WIN_POS_CENTER);
    return true;
#else
    set_error("Center not supported on this platform");
    return false;
#endif
}

// Stub implementations for advanced features
WebView* webview_create_child(WebView* parent, const char* title, WebViewSettings* settings) {
    (void)parent; (void)title; (void)settings;  // Suppress unused parameter warnings
    set_error("Child windows not implemented yet");
    return NULL;
}


#ifdef WEBVIEW_GTK

// Struct to hold bridge callback info
typedef struct {
    WebView* wv;
    WebViewJSCallback cb;
    void* user;
    char* name;
} BridgeData;

// Global JS → C message handler
static void on_js_message(WebKitUserContentManager* manager,
                          WebKitJavascriptResult* js_result,
                          gpointer user_data) {
    (void)manager;
    BridgeData* b = (BridgeData*)user_data;
    if (!b || !b->cb)
        return;

    JSCValue* value = webkit_javascript_result_get_js_value(js_result);
    char* msg = NULL;

    if (jsc_value_is_string(value))
        msg = jsc_value_to_string(value);
    else
        msg = g_strdup("{}");

    b->cb(b->wv, b->name, msg, b->user);
    g_free(msg);
}

#endif // WEBVIEW_GTK

bool webview_bind_function(WebView* webview, const char* name,
                           WebViewJSCallback callback, void* user_data) {
#ifdef WEBVIEW_GTK
    if (!webview || !webview->is_valid || !name || !callback) {
        set_error("Invalid parameters for function binding");
        return false;
    }

    WebKitWebView* wv = WEBKIT_WEB_VIEW(webview->webview);
    WebKitUserContentManager* manager =
        webkit_web_view_get_user_content_manager(wv);

    // Structure to carry callback context
    typedef struct {
        WebView* wv;
        WebViewJSCallback cb;
        void* user;
        char* name;
    } BridgeData;

    BridgeData* bridge = g_new0(BridgeData, 1);
    bridge->wv = webview;
    bridge->cb = callback;
    bridge->user = user_data;
    bridge->name = g_strdup(name);

    // Build signal name like "script-message-received::foo"
    char signal_name[256];
    snprintf(signal_name, sizeof(signal_name),
             "script-message-received::%s", name);

    // Connect the message signal
    g_signal_connect(manager, signal_name,
                     G_CALLBACK(on_js_message), bridge);

    // Register this handler name
    if (!webkit_user_content_manager_register_script_message_handler(manager, name)) {
        set_error("Failed to register message handler");
        g_free(bridge->name);
        g_free(bridge);
        return false;
    }

    // Inject JS shim
    char inject[512];
    snprintf(inject, sizeof(inject),
        "window.%s = function(data) { "
        "  window.webkit.messageHandlers['%s'].postMessage(JSON.stringify(data)); "
        "};",
        name, name);

    webkit_web_view_run_javascript(wv, inject, NULL, NULL, NULL);

    printf("[BIND] ✅ JS bridge registered for '%s'\n", name);
    return true;
#else
    set_error("WebView binding not supported on this platform");
    return false;
#endif
}

char* webview_show_open_dialog(WebView* webview, const char* title, const char* default_path, 
                               const char* filter, bool multiple) {
    (void)webview; (void)title; (void)default_path; (void)filter; (void)multiple;  // Suppress unused parameter warnings
    set_error("File dialogs not implemented yet");
    return NULL;
}

char* webview_show_save_dialog(WebView* webview, const char* title, const char* default_path, 
                               const char* filter) {
    (void)webview; (void)title; (void)default_path; (void)filter;  // Suppress unused parameter warnings
    set_error("File dialogs not implemented yet");
    return NULL;
}

char* webview_show_message_box(WebView* webview, const char* title, const char* message, 
                               const char* type, const char* buttons) {
    (void)webview; (void)title; (void)message; (void)type; (void)buttons;  // Suppress unused parameter warnings
    set_error("Message boxes not implemented yet");
    return NULL;
}

bool webview_set_console_callback(WebView* webview, WebViewConsoleCallback callback, void* user_data) {
    if (webview) {
        webview->console_callback = callback;
        webview->console_callback_data = user_data;
        return true;
    }
    return false;
}

bool webview_add_protocol_handler(WebView* webview, const char* protocol, 
                                  void (*handler)(WebView*, const char*, void*), void* user_data) {
    (void)webview; (void)protocol; (void)handler; (void)user_data;  // Suppress unused parameter warnings
    set_error("Protocol handlers not implemented yet");
    return false;
}

bool webview_remove_protocol_handler(WebView* webview, const char* protocol) {
    (void)webview; (void)protocol;  // Suppress unused parameter warnings
    set_error("Protocol handler removal not implemented yet");
    return false;
}

bool webview_set_user_agent(WebView* webview, const char* user_agent) {
    (void)webview; (void)user_agent;  // Suppress unused parameter warnings
    set_error("User agent setting not implemented yet");
    return false;
}

char* webview_get_user_agent(WebView* webview) {
    (void)webview;  // Suppress unused parameter warning
    set_error("User agent getting not implemented yet");
    return NULL;
}

bool webview_enable_dev_tools(WebView* webview, bool enable) {
    if (!webview || !webview->is_valid) {
        set_error("Invalid webview");
        return false;
    }
    
#ifdef WEBVIEW_GTK
    if (!webview->webview) {
        set_error("WebView not initialized");
        return false;
    }
    
    // webkit_web_view_get_settings MUST be called from the GTK main thread.
    // Use g_idle_add to schedule the call safely.
    
    typedef struct {
        GtkWidget* webview_widget;
        gboolean enable;
    } DevToolsData;
    
    DevToolsData* data = g_new0(DevToolsData, 1);
    data->webview_widget = webview->webview;
    data->enable = enable ? TRUE : FALSE;
    
    // Idle callback - runs on GTK main thread
    gboolean idle_callback(gpointer user_data) {
        DevToolsData* d = (DevToolsData*)user_data;
        if (d && d->webview_widget && GTK_IS_WIDGET(d->webview_widget)) {
            WebKitSettings* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(d->webview_widget));
            if (settings) {
                webkit_settings_set_enable_developer_extras(settings, d->enable);
            }
        }
        g_free(d);
        return G_SOURCE_REMOVE; // one-shot callback
    }
    
    g_idle_add(idle_callback, data);
    return true;
#else
    (void)enable;
    set_error("Developer tools only supported on GTK platform");
    return false;
#endif
}

bool webview_set_zoom_level(WebView* webview, double zoom) {
    (void)webview; (void)zoom;  // Suppress unused parameter warnings
    set_error("Zoom level setting not implemented yet");
    return false;
}

double webview_get_zoom_level(WebView* webview) {
    (void)webview;  // Suppress unused parameter warning
    set_error("Zoom level getting not implemented yet");
    return 1.0;
}

void webview_refresh(WebView* webview) {
#ifdef WEBVIEW_GTK
    if (webview && webview->is_valid) {
        webkit_web_view_reload(WEBKIT_WEB_VIEW(webview->webview));
    }
#endif
}

bool webview_can_go_back(WebView* webview) {
#ifdef WEBVIEW_GTK
    if (webview && webview->is_valid) {
        return webkit_web_view_can_go_back(WEBKIT_WEB_VIEW(webview->webview));
    }
#endif
    return false;
}

bool webview_can_go_forward(WebView* webview) {
#ifdef WEBVIEW_GTK
    if (webview && webview->is_valid) {
        return webkit_web_view_can_go_forward(WEBKIT_WEB_VIEW(webview->webview));
    }
#endif
    return false;
}

bool webview_go_back(WebView* webview) {
#ifdef WEBVIEW_GTK
    if (webview && webview->is_valid) {
        webkit_web_view_go_back(WEBKIT_WEB_VIEW(webview->webview));
        return true;
    }
#endif
    return false;
}

bool webview_go_forward(WebView* webview) {
#ifdef WEBVIEW_GTK
    if (webview && webview->is_valid) {
        webkit_web_view_go_forward(WEBKIT_WEB_VIEW(webview->webview));
        return true;
    }
#endif
    return false;
}

char* webview_get_url(WebView* webview) {
#ifdef WEBVIEW_GTK
    if (webview && webview->is_valid) {
        const char* url = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(webview->webview));
        return url ? strdup(url) : NULL;
    }
#endif
    return NULL;
}

char* webview_get_title(WebView* webview) {
#ifdef WEBVIEW_GTK
    if (webview && webview->is_valid) {
        const char* title = webkit_web_view_get_title(WEBKIT_WEB_VIEW(webview->webview));
        return title ? strdup(title) : NULL;
    }
#endif
    return NULL;
}
// Register a callback to process VM task queue during GTK main loop
void webview_set_task_processor(int (*processor)(int max_tasks)) {
    g_task_processor = processor;
}
