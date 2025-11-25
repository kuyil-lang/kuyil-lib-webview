# Build script for WebView example application
#!/bin/bash

WEBVIEW_DIR="./shared_libs/webview"
BUILD_DIR="$WEBVIEW_DIR/build"

echo "Building Kuyil WebView Example Application"
echo "========================================"

# Create build directory
mkdir -p "$BUILD_DIR"

# Check if webview library exists
if [ ! -f "$WEBVIEW_DIR/webview_utils.so" ]; then
    echo "Building webview shared library first..."
    cd "$WEBVIEW_DIR"
    make clean
    make
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to build webview library"
        exit 1
    fi
fi

echo "Building example application..."

# Detect platform for linking
case "$(uname -s)" in
    Linux*)
        PLATFORM_LIBS="-lwebkit2gtk-4.0 -lgtk-3-0 -lgobject-2.0 -lglib-2.0"
        PKG_CONFIG_PACKAGES="webkit2gtk-4.0 gtk+-3.0"
        
        # Check if required packages are installed
        if ! pkg-config --exists $PKG_CONFIG_PACKAGES; then
            echo "Error: Required packages not found. Please install:"
            echo "  sudo apt-get install libwebkit2gtk-4.0-dev libgtk-3-dev"
            exit 1
        fi
        
        PLATFORM_CFLAGS=$(pkg-config --cflags $PKG_CONFIG_PACKAGES)
        PLATFORM_LIBS=$(pkg-config --libs $PKG_CONFIG_PACKAGES)
        ;;
    Darwin*)
        PLATFORM_LIBS="-framework WebKit -framework Cocoa"
        PLATFORM_CFLAGS=""
        ;;
    CYGWIN*|MINGW*|MSYS*)
        PLATFORM_LIBS="-lwebview2 -luser32 -lkernel32"
        PLATFORM_CFLAGS=""
        ;;
    *)
        echo "Error: Unsupported platform: $(uname -s)"
        exit 1
        ;;
esac

# Compile the example application
gcc -std=c99 -Wall -Wextra -O2 \
    $PLATFORM_CFLAGS \
    -I"$WEBVIEW_DIR" \
    -L"$WEBVIEW_DIR" \
    -o "$BUILD_DIR/webview_example" \
    "$WEBVIEW_DIR/example_app.c" \
    -lwebview_utils \
    $PLATFORM_LIBS \
    -lpthread

if [ $? -eq 0 ]; then
    echo "✓ Example application built successfully"
    echo "✓ Executable: $BUILD_DIR/webview_example"
    
    # Copy shared library to build directory for easy distribution
    cp "$WEBVIEW_DIR/libwebview_utils.so" "$BUILD_DIR/"
    
    echo ""
    echo "To run the example application:"
    echo "  cd $BUILD_DIR"
    echo "  LD_LIBRARY_PATH=. ./webview_example"
    echo ""
    echo "Or with the Kuyil script:"
    echo "  cd $WEBVIEW_DIR"
    echo "  kuyil kuyil_desktop_app.kyl"
    
else
    echo "✗ Failed to build example application"
    exit 1
fi