# Makefile for Simple OpenGL file

# --- Configuration ---
# C++ Compiler to use
CXX = g++

# Compiler flags:
# -Wall: Enable all standard warnings
# -g: Include debugging information
# -std=c++11: Use C++11 standard (or c++14, c++17, c++20 depending on your preference and compiler support)
CXXFLAGS = -Wall -g -std=c++11

# Linker flags:
# -lGLEW: Link against the GLEW library
# -lglfw: Link against the GLFW library
# -lGL: Link against the core OpenGL library
# -lGLU: Link against the OpenGL Utility Library (optional, but often used)
#
# For Linux (Ubuntu/Debian):
# These are common additional libraries required by GLFW on Linux (X11 system)
LDFLAGS = -lGLEW -lglfw -lGL -lGLU -lXrandr -lrt -lm -pthread -ldl

# Source files
SRCS = main.cpp

# Build directory
BUILD_DIR = build

# Executable name (within the build directory)
TARGET_NAME = main
TARGET = $(BUILD_DIR)/$(TARGET_NAME)

# --- Rules ---

# Default target: builds the executable
all: $(BUILD_DIR) $(TARGET)

# Rule to create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

# Clean rule: removes compiled files
clean:
	rm -f $(TARGET)
	rmdir $(BUILD_DIR)

.PHONY: all clean