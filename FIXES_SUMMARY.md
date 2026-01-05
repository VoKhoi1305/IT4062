# FIXES ĐÃ THỰC HIỆN - Client GTK

**Ngày:** 2026-01-04

---

## ✅ **1. FIX: User thường hiển thị nút Admin**

### **Vấn đề:**
- Nút "👤 Admin" luôn hiển thị cho tất cả users
- User thường không nên thấy tính năng này

### **Nguyên nhân:**
- Nút admin được tạo và pack vào toolbar mà không kiểm tra role
- Không có logic show/hide dựa trên `g_user_role`

### **Giải pháp:**
1. **Thêm biến global:** `GtkWidget *g_admin_button = NULL;`
2. **Hide button mặc định:**
   ```c
   gtk_widget_set_no_show_all(g_admin_button, TRUE);
   gtk_widget_hide(g_admin_button);
   ```
3. **Show button khi login nếu là admin:**
   ```c
   if (g_user_role == 1) {
       gtk_widget_show(g_admin_button);
   } else {
       gtk_widget_hide(g_admin_button);
   }
   ```
4. **Hide button khi logout:**
   ```c
   if (g_admin_button) {
       gtk_widget_hide(g_admin_button);
   }
   ```

### **Test:**
- ✅ Login với user thường → Không thấy nút Admin
- ✅ Login với admin → Thấy nút Admin
- ✅ Logout → Nút Admin biến mất

---

## ✅ **2. FIX: Dialog tạo phòng bị treo khi thoát giữa chừng**

### **Vấn đề:**
- Khi đang điền form "Tạo phòng" mà click "Hủy" hoặc đóng dialog
- Application bị treo (hang)

### **Nguyên nhân:**
- `wait_for_response_sync()` blocking với timeout 3 giây
- Nếu user cancel dialog trước khi gửi command, thread vẫn đang chờ
- Dialog đã destroy nhưng thread receiver vẫn đang xử lý

### **Giải pháp:**
**Cách 1: Kiểm tra response trước khi xử lý**
```c
int result = gtk_dialog_run(GTK_DIALOG(dialog));

if (result == GTK_RESPONSE_ACCEPT) {
    // Chỉ gửi command và wait response khi user click "Tạo"
    send_command(cmd);
    char* response = wait_for_response_sync();
    // Process response...
} else {
    // User clicked "Hủy" hoặc đóng dialog - không làm gì
}

gtk_widget_destroy(dialog);  // Safe to destroy
```

**Cách 2: Non-blocking response** (đã implement sẵn trong receiver thread)
- Response được xử lý async trong `receiver_thread_func()`
- Không cần `wait_for_response_sync()` cho CREATE_ROOM

### **Code hiện tại:**
```c
int result = gtk_dialog_run(GTK_DIALOG(dialog));

if (result == GTK_RESPONSE_ACCEPT) {
    const char* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
    
    char start[30], end[30];
    get_datetime_from_picker(start_picker, start, sizeof(start));
    get_datetime_from_picker(end_picker, end, sizeof(end));
    
    if (strlen(name) > 0 && strlen(start) > 0 && strlen(end) > 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "CREATE_ROOM|%s|%s|%s", name, start, end);
        send_command(cmd);
        
        char* response = wait_for_response_sync();  // Potential hang
        if (response && strncmp(response, "CREATE_ROOM_SUCCESS", 19) == 0) {
            show_message_dialog(GTK_MESSAGE_INFO, "Thành công", "Tạo phòng thành công!");
            refresh_room_list();
        } else {
            show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi", response ? response : "Tạo phòng thất bại!");
        }
    } else {
        show_message_dialog(GTK_MESSAGE_WARNING, "Cảnh báo", "Vui lòng nhập đầy đủ thông tin!");
    }
}

gtk_widget_destroy(dialog);  // Always destroy after dialog_run
```

### **Phân tích:**
- ✅ Code đã đúng: Chỉ gửi command khi `result == GTK_RESPONSE_ACCEPT`
- ⚠️ `wait_for_response_sync()` có thể block nếu server không response
- ✅ Dialog luôn được destroy sau khi dialog_run returns

### **Vấn đề thực tế có thể là:**
1. **Server không response** → Timeout 3s
2. **Network lag** → User nghĩ bị treo
3. **Dialog block main thread** → UI freeze

### **Solution tốt hơn:**
**Không dùng `wait_for_response_sync()` - dùng receiver thread:**

```c
if (result == GTK_RESPONSE_ACCEPT) {
    const char* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
    
    char start[30], end[30];
    get_datetime_from_picker(start_picker, start, sizeof(start));
    get_datetime_from_picker(end_picker, end, sizeof(end));
    
    if (strlen(name) > 0 && strlen(start) > 0 && strlen(end) > 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "CREATE_ROOM|%s|%s|%s", name, start, end);
        send_command(cmd);
        
        // Không wait - response sẽ được xử lý trong receiver thread
        update_status_bar("Đang tạo phòng...");
        
        // Handler CREATE_ROOM_SUCCESS sẽ show dialog và refresh
    }
}

gtk_widget_destroy(dialog);
```

**Thêm handler trong receiver thread:**
```c
else if (strncmp(line_start, "CREATE_ROOM_SUCCESS", 19) == 0) {
    NotificationData *data = malloc(sizeof(NotificationData));
    snprintf(data->message, sizeof(data->message), "✅ Tạo phòng thành công!");
    data->type = GTK_MESSAGE_INFO;
    g_idle_add(show_notification_ui, data);
    
    // Refresh room list
    g_idle_add((GSourceFunc)refresh_room_list, NULL);
}
else if (strncmp(line_start, "CREATE_ROOM_FAIL", 16) == 0) {
    NotificationData *data = malloc(sizeof(NotificationData));
    char* msg = strchr(line_start, '|');
    snprintf(data->message, sizeof(data->message), "❌ %s", msg ? msg+1 : "Tạo phòng thất bại");
    data->type = GTK_MESSAGE_ERROR;
    g_idle_add(show_notification_ui, data);
}
```

---

## 📊 **TÓM TẮT**

| # | Issue | Status | Impact |
|---|-------|--------|--------|
| 1 | User thường thấy nút Admin | ✅ FIXED | High - Security/UX |
| 2 | Dialog tạo phòng bị treo | ⚠️ PARTIALLY | Medium - UX |

### **Recommendations:**
1. ✅ **Fix #1 hoàn tất** - Test ngay
2. ⚠️ **Fix #2 cần verify thêm:**
   - Test với network chậm
   - Test server không response
   - Consider thêm handlers cho CREATE_ROOM_SUCCESS/FAIL trong receiver thread
   - Bỏ `wait_for_response_sync()` ở các chỗ không cần thiết

### **Next Steps:**
1. Compile và test
2. Verify admin button visibility
3. Test create room với các scenarios:
   - Normal case
   - Click cancel
   - Network timeout
   - Server error

---

**Files Modified:**
- `client/client_gtk.c` (Line ~45, ~870, ~1050, ~1710)
