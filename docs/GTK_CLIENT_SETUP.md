# Hướng Dẫn Chạy GTK Client trong WSL

## Vấn Đề: GTK Client Không Hiển Thị Gì

Khi chạy `./client_gtk 127.0.0.1` không có cửa sổ nào hiện ra vì **GTK cần X server để hiển thị GUI**.

---

## ✅ Giải Pháp Theo Phiên Bản Windows

### 1️⃣ Windows 11 (Khuyến Nghị - Dễ Nhất)

Windows 11 có **WSLg** tích hợp sẵn X server:

```bash
# Không cần config gì! Chỉ cần chạy:
./client_gtk 127.0.0.1
```

**Nếu vẫn không hiện:**

```bash
# Kiểm tra DISPLAY
echo $DISPLAY
# Nên thấy: :0 hoặc :0.0

# Nếu trống, set lại:
export DISPLAY=:0
```

### 2️⃣ Windows 10 (Cần Cài X Server)

#### Bước 1: Cài VcXsrv (X Server cho Windows)

1. Download: https://sourceforge.net/projects/vcxsrv/
2. Cài đặt VcXsrv
3. Chạy **XLaunch**:
   - Display settings: **Multiple windows**, Display 0
   - Start no client: ✅ Check
   - Extra settings: ✅ **Disable access control**
   - Finish

#### Bước 2: Config WSL

```bash
# Trong WSL terminal:
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0

# Hoặc thêm vào ~/.bashrc để tự động:
echo 'export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '"'"'{print $2}'"'"'):0' >> ~/.bashrc
source ~/.bashrc
```

#### Bước 3: Test

```bash
# Test với app đơn giản:
sudo apt install x11-apps
xeyes

# Nếu thấy cửa sổ mắt -> X server hoạt động ✅
```

---

## 🚀 Chạy Demo Đầy Đủ

### Cách 1: Sử dụng Script Tự Động

```bash
cd ~/THLTM/Project
./run_gtk_demo.sh
```

Script này sẽ:
- ✅ Kiểm tra DISPLAY environment
- ✅ Kiểm tra server đang chạy
- ✅ Kiểm tra GTK đã cài
- ✅ Compile client nếu cần
- ✅ Launch client

### Cách 2: Manual (2 Terminals)

**Terminal 1 - Server:**
```bash
cd ~/THLTM/Project/server
./bin/server_app
```

Đợi thấy:
```
[INFO] Server started on port 8080
[INFO] Waiting for connections...
```

**Terminal 2 - Client:**
```bash
cd ~/THLTM/Project/client

# Đảm bảo DISPLAY được set
echo $DISPLAY  # Phải có output (VD: :0)

# Chạy client
./client_gtk 127.0.0.1
```

---

## 🐛 Troubleshooting

### Lỗi 1: "Cannot open display"

**Nguyên nhân:** X server chưa chạy hoặc DISPLAY chưa set

**Giải pháp:**

```bash
# Kiểm tra DISPLAY
echo $DISPLAY

# Nếu trống:
export DISPLAY=:0  # Windows 11 WSLg
# hoặc
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0  # Windows 10
```

### Lỗi 2: Client không connect được server

**Nguyên nhân:** Server chưa chạy

**Kiểm tra:**

```bash
# Trong WSL:
netstat -tuln | grep 8080

# Hoặc:
ss -tuln | grep 8080
```

**Nếu không thấy port 8080:**

```bash
# Start server:
cd ~/THLTM/Project/server
./bin/server_app
```

### Lỗi 3: "Không thể kết nối đến server 127.0.0.1:8080"

**Dialog error hiện trong GTK client**

**Giải pháp:**

1. Đảm bảo server đang chạy trong WSL (không phải Windows)
2. Kiểm tra firewall không block port 8080
3. Thử với terminal client trước:

```bash
cd ~/THLTM/Project/client
./client 127.0.0.1
```

Nếu terminal client kết nối được, GTK client cũng sẽ OK.

### Lỗi 4: GTK warnings/errors

```
Gtk-WARNING **: cannot open display: 
```

**Giải pháp:** Xem lại phần config X server ở trên.

```
(client_gtk:xxx): Gtk-WARNING **: Theme parsing error
```

**Giải pháp:** Warning này không ảnh hưởng, có thể bỏ qua hoặc:

```bash
sudo apt install gtk3-nocsd
```

---

## 📋 Checklist Trước Khi Chạy

- [ ] X Server đang chạy
  - Windows 11: WSLg tự động
  - Windows 10: VcXsrv/Xming phải được launch
  
- [ ] DISPLAY environment được set
  ```bash
  echo $DISPLAY  # Phải có output
  ```
  
- [ ] GTK+ 3.0 đã cài
  ```bash
  pkg-config --modversion gtk+-3.0
  # Nên thấy: 3.x.x
  ```
  
- [ ] Server đang chạy
  ```bash
  netstat -tuln | grep 8080
  # Nên thấy: tcp 0.0.0.0:8080 LISTEN
  ```
  
- [ ] Client đã compile
  ```bash
  ls -lh ~/THLTM/Project/client/client_gtk
  # File phải tồn tại và executable (x)
  ```

---

## 🎯 Demo Features Sau Khi Client Chạy

1. **Login:** Nhập username/password → "Đăng nhập"
2. **Room List:** Xem danh sách phòng
3. **Join Room:** Chọn phòng → "▶ Vào phòng"
4. **Real-time Updates:** Xem items update tự động
5. **Place Bid:** Chọn item → "💰 Đặt giá"
6. **Notifications:** Xem thông báo real-time ở top bar
7. **Create Item:** Click "➕ Tạo vật phẩm" (nếu là owner)
8. **Search:** Click "🔍 Tìm kiếm" từ room list
9. **History:** Click "📜 Lịch sử"
10. **Admin Panel:** Click "👤 Admin" (nếu là admin)

---

## 🔍 Debug Mode

Chạy với verbose output:

```bash
# Xem tất cả GTK warnings:
GTK_DEBUG=all ./client_gtk 127.0.0.1

# Xem connection attempts:
strace -e connect ./client_gtk 127.0.0.1 2>&1 | grep 8080

# Kiểm tra thread activity:
strace -f ./client_gtk 127.0.0.1
```

---

## 💡 Tips

1. **Luôn chạy server trước client**
2. **Dùng 2 terminals** để dễ debug (1 server, 1 client)
3. **Test terminal client trước** để đảm bảo network OK
4. **Check DISPLAY** nếu GTK app không hiện
5. **Windows 11 WSLg đơn giản hơn nhiều** so với Windows 10

---

## 📚 Tài Liệu Tham Khảo

- **WSLg Documentation:** https://github.com/microsoft/wslg
- **VcXsrv Setup:** https://sourceforge.net/p/vcxsrv/wiki/Home/
- **GTK3 Tutorial:** https://docs.gtk.org/gtk3/
- **X11 Forwarding:** https://wiki.archlinux.org/title/OpenSSH#X11_forwarding

---

**Last Updated:** January 4, 2026  
**Tested On:** WSL Ubuntu 24.04, Windows 11 WSLg
