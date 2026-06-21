CC = clang++
CFLAGS = -std=c++11 -fPIC -O2 -Wall -I. -Iinclude
CFLAGS += $(shell pkg-config --cflags fdk-aac)

LDFLAGS = -shared -Wl,-rpath,'$$ORIGIN' -lpthread
LDFLAGS += $(shell pkg-config --libs fdk-aac)

TARGET = bin/aac_fdk_plugin.dvcp
OBJDIR = build
BINDIR = bin

# Bundle layout Resolve actually loads on Linux:
#   <name>.dvcp.bundle/Contents/Linux-x86-64/<name>.dvcp
BUNDLE_NAME = aac_fdk_plugin.dvcp.bundle
BUNDLE_DIR = $(BUNDLE_NAME)/Contents/Linux-x86-64
RESOLVE_IOPLUGINS = /opt/resolve/IOPlugins

SRCS = plugin.cpp aac_encoder.cpp
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)
WRAPPER_OBJS = $(OBJDIR)/host_api.o $(OBJDIR)/plugin_api.o

all: prereq wrapper-objs $(OBJS) $(TARGET)

prereq:
	mkdir -p $(OBJDIR)
	mkdir -p $(BINDIR)

wrapper-objs:
	$(CC) -c wrapper/host_api.cpp -o $(OBJDIR)/host_api.o $(CFLAGS)
	$(CC) -c wrapper/plugin_api.cpp -o $(OBJDIR)/plugin_api.o $(CFLAGS)

$(OBJDIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS) $(WRAPPER_OBJS)
	$(CC) $(OBJDIR)/*.o $(LDFLAGS) -o $(TARGET)

# Packages the just-built binary into a fresh .dvcp.bundle (deleting any
# stale one first, so an old binary can never silently linger here) and
# installs it into Resolve's actual plugin search path. Requires sudo
# because /opt/resolve/IOPlugins is root-owned.
install: all
	rm -rf $(BUNDLE_NAME)
	mkdir -p $(BUNDLE_DIR)
	cp $(TARGET) $(BUNDLE_DIR)/
	sudo mkdir -p $(RESOLVE_IOPLUGINS)
	sudo rm -rf $(RESOLVE_IOPLUGINS)/$(BUNDLE_NAME)
	sudo cp -r $(BUNDLE_NAME) $(RESOLVE_IOPLUGINS)/
	@echo "Installed $(BUNDLE_NAME) -> $(RESOLVE_IOPLUGINS)/$(BUNDLE_NAME)"
	@echo "Restart DaVinci Resolve for the change to take effect."

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(BUNDLE_NAME)

.PHONY: all prereq wrapper-objs install clean
