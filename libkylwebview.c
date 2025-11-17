#include "webview_utils.h"
#include <stddef.h>
#include <string.h>
#include "../../src/ast.h"  // Use VM's Value definition
#include "../../src/bytecode.h"  // For Function struct
#include "../../src/vm_library_integration.h"  // For call_kuyil_function

// External VM function to process task queue
extern int vm_process_pending_tasks(int max_tasks);

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
    "webview step(window: int32) -> bool\n"
    "webview is_open(window: int32) -> bool\n"
    "webview eval(window: int32, script: string) -> bool\n"
    "webview setTitle(window: int32, title: string) -> bool\n"
    "webview setSize(window: int32, width: int32, height: int32) -> bool\n"
    "webview bind(window: int32, func_name: string) -> bool\n"
    "webview enableDevTools(window: int32, enable: bool) -> bool\n"
    "webview destroy(window: int32) -> bool\n"
    "webview cleanup() -> void\n";

// Global webview handles
static WebView* g_webviews[10] = {NULL};
static int g_webview_count = 0;

// Callback storage for JS->Kuyil bridge
#define MAX_BOUND_FUNCTIONS 50
typedef struct {
    char name[64];
    int webview_handle;
    void* kuyil_callback;  // Will store Kuyil function pointer
} BoundFunction;

static BoundFunction g_bound_functions[MAX_BOUND_FUNCTIONS];
static int g_bound_count = 0;

// Helper: Get webview from handle (number)
static WebView* get_webview(int handle) {
    if (handle < 0 || handle >= g_webview_count || handle >= 10) {
        return NULL;
    }
    return g_webviews[handle];
}

// Native callback that bridges to Kuyil
static void native_callback_bridge(WebView* webview, const char* name, const char* args, void* user_data) {
    // fprintf(stderr, "[BRIDGE CALLED] name=%s, args=%s\n", name, args);
    BoundFunction* bound = (BoundFunction*)user_data;
    if (!bound || !bound->kuyil_callback) {
        fprintf(stderr, "[ERROR] No Kuyil callback stored for %s\n", name);
        return;
    }
    
    // Get the stored Kuyil function
    Value* callback_value = (Value*)bound->kuyil_callback;
    if (callback_value->type != VALUE_FUNCTION) {
        fprintf(stderr, "[ERROR] Stored callback for %s is not a function (type=%d)\n", name, callback_value->type);
        return;
    }
    
    // TODO: Parse args JSON and convert to Kuyil Values
    // For now, just call with the raw string as a single argument
    // WebView passes args as JSON, so a string "hello" comes as "\"hello\""
    // We need to strip the outer quotes for simple strings
    const char* actual_string = args;
    char* temp_buffer = NULL;
    
    if (args && strlen(args) >= 2 && args[0] == '"' && args[strlen(args)-1] == '"') {
        // Strip outer quotes and unescape
        size_t len = strlen(args) - 2;
        temp_buffer = malloc(len + 1);
        size_t j = 0;
        for (size_t i = 1; i < strlen(args) - 1; i++) {
            if (args[i] == '\\' && i + 1 < strlen(args) - 1) {
                // Unescape common sequences
                if (args[i+1] == '"') {
                    temp_buffer[j++] = '"';
                    i++;
                } else if (args[i+1] == '\\') {
                    temp_buffer[j++] = '\\';
                    i++;
                } else if (args[i+1] == 'n') {
                    temp_buffer[j++] = '\n';
                    i++;
                } else if (args[i+1] == 't') {
                    temp_buffer[j++] = '\t';
                    i++;
                } else {
                    temp_buffer[j++] = args[i];
                }
            } else {
                temp_buffer[j++] = args[i];
            }
        }
        temp_buffer[j] = '\0';
        actual_string = temp_buffer;
    }
    
    Value kuyil_args[1];
    kuyil_args[0].type = VALUE_STRING;
    kuyil_args[0].as.string = (char*)actual_string;
    
    // Call the Kuyil function
    Value result;
    bool success = call_kuyil_function(*callback_value, 1, kuyil_args, &result);
    
    // Clean up temp buffer if we allocated one
    if (temp_buffer) {
        free(temp_buffer);
    }
    
    if (!success) {
        Function* func = callback_value->as.function.function;
        const char* func_name = (func && func->name) ? func->name : "<anonymous>";
        fprintf(stderr, "[ERROR] Failed to call Kuyil function %s\n", func_name);
        return;
    }
    
    // Store the return value in a global JS variable that can be retrieved
    if (result.type == VALUE_STRING && result.as.string) {
        // fprintf(stderr, "[BRIDGE] Function %s returned string, setting __kuyilReturn\n", name);
        // Escape the string for JavaScript
        // For now, store it directly - JS will need to handle escaping
        char js_cmd[32768];  // Large buffer for response
        snprintf(js_cmd, sizeof(js_cmd), "window.__kuyilReturn = %s;", result.as.string);
        webview_eval_js(webview, js_cmd, WEBVIEW_CONTEXT_MAIN);
    }
    
    (void)name;     // Unused for now
}

// Kuyil wrapper functions
Value kyl_webview_init(int arg_count, Value* args) {
    (void)arg_count; (void)args;
    
    bool success = webview_init();
    
    // Register VM task processor to be called from GTK main loop
    if (success) {
        webview_set_task_processor(vm_process_pending_tasks);
    }
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = success;
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

Value kyl_webview_step(int arg_count, Value* args) {
    if (arg_count < 1 || args[0].type != VALUE_NUMBER) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    bool should_continue = webview_step(webview);
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = should_continue;
    return result;
}

Value kyl_webview_is_open(int arg_count, Value* args) {
    if (arg_count < 1 || args[0].type != VALUE_NUMBER) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    WebView* webview = get_webview((int)args[0].as.number);
    if (!webview) {
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    // Check if the webview is still valid/open using the utility function
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = webview_is_valid(webview);
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

// NEW: Bind a JavaScript function to call back into Kuyil
// Usage: webview.bind(window, "functionName", kuyilCallbackFunction)
// In JS: window.functionName('data') will trigger the Kuyil callback
Value kyl_webview_bind(int arg_count, Value* args) {
    fprintf(stderr, "[DEBUG] kyl_webview_bind called with %d args\n", arg_count);
    
    if (arg_count < 3) {
        fprintf(stderr, "[ERROR] webview_bind requires 3 args, got %d\n", arg_count);
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    fprintf(stderr, "[DEBUG] arg types: [0]=%d [1]=%d [2]=%d\n", args[0].type, args[1].type, args[2].type);
    
    if (args[0].type != VALUE_NUMBER) {
        fprintf(stderr, "[ERROR] webview_bind: arg 0 must be NUMBER (window handle), got type %d\n", args[0].type);
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    if (args[1].type != VALUE_STRING) {
        fprintf(stderr, "[ERROR] webview_bind: arg 1 must be STRING (func_name), got type %d\n", args[1].type);
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    if (args[2].type != VALUE_FUNCTION) {
        fprintf(stderr, "[ERROR] webview_bind: arg 2 must be FUNCTION (callback), got type %d\n", args[2].type);
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    int handle = (int)args[0].as.number;
    fprintf(stderr, "[DEBUG] window handle: %d\n", handle);
    
    WebView* webview = get_webview(handle);
    if (!webview) {
        fprintf(stderr, "[ERROR] webview_bind: Invalid window handle %d\n", handle);
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    const char* func_name = args[1].as.string;
    fprintf(stderr, "[DEBUG] func_name: %s\n", func_name);
    
    Value callback = args[2];
    
    // Store bound function info
    if (g_bound_count >= MAX_BOUND_FUNCTIONS) {
        fprintf(stderr, "[ERROR] webview_bind: Max bound functions reached\n");
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    BoundFunction* bound = &g_bound_functions[g_bound_count];
    strncpy(bound->name, func_name, sizeof(bound->name) - 1);
    bound->webview_handle = handle;
    bound->kuyil_callback = malloc(sizeof(Value));
    if (!bound->kuyil_callback) {
        fprintf(stderr, "[ERROR] webview_bind: Failed to allocate callback memory\n");
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    memcpy(bound->kuyil_callback, &callback, sizeof(Value));
    g_bound_count++;
    
    fprintf(stderr, "[DEBUG] Calling webview_bind_function for '%s'\n", func_name);
    
    // Bind the function in the webview
    bool success = webview_bind_function(webview, func_name, native_callback_bridge, bound);
    
    fprintf(stderr, "[DEBUG] webview_bind_function returned: %d\n", success);
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = success;
    
    return result;
}

// Enable developer tools/inspector
Value kyl_webview_enableDevTools(int arg_count, Value* args) {
    if (arg_count < 2 || args[0].type != VALUE_NUMBER || args[1].type != VALUE_BOOL) {
        fprintf(stderr, "[ERROR] webview_enableDevTools requires 2 args: window (number), enable (bool)\n");
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    int handle = (int)args[0].as.number;
    WebView* webview = get_webview(handle);
    
    if (!webview) {
        fprintf(stderr, "[ERROR] webview_enableDevTools: Invalid window handle %d\n", handle);
        Value result = {VALUE_BOOL};
        result.as.boolean = false;
        return result;
    }
    
    bool enable = args[1].as.boolean;
    bool success = webview_enable_dev_tools(webview, enable);
    
    Value result;
    result.type = VALUE_BOOL;
    result.as.boolean = success;
    
    return result;
}
