# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -I/opt/homebrew/include

# Libraries to link (GLFW + OpenGL on macOS)
LIBS = -L/opt/homebrew/lib -lglfw -framework OpenGL

# Directories
SRC_DIR = src
OUT_DIR = out

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OUT_DIR)/%.o,$(SRCS))

# Final executable
TARGET = $(OUT_DIR)/main

# Default target
all: $(TARGET)

# Link object files into final executable
$(TARGET): $(OBJS)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile each .c file to .o file in /out/
$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OUT_DIR)/*
