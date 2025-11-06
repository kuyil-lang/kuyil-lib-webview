#include "webview_utils.h"
#include <stddef.h>
#include <string.h>
#include "../../src/ast.h"  // Use VM's Value definition

// Kuyil interface signature metadata
// Format: "interface method(param:type,...) -> return_type"
// This allows the loader to auto-discover and bind methods with type information
__attribute__((visibility("default")))
const char* kyl_interface_signature_text = 
    "webview init() -> bool\n"
    "webview createDefaultSettings() -> int32\n"
    "webview create(title: string, settings: int32) -> int32\n"
    "webview show(window: int32) -> bool\n"
    "webview hide(window: int32) -> bool\n"
    "webview loadHtml(window: int32, html: string) -> bool\n"
    "webview loadUrl(window: int32, url: string) -> bool\n"
    "webview run(window: int32) -> int32\n"
    "webview eval(window: int32, script: string) -> bool\n"
    "webview setTitle(window: int32, title: string) -> bool\n"
    "webview setSize(window: int32, width: int32, height: int32) -> bool\n"
    "webview destroy(window: int32) -> bool\n"
    "webview cleanup() -> void\n";

// Global webview handles
static WebView* g_webviews[10] = {NULL};
static int g_webview_count = 0;

// Helper: Get webview from handle (number)
static WebView* get_webview(int handle) {
    if (handle < 0 || handle >= g_webview_count || handle >= 10) {
        return NULL;
    }
    return g_webviews[handle];
}

// Kuyil wrapper functions
Value kyl_webview_init(int arg_count, Value* args) {
    (void)arg_count; (void)args;
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = webview_init();
    return result;
}

Value kyl_webview_create_default_settings(int arg_count, Value* args) {
    (void)arg_count; (void)args;

    WebViewSettings* settings = webview_create_default_settings();
    Value result;
    if (!settings) {
        result.type = VALUE_NIL;
        return result;
    }
    result.type = VALUE_NUMBER;
    result.as.number = (double)(intptr_t)settings;
    return result;
}

Value kyl_webview_create(int arg_count, Value* args) {
    if (arg_count < 2 || args[0].type != VALUE_STRING || args[1].type != VALUE_NUMBER) {
        Value result = {VALUE_NIL};
        return result;
    }

    const char* title = args[0].as.string;
    WebViewSettings* settings = (WebViewSettings*)(intptr_t)args[1].as.number;

    WebView* webview = webview_create(title, settings);
    if (!webview || g_webview_count >= 10) {
        Value result = {VALUE_NIL};
        return result;
    }

    int handle = g_webview_count;
    g_webviews[handle] = webview;
    g_webview_count++;

    Value result;
    result.type = VALUE_NUMBER;
    result.as.number = handle;
    return result;
}

Value kyl_webview_show(int arg_count, Value* args) {
    if (arg_count < 1 || args[0].type != VALUE_NUMBER) {
        Value result = {VALUE_NIL};
        return result;
    }
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    webview_show(webview);
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = true;
    return result;
}

Value kyl_webview_hide(int arg_count, Value* args) {
    if (arg_count < 1 || args[0].type != VALUE_NUMBER) {
        Value result = {VALUE_NIL};
        return result;
    }
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    webview_hide(webview);
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = true;
    return result;
}

Value kyl_webview_cleanup(int arg_count, Value* args) {
    (void)arg_count; (void)args;
    
    webview_cleanup();
    g_webview_count = 0;
    
    Value result;
    result.type = VALUE_NIL;
    return result;
}

Value kyl_webview_create_window(int arg_count, Value* args) {
    if (arg_count < 1 || args[0].type != VALUE_STRING) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    // Create default settings
    WebViewSettings* settings = webview_create_default_settings();
    if (!settings) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    WebView* webview = webview_create(args[0].as.string, settings);
    if (!webview || g_webview_count >= 10) {
        webview_destroy_settings(settings);
        Value result = {VALUE_NIL};
        return result;
    }
    
    int handle = g_webview_count;
    g_webviews[handle] = webview;
    g_webview_count++;
    
    Value result;
    result.type = VALUE_NUMBER;
    result.as.number = handle;
    return result;
}

Value kyl_webview_load_html(int arg_count, Value* args) {
    if (arg_count < 2 || args[0].type != VALUE_NUMBER || args[1].type != VALUE_STRING) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    bool success = webview_load_html(webview, args[1].as.string);
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = success;
    return result;
}

Value kyl_webview_load_url(int arg_count, Value* args) {
    if (arg_count < 2 || args[0].type != VALUE_NUMBER || args[1].type != VALUE_STRING) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    bool success = webview_load_url(webview, args[1].as.string);
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = success;
    return result;
}

Value kyl_webview_run(int arg_count, Value* args) {
    if (arg_count < 1 || args[0].type != VALUE_NUMBER) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_NUMBER};
        result.as.number = -1;
        return result;
    }
    
    int exit_code = webview_run(webview);
    
    Value result;
    result.type = VALUE_NUMBER;
    result.as.number = exit_code;
    return result;
}

Value kyl_webview_eval(int arg_count, Value* args) {
    if (arg_count < 2 || args[0].type != VALUE_NUMBER || args[1].type != VALUE_STRING) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    bool success = webview_eval_js(webview, args[1].as.string, WEBVIEW_CONTEXT_MAIN);
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = success;
    return result;
}

Value kyl_webview_set_title(int arg_count, Value* args) {
    if (arg_count < 2 || args[0].type != VALUE_NUMBER || args[1].type != VALUE_STRING) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    webview_set_title(webview, args[1].as.string);
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = true;
    return result;
}

Value kyl_webview_set_size(int arg_count, Value* args) {
    if (arg_count < 3 || args[0].type != VALUE_NUMBER || 
        args[1].type != VALUE_NUMBER || args[2].type != VALUE_NUMBER) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    webview_set_size(webview, (int)args[1].as.number, (int)args[2].as.number);
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = true;
    return result;
}

Value kyl_webview_destroy(int arg_count, Value* args) {
    if (arg_count < 1 || args[0].type != VALUE_NUMBER) {
        Value result = {VALUE_NIL};
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    webview_destroy(webview);
    g_webviews[(int)args[0].as.number] = NULL;
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = true;
    return result;
}
