CC = clang++
CFLAGS = -std=c++11 -fPIC -O2 -Wall -I. -Iinclude
CFLAGS += $(shell pkg-config --cflags fdk-aac)

LDFLAGS = -shared -Wl,-rpath,'$$ORIGIN' -lpthread -stdlib=libc++
LDFLAGS += $(shell pkg-config --libs fdk-aac)

TARGET = bin/aac_fdk_plugin.dvcp
OBJDIR = build
BINDIR = bin

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

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all prereq wrapper-objs clean
