# WebView Desktop Applications with Kuyil

The WebView utilities library enables you to create modern desktop applications using HTML, CSS, and JavaScript for the user interface, while leveraging Kuyil's native performance for application logic.

## Features

- **Modern UI**: Use HTML5, CSS3, and JavaScript for rich user interfaces
- **Native Performance**: Core application logic runs natively in Kuyil
- **Cross-Platform**: Works on Linux, macOS, and Windows
- **JavaScript Integration**: Seamless communication between web UI and native code
- **Native Dialogs**: File dialogs, message boxes, and system integration
- **Multi-Window Support**: Create multiple windows and dialogs
- **Developer Tools**: Built-in debugging and development tools

## Quick Start

### 1. Build the WebView Library

```bash
cd ./shared_libs/webview
make clean && make
```

### 2. Run the C Example

```bash
# Build and run the C example application
./build_example.sh
cd build
LD_LIBRARY_PATH=. ./webview_example
```

### 3. Run the Kuyil Example

```kuyil
# Load and run the Kuyil desktop app
kuyil kuyil_desktop_app.kyl
```

## Creating Your Own Desktop App

### Basic Kuyil Desktop App Template

```kuyil
# Load the webview shared library
load_shared_lib("./shared_libs/webview/webview_utils.so", "webview")

# Your HTML content
var app_html = "
<!DOCTYPE html>
<html>
<head>
    <title>My Kuyil App</title>
    <style>
        body { 
            font-family: system-ui; 
            padding: 20px; 
            background: #f0f0f0; 
        }
        button { 
            padding: 10px 20px; 
            margin: 5px; 
            border: none; 
            border-radius: 5px; 
            background: #007acc; 
            color: white; 
            cursor: pointer; 
        }
    </style>
</head>
<body>
    <h1>Hello from Kuyil!</h1>
    <button onclick='myNativeFunction()'>Click Me</button>
    <div id='output'>Ready</div>
</body>
</html>
"

# Native function called from JavaScript
func handle_button_click() {
    print("Button clicked from JavaScript!")
    
    # Update the UI
    var js = "document.getElementById('output').textContent = 'Button was clicked!';"
    webview_eval_js(window, js, 0)
}

# Page ready callback
func on_ready() {
    # Bind native function to JavaScript
    webview_bind_function(window, "myNativeFunction", "handle_button_click")
}

var window = null

func main() {
    # Initialize WebView
    webview_init()
    
    # Create settings
    var settings = webview_create_default_settings()
    settings.width = 600
    settings.height = 400
    settings.ready_callback = "on_ready"
    
    # Create window
    window = webview_create("My Kuyil App", settings)
    webview_load_html(window, app_html)
    
    # Run the app
    webview_run(window)
    
    # Cleanup
    webview_destroy(window)
    webview_cleanup()
}

main()
```

## API Reference

### Window Management

```kuyil
# Create a new window
var settings = webview_create_default_settings()
settings.width = 800
settings.height = 600
settings.debug = true

var window = webview_create("Window Title", settings)

# Load HTML content
webview_load_html(window, html_string)

# Load a URL
webview_load_url(window, "https://example.com")

# Show/hide window
webview_show(window)
webview_hide(window)

# Center window on screen
webview_center(window)

# Run the event loop
webview_run(window)
```

### JavaScript Integration

```kuyil
# Execute JavaScript code
webview_eval_js(window, "console.log('Hello from native code!');", 0)

# Bind native function to JavaScript
webview_bind_function(window, "jsFunction", "native_function_name")

# In your HTML/JavaScript:
# <button onclick="jsFunction()">Call Native</button>
```

### Native Dialogs

```kuyil
# File open dialog
var file_path = webview_show_open_dialog(
    window,
    "Select a file",
    "/home/user",
    "Text files (*.txt)|*.txt|All files (*)|*",
    false  # multiple selection
)

# File save dialog
var save_path = webview_show_save_dialog(
    window,
    "Save file",
    "/home/user/document.txt",
    "Text files (*.txt)|*.txt"
)

# Message box
var result = webview_show_message_box(
    window,
    "Confirmation",
    "Are you sure you want to continue?",
    "question",  # info, warning, error, question
    "yesno"     # ok, okcancel, yesno, yesnocancel
)
```

### Multi-Window Apps

```kuyil
# Create child window
var child_settings = webview_create_default_settings()
child_settings.width = 400
child_settings.height = 300

var child_window = webview_create_child(parent_window, "Child Window", child_settings)
webview_load_html(child_window, child_html)
```

## Architecture Best Practices

### 1. Separation of Concerns

- **UI Logic**: HTML/CSS/JavaScript handles presentation and user interaction
- **Business Logic**: Kuyil handles data processing, file I/O, and system operations
- **Communication**: Use bound functions for UI ↔ Native communication

### 2. State Management

```kuyil
# Keep application state in native code
var app_state = {
    "user_name": "John",
    "document_path": null,
    "is_modified": false
}

# Update UI when state changes
func update_ui() {
    var js = "updateUserInterface(" + json_encode(app_state) + ");"
    webview_eval_js(window, js, 0)
}
```

### 3. Error Handling

```kuyil
# Always check for initialization errors
if !webview_init() {
    print("Error: WebView system not available")
    return 1
}

# Check window creation
var window = webview_create("App", settings)
if window == null {
    print("Error: Failed to create window")
    webview_cleanup()
    return 1
}
```

## Platform-Specific Notes

### Linux
- Requires WebKit2GTK development packages
- Install: `sudo apt-get install libwebkit2gtk-4.0-dev`

### macOS
- Uses native WebKit framework
- No additional dependencies required

### Windows
- Uses Microsoft WebView2 runtime
- May require WebView2 installation for end users

## Performance Tips

1. **Minimize JavaScript ↔ Native calls**: Batch operations when possible
2. **Use efficient HTML**: Avoid heavy DOM manipulation
3. **Cache resources**: Store frequently used HTML/CSS in variables
4. **Lazy loading**: Load content only when needed
5. **Memory management**: Always call `webview_destroy()` and `webview_cleanup()`

## Debugging

### Enable Developer Tools
```kuyil
settings.debug = true
settings.dev_tools = true
```

### Console Logging
```kuyil
# Set up console message handler
func on_console(webview, message, level, user_data) {
    print("Console [" + level + "]: " + message)
}

webview_set_console_callback(window, "on_console", null)
```

### Debug Output
```kuyil
settings.debug_level = 2  # WEBVIEW_DEBUG_INFO for detailed logging
```

## Examples Included

1. **`example_app.c`**: Full-featured C application demonstrating all WebView features
2. **`kuyil_desktop_app.kyl`**: Complete Kuyil desktop application with modern UI
3. **`test_webview.c`**: Comprehensive test suite for the WebView API

## Building Distribution Packages

The WebView library creates self-contained applications. For distribution:

1. Build your application
2. Include the `webview_utils.so` library
3. Package platform-specific WebView dependencies
4. Create installer/package for your target platform

This enables you to create professional desktop applications with the rapid development capabilities of web technologies and the performance of native Kuyil code.