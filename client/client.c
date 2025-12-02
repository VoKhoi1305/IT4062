// /*
//  * client.c
//  * (Không thay đổi)
//  */

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <sys/select.h>

// #define PORT 8080
// #define BUFFER_SIZE 4096

// int g_socket_fd;
// char g_socket_buffer[BUFFER_SIZE];
// int g_socket_buffer_pos = 0;
// char g_keyboard_buffer[BUFFER_SIZE];
// int g_keyboard_buffer_pos = 0;

// void process_socket_buffer() {
//     char* newline_ptr;
//     while ((newline_ptr = memchr(g_socket_buffer, '\n', g_socket_buffer_pos)) != NULL) {
//         int line_len = newline_ptr - g_socket_buffer;
//         char line_buffer[BUFFER_SIZE];
//         memcpy(line_buffer, g_socket_buffer, line_len);
//         line_buffer[line_len] = '\0';
//         printf("\nServer: %s\n", line_buffer);
//         int remaining_len = g_socket_buffer_pos - (line_len + 1);
//         memmove(g_socket_buffer, newline_ptr + 1, remaining_len);
//         g_socket_buffer_pos = remaining_len;
//     }
// }

// void process_keyboard_buffer() {
//     char* newline_ptr;
//     while ((newline_ptr = memchr(g_keyboard_buffer, '\n', g_keyboard_buffer_pos)) != NULL) {
//         int line_len = newline_ptr - g_keyboard_buffer;
//         char line_buffer[BUFFER_SIZE];
//         memcpy(line_buffer, g_keyboard_buffer, line_len);
//         line_buffer[line_len] = '\0';
//         int remaining_len = g_keyboard_buffer_pos - (line_len + 1);
//         memmove(g_keyboard_buffer, newline_ptr + 1, remaining_len);
//         g_keyboard_buffer_pos = remaining_len;
        
//         if (line_len > 0) {
//             if (strcmp(line_buffer, "QUIT") == 0) {
//                  close(g_socket_fd);
//                  printf("Da thoat.\n");
//                  exit(0);
//             }
//             char send_buf[BUFFER_SIZE];
//             snprintf(send_buf, BUFFER_SIZE, "%s\n", line_buffer);
//             send(g_socket_fd, send_buf, strlen(send_buf), 0);
//         }
//     }
// }

// int main(int argc, char *argv[]) {
//     struct sockaddr_in server_addr;

//     if (argc != 2) {
//         fprintf(stderr, "Su dung: %s <IP_SERVER>\n", argv[0]);
//         exit(EXIT_FAILURE);
//     }
//     const char* server_ip = argv[1];
//     g_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
//     memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT);
    
//     if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
//         perror("inet_pton() failed"); exit(EXIT_FAILURE);
//     }
//     if (connect(g_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         perror("connect() failed"); exit(EXIT_FAILURE);
//     }
//     printf("Da ket noi den server tai %s.\n", server_ip);
//     printf("Ban: ");
//     fflush(stdout); 

//     fd_set read_fds;
//     int max_fd;

//     while (1) {
//         FD_ZERO(&read_fds);
//         FD_SET(STDIN_FILENO, &read_fds);
//         FD_SET(g_socket_fd, &read_fds);
//         max_fd = (g_socket_fd > STDIN_FILENO) ? g_socket_fd : STDIN_FILENO;
//         int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

//         if (activity < 0) {
//             perror("select() error");
//             break;
//         }

//         if (FD_ISSET(STDIN_FILENO, &read_fds)) {
//             int bytes = read(STDIN_FILENO, g_keyboard_buffer + g_keyboard_buffer_pos, BUFFER_SIZE - g_keyboard_buffer_pos);
//             if (bytes <= 0) break;
//             g_keyboard_buffer_pos += bytes;
//             process_keyboard_buffer();
//             printf("Ban: ");
//             fflush(stdout);
//         }

//         if (FD_ISSET(g_socket_fd, &read_fds)) {
//             int bytes = recv(g_socket_fd, g_socket_buffer + g_socket_buffer_pos, BUFFER_SIZE - g_socket_buffer_pos, 0);
//             if (bytes <= 0) {
//                 printf("\n*** Server da ngat ket noi. ***\n");
//                 break;
//             }
//             g_socket_buffer_pos += bytes;
//             process_socket_buffer();
//             printf("Ban: ");
//             fflush(stdout);
//         }
//     }
//     close(g_socket_fd);
//     return 0;
// }

/*
 * client.c - Enhanced Client with Menu UI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define CLEAR_SCREEN "\033[2J\033[H"
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_BOLD "\033[1m"

int g_socket_fd;
char g_socket_buffer[BUFFER_SIZE];
int g_socket_buffer_pos = 0;
int g_is_logged_in = 0;
char g_username[50] = "";
int g_user_role = 0; // 0 = user, 1 = admin
int g_current_room_id = 0;

// Hàm hiển thị
void clear_screen() {
    printf(CLEAR_SCREEN);
}

void print_header(const char* title) {
    printf("\n");
    printf(COLOR_CYAN COLOR_BOLD);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║          %-49s ║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    
    if (g_is_logged_in) {
        printf(COLOR_GREEN "Đăng nhập: %s" COLOR_RESET, g_username);
        if (g_user_role == 1) {
            printf(COLOR_YELLOW " [ADMIN]" COLOR_RESET);
        }
        if (g_current_room_id > 0) {
            printf(COLOR_BLUE " | Phòng: #%d" COLOR_RESET, g_current_room_id);
        }
        printf("\n");
    }
    printf("\n");
}

void print_separator() {
    printf(COLOR_CYAN "─────────────────────────────────────────────────────────────\n" COLOR_RESET);
}

void print_menu_item(int number, const char* text) {
    printf(COLOR_YELLOW "  [%d]" COLOR_RESET " %s\n", number, text);
}

void print_success(const char* msg) {
    printf(COLOR_GREEN "✓ %s\n" COLOR_RESET, msg);
}

void print_error(const char* msg) {
    printf(COLOR_RED "✗ %s\n" COLOR_RESET, msg);
}

void print_info(const char* msg) {
    printf(COLOR_BLUE "ℹ %s\n" COLOR_RESET, msg);
}

void wait_enter() {
    printf("\n" COLOR_CYAN "Nhấn Enter để tiếp tục..." COLOR_RESET);
    getchar();
}

// Gửi lệnh đến server
void send_command(const char* cmd) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%s\n", cmd);
    send(g_socket_fd, buffer, strlen(buffer), 0);
}

// Đợi và nhận phản hồi từ server
char* wait_for_response() {
    static char response[BUFFER_SIZE];
    fd_set read_fds;
    struct timeval timeout;
    
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    FD_ZERO(&read_fds);
    FD_SET(g_socket_fd, &read_fds);
    
    int activity = select(g_socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (activity > 0 && FD_ISSET(g_socket_fd, &read_fds)) {
        int bytes = recv(g_socket_fd, g_socket_buffer + g_socket_buffer_pos, 
                        BUFFER_SIZE - g_socket_buffer_pos, 0);
        if (bytes <= 0) {
            return NULL;
        }
        g_socket_buffer_pos += bytes;
        
        char* newline = memchr(g_socket_buffer, '\n', g_socket_buffer_pos);
        if (newline) {
            int line_len = newline - g_socket_buffer;
            memcpy(response, g_socket_buffer, line_len);
            response[line_len] = '\0';
            
            int remaining = g_socket_buffer_pos - (line_len + 1);
            memmove(g_socket_buffer, newline + 1, remaining);
            g_socket_buffer_pos = remaining;
            
            return response;
        }
    }
    
    return NULL;
}

// Menu chính
void show_main_menu() {
    clear_screen();
    print_header("HỆ THỐNG ĐẤU GIÁ TRỰC TUYẾN");
    
    if (!g_is_logged_in) {
        print_menu_item(1, "Đăng nhập");
        print_menu_item(2, "Đăng ký");
        print_menu_item(0, "Thoát");
    } else {
        print_menu_item(1, "Quản lý phòng đấu giá");
        print_menu_item(2, "Tìm kiếm vật phẩm");
        print_menu_item(3, "Lịch sử đấu giá của tôi");
        if (g_user_role == 1) {
            print_menu_item(4, "Quản lý người dùng (Admin)");
        }
        print_menu_item(9, "Đăng xuất");
        print_menu_item(0, "Thoát");
    }
    print_separator();
}

// Đăng nhập
void handle_login() {
    clear_screen();
    print_header("ĐĂNG NHẬP");
    
    char username[50], password[50];
    printf("Tên đăng nhập: ");
    scanf("%s", username);
    printf("Mật khẩu: ");
    scanf("%s", password);
    getchar(); // Clear newline
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "LOGIN|%s|%s", username, password);
    send_command(cmd);
    
    char* response = wait_for_response();
    if (response) {
        if (strncmp(response, "LOGIN_SUCCESS", 13) == 0) {
            // Parse: LOGIN_SUCCESS|dang nhap thanh cong|username|role
            char* token = strtok(response, "|");
            token = strtok(NULL, "|"); // message
            token = strtok(NULL, "|"); // username
            strncpy(g_username, token, sizeof(g_username));
            token = strtok(NULL, "|"); // role
            g_user_role = atoi(token);
            g_is_logged_in = 1;
            
            print_success("Đăng nhập thành công!");
        } else {
            print_error("Đăng nhập thất bại!");
            printf("Chi tiết: %s\n", response);
        }
    } else {
        print_error("Không nhận được phản hồi từ server");
    }
    
    wait_enter();
}

// Đăng ký
void handle_register() {
    clear_screen();
    print_header("ĐĂNG KÝ TÀI KHOẢN");
    
    char username[50], password[50];
    printf("Tên đăng nhập: ");
    scanf("%s", username);
    printf("Mật khẩu: ");
    scanf("%s", password);
    getchar();
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "REGISTER|%s|%s", username, password);
    send_command(cmd);
    
    char* response = wait_for_response();
    if (response) {
        if (strncmp(response, "REGISTER_SUCCESS", 16) == 0) {
            print_success("Đăng ký thành công! Hãy đăng nhập.");
        } else {
            print_error("Đăng ký thất bại!");
            printf("Chi tiết: %s\n", response);
        }
    }
    
    wait_enter();
}

// Menu phòng đấu giá
void show_room_menu() {
    clear_screen();
    print_header("QUẢN LÝ PHÒNG ĐẤU GIÁ");
    
    print_menu_item(1, "Xem danh sách phòng");
    print_menu_item(2, "Tạo phòng mới");
    if (g_current_room_id > 0) {
        print_menu_item(3, "Xem chi tiết phòng hiện tại");
        print_menu_item(4, "Đặt giá cho vật phẩm");
        print_menu_item(5, "Mua ngay");
        print_menu_item(6, "Rời phòng");
    } else {
        print_menu_item(3, "Tham gia phòng");
    }
    print_menu_item(0, "Quay lại");
    print_separator();
}

// void handle_room_list() {
//     clear_screen();
//     print_header("DANH SÁCH PHÒNG");
    
//     printf("Lọc theo trạng thái (ALL/PENDING/ACTIVE/CLOSED): ");
//     char status[20];
//     scanf("%s", status);
//     getchar();
    
//     char cmd[256];
//     snprintf(cmd, sizeof(cmd), "GET_ROOM_LIST|%s|1|20", status);
//     send_command(cmd);
    
//     char* response = wait_for_response();
//     if (response && strncmp(response, "ROOM_LIST", 9) == 0) {
//         // Parse: ROOM_LIST|totalCount|roomData;...
//         char* token = strtok(response, "|");
//         token = strtok(NULL, "|"); // totalCount
//         int total = atoi(token);
        
//         printf("\n" COLOR_GREEN "Tổng số phòng: %d\n\n" COLOR_RESET, total);
        
//         token = strtok(NULL, "|"); // roomData
//         if (token && strlen(token) > 0) {
//             char* room = strtok(token, ";");
//             int count = 1;
//             while (room) {
//                 // Parse: roomId|roomName|ownerName|status|itemCount|participantCount|startTime|endTime
//                 char room_copy[512];
//                 strncpy(room_copy, room, sizeof(room_copy));
                
//                 char* r_token = strtok(room_copy, "|");
//                 int room_id = atoi(r_token);
//                 char* room_name = strtok(NULL, "|");
//                 char* owner = strtok(NULL, "|");
//                 char* r_status = strtok(NULL, "|");
//                 char* item_count = strtok(NULL, "|");
//                 char* participant = strtok(NULL, "|");
                
//                 printf(COLOR_YELLOW "[%d]" COLOR_RESET " Phòng #%d: %s\n", count++, room_id, room_name);
//                 printf("    Chủ phòng: %s | Trạng thái: %s\n", owner, r_status);
//                 printf("    Vật phẩm: %s | Người tham gia: %s\n\n", item_count, participant);
                
//                 room = strtok(NULL, ";");
//             }
//         } else {
//             print_info("Không có phòng nào");
//         }
//     }
    
//     wait_enter();
// }

void handle_room_list() {
    clear_screen();
    print_header("DANH SÁCH PHÒNG ĐẤU GIÁ");
    
    printf("Lọc theo trạng thái (ALL/PENDING/ACTIVE/CLOSED): ");
    char status[20];
    scanf("%s", status);
    getchar();
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "GET_ROOM_LIST|%s|1|20", status);
    send_command(cmd);
    
    char* response = wait_for_response();
    
    if (response && strncmp(response, "ROOM_LIST", 9) == 0) {
        // Cấu trúc response từ server: 
        // ROOM_LIST|TotalCount|id|name|ownerName|status|items|participants|start|end;...
        
        char* data_ptr = strchr(response, '|'); 
        int total = 0;
        
        if (data_ptr) {
            data_ptr++; 
            total = atoi(data_ptr); // Lấy TotalCount
            data_ptr = strchr(data_ptr, '|'); // Nhảy đến phần dữ liệu phòng
        }

        printf("\n" COLOR_GREEN "Tổng số phòng tìm thấy: %d\n" COLOR_RESET, total);
        print_separator();

        if (data_ptr) {
            data_ptr++; 
            
            char* room_token = strtok(data_ptr, ";");
            int count = 1;
            
            while (room_token != NULL) {
                int r_id, r_item_count, r_part_count;
                char r_name[100], r_owner[50], r_status[20], r_start[30], r_end[30];
                
                // QUAN TRỌNG: sscanf này khớp 100% với snprintf bên Server của bạn
                // Định dạng: ID | Name | OwnerName | Status | ItemCount | PartCount | Start | End
                int parsed = sscanf(room_token, "%d|%[^|]|%[^|]|%[^|]|%d|%d|%[^|]|%s", 
                                   &r_id, r_name, r_owner, r_status, 
                                   &r_item_count, &r_part_count, r_start, r_end);
                
                if (parsed >= 8) { // Đảm bảo đọc đủ 8 trường
                    printf(COLOR_YELLOW " [%d]" COLOR_RESET " Phòng #%d: " COLOR_BOLD "%s" COLOR_RESET "\n", 
                           count++, r_id, r_name);
                    
                    // Màu sắc trạng thái
                    char status_color[20] = COLOR_RESET;
                    if (strcmp(r_status, "ACTIVE") == 0) strcpy(status_color, COLOR_GREEN);
                    else if (strcmp(r_status, "CLOSED") == 0) strcpy(status_color, COLOR_RED);
                    else strcpy(status_color, COLOR_BLUE);

                    printf("    ├── Chủ phòng: %s\n", r_owner); // Giờ đã hiện tên thật thay vì ID
                    printf("    ├── Trạng thái: %s%s%s\n", status_color, r_status, COLOR_RESET);
                    printf("    ├── Thống kê: %d vật phẩm | %d người tham gia\n", r_item_count, r_part_count);
                    printf("    └── Thời gian: %s -> %s\n\n", r_start, r_end);
                }
                
                room_token = strtok(NULL, ";");
            }
        } else {
            print_info("Chưa có phòng nào được tạo.");
        }
    } else {
        print_error("Lỗi kết nối hoặc phản hồi không hợp lệ.");
    }
    
    wait_enter();
}

void handle_create_room() {
    clear_screen();
    print_header("TẠO PHÒNG MỚI");
    
    char room_name[100], start_time[30], end_time[30];
    
    printf("Tên phòng: ");
    getchar();
    fgets(room_name, sizeof(room_name), stdin);
    room_name[strcspn(room_name, "\n")] = 0;
    
    printf("Thời gian bắt đầu (YYYY-MM-DD HH:MM:SS): ");
    fgets(start_time, sizeof(start_time), stdin);
    start_time[strcspn(start_time, "\n")] = 0;
    
    printf("Thời gian kết thúc (YYYY-MM-DD HH:MM:SS): ");
    fgets(end_time, sizeof(end_time), stdin);
    end_time[strcspn(end_time, "\n")] = 0;
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "CREATE_ROOM|%s|%s|%s", room_name, start_time, end_time);
    send_command(cmd);
    
    char* response = wait_for_response();
    if (response && strncmp(response, "CREATE_ROOM_SUCCESS", 19) == 0) {
        print_success("Tạo phòng thành công!");
        printf("Chi tiết: %s\n", response);
    } else if (response) {
        print_error("Tạo phòng thất bại!");
        printf("Chi tiết: %s\n", response);
    }
    
    wait_enter();
}

void handle_join_room() {
    clear_screen();
    print_header("THAM GIA PHÒNG");
    
    int room_id;
    printf("Nhập ID phòng: ");
    scanf("%d", &room_id);
    getchar();
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "JOIN_ROOM|%d", room_id);
    send_command(cmd);
    
    char* response = wait_for_response();
    if (response && strncmp(response, "JOIN_ROOM_SUCCESS", 17) == 0) {
        g_current_room_id = room_id;
        print_success("Tham gia phòng thành công!");
    } else if (response) {
        print_error("Tham gia phòng thất bại!");
        printf("Chi tiết: %s\n", response);
    }
    
    wait_enter();
}

void handle_leave_room() {
    send_command("LEAVE_ROOM");
    
    char* response = wait_for_response();
    if (response && strncmp(response, "LEAVE_ROOM_SUCCESS", 18) == 0) {
        g_current_room_id = 0;
        print_success("Đã rời phòng!");
    }
    
    wait_enter();
}

void handle_place_bid() {
    clear_screen();
    print_header("ĐẶT GIÁ");
    
    int item_id;
    double amount;
    
    printf("ID vật phẩm: ");
    scanf("%d", &item_id);
    printf("Số tiền đặt giá: ");
    scanf("%lf", &amount);
    getchar();
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "PLACE_BID|%d|%.0f", item_id, amount);
    send_command(cmd);
    
    char* response = wait_for_response();
    if (response) {
        if (strncmp(response, "BID_SUCCESS", 11) == 0) {
            print_success("Đặt giá thành công!");
        } else {
            print_error("Đặt giá thất bại!");
        }
        printf("Chi tiết: %s\n", response);
    }
    
    wait_enter();
}

void handle_buy_now() {
    clear_screen();
    print_header("MUA NGAY");
    
    int item_id;
    printf("ID vật phẩm: ");
    scanf("%d", &item_id);
    getchar();
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "BUY_NOW|%d", item_id);
    send_command(cmd);
    
    char* response = wait_for_response();
    if (response) {
        if (strncmp(response, "BUY_NOW_SUCCESS", 15) == 0) {
            print_success("Mua thành công!");
        } else {
            print_error("Mua thất bại!");
        }
        printf("Chi tiết: %s\n", response);
    }
    
    wait_enter();
}

void handle_auction_history() {
    clear_screen();
    print_header("LỊCH SỬ ĐẤU GIÁ");
    
    printf("Lọc (ALL/WON/LOST/BIDDING): ");
    char filter[20];
    scanf("%s", filter);
    getchar();
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "GET_MY_AUCTION_HISTORY|%s|1|20", filter);
    send_command(cmd);
    
    char* response = wait_for_response();
    if (response && strncmp(response, "AUCTION_HISTORY", 15) == 0) {
        // Parse và hiển thị
        char* token = strtok(response, "|");
        token = strtok(NULL, "|"); // totalCount
        int total = atoi(token);
        
        printf("\n" COLOR_GREEN "Tổng: %d phiên\n\n" COLOR_RESET, total);
        
        token = strtok(NULL, "|");
        if (token && strlen(token) > 0) {
            // Parse history data
            print_info("Chi tiết lịch sử:");
            printf("%s\n", token);
        } else {
            print_info("Chưa có lịch sử");
        }
    }
    
    wait_enter();
}

void handle_admin_user_list() {
    clear_screen();
    print_header("DANH SÁCH NGƯỜI DÙNG (ADMIN QUẢN LÝ)");

    send_command("GET_USER_LIST");
    char* response = wait_for_response();

    if (response) {
        if (strncmp(response, "USER_LIST", 9) == 0) {
            char* data_part = strchr(response, '|'); 
            
            if (data_part) {
                data_part++; // Nhảy qua dấu '|'
                
                printf(COLOR_BOLD "%-5s %-20s %-15s %-10s" COLOR_RESET "\n", "ID", "USERNAME", "TRẠNG THÁI", "VAI TRÒ");
                print_separator();

                // Dùng strtok cho vòng ngoài (tách các user bằng dấu ;)
                char* user_entry = strtok(data_part, ";");
                
                int count = 0;
                while (user_entry != NULL) {
                    // Biến chứa thông tin sau khi tách
                    char u_id[20], u_name[50], u_status_str[5], u_role_str[5];
                    
                    // SỬA LỖI: Dùng sscanf thay vì strtok để không làm hỏng vòng lặp ngoài
                    // Cú pháp: %[^|] nghĩa là đọc mọi ký tự cho đến khi gặp dấu |
                    int parsed = sscanf(user_entry, "%[^|]|%[^|]|%[^|]|%s", 
                                      u_id, u_name, u_status_str, u_role_str);

                    if (parsed == 4) {
                        int is_online = atoi(u_status_str);
                        int role = atoi(u_role_str);

                        printf("%-5s %-20s ", u_id, u_name);
                        
                        if (is_online) {
                            printf(COLOR_GREEN "%-15s" COLOR_RESET, "Online");
                        } else {
                            printf(COLOR_RED "%-15s" COLOR_RESET, "Offline");
                        }

                        if (role == 1) {
                            printf(COLOR_YELLOW "%-10s" COLOR_RESET, "ADMIN");
                        } else {
                            printf("%-10s", "User");
                        }
                        printf("\n");
                        count++;
                    }

                    // Tiếp tục lấy user tiếp theo từ chuỗi gốc
                    user_entry = strtok(NULL, ";");
                }
                
                printf("\n" COLOR_CYAN "Tổng cộng: %d tài khoản." COLOR_RESET "\n", count);
            } else {
                print_info("Danh sách trống.");
            }
        } else {
            print_error("Lỗi: Server trả về dữ liệu không đúng định dạng.");
            printf("Chi tiết: %s\n", response);
        }
    } else {
        print_error("Không nhận được phản hồi từ server.");
    }

    wait_enter();
}


void handle_room_detail() {
    clear_screen();
    
    // 1. Kiểm tra xem người dùng có đang ở trong phòng không
    if (g_current_room_id <= 0) {
        print_error("Bạn chưa tham gia phòng nào!");
        wait_enter();
        return;
    }

    // 2. Gửi lệnh lấy chi tiết phòng
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "GET_ROOM_DETAIL|%d", g_current_room_id);
    send_command(cmd);

    // 3. Nhận phản hồi
    char* response = wait_for_response();

    if (response && strncmp(response, "ROOM_DETAIL", 11) == 0) {
        // Cấu trúc: ROOM_DETAIL|id|name|status|start|end|item_data...
        
        char* data_ptr = strchr(response, '|');
        if (data_ptr) {
            data_ptr++; // Bỏ qua chữ "ROOM_DETAIL|"
            
            // --- PARSE THÔNG TIN PHÒNG ---
            // Server gửi: id|name|status|start|end|
            // Lưu ý: Server có dấu | ở cuối cùng của phần thông tin phòng trước khi đến item
            
            char r_id_str[20], r_name[100], r_status[20], r_start[30], r_end[30];
            
            // Tách các trường thông tin phòng đầu tiên
            // Ta dùng logic copy từng phần vì strtok có thể làm hỏng phần item phía sau nếu không cẩn thận
            char* token = strtok(data_ptr, "|"); // ID
            if(token) strcpy(r_id_str, token);
            
            token = strtok(NULL, "|"); // Name
            if(token) strcpy(r_name, token);
            
            token = strtok(NULL, "|"); // Status
            if(token) strcpy(r_status, token);
            
            token = strtok(NULL, "|"); // Start Time
            if(token) strcpy(r_start, token);
            
            token = strtok(NULL, "|"); // End Time
            if(token) strcpy(r_end, token);

            // --- HIỂN THỊ THÔNG TIN PHÒNG ---
            print_header("CHI TIẾT PHÒNG ĐẤU GIÁ");
            printf(COLOR_CYAN " [%s] " COLOR_BOLD "%s" COLOR_RESET "\n", r_id_str, r_name);
            
            char status_color[20] = COLOR_RESET;
            if (strcmp(r_status, "ACTIVE") == 0) strcpy(status_color, COLOR_GREEN);
            else if (strcmp(r_status, "CLOSED") == 0) strcpy(status_color, COLOR_RED);
            
            printf(" Trạng thái: %s%s" COLOR_RESET "\n", status_color, r_status);
            printf(" Thời gian:  %s -> %s\n", r_start, r_end);
            print_separator();

            // --- PARSE VÀ HIỂN THỊ DANH SÁCH VẬT PHẨM ---
            printf(COLOR_BOLD " DANH SÁCH VẬT PHẨM ĐANG ĐẤU GIÁ:\n" COLOR_RESET);
            printf(" %-5s %-25s %-12s %-12s %-12s\n", "ID", "Tên vật phẩm", "Giá KĐ", "Giá hiện tại", "Mua ngay");
            print_separator();

            // Lấy phần chuỗi còn lại (chứa items)
            // strtok ghi đè \0 vào các dấu |, nên ta cần lấy con trỏ từ lần strtok cuối cùng
            // Tuy nhiên, để an toàn, ta nên tìm lại vị trí bắt đầu của items
            // items bắt đầu sau token End Time.
            
            token = strtok(NULL, ""); // Lấy toàn bộ phần còn lại của chuỗi (nếu có)
            
            if (token && strlen(token) > 0) {
                // Token lúc này chứa: item1;item2;...
                char* item_row = strtok(token, ";");
                int item_count = 0;
                
                while (item_row != NULL) {
                    // Item row format: id|name|status|start_price|current_price|buy_now
                    // Dùng sscanf để parse dòng này an toàn
                    
                    int i_id;
                    char i_name[100], i_status[20];
                    double i_start, i_curr, i_buy;
                    
                    int parsed = sscanf(item_row, "%d|%[^|]|%[^|]|%lf|%lf|%lf",
                                      &i_id, i_name, i_status, &i_start, &i_curr, &i_buy);
                                      
                    if (parsed >= 6) {
                        printf(" %-5d %-25s %-12.0f " COLOR_GREEN "%-12.0f" COLOR_RESET " %-12.0f\n", 
                               i_id, i_name, i_start, i_curr, i_buy);
                        item_count++;
                    }
                    
                    item_row = strtok(NULL, ";");
                }
                
                if (item_count == 0) {
                    print_info("Chưa có vật phẩm nào trong phòng này.");
                }
            } else {
                print_info("Chưa có vật phẩm nào trong phòng này.");
            }
        }
    } else if (response && strncmp(response, "ROOM_DETAIL_FAIL", 16) == 0) {
        print_error("Lỗi lấy thông tin phòng:");
        printf("%s\n", strchr(response, '|') + 1);
        // Nếu phòng lỗi/không tồn tại, reset room ID hiện tại
        g_current_room_id = 0;
    } else {
        print_error("Không nhận được phản hồi hợp lệ từ server.");
    }
    
    // Hướng dẫn người dùng
    printf("\n" COLOR_YELLOW " [4] Đặt giá   [5] Mua ngay   [6] Rời phòng" COLOR_RESET "\n");
    wait_enter();
}



// Main loop
int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;

    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <IP_SERVER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    const char* server_ip = argv[1];
    g_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton() failed");
        exit(EXIT_FAILURE);
    }
    
    if (connect(g_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }
    
    // Nhận welcome message
    char* welcome = wait_for_response();
    
    while (1) {
        show_main_menu();
        printf("Chọn: ");
        int choice;
        scanf("%d", &choice);
        getchar();
        
        if (!g_is_logged_in) {
            switch (choice) {
                case 1: handle_login(); break;
                case 2: handle_register(); break;
                case 0: 
                    close(g_socket_fd);
                    printf("Tạm biệt!\n");
                    return 0;
            }
        } else {
            switch (choice) {
                case 1: {
                    int room_choice;
                    do {
                        show_room_menu();
                        printf("Chọn: ");
                        scanf("%d", &room_choice);
                        getchar();
                        
                        switch (room_choice) {
                            case 1: handle_room_list(); break;
                            case 2: handle_create_room(); break;
                            case 3: 
                                if (g_current_room_id > 0) {
                                     handle_room_detail();
                                } else {
                                    handle_join_room();
                                }
                                break;
                            case 4: handle_place_bid(); break;
                            case 5: handle_buy_now(); break;
                            case 6: handle_leave_room(); break;
                        }
                    } while (room_choice != 0);
                    break;
                }
                case 2: 
                    // TODO: Search items
                    break;
                case 3: 
                    handle_auction_history();
                    break;
                case 4:
                    if (g_user_role == 1) {
                        handle_admin_user_list();
                    } else {
                        print_error("Bạn không có quyền truy cập chức năng này!");
                        wait_enter();
                    }
                    break;
                case 9:
                    send_command("LOGOUT");
                    g_is_logged_in = 0;
                    g_user_role = 0;
                    g_current_room_id = 0;
                    memset(g_username, 0, sizeof(g_username));
                    print_success("Đã đăng xuất!");
                    wait_enter();
                    break;
                case 0:
                    close(g_socket_fd);
                    printf("Tạm biệt!\n");
                    return 0;
            }
        }
    }
    
    return 0;
}