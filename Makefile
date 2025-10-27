# WebView Utils Library Makefile
# Compatible with Makefile.libs structure

CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2 -fPIC
TARGET ?= ../../libs/libkylwebview.so
SRC = webview_utils.c libkylwebview.c

# Platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    PKG_CONFIG_CFLAGS := $(shell pkg-config --cflags webkit2gtk-4.0 gtk+-3.0 2>/dev/null || echo "")
    PKG_CONFIG_LIBS := $(shell pkg-config --libs webkit2gtk-4.0 gtk+-3.0 2>/dev/null || echo "")
    EXTRA_CFLAGS = $(PKG_CONFIG_CFLAGS)
    LIBS = $(PKG_CONFIG_LIBS)
endif
ifeq ($(UNAME_S),Darwin)
    LIBS = -framework WebKit -framework Cocoa
    EXTRA_CFLAGS = -DWV_COCOA
endif
ifeq ($(OS),Windows_NT)
    LIBS = -lole32 -lcomctl32 -loleaut32 -luuid -lgdi32
    EXTRA_CFLAGS = -DWV_WINAPI
endif

ifeq ($(OS),Windows_NT)
    LIBS = -lole32 -lcomctl32 -loleaut32 -luuid -lgdi32
    EXTRA_CFLAGS = -DWV_WINAPI
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p ../../libs
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -shared -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

# Legacy targets for standalone testing
test: test_webview

test_webview: test_webview.c $(TARGET)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ test_webview.c -L../../libs -lkylwebview $(LIBS)

example: example_app

example_app: example_app.c $(TARGET)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ example_app.c -L../../libs -lkylwebview $(LIBS)