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

void handle_room_list() {
    clear_screen();
    print_header("DANH SÁCH PHÒNG");
    
    printf("Lọc theo trạng thái (ALL/PENDING/ACTIVE/CLOSED): ");
    char status[20];
    scanf("%s", status);
    getchar();
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "GET_ROOM_LIST|%s|1|20", status);
    send_command(cmd);
    
    char* response = wait_for_response();
    if (response && strncmp(response, "ROOM_LIST", 9) == 0) {
        // Parse: ROOM_LIST|totalCount|roomData;...
        char* token = strtok(response, "|");
        token = strtok(NULL, "|"); // totalCount
        int total = atoi(token);
        
        printf("\n" COLOR_GREEN "Tổng số phòng: %d\n\n" COLOR_RESET, total);
        
        token = strtok(NULL, "|"); // roomData
        if (token && strlen(token) > 0) {
            char* room = strtok(token, ";");
            int count = 1;
            while (room) {
                // Parse: roomId|roomName|ownerName|status|itemCount|participantCount|startTime|endTime
                char room_copy[512];
                strncpy(room_copy, room, sizeof(room_copy));
                
                char* r_token = strtok(room_copy, "|");
                int room_id = atoi(r_token);
                char* room_name = strtok(NULL, "|");
                char* owner = strtok(NULL, "|");
                char* r_status = strtok(NULL, "|");
                char* item_count = strtok(NULL, "|");
                char* participant = strtok(NULL, "|");
                
                printf(COLOR_YELLOW "[%d]" COLOR_RESET " Phòng #%d: %s\n", count++, room_id, room_name);
                printf("    Chủ phòng: %s | Trạng thái: %s\n", owner, r_status);
                printf("    Vật phẩm: %s | Người tham gia: %s\n\n", item_count, participant);
                
                room = strtok(NULL, ";");
            }
        } else {
            print_info("Không có phòng nào");
        }
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
                                    // TODO: handle_room_detail
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
                        // TODO: Admin menu
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