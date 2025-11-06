#define _GNU_SOURCE  // For strdup
#include "webview_utils.h"
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

// Helper functions
static void set_error(const char* message) {
    snprintf(g_webview_error, sizeof(g_webview_error), "%s", message);
}

#ifdef WEBVIEW_GTK
// GTK/WebKit implementation

static gboolean on_window_delete(GtkWidget* widget, GdkEvent* event, gpointer data) {
    (void)widget; (void)event;  // Suppress unused parameter warnings
    WebView* webview = (WebView*)data;
    if (webview) {
        webview->exit_code = 0;
        gtk_main_quit();
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

int webview_run(WebView* webview) {
    if (!webview || !webview->is_valid) {
        set_error("Invalid webview");
        return -1;
    }
    
    gtk_widget_show_all(webview->window);
    gtk_main();
    
    return webview->exit_code;
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

bool webview_bind_function(WebView* webview, const char* name, WebViewJSCallback callback, void* user_data) {
    (void)webview; (void)name; (void)callback; (void)user_data;  // Suppress unused parameter warnings
    set_error("Function binding not implemented yet");
    return false;
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
    (void)webview; (void)enable;  // Suppress unused parameter warnings
    set_error("Developer tools not implemented yet");
    return false;
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