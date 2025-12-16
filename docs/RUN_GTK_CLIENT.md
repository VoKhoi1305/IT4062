# Hướng dẫn chạy Client GTK

Tài liệu này hướng dẫn cách biên dịch và chạy ứng dụng Client giao diện đồ họa (GTK+3) cho hệ thống đấu giá.

## 1. Yêu cầu hệ thống

Để biên dịch và chạy client này, bạn cần cài đặt thư viện phát triển GTK+ 3.0.

### Trên Ubuntu/Debian/WSL:
```bash
sudo apt-get update
sudo apt-get install libgtk-3-dev
```

## 2. Biên dịch

Di chuyển vào thư mục `client` và chạy lệnh biên dịch sau:

```bash
cd client
gcc -o client_gtk client_gtk.c `pkg-config --cflags --libs gtk+-3.0`
```

Nếu không có lỗi, một file thực thi tên `client_gtk` sẽ được tạo ra.

## 3. Chạy ứng dụng

Trước khi chạy client, hãy đảm bảo **Server** đang chạy.

### Bước 1: Chạy Server
Mở một terminal mới, di chuyển đến thư mục `server` và chạy:
```bash
cd server
make
./bin/server_app
```

### Bước 2: Chạy Client GTK
Tại terminal của thư mục `client`:
```bash
./client_gtk
```
*Lưu ý: Mặc định client sẽ kết nối đến `127.0.0.1:8080`. Nếu server chạy ở IP khác, bạn cần sửa dòng `inet_pton` trong hàm `main` của file `client_gtk.c`.*

## 4. Hướng dẫn sử dụng

### Đăng nhập / Đăng ký
- Khi khởi động, màn hình Đăng nhập sẽ hiện ra.
- Nếu chưa có tài khoản, nhấn "Chưa có tài khoản? Đăng ký" để chuyển sang màn hình Đăng ký.
- Nhập Username và Password để đăng nhập.

### Màn hình chính (Dashboard)
Sau khi đăng nhập thành công, bạn sẽ thấy:
- **Tab Phòng Đấu Giá:** Danh sách các phòng đang có.
    - Nhấn **Làm mới** để cập nhật danh sách.
    - Chọn một phòng và nhấn **Tham gia** để vào phòng.
- **Tab Admin (Nếu là Admin):** Xem danh sách người dùng và trạng thái online/offline.

### Trong phòng đấu giá
- **Danh sách vật phẩm:** Hiển thị các vật phẩm trong phòng cùng giá hiện tại.
- **Đặt giá:** Chọn một vật phẩm, nhập số tiền vào ô bên dưới và nhấn **Đặt giá**.
- **Mua ngay:** Chọn vật phẩm và nhấn **Mua ngay** để mua với giá chốt.
- **Rời phòng:** Nhấn nút này để quay lại danh sách phòng.

## 5. Khắc phục sự cố

- **Lỗi "Connect failed":** Kiểm tra xem Server có đang chạy không và Port 8080 có bị chặn không.
- **Lỗi biên dịch `gtk/gtk.h not found`:** Bạn chưa cài đặt `libgtk-3-dev`. Hãy xem lại mục 1.
- **Lỗi hiển thị (WSL):** Nếu chạy trên WSL, bạn cần cài đặt XServer (như VcXsrv) trên Windows và cấu hình `DISPLAY`.
    - Ví dụ: `export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0`
