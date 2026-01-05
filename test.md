Read [](file://wsl.localhost/Ubuntu-24.04/home/pikasonix/THLTM/Project/client/client.c#1-1), lines 1 to 100

Searched for regex `void (menu_|handle_|show_)` (`**/client/client.c`), 20 results

Read [](file://wsl.localhost/Ubuntu-24.04/home/pikasonix/THLTM/Project/client/client.c#1200-1200), lines 1200 to 1450

Read [](file://wsl.localhost/Ubuntu-24.04/home/pikasonix/THLTM/Project/client/client.c#1070-1070), lines 1070 to 1200

Dựa trên việc phân tích file client.c, tôi đã hiểu đầy đủ các tính năng. Đây là bảng test chi tiết và hoàn chỉnh:

---

## **BẢNG TEST TÍNH NĂNG HỆ THỐNG ĐẤU GIÁ TRỰC TUYẾN**

### **1. ĐĂNG NHẬP (LOGIN)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 1.1 | Đăng nhập thành công | User có tài khoản hợp lệ | Username: `user1`<br>Password: `pass1` | - Hiển thị "LOGIN_SUCCESS"<br>- Cập nhật `g_username`<br>- Set `g_is_logged_in = 1`<br>- Hiển thị role (User/Admin)<br>- Chuyển sang menu chính | Kiểm tra cả user thường và admin |
| 1.2 | Đăng nhập sai mật khẩu | Username đúng, password sai | Username: `user1`<br>Password: `wrongpass` | - Hiển thị "LOGIN_FAIL_WRONG_PASS"<br>- Không cho vào hệ thống<br>- `g_is_logged_in = 0` | Test thông báo lỗi rõ ràng |
| 1.3 | Đăng nhập tài khoản không tồn tại | Username không có trong DB | Username: `nonexist`<br>Password: `anypass` | - Hiển thị "LOGIN_FAIL_WRONG_PASS"<br>- Không cho vào hệ thống | Bảo mật: không tiết lộ user có tồn tại hay không |
| 1.4 | Đăng nhập trùng (multi-device) | Cùng 1 user đăng nhập 2 nơi | Device 1: `user1/pass1` đã login<br>Device 2: `user1/pass1` login lại | - Device 2 nhận "LOGIN_FAIL_LOGGED_IN"<br>- Chỉ Device 1 được phép hoạt động | Ngăn chặn đăng nhập đồng thời |
| 1.5 | Đăng nhập với field trống | Không nhập username hoặc password | Username: `` (empty)<br>Password: `pass` | - Client không gửi request (validate)<br>- Hoặc server trả về lỗi | Kiểm tra validation |
| 1.6 | Đăng nhập admin | User có role = 1 | Username: `admin`<br>Password: `adminpass` | - Login thành công<br>- Hiển thị "[ADMIN]" badge<br>- Menu có thêm "[8] Quản lý User" | Kiểm tra quyền admin |

---

### **2. ĐĂNG KÝ (REGISTER)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 2.1 | Đăng ký thành công | Username chưa tồn tại | Username: `newuser`<br>Password: `newpass` | - Hiển thị "REGISTER_SUCCESS"<br>- Tạo user mới trong DB<br>- Role mặc định = 0 (user) | Kiểm tra tạo file/ghi DB |
| 2.2 | Đăng ký trùng username | Username đã tồn tại | Username: `user1` (đã có)<br>Password: `anypass` | - Hiển thị "REGISTER_FAIL"<br>- Không tạo user mới | Test unique constraint |
| 2.3 | Đăng ký với ký tự đặc biệt | Username có ký tự đặc biệt | Username: `user@123!`<br>Password: `pass` | - Có thể thành công hoặc bị reject | Kiểm tra validation rules |
| 2.4 | Đăng ký với field trống | Username hoặc password trống | Username: ``<br>Password: `pass` | - Client validate hoặc server reject | Test input validation |
| 2.5 | Đăng ký username quá dài | Username > 50 ký tự | Username: `verylongusername...` (>50 chars) | - Bị cắt hoặc reject | Kiểm tra buffer overflow |

---

### **3. TRANG CHÍNH (MAIN MENU)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 3.1 | Hiển thị menu chưa login | Chưa đăng nhập | Khởi động client | Menu hiển thị:<br>- [1] Đăng nhập<br>- [2] Đăng ký<br>- [0] Thoát | Không có options khác |
| 3.2 | Hiển thị menu sau khi login | Đã đăng nhập (user thường) | Login thành công | Menu hiển thị:<br>- User info header<br>- [1] Danh sách phòng<br>- [2] Tìm kiếm vật phẩm<br>- [3] Tạo phòng<br>- [4] Vào phòng<br>- [5] Lịch sử<br>- [0] Đăng xuất | Không có "[8] Admin" |
| 3.3 | Hiển thị menu admin | Đăng nhập với role = 1 | Login với admin | Menu có thêm:<br>- [8] Quản lý User (Admin) | Hiển thị màu vàng |
| 3.4 | Hiển thị trạng thái trong phòng | Đang ở trong 1 phòng | `g_current_room_id > 0` | Menu hiển thị:<br>- "[9] Vào lại phòng #X (LIVE)" | Hiển thị màu xanh |
| 3.5 | Header thông tin user | Sau khi login | Ở menu chính | Header hiển thị:<br>- "User: [username]"<br>- "[ADMIN]" nếu là admin<br>- "Room: #X" nếu đang trong phòng | Format đúng |

---

### **4. TẠO PHÒNG (CREATE ROOM)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 4.1 | Tạo phòng thành công | Nhập đầy đủ thông tin hợp lệ | Tên: `Phong Test`<br>Start: `2026-01-05 10:00:00`<br>End: `2026-01-05 12:00:00` | - Hiển thị "CREATE_ROOM_SUCCESS"<br>- Phòng mới xuất hiện trong danh sách<br>- Owner = current user | Kiểm tra DB/file rooms.txt |
| 4.2 | Tạo phòng thiếu tên | Không nhập tên phòng | Tên: `` (empty) | - Hiển thị lỗi<br>- Không tạo phòng | Client hoặc server validate |
| 4.3 | Tạo phòng với thời gian sai | End time < Start time | Start: `2026-01-05 12:00:00`<br>End: `2026-01-05 10:00:00` | - Hiển thị "CREATE_ROOM_FAIL"<br>- Thông báo lỗi thời gian | Server validation |
| 4.4 | Tạo phòng với format time sai | Format datetime không đúng | Start: `05-01-2026`<br>End: `invalid` | - Parse lỗi<br>- Hiển thị error | Test format validation |
| 4.5 | Tạo phòng với thời gian quá khứ | Start time < current time | Start: `2025-01-01 10:00:00` | - Có thể tạo nhưng trạng thái EXPIRED<br>- Hoặc server reject | Tùy logic server |
| 4.6 | Tạo nhiều phòng liên tiếp | Tạo 5 phòng liên tục | Loop 5 lần | - Tất cả phòng được tạo<br>- ID tăng dần | Test concurrent creation |

---

### **5. THAM GIA PHÒNG (JOIN ROOM)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 5.1 | Tham gia phòng thành công | Vào phòng ACTIVE | Room ID: `1` (trạng thái ACTIVE) | - Hiển thị "JOIN_ROOM_SUCCESS"<br>- `g_current_room_id = 1`<br>- Chuyển vào room detail view<br>- Hiển thị danh sách items | User thường |
| 5.2 | Owner vào phòng của mình | Chủ phòng vào phòng | Owner join own room | - Hiển thị "Chao mung Chu phong"<br>- Có thêm quyền tạo/xóa item<br>- Menu hiển thị [7] Tạo item, [8] Xóa item | Kiểm tra owner privileges |
| 5.3 | Broadcast thông báo user join | User khác vào phòng có người | User B join phòng đang có User A | - User A nhận message "USER_JOINED\|[username]"<br>- Hiển thị realtime notification | Test broadcasting |
| 5.4 | Broadcast khi owner join | Owner vào phòng có user | Owner join phòng đang có User | - User nhận "USER_JOINED\|[owner_name]"<br>- Notification realtime | Fix bug này |
| 5.5 | Tham gia phòng không tồn tại | Room ID không có trong DB | Room ID: `999` | - Hiển thị "JOIN_ROOM_FAIL\|Phong khong ton tai"<br>- Không vào được | Error handling |
| 5.6 | Tham gia phòng chưa mở | Phòng trạng thái PENDING | Room ID: `2` (PENDING) | - User thường: "JOIN_ROOM_FAIL\|Phong chua bat dau"<br>- Owner: Vẫn vào được | Test room status logic |
| 5.7 | Tham gia phòng đã đóng | Phòng trạng thái CLOSED | Room ID: `3` (CLOSED) | - Hiển thị lỗi không vào được | Test status check |
| 5.8 | Nhiều user vào cùng lúc | 5 users join room đồng thời | Concurrent join | - Tất cả đều nhận broadcast của nhau<br>- Không có race condition | Stress test |

---

### **6. DANH SÁCH PHÒNG (ROOM LIST)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 6.1 | Hiển thị danh sách phòng | Load tất cả phòng | Command: `GET_ROOM_LIST\|ALL\|1\|50` | - Hiển thị table format<br>- Columns: ID, TÊN, OWNER, STATUS, ITEMS<br>- Số phòng đúng | Pagination 1-50 |
| 6.2 | Hiển thị phòng trống | Chưa có phòng nào | DB rỗng | - "Tim thay 0 phong"<br>- Table header vẫn hiển thị | Edge case |
| 6.3 | Hiển thị phòng nhiều status | Phòng PENDING/ACTIVE/CLOSED | Mix statuses | - Mỗi phòng hiển thị đúng status<br>- Format rõ ràng | Visual check |
| 6.4 | Refresh danh sách | Sau khi tạo phòng mới | Tạo phòng → quay lại list | - Phòng mới xuất hiện<br>- Realtime update | Test data sync |
| 6.5 | Phòng với tên dài | Tên phòng > 25 ký tự | Tên: `Phòng Đấu Giá Đặc Biệt Cho Test Case Này Rất Dài` | - Tên bị cắt với `%-25.25s`<br>- Không bị overflow | Truncation test |

---

### **7. CHI TIẾT PHÒNG (ROOM DETAIL)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 7.1 | Hiển thị chi tiết phòng | Vào phòng có items | Sau JOIN_ROOM_SUCCESS | - Room info header<br>- Danh sách items realtime<br>- Menu actions: [4] Bid, [5] Buy, [6] Leave | Auto request GET_ROOM_DETAIL |
| 7.2 | Hiển thị item ACTIVE | Item đang đấu giá | Item status = ACTIVE | - Hiển thị màu xanh<br>- Current price update realtime | Color coding |
| 7.3 | Hiển thị item PENDING | Item chưa bắt đầu | Item status = PENDING | - Hiển thị màu vàng<br>- Không cho bid | Status check |
| 7.4 | Hiển thị item SOLD | Item đã bán | Item status = SOLD | - Hiển thị màu đỏ<br>- Hiển thị winner | Final state |
| 7.5 | Owner menu | Chủ phòng vào | Owner in room | Menu thêm:<br>- [7] Tạo vật phẩm<br>- [8] Xóa vật phẩm | Owner privileges |
| 7.6 | Realtime bid update | User khác đặt giá | User B bid → User A thấy | - User A nhận "NEW_BID" message<br>- Current price update<br>- Countdown update | Test websocket-like |
| 7.7 | Countdown timer | Item sắp hết hạn | Item còn <60s | - Hiển thị countdown<br>- Cảnh báo "AUCTION_WARNING" | Timer accuracy |

---

### **8. ĐẶT GIÁ (PLACE BID)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 8.1 | Đặt giá thành công | Bid cao hơn current price | Item: `1`<br>Amount: `150000` | - Hiển thị "BID_SUCCESS"<br>- Current price = 150000<br>- Broadcast "NEW_BID" đến users khác | Update realtime |
| 8.2 | Đặt giá thấp hơn | Bid <= current price | Amount: `100000` (current = 120000) | - Hiển thị "BID_ERROR\|Gia thap hon"<br>- Không cập nhật | Validation |
| 8.3 | Đặt giá item không tồn tại | Item ID sai | Item: `999` | - Hiển thị "BID_ERROR\|Item khong ton tai" | Error handling |
| 8.4 | Đặt giá item PENDING | Item chưa active | Item status = PENDING | - Hiển thị lỗi không thể bid | Status check |
| 8.5 | Đặt giá item SOLD | Item đã bán | Item status = SOLD | - Hiển thị lỗi<br>- Không cho bid | Final state |
| 8.6 | Bid trong 5s cuối | Anti-snipe | Item còn <5s | - Extend time thêm 5s<br>- Broadcast "TIME_EXTENDED" | Anti-sniping |
| 8.7 | Nhiều user bid cùng lúc | Race condition | 3 users bid đồng thời | - Chỉ 1 bid được chấp nhận<br>- 2 còn lại nhận lỗi | Concurrency test |

---

### **9. MUA NGAY (BUY NOW)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 9.1 | Mua ngay thành công | Item có giá buy now | Item: `1` (buy_now = 500000) | - Hiển thị "BUY_NOW_SUCCESS"<br>- Item status = SOLD<br>- Winner = current user<br>- Broadcast "ITEM_SOLD" | Instant purchase |
| 9.2 | Mua ngay item không có giá | buy_now_price = 0 | Item không set buy now | - Hiển thị lỗi "Khong co gia mua ngay" | Validation |
| 9.3 | Mua ngay item đã bán | Item status = SOLD | Item đã được mua | - Hiển thị lỗi | Prevent double buy |
| 9.4 | Notification cho user khác | User A mua → User B thấy | User B đang xem item | - User B nhận "ITEM_SOLD"<br>- Item disappear/update status | Realtime update |

---

### **10. TẠO VẬT PHẨM (CREATE ITEM)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 10.1 | Tạo item thành công (owner) | Owner tạo item cơ bản | Name: `Bình gốm`<br>Start price: `100000`<br>Buy now: `500000`<br>Duration: `30` (phút) | - Hiển thị "CREATE_ITEM_SUCCESS"<br>- Item xuất hiện trong room<br>- Broadcast đến users | Basic creation |
| 10.2 | Tạo item với khung giờ | Set scheduled start/end | Schedule: `2026-01-05 10:00 - 12:00`<br>Duration: `30` min | - Item tạo thành công<br>- Status = PENDING<br>- Auto activate khi đến giờ | Scheduled auction |
| 10.3 | Tạo item thời gian vượt khung | Duration > slot | Schedule: 1 giờ<br>Duration: 90 min | - Cảnh báo "vuot qua khung gio"<br>- Có thể tạo nhưng auto stop | Validation |
| 10.4 | User thường tạo item | Không phải owner | Non-owner try create | - Hiển thị "CREATE_ITEM_FAIL\|Ban khong phai chu phong" | Permission check |
| 10.5 | Tạo item giá <= 0 | Start price = 0 hoặc âm | Start price: `0` | - Hiển thị lỗi "Gia phai lon hon 0" | Validation |
| 10.6 | Tạo item buy_now < start | Buy now thấp hơn giá khởi điểm | Start: `100000`<br>Buy now: `50000` | - Hiển thị lỗi | Logic validation |
| 10.7 | Fix bug: thêm room_id | Command format đúng | CREATE_ITEM phải có room_id | - Command: `CREATE_ITEM\|[room_id]\|...`<br>- Gửi đúng format | **BUG FIX** |

---

### **11. XÓA VẬT PHẨM (DELETE ITEM)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 11.1 | Xóa item thành công (owner) | Owner xóa item | Item ID: `1` | - Hiển thị "DELETE_ITEM_SUCCESS"<br>- Item biến mất<br>- Broadcast "ITEM_DELETED" | Owner privilege |
| 11.2 | User thường xóa item | Không phải owner | Non-owner try delete | - Hiển thị lỗi permission | Permission check |
| 11.3 | Xóa item đang đấu giá | Item status = ACTIVE | Active item | - Có thể xóa (owner force)<br>- Refund bids | Business logic |
| 11.4 | Xóa item đã bán | Item status = SOLD | Sold item | - Có thể xóa hoặc reject<br>- Tùy logic | Policy check |

---

### **12. TÌM KIẾM VẬT PHẨM (SEARCH ITEMS)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 12.1 | Tìm theo tên | Search by name | Type: NAME<br>Keyword: `bình` | - Hiển thị items có "bình" trong tên<br>- Table format đầy đủ | Case insensitive |
| 12.2 | Tìm theo thời gian | Search by date range | Type: TIME<br>From: `2026-01-01`<br>To: `2026-01-31` | - Hiển thị items trong khoảng thời gian<br>- Sort by time | Date range |
| 12.3 | Tìm kết hợp | Search by name + time | Type: BOTH<br>Keyword: `bình`<br>Date range: ... | - Kết quả thỏa cả 2 điều kiện<br>- AND logic | Combined search |
| 12.4 | Không tìm thấy | No results | Keyword: `khongcogi` | - "Tim thay: 0 vat pham"<br>- Empty table | Edge case |
| 12.5 | Tìm với ký tự đặc biệt | Special chars | Keyword: `bình@#$` | - Xử lý đúng hoặc escape<br>- Không bị SQL injection | Security test |

---

### **13. LỊCH SỬ ĐẤU GIÁ (AUCTION HISTORY)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 13.1 | Xem lịch sử | User đã tham gia đấu giá | Command: `GET_MY_AUCTION_HISTORY` | - Hiển thị danh sách items đã bid<br>- Tổng số phiên<br>- Status (Won/Lost) | Personal history |
| 13.2 | Lịch sử rỗng | User mới chưa bid | New user | - "Tong: 0 phien tham gia"<br>- Empty list | Edge case |
| 13.3 | Filter theo status | Lọc WON/LOST | Filter: WON | - Chỉ hiển thị items đã thắng | Filter logic |

---

### **14. QUẢN LÝ USER (ADMIN)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 14.1 | Xem danh sách user (admin) | Admin view users | Command: `GET_USER_LIST` | - Table: ID, USER, STATUS, ROLE<br>- Online/Offline status<br>- Admin/User badge | Admin only |
| 14.2 | User thường truy cập | Non-admin try access | Non-admin access [8] | - Hiển thị lỗi "Ban khong co quyen" | Permission check |
| 14.3 | Hiển thị status online | User đang login | User A online | - Status hiển thị "Online" màu xanh | Realtime status |
| 14.4 | **FIX BUG: Không hiển thị** | Bug không load danh sách | Admin click [8] | - **BUG**: Response không xử lý trong receiver thread<br>- **FIX**: Thêm handler cho USER_LIST | **BUG REPORT** |

---

### **15. REALTIME NOTIFICATIONS**

| STT | Test Case | Mô tả | Trigger | Expected Output | Ghi chú |
|-----|-----------|-------|---------|-----------------|---------|
| 15.1 | NEW_BID notification | User khác bid | User B bid | - User A nhận: "NEW_BID\|[item]\|[bidder]\|[amount]"<br>- Display màu xanh | Live update |
| 15.2 | ITEM_STARTED | Item được kích hoạt | Owner start item | - All users nhận "ITEM_STARTED"<br>- Item status → ACTIVE | Broadcast |
| 15.3 | ITEM_SOLD | Item được mua/bán | Buy now hoặc end | - Broadcast "ITEM_SOLD\|[winner]\|[price]" | Final notification |
| 15.4 | YOU_WON | User thắng đấu giá | User win | - Winner nhận "YOU_WON\|[item]\|[price]"<br>- Highlight special | Personal notif |
| 15.5 | AUCTION_WARNING | Sắp hết thời gian | <60s remaining | - "AUCTION_WARNING\|[item]\|[seconds]" | Countdown alert |
| 15.6 | TIME_EXTENDED | Gia hạn thời gian | Bid trong 5s cuối | - "TIME_EXTENDED\|[item]\|[new_end]" | Anti-snipe notif |
| 15.7 | ROOM_CLOSED | Phòng đóng cửa | Room status → CLOSED | - "ROOM_CLOSED"<br>- Auto kick users | Room lifecycle |
| 15.8 | USER_JOINED | User tham gia | New user join | - "USER_JOINED\|[username]"<br>- **BUG**: Owner join không broadcast | **BUG FIXED** |
| 15.9 | USER_LEFT | User rời phòng | User leave | - "USER_LEFT\|[username]" | Broadcast |

---

### **16. ĐĂNG XUẤT (LOGOUT)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 16.1 | Đăng xuất bình thường | User logout | Command: LOGOUT | - `g_is_logged_in = 0`<br>- Reset username, role, room_id<br>- Quay về login screen | Clean logout |
| 16.2 | Đăng xuất khi trong phòng | Logout while in room | Logout with `g_current_room_id > 0` | - Auto leave room<br>- Broadcast USER_LEFT<br>- Logout | Auto cleanup |

---

### **17. MẤT KẾT NỐI (DISCONNECT)**

| STT | Test Case | Mô tả | Trigger | Expected Output | Ghi chú |
|-----|-----------|-------|---------|-----------------|---------|
| 17.1 | Server down | Server tắt đột ngột | Kill server | - Client hiển thị "Mat ket noi server"<br>- Thoát gracefully | Error handling |
| 17.2 | Client disconnect | Client đóng đột ngột | Kill client | - Server remove client khỏi room<br>- Broadcast USER_LEFT | Server cleanup |
| 17.3 | Network timeout | Mất mạng | Disconnect network | - Timeout after 30s<br>- Auto cleanup | Timeout handling |

---

### **18. TIMEZONE & DATETIME**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 18.1 | **BUG: Timezone không đúng** | Server dùng UTC thay vì UTC+7 | Check log timestamps | - **BUG**: Time hiển thị sai múi giờ<br>- **FIX**: Set `TZ=Asia/Ho_Chi_Minh` | **BUG REPORT** |
| 18.2 | Datetime input validation | Nhập format sai | Input: `05/01/2026` | - Server reject hoặc parse lỗi | Format check |

---

### **19. UI/UX (GTK CLIENT)**

| STT | Test Case | Mô tả | Input | Expected Output | Ghi chú |
|-----|-----------|-------|-------|-----------------|---------|
| 19.1 | **ADD: User info display** | Hiển thị user đang login | After login | - **NEW**: Header hiển thị "👤 [username] \| [role]"<br>- Màu xanh cho role | **FEATURE ADD** |
| 19.2 | **ADD: DateTime picker** | UI chọn ngày giờ | Create room/item | - **NEW**: Date picker + Time spinners<br>- Thay thế text entry | **FEATURE ADD** |
| 19.3 | Notification bar | Realtime notifications | Receive broadcast | - GTK InfoBar hiển thị<br>- Auto-hide sau 5s | GTK widget |

---

## **TÓM TẮT BUGS ĐÃ TÌM THẤY VÀ FIX**

| # | Bug | Status | Fix |
|---|-----|--------|-----|
| 1 | Owner join phòng không broadcast USER_JOINED | ✅ FIXED | Thêm broadcast_to_room() trong phần owner join |
| 2 | Admin không hiển thị danh sách user | ✅ FIXED | Thêm handler USER_LIST trong receiver thread |
| 3 | CREATE_ITEM thiếu room_id parameter | ✅ FIXED | Sửa format command thêm `g_current_room_id` |
| 4 | Timezone server không đúng UTC+7 | ⏳ TO FIX | Set environment variable TZ |
| 5 | Client không hiển thị thông tin user sau login | ✅ FIXED | Thêm g_user_info_label |

---

## **FEATURES MỚI ĐÃ THÊM**

1. ✅ Hiển thị thông tin user (username, role) ở header
2. ✅ DateTime picker với GTK widgets (date entry + time spinners)
3. ✅ Notification bar với GTK InfoBar
4. ✅ Color-coded status cho items (ACTIVE=xanh, SOLD=đỏ, PENDING=vàng)

---

**Hướng dẫn sử dụng bảng test:**
- ✅ = Test case đã pass
- ❌ = Test case fail (cần fix)
- ⏳ = Chưa test
- 🔄 = Đang test