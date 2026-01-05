# Makefile for client_v2 - Modular GTK Client

CC = gcc
CFLAGS = -Wall -g `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0` -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = .

# Target executable
TARGET = $(BIN_DIR)/client_gtk_v2

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/globals.c \
       $(SRC_DIR)/network.c \
       $(SRC_DIR)/ui_components.c \
       $(SRC_DIR)/ui_login.c \
       $(SRC_DIR)/ui_room_list.c \
       $(SRC_DIR)/ui_room_detail.c \
       $(SRC_DIR)/response_handlers.c

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Include path
INC = -I$(INC_DIR)

# Default target
all: $(OBJ_DIR) $(TARGET)
	@echo "Build successful! Executable: $(TARGET)"

# Create obj directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Cleaned client_v2 build artifacts."

# Run the client
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
