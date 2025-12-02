# 1. Khai báo trình biên dịch và cờ (Flags)
CC = gcc
# -Iinclude: Chỉ định thư mục chứa file header (.h)
# -Wall: Bật cảnh báo (warning) để dễ debug
# -g: Thêm thông tin debug (để dùng gdb nếu cần)
# -pthread: Thường server socket cần đa luồng, mình thêm sẵn vào đây
CFLAGS = -Wall -g -Iinclude -pthread

# 2. Định nghĩa các thư mục
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INC_DIR = include

# 3. Tên file chạy đầu ra (Executable)
TARGET = $(BIN_DIR)/server_app

# 4. Tự động tìm tất cả file .c trong src và chuyển đổi tên thành file .o tương ứng
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# 5. Rule mặc định (gõ 'make' sẽ chạy cái này)
all: $(TARGET)

# 6. Link: Tạo file chạy từ các file object
$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "Dang lien ket (Linking)..."
	$(CC) $(CFLAGS) $(OBJS) -o $@
	@echo "Build thanh cong! File chay nam tai: $@"

# 7. Compile: Tạo file object .o từ file source .c
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Dang bien dich (Compiling) $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 8. Tạo thư mục bin và obj nếu chưa tồn tại
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	@echo "Dang don dep..."
	rm -rf $(OBJ_DIR)/*.o $(TARGET)
	@echo "Da xoa file build va object."

run: all
	./$(TARGET)

.PHONY: all clean run