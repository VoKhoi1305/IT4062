# Client V2 - Modular Structure

## 📁 Cấu trúc thư mục

```
client_v2/
├── Makefile                      # Build system
├── README.md                     # File này
├── include/                      # Header files
│   ├── globals.h                 # Global variables & constants
│   ├── network.h                 # Network functions
│   ├── ui_components.h           # Reusable UI widgets
│   ├── ui_login.h                # Login/Register pages
│   ├── ui_room_list.h            # Room list page
│   ├── ui_room_detail.h          # Room detail page
│   └── response_handlers.h       # Server response handlers
└── src/                          # Source files
    ├── main.c                    # Entry point
    ├── globals.c                 # Global variables definitions
    ├── network.c                 # Socket & receiver thread
    ├── ui_components.c           # [CẦN THÊM CODE]
    ├── ui_login.c                # [CẦN THÊM CODE]
    ├── ui_room_list.c            # [CẦN THÊM CODE]
    ├── ui_room_detail.c          # [CẦN THÊM CODE]
    └── response_handlers.c       # [CẦN THÊM CODE]
```

## 🚀 Cách build

```bash
cd client_v2
make clean
make
./client_gtk_v2 [server_ip]
```

## ✏️ Nhiệm vụ tiếp theo

**Bạn cần copy code từ client_gtk.c vào các file sau:**

### 1. ui_components.c
- `create_datetime_picker()`
- `get_datetime_from_picker()` 
- `show_message_dialog()`
- `show_error_dialog()`
- `update_status_bar()`
- `format_countdown()`
- `update_countdown_timer()`

### 2. ui_login.c
- `create_login_page()`
- `create_register_page()`
- `on_login_clicked()`
- `on_register_clicked()`
- `on_show_register_clicked()`
- `on_show_login_clicked()`

### 3. ui_room_list.c
- `create_room_list_page()`
- `update_room_list_ui()`
- `refresh_room_list()`
- `on_create_room_clicked()`
- `on_join_room_clicked()`
- `on_refresh_rooms_clicked()`
- `on_search_items_clicked()`
- `on_view_history_clicked()`
- `on_logout_clicked()`
- `on_admin_panel_clicked()`

### 4. ui_room_detail.c
- `create_room_detail_page()`
- `update_room_detail_ui()`
- `refresh_room_detail()`
- `auto_refresh_room()`
- `on_leave_room_clicked()`
- `on_bid_clicked()`
- `on_buy_now_clicked()`
- `on_create_item_clicked()`
- `on_delete_item_clicked()`

### 5. response_handlers.c
- `handle_server_message()` - main dispatcher
- `process_room_list_response()`
- `process_room_detail_response()`
- `show_notification_ui()`
- Các handler cho: LOGIN, REGISTER, BID, BUY_NOW, CREATE_ITEM, etc.

## 📝 Lưu ý

1. **Include đúng thứ tự**: Mỗi .c file cần include header của nó trước
2. **Không duplicate code**: Dùng functions từ các module khác qua headers
3. **Thread-safe**: Nhớ dùng `g_idle_add()` cho GTK updates từ threads
4. **Compile testing**: Sau khi thêm code vào mỗi file, chạy `make` để kiểm tra

## 🔧 Troubleshooting

- **Lỗi linking**: Kiểm tra tất cả functions đã được implement trong .c files
- **Undefined reference**: Kiểm tra include paths và declarations trong headers
- **Segfault**: Kiểm tra global variables đã được khởi tạo trong globals.c

## ✅ Ưu điểm của cấu trúc này

- ✨ Mỗi file < 300 dòng → dễ đọc, dễ maintain
- 🔍 Tìm code nhanh: biết function ở file nào
- 🚀 Compile nhanh: chỉ compile file thay đổi
- 👥 Team-friendly: nhiều người làm cùng lúc không conflict
- 🧪 Testing: dễ test từng module riêng
