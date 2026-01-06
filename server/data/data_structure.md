# Cấu trúc dữ liệu hệ thống đấu giá

## 1. users.txt

**Format:** `user_id|username|password|role`

### Các trường:
- **user_id** (int): ID duy nhất của người dùng
- **username** (string): Tên đăng nhập
- **password** (string): Mật khẩu (plaintext)
- **role** (int): Vai trò người dùng
  - `0`: Người dùng thường
  - `1`: Quản trị viên (admin)

### Ví dụ:
```
1|admin|admin|1
2|user1|pass1|0
3|user2|pass2|0
```

---

## 2. rooms.txt

**Format:** `room_id|room_name|owner_id|status|start_time|end_time`

### Các trường:
- **room_id** (int): ID duy nhất của phòng
- **room_name** (string): Tên phòng đấu giá
- **owner_id** (int): ID của người tạo phòng (tham chiếu đến users.txt)
- **status** (string): Trạng thái phòng
  - `PENDING`: Chưa mở
  - `ACTIVE`: Đang hoạt động
  - `CLOSED`: Đã đóng
- **start_time** (datetime): Thời gian bắt đầu phòng
  - Format: `YYYY-MM-DD HH:MM:SS` hoặc `DD-MM-YYYY HH:MM:SS`
- **end_time** (datetime): Thời gian kết thúc phòng
  - Format: `YYYY-MM-DD HH:MM:SS` hoặc `YYYY-MM-DD` (không có giờ)

### Ví dụ:
```
1|Phong Dau Gia Do Co|1|CLOSED|2024-12-01 08:00:00|2024-12-31 20:00:00
30|open|1|ACTIVE|2026-01-04 00:00:00|2026-02-04 00:00:00
29|12|2|PENDING|05-01-2026 00:00:00|05-01-2026 00:00:00
```

---

## 3. items.txt

**Format:** `item_id|room_id|name|description|start_price|current_price|buy_now_price|status|sold|final_price|start_time|end_time|winner_id|duration|created_at|scheduled_start|scheduled_end|bid_history`

### Các trường:
- **item_id** (int): ID duy nhất của vật phẩm
- **room_id** (int): ID phòng chứa vật phẩm (tham chiếu đến rooms.txt)
- **name** (string): Tên vật phẩm
- **description** (string): Mô tả chi tiết vật phẩm (có thể để trống)
- **start_price** (int): Giá khởi điểm (VNĐ)
- **current_price** (int): Giá hiện tại sau các lượt đặt giá
- **buy_now_price** (int): Giá mua ngay (0 nếu không có)
- **status** (string): Trạng thái vật phẩm
  - `PENDING`: Chờ bắt đầu
  - `ACTIVE`: Đang đấu giá
  - `SOLD`: Đã bán
  - `CLOSED`: Đã đóng (hết thời gian, không bán được)
- **sold** (int): Đã bán hay chưa
  - `0`: Chưa bán
  - `1`: Đã bán qua đấu giá thường
  - `2`: Đã bán qua mua ngay
  - `3`: Đã bán qua đấu giá (bid)
- **final_price** (int/float): Giá cuối cùng khi bán (0 hoặc 0.0 nếu chưa bán)
- **start_time** (datetime): Thời gian bắt đầu đấu giá thực tế
  - Format: `YYYY-MM-DD HH:MM:SS` hoặc để trống
- **end_time** (datetime): Thời gian kết thúc đấu giá thực tế
  - Format: `YYYY-MM-DD HH:MM:SS` hoặc để trống
- **winner_id** (int): ID người thắng đấu giá (0 nếu chưa có)
- **duration** (int): Thời lượng đấu giá tính bằng giây
- **created_at** (datetime): Thời gian tạo vật phẩm
  - Format: `YYYY-MM-DD HH:MM:SS`
- **scheduled_start** (datetime): Thời gian lên lịch bắt đầu (tùy chọn)
  - Format: `YYYY-MM-DD HH:MM:SS` hoặc `HH:MM`
- **scheduled_end** (datetime): Thời gian lên lịch kết thúc (tùy chọn)
  - Format: `YYYY-MM-DD HH:MM:SS` hoặc `HH:MM`
- **bid_history** (string): Lịch sử đặt giá
  - Format: `user_id:price:timestamp;user_id:price:timestamp;...`
  - Ví dụ: `2:5010000:2024-12-02 10:05:30;3:5100000:2024-12-02 10:15:20;`
  - Có thể là `0` nếu không có lịch sử

### Ví dụ:

**Vật phẩm ACTIVE với bid history:**
```
1|1|Binh gom Bat Trang|Binh gom Bat Trang co the ky 18, hoa tiet rong phung|5000000|5230000|8000000|ACTIVE|0|0|2024-12-02 10:00:00|2024-12-02 11:00:00|2|2:5010000:2024-12-02 10:05:30;3:5100000:2024-12-02 10:15:20;4:5230000:2024-12-02 10:45:10;
```

**Vật phẩm PENDING với scheduled times:**
```
22|14|vp2||12000|12000|200000|PENDING|0|0.0|||0|10|2025-12-17 03:18:44|2025-12-17 03:20:00|2025-12-17 03:30:00|
```

**Vật phẩm SOLD:**
```
4|2|Tranh son dau Bui Xuan Phai|Tranh son dau Pho co Ha Noi, kich thuoc 60x80cm|15000000|17000000|25000000|SOLD|1|25000000|2024-12-02 09:30:00|2024-12-02 10:30:00|1|4:15200000:2024-12-02 09:35:10;5:15500000:2024-12-02 09:50:30;6:16000000:2024-12-02 10:05:45;4:16500000:2024-12-02 10:15:20;1:17000000:2025-12-02 06:08:01;
```

---

## Quan hệ giữa các bảng

```
users.txt (user_id)
    ├─── rooms.txt (owner_id) - Người tạo phòng
    ├─── items.txt (winner_id) - Người thắng đấu giá
    └─── items.txt (bid_history) - Người đặt giá

rooms.txt (room_id)
    └─── items.txt (room_id) - Vật phẩm trong phòng
```

---

## Lưu ý

1. **Định dạng thời gian không nhất quán:**
   - Một số dùng `YYYY-MM-DD HH:MM:SS`
   - Một số dùng `DD-MM-YYYY HH:MM:SS`
   - Một số chỉ có `YYYY-MM-DD` hoặc `HH:MM`

2. **Trường có thể để trống:**
   - `description` trong items.txt
   - `start_time`, `end_time` trong items.txt (khi PENDING)
   - `scheduled_start`, `scheduled_end` trong items.txt

3. **Duration:**
   - Lưu trữ bằng giây
   - Ví dụ: 120 = 2 phút, 300 = 5 phút

4. **Bid history:**
   - Kết thúc bằng dấu `;`
   - Có thể là `0` hoặc để trống nếu không có lịch sử

5. **Final price:**
   - Có thể là `0` (int) hoặc `0.0` (float)
   - Cần xử lý cả hai trường hợp khi parse
