#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "webview_utils.h"

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("✓ %s\n", message); \
    } else { \
        printf("✗ %s\n", message); \
        printf("  Error: %s\n", webview_get_last_error()); \
    } \
} while(0)

// Test data and callback functions
static bool callback_called = false;
static char* callback_message = NULL;

void test_js_callback(WebView* webview, const char* name, const char* args, void* user_data) {
    callback_called = true;
    if (callback_message) free(callback_message);
    callback_message = strdup(args);
    printf("  JS Callback called: %s with args: %s\n", name, args);
}

void test_message_handler(WebView* webview, const char* message, void* user_data) {
    printf("  Message received: %s\n", message);
}

void test_console_callback(WebView* webview, const char* message, const char* level, void* user_data) {
    printf("  Console [%s]: %s\n", level, message);
}

void test_initialization() {
    printf("\n=== Testing WebView Initialization ===\n");
    
    // Test system initialization
    bool init_ok = webview_init();
    TEST_ASSERT(init_ok, "WebView system initialization");
    
    // Test default settings creation
    WebViewSettings* settings = webview_create_default_settings();
    TEST_ASSERT(settings != NULL, "Create default settings");
    
    if (settings) {
        TEST_ASSERT(settings->width == 800, "Default width is 800");
        TEST_ASSERT(settings->height == 600, "Default height is 600");
        TEST_ASSERT(settings->resizable == true, "Default resizable is true");
        TEST_ASSERT(settings->javascript == true, "JavaScript enabled by default");
        
        webview_free_settings(settings);
    }
    
    // Test version information
    const char* version = webview_utils_get_version();
    TEST_ASSERT(version && strcmp(version, "1.0.0") == 0, "Library version is correct");
    
    char* engine_info = webview_get_engine_info();
    TEST_ASSERT(engine_info != NULL, "Engine information available");
    if (engine_info) {
        printf("  Engine: %s\n", engine_info);
        free(engine_info);
    }
}

void test_webview_creation() {
    printf("\n=== Testing WebView Creation ===\n");
    
    // Test basic webview creation
    WebView* webview = webview_create("Test Window", NULL);
    TEST_ASSERT(webview != NULL, "Create webview with default settings");
    TEST_ASSERT(webview_is_valid(webview), "Webview is valid");
    
    if (webview) {
        // Test window properties
        char* title = webview_get_window_title(webview);
        TEST_ASSERT(title && strcmp(title, "Test Window") == 0, "Window title is correct");
        free(title);
        
        // Test size getting/setting
        int width, height;
        bool size_ok = webview_get_size(webview, &width, &height);
        TEST_ASSERT(size_ok, "Get window size");
        printf("  Window size: %dx%d\n", width, height);
        
        bool resize_ok = webview_set_size(webview, 1024, 768);
        TEST_ASSERT(resize_ok, "Set window size");
        
        // Test title setting
        bool title_ok = webview_set_title(webview, "Updated Title");
        TEST_ASSERT(title_ok, "Set window title");
        
        webview_destroy(webview);
    }
}

void test_custom_settings() {
    printf("\n=== Testing Custom Settings ===\n");
    
    WebViewSettings* settings = webview_create_default_settings();
    if (!settings) return;
    
    // Customize settings
    settings->width = 1200;
    settings->height = 800;
    settings->debug = true;
    settings->debug_level = WEBVIEW_DEBUG_INFO;
    settings->dev_tools = true;
    settings->transparent = false;
    
    if (settings->app_name) free(settings->app_name);
    settings->app_name = strdup("Custom Test App");
    
    WebView* webview = webview_create("Custom Window", settings);
    TEST_ASSERT(webview != NULL, "Create webview with custom settings");
    
    if (webview) {
        int width, height;
        webview_get_size(webview, &width, &height);
        TEST_ASSERT(width == 1200 && height == 800, "Custom size applied");
        
        webview_destroy(webview);
    }
    
    // Don't free settings here - webview_destroy should handle it
}

void test_content_loading() {
    printf("\n=== Testing Content Loading ===\n");
    
    WebView* webview = webview_create("Content Test", NULL);
    if (!webview) return;
    
    // Test HTML loading
    const char* test_html = 
        "<!DOCTYPE html>"
        "<html><head><title>Test Page</title></head>"
        "<body><h1>Hello, WebView!</h1>"
        "<p id='test-content'>This is a test page.</p>"
        "<script>console.log('Page loaded!');</script>"
        "</body></html>";
    
    bool html_ok = webview_load_html(webview, test_html);
    TEST_ASSERT(html_ok, "Load HTML content");
    
    // Test URL navigation (commented out as it requires network)
    /*
    bool nav_ok = webview_navigate(webview, "https://example.com");
    TEST_ASSERT(nav_ok, "Navigate to URL");
    */
    
    // Test JavaScript evaluation
    bool js_ok = webview_eval_js(webview, "document.title = 'Updated by JS';", WEBVIEW_CONTEXT_MAIN);
    TEST_ASSERT(js_ok, "Execute JavaScript");
    
    webview_destroy(webview);
}

void test_javascript_integration() {
    printf("\n=== Testing JavaScript Integration ===\n");
    
    WebView* webview = webview_create("JS Test", NULL);
    if (!webview) return;
    
    // Load test HTML
    const char* test_html = 
        "<!DOCTYPE html>"
        "<html><body>"
        "<div id='output'>Initial content</div>"
        "<script>"
        "function updateContent(text) {"
        "  document.getElementById('output').textContent = text;"
        "  return 'Content updated to: ' + text;"
        "}"
        "function getContent() {"
        "  return document.getElementById('output').textContent;"
        "}"
        "</script>"
        "</body></html>";
    
    webview_load_html(webview, test_html);
    
    // Test function binding
    callback_called = false;
    bool bind_ok = webview_bind_function(webview, "nativeFunction", test_js_callback, NULL);
    TEST_ASSERT(bind_ok, "Bind native function to JavaScript");
    
    // Test JavaScript function calling
    char* result = webview_call_js_function(webview, "updateContent", "[\"Hello from native!\"]");
    TEST_ASSERT(result != NULL, "Call JavaScript function from native");
    if (result) {
        printf("  JS function result: %s\n", result);
        free(result);
    }
    
    // Test script addition
    const char* init_script = "window.nativeReady = true;";
    bool script_ok = webview_add_script(webview, init_script, WEBVIEW_CONTEXT_MAIN);
    TEST_ASSERT(script_ok, "Add initialization script");
    
    webview_destroy(webview);
}

void test_window_management() {
    printf("\n=== Testing Window Management ===\n");
    
    WebView* webview = webview_create("Window Management Test", NULL);
    if (!webview) return;
    
    // Test position getting/setting
    int x, y;
    bool pos_ok = webview_get_position(webview, &x, &y);
    TEST_ASSERT(pos_ok, "Get window position");
    printf("  Window position: %d, %d\n", x, y);
    
    bool set_pos_ok = webview_set_position(webview, 100, 100);
    TEST_ASSERT(set_pos_ok, "Set window position");
    
    // Test centering
    bool center_ok = webview_center(webview);
    TEST_ASSERT(center_ok, "Center window on screen");
    
    // Test window state
    WebViewWindowState state = webview_get_state(webview);
    TEST_ASSERT(state == WEBVIEW_STATE_NORMAL, "Get window state");
    
    bool state_ok = webview_set_state(webview, WEBVIEW_STATE_MAXIMIZED);
    TEST_ASSERT(state_ok, "Set window state");
    
    // Test always on top
    bool ontop_ok = webview_set_always_on_top(webview, true);
    TEST_ASSERT(ontop_ok, "Set always on top");
    
    webview_destroy(webview);
}

void test_developer_tools() {
    printf("\n=== Testing Developer Tools ===\n");
    
    WebViewSettings* settings = webview_create_default_settings();
    if (!settings) return;
    
    settings->debug = true;
    settings->dev_tools = true;
    settings->debug_level = WEBVIEW_DEBUG_VERBOSE;
    
    WebView* webview = webview_create("DevTools Test", settings);
    if (!webview) return;
    
    // Test debug mode setting
    bool debug_ok = webview_set_debug_mode(webview, true, WEBVIEW_DEBUG_INFO);
    TEST_ASSERT(debug_ok, "Set debug mode");
    
    // Test console callback
    bool console_ok = webview_set_console_callback(webview, test_console_callback, NULL);
    TEST_ASSERT(console_ok, "Set console callback");
    
    // Test developer tools
    bool devtools_open_ok = webview_open_dev_tools(webview);
    TEST_ASSERT(devtools_open_ok, "Open developer tools");
    
    bool devtools_close_ok = webview_close_dev_tools(webview);
    TEST_ASSERT(devtools_close_ok, "Close developer tools");
    
    // Test debug printing
    webview_debug_print(webview, WEBVIEW_DEBUG_INFO, "This is a debug message");
    
    webview_destroy(webview);
}

void test_utility_functions() {
    printf("\n=== Testing Utility Functions ===\n");
    
    // Test screen size
    int screen_width, screen_height;
    bool screen_ok = webview_get_screen_size(&screen_width, &screen_height);
    TEST_ASSERT(screen_ok, "Get screen size");
    printf("  Screen size: %dx%d\n", screen_width, screen_height);
    
    // Test work area
    int work_x, work_y, work_width, work_height;
    bool work_ok = webview_get_work_area(&work_x, &work_y, &work_width, &work_height);
    TEST_ASSERT(work_ok, "Get work area");
    printf("  Work area: %d,%d %dx%d\n", work_x, work_y, work_width, work_height);
    
    // Test feature support
    bool js_supported = webview_is_feature_supported("javascript");
    TEST_ASSERT(js_supported, "JavaScript feature supported");
    
    bool fake_supported = webview_is_feature_supported("fake_feature");
    TEST_ASSERT(!fake_supported, "Fake feature not supported");
    
    // Test supported features list
    int feature_count;
    char** features = webview_get_supported_features(&feature_count);
    TEST_ASSERT(features != NULL && feature_count > 0, "Get supported features list");
    
    if (features) {
        printf("  Supported features (%d):\n", feature_count);
        for (int i = 0; i < feature_count; i++) {
            printf("    - %s\n", features[i]);
            free(features[i]);
        }
        free(features);
    }
}

void test_multi_window() {
    printf("\n=== Testing Multi-Window Support ===\n");
    
    // Create parent window
    WebView* parent = webview_create("Parent Window", NULL);
    TEST_ASSERT(parent != NULL, "Create parent window");
    
    if (parent) {
        // Create child windows
        WebView* child1 = webview_create_child(parent, "Child Window 1", NULL);
        TEST_ASSERT(child1 != NULL, "Create first child window");
        
        WebView* child2 = webview_create_child(parent, "Child Window 2", NULL);
        TEST_ASSERT(child2 != NULL, "Create second child window");
        
        if (child1 && child2) {
            // Test parent-child relationships
            WebView* parent_of_child1 = webview_get_parent(child1);
            TEST_ASSERT(parent_of_child1 == parent, "Child1 has correct parent");
            
            WebView* parent_of_child2 = webview_get_parent(child2);
            TEST_ASSERT(parent_of_child2 == parent, "Child2 has correct parent");
            
            // Test children list
            int child_count;
            WebView** children = webview_get_children(parent, &child_count);
            TEST_ASSERT(children != NULL && child_count == 2, "Parent has correct children count");
            
            if (children) {
                TEST_ASSERT(children[0] == child1 || children[0] == child2, "First child is correct");
                TEST_ASSERT(children[1] == child1 || children[1] == child2, "Second child is correct");
                free(children);
            }
        }
        
        // Test closing all children
        int closed_count = webview_close_all_children(parent);
        TEST_ASSERT(closed_count == 2, "Closed correct number of children");
        
        webview_destroy(parent);
    }
}

void test_message_system() {
    printf("\n=== Testing Message System ===\n");
    
    WebView* webview = webview_create("Message Test", NULL);
    if (!webview) return;
    
    // Set up message handler
    bool handler_ok = webview_set_message_handler(webview, test_message_handler, NULL);
    TEST_ASSERT(handler_ok, "Set message handler");
    
    // Test message posting
    const char* test_message = "{\"type\":\"test\",\"data\":\"Hello from native!\"}";
    bool post_ok = webview_post_message(webview, test_message);
    TEST_ASSERT(post_ok, "Post message to JavaScript");
    
    webview_destroy(webview);
}

void test_error_handling() {
    printf("\n=== Testing Error Handling ===\n");
    
    // Test error with invalid webview
    bool invalid_result = webview_set_title(NULL, "Invalid");
    TEST_ASSERT(!invalid_result, "Invalid webview handled correctly");
    
    const char* error = webview_get_last_error();
    TEST_ASSERT(error && strlen(error) > 0, "Error message set");
    printf("  Expected error: %s\n", error);
    
    // Test error clearing
    webview_clear_error();
    error = webview_get_last_error();
    TEST_ASSERT(error && strlen(error) == 0, "Error cleared");
    
    // Test invalid parameters
    WebView* webview = webview_create("Error Test", NULL);
    if (webview) {
        bool bind_result = webview_bind_function(webview, NULL, NULL, NULL);
        TEST_ASSERT(!bind_result, "Invalid bind parameters handled");
        
        webview_destroy(webview);
    }
}

void test_protocol_handlers() {
    printf("\n=== Testing Protocol Handlers ===\n");
    
    WebView* webview = webview_create("Protocol Test", NULL);
    if (!webview) return;
    
    // Test protocol handler setting (simplified test)
    bool protocol_ok = webview_set_protocol_handler(webview, "app", 
        (void (*)(WebView*, const char*, void*))test_message_handler, NULL);
    TEST_ASSERT(protocol_ok, "Set custom protocol handler");
    
    // Test protocol handler removal
    bool remove_ok = webview_remove_protocol_handler(webview, "app");
    TEST_ASSERT(remove_ok, "Remove protocol handler");
    
    // Test file serving (placeholder test)
    bool serve_ok = webview_serve_file(webview, "/test.html", "/path/to/test.html");
    TEST_ASSERT(serve_ok, "Set up file serving");
    
    webview_destroy(webview);
}

void test_cleanup() {
    printf("\n=== Testing Cleanup ===\n");
    
    // Test system cleanup
    webview_cleanup();
    printf("✓ WebView system cleanup completed\n");
    
    // Cleanup test data
    if (callback_message) {
        free(callback_message);
        callback_message = NULL;
    }
}

int main() {
    printf("WebView Utils Library Test Suite\n");
    printf("================================\n");
    
    // Note: Some tests may not work in headless environments
    // These tests are designed to verify the API without requiring a display
    
    test_initialization();
    test_webview_creation();
    test_custom_settings();
    test_content_loading();
    test_javascript_integration();
    test_window_management();
    test_developer_tools();
    test_utility_functions();
    test_multi_window();
    test_message_system();
    test_error_handling();
    test_protocol_handlers();
    test_cleanup();
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", 
           tests_run > 0 ? (double)tests_passed / tests_run * 100 : 0);
    
    if (tests_passed == tests_run) {
        printf("\n🎉 All tests passed!\n");
        printf("\nNote: This test suite verifies the API functionality.\n");
        printf("Some features require a display server to test completely.\n");
        printf("Run the example application to test visual functionality.\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed!\n");
        return 1;
    }
}