# Makefile (Nằm trong thư mục server/)
CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS = 
TARGET_NAME = server
TARGET = bin/$(TARGET_NAME)
SRC_DIR = src
OBJ_DIR = obj
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Bien dich SERVER thanh cong! File thuc thi la: $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJ_DIR) bin
	@echo "Da don dep server."