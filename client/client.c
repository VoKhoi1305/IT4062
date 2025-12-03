// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <sys/select.h>
// #include <ctype.h>

// // =============================================================
// // CONFIGURATION & CONSTANTS
// // =============================================================
// #define PORT 8080
// #define BUFFER_SIZE 4096
// #define APP_WIDTH 80

// // ANSI Colors
// #define RESET   "\033[0m"
// #define RED     "\033[31m"
// #define GREEN   "\033[32m"
// #define YELLOW  "\033[33m"
// #define BLUE    "\033[34m"
// #define MAGENTA "\033[35m"
// #define CYAN    "\033[36m"
// #define WHITE   "\033[37m"
// #define BOLD    "\033[1m"
// #define CLEAR   "\033[2J\033[H"

// // Global State
// int g_socket_fd;
// char g_socket_buffer[BUFFER_SIZE];
// int g_socket_buffer_pos = 0;
// int g_is_logged_in = 0;
// char g_username[50] = "";
// int g_user_role = 0; // 0 = user, 1 = admin
// int g_current_room_id = 0;

// // =============================================================
// // UI UTILITIES (HELPER FUNCTIONS)
// // =============================================================

// void clear_screen() {
//     printf(CLEAR);
// }

// // In một dòng kẻ ngang
// void print_divider(char ch, int length) {
//     printf(CYAN);
//     for (int i = 0; i < length; i++) printf("%c", ch);
//     printf(RESET "\n");
// }

// // In tiêu đề căn giữa trong hộp
// void print_header(const char* title) {
//     clear_screen();
//     print_divider('=', APP_WIDTH);
    
//     int padding = (APP_WIDTH - strlen(title)) / 2;
//     printf(CYAN BOLD "%*s%s%*s" RESET "\n", padding, "", title, padding, "");
    
//     print_divider('=', APP_WIDTH);
    
//     if (g_is_logged_in) {
//         printf(GREEN " User: %-15s" RESET, g_username);
//         if (g_user_role == 1) printf(YELLOW " [ADMIN]" RESET);
//         if (g_current_room_id > 0) printf(BLUE " | Room: #%d" RESET, g_current_room_id);
//         printf("\n");
//         print_divider('-', APP_WIDTH);
//     }
//     printf("\n");
// }

// // Hàm hỗ trợ nhập liệu an toàn (thay thế scanf/fgets)
// void get_input_string(const char* prompt, char* buffer, int size) {
//     printf(BOLD "%s" RESET, prompt);
//     if (fgets(buffer, size, stdin) != NULL) {
//         size_t len = strlen(buffer);
//         if (len > 0 && buffer[len - 1] == '\n') {
//             buffer[len - 1] = '\0';
//         }
//     } else {
//         buffer[0] = '\0';
//     }
// }

// int get_input_int(const char* prompt) {
//     char buffer[100];
//     get_input_string(prompt, buffer, sizeof(buffer));
//     return atoi(buffer);
// }

// double get_input_double(const char* prompt) {
//     char buffer[100];
//     get_input_string(prompt, buffer, sizeof(buffer));
//     return atof(buffer);
// }

// void wait_enter() {
//     printf("\n" CYAN ">> Nhấn Enter để tiếp tục..." RESET);
//     getchar();
// }

// void print_message(const char* type, const char* msg) {
//     if (strcmp(type, "SUCCESS") == 0) printf(GREEN " [OK] %s" RESET "\n", msg);
//     else if (strcmp(type, "ERROR") == 0) printf(RED " [ERR] %s" RESET "\n", msg);
//     else if (strcmp(type, "INFO") == 0) printf(BLUE " [INFO] %s" RESET "\n", msg);
//     else printf(" %s\n", msg);
// }

// // =============================================================
// // NETWORK UTILITIES
// // =============================================================

// void send_command(const char* cmd) {
//     char buffer[BUFFER_SIZE];
//     snprintf(buffer, BUFFER_SIZE, "%s\n", cmd);
//     if (send(g_socket_fd, buffer, strlen(buffer), 0) < 0) {
//         perror("Send failed");
//     }
// }

// char* wait_for_response() {
//     static char response[BUFFER_SIZE];
//     fd_set read_fds;
//     struct timeval timeout;
    
//     timeout.tv_sec = 5; // Timeout 5s
//     timeout.tv_usec = 0;
    
//     FD_ZERO(&read_fds);
//     FD_SET(g_socket_fd, &read_fds);
    
//     int activity = select(g_socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    
//     if (activity > 0 && FD_ISSET(g_socket_fd, &read_fds)) {
//         int bytes = recv(g_socket_fd, g_socket_buffer + g_socket_buffer_pos, 
//                         BUFFER_SIZE - g_socket_buffer_pos - 1, 0);
//         if (bytes <= 0) return NULL;
        
//         g_socket_buffer_pos += bytes;
//         g_socket_buffer[g_socket_buffer_pos] = '\0'; // Null terminate
        
//         char* newline = strchr(g_socket_buffer, '\n');
//         if (newline) {
//             int line_len = newline - g_socket_buffer;
//             memcpy(response, g_socket_buffer, line_len);
//             response[line_len] = '\0';
            
//             int remaining = g_socket_buffer_pos - (line_len + 1);
//             memmove(g_socket_buffer, newline + 1, remaining);
//             g_socket_buffer_pos = remaining;
            
//             return response;
//         }
//     }
//     return NULL;
// }

// // =============================================================
// // FEATURE HANDLERS
// // =============================================================

// // --- AUTHENTICATION ---
// void handle_login() {
//     print_header("DANG NHAP HE THONG");
//     char username[50], password[50];
    
//     get_input_string(" Ten dang nhap: ", username, sizeof(username));
//     get_input_string(" Mat khau: ", password, sizeof(password));
    
//     char cmd[256];
//     snprintf(cmd, sizeof(cmd), "LOGIN|%s|%s", username, password);
//     send_command(cmd);
    
//     char* response = wait_for_response();
//     if (response && strncmp(response, "LOGIN_SUCCESS", 13) == 0) {
//         // format: LOGIN_SUCCESS|msg|username|role
//         char* ptr = strtok(response, "|"); // Skip CMD
//         ptr = strtok(NULL, "|"); // Msg
//         ptr = strtok(NULL, "|"); // Username
//         if (ptr) strncpy(g_username, ptr, sizeof(g_username));
//         ptr = strtok(NULL, "|"); // Role
//         if (ptr) g_user_role = atoi(ptr);
        
//         g_is_logged_in = 1;
//         print_message("SUCCESS", "Dang nhap thanh cong!");
//     } else {
//         print_message("ERROR", response ? response : "Loi ket noi");
//     }
//     wait_enter();
// }

// void handle_register() {
//     print_header("DANG KY TAI KHOAN");
//     char username[50], password[50];
    
//     get_input_string(" Ten dang nhap moi: ", username, sizeof(username));
//     get_input_string(" Mat khau: ", password, sizeof(password));
    
//     char cmd[256];
//     snprintf(cmd, sizeof(cmd), "REGISTER|%s|%s", username, password);
//     send_command(cmd);
    
//     char* response = wait_for_response();
//     if (response && strncmp(response, "REGISTER_SUCCESS", 16) == 0) {
//         print_message("SUCCESS", "Dang ky thanh cong! Vui long dang nhap.");
//     } else {
//         print_message("ERROR", response ? response : "Dang ky that bai");
//     }
//     wait_enter();
// }

// // --- ROOM MANAGEMENT ---
// void handle_room_list() {
//     print_header("DANH SACH PHONG DAU GIA");
//     char status[20];
//     get_input_string(" Loc trang thai (ALL/ACTIVE/CLOSED): ", status, sizeof(status));
//     if(strlen(status) == 0) strcpy(status, "ALL");

//     char cmd[256];
//     snprintf(cmd, sizeof(cmd), "GET_ROOM_LIST|%s|1|50", status);
//     send_command(cmd);

//     char* response = wait_for_response();
//     if (!response || strncmp(response, "ROOM_LIST", 9) != 0) {
//         print_message("ERROR", "Khong tai duoc danh sach phong");
//         wait_enter(); return;
//     }

//     char* data = strchr(response, '|');
//     if (!data) { wait_enter(); return; }
    
//     data++; // Skip pipe
//     int total = atoi(data);
//     printf(GREEN "\n Tim thay: %d phong\n" RESET, total);
    
//     data = strchr(data, '|'); // Move to room items
    
//     printf(BOLD "\n %-5s %-25s %-15s %-10s %-10s" RESET "\n", 
//            "ID", "TEN PHONG", "CHU PHONG", "TRANG THAI", "VAT PHAM");
//     print_divider('-', 80);

//     if (data) {
//         data++; // Skip pipe
//         char* item = strtok(data, ";");
//         while(item) {
//             int id, i_count, p_count;
//             char name[100], owner[50], st[20], s_time[30], e_time[30];
            
//             // Format: id|name|owner|status|item_count|part_count|start|end
//             sscanf(item, "%d|%[^|]|%[^|]|%[^|]|%d|%d|%[^|]|%s",
//                    &id, name, owner, st, &i_count, &p_count, s_time, e_time);
            
//             char color[10] = RESET;
//             if(strcmp(st, "ACTIVE") == 0) strcpy(color, GREEN);
//             else if(strcmp(st, "CLOSED") == 0) strcpy(color, RED);

//             printf(" %-5d %-25.25s %-15.15s %s%-10s" RESET " %-10d\n", 
//                    id, name, owner, color, st, i_count);
            
//             item = strtok(NULL, ";");
//         }
//     }
//     wait_enter();
// }

// void handle_create_room() {
//     print_header("TAO PHONG MOI");
//     char name[100], start[30], end[30];
    
//     get_input_string(" Ten phong: ", name, sizeof(name));
//     get_input_string(" Bat dau (YYYY-MM-DD HH:MM:SS): ", start, sizeof(start));
//     get_input_string(" Ket thuc (YYYY-MM-DD HH:MM:SS): ", end, sizeof(end));
    
//     char cmd[512];
//     snprintf(cmd, sizeof(cmd), "CREATE_ROOM|%s|%s|%s", name, start, end);
//     send_command(cmd);
    
//     char* response = wait_for_response();
//     if(response && strncmp(response, "CREATE_ROOM_SUCCESS", 19) == 0)
//         print_message("SUCCESS", "Tao phong thanh cong!");
//     else 
//         print_message("ERROR", response);
//     wait_enter();
// }

// void handle_join_room() {
//     int id = get_input_int(" Nhap ID phong muon vao: ");
//     char cmd[50];
//     snprintf(cmd, sizeof(cmd), "JOIN_ROOM|%d", id);
//     send_command(cmd);
    
//     char* res = wait_for_response();
//     if(res && strncmp(res, "JOIN_ROOM_SUCCESS", 17) == 0) {
//         g_current_room_id = id;
//         print_message("SUCCESS", "Vao phong thanh cong!");
//     } else {
//         print_message("ERROR", res);
//     }
//     wait_enter();
// }



// void handle_place_bid() {
//     int item_id = get_input_int(" Nhap ID vat pham: ");
//     double amount = get_input_double(" Nhap gia muon dat: ");
    
//     char cmd[100];
//     snprintf(cmd, sizeof(cmd), "PLACE_BID|%d|%.0f", item_id, amount);
//     send_command(cmd);
    
//     char* res = wait_for_response();
//     if(res && strncmp(res, "BID_SUCCESS", 11) == 0) print_message("SUCCESS", "Dat gia thanh cong!");
//     else print_message("ERROR", res);
//     wait_enter();
// }

// void handle_buy_now() {
//     int item_id = get_input_int(" Nhap ID vat pham mua ngay: ");
//     char cmd[100];
//     snprintf(cmd, sizeof(cmd), "BUY_NOW|%d", item_id);
//     send_command(cmd);
    
//     char* res = wait_for_response();
//     if(res && strncmp(res, "BUY_NOW_SUCCESS", 15) == 0) print_message("SUCCESS", "Mua vat pham thanh cong!");
//     else print_message("ERROR", res);
//     wait_enter();
// }

// // --- SEARCH & HISTORY ---
// void handle_search_items() {
//     print_header("TIM KIEM VAT PHAM");
//     printf(" 1. Tim theo Ten\n");
//     printf(" 2. Tim theo Thoi gian\n");
//     printf(" 3. Tim ket hop (Ten & Thoi gian)\n");
//     printf(" 0. Quay lai\n");
    
//     int choice = get_input_int(" Chon: ");
//     if(choice == 0) return;

//     char type[10], kw[100] = "", t_from[30] = "", t_to[30] = "";
    
//     if(choice == 1 || choice == 3) {
//         get_input_string(" Nhap tu khoa: ", kw, sizeof(kw));
//     }
//     if(choice == 2 || choice == 3) {
//         get_input_string(" Tu ngay (YYYY-MM-DD): ", t_from, sizeof(t_from));
//         get_input_string(" Den ngay (YYYY-MM-DD): ", t_to, sizeof(t_to));
//     }
    
//     strcpy(type, (choice==1) ? "NAME" : (choice==2 ? "TIME" : "BOTH"));
    
//     char cmd[512];
//     snprintf(cmd, sizeof(cmd), "SEARCH_ITEMS|%s|%s|%s|%s", type, kw, t_from, t_to);
//     send_command(cmd);
    
//     char* res = wait_for_response();
//     if(!res || strncmp(res, "SEARCH_RESULT", 13) != 0) {
//         print_message("ERROR", "Loi tim kiem"); wait_enter(); return;
//     }
    
//     // Parse: SEARCH_RESULT|count|id|roomid|roomname|itemname|startp|currp|status|start|end;...
//     char* ptr = strchr(res, '|') + 1;
//     int count = atoi(ptr);
//     ptr = strchr(ptr, '|') + 1;
    
//     printf(GREEN "\n Ket qua: %d vat pham\n" RESET, count);
//     if(count == 0) { wait_enter(); return; }

//     printf(BOLD "\n %-5s %-15s %-20s %-10s %-10s" RESET "\n", 
//            "ID", "PHONG", "VAT PHAM", "GIA", "TRANG THAI");
//     print_divider('.', 80);
    
//     char* item = strtok(ptr, ";");
//     while(item) {
//         int id, rid;
//         char rname[100], iname[100], st[20], dum[30];
//         double sp, cp;
//         // id|rid|rname|iname|sp|cp|st|...
//         sscanf(item, "%d|%d|%[^|]|%[^|]|%lf|%lf|%[^|]|%[^|]|%[^|]", 
//                &id, &rid, rname, iname, &sp, &cp, st, dum, dum);
               
//         printf(" %-5d %-15.15s %-20.20s %-10.0f %s\n", 
//                id, rname, iname, cp, st);
//         item = strtok(NULL, ";");
//     }
//     wait_enter();
// }

// void handle_auction_history() {
//     print_header("LICH SU DAU GIA CUA TOI");
//     char filter[20];
//     get_input_string(" Loc (ALL/WON/LOST): ", filter, sizeof(filter));
//     if(strlen(filter)==0) strcpy(filter, "ALL");
    
//     char cmd[100];
//     snprintf(cmd, sizeof(cmd), "GET_MY_AUCTION_HISTORY|%s|1|20", filter);
//     send_command(cmd);
    
//     char* res = wait_for_response();
//     if(res) {
//         printf("\n %s\n", res); // Raw print for now as parsing logic same as above
//     }
//     wait_enter();
// }

// void handle_admin_user_list() {
//     print_header("QUAN LY NGUOI DUNG (ADMIN)");
//     send_command("GET_USER_LIST");
//     char* res = wait_for_response();
    
//     if(res && strncmp(res, "USER_LIST", 9) == 0) {
//         char* ptr = strchr(res, '|') + 1;
//         printf(BOLD " %-5s %-20s %-10s %-10s" RESET "\n", "ID", "USERNAME", "STATUS", "ROLE");
//         print_divider('-', 60);
        
//         char* u = strtok(ptr, ";");
//         while(u) {
//             char uid[10], uname[50], onl[5], role[5];
//             sscanf(u, "%[^|]|%[^|]|%[^|]|%s", uid, uname, onl, role);
            
//             printf(" %-5s %-20s %s%-10s" RESET " %s\n", 
//                    uid, uname, 
//                    strcmp(onl,"1")==0 ? GREEN : RED, 
//                    strcmp(onl,"1")==0 ? "Online" : "Offline",
//                    strcmp(role,"1")==0 ? YELLOW "ADMIN" RESET : "User");
//             u = strtok(NULL, ";");
//         }
//     }
//     wait_enter();
// }

// // --- ROOM ACTION & DETAIL ---
// void handle_room_detail() {
//     if (g_current_room_id == 0) {
//         print_message("ERROR", "Ban chua tham gia phong nao!");
//         wait_enter();
//         return;
//     }

//     int in_room_menu = 1;
//     while (in_room_menu) {
//         // 1. Gửi lệnh lấy thông tin mới nhất
//         char cmd[50];
//         snprintf(cmd, sizeof(cmd), "GET_ROOM_DETAIL|%d", g_current_room_id);
//         send_command(cmd);

//         char* res = wait_for_response();
        
//         // 2. Xóa màn hình
//         clear_screen(); 
        
//         if (!res || strncmp(res, "ROOM_DETAIL", 11) != 0) {
//             print_message("ERROR", "Khong lay duoc thong tin phong hoac phong da dong.");
//             g_current_room_id = 0; 
//             wait_enter();
//             return;
//         }

//         // --- Parse thông tin phòng ---
//         char *ptr = strchr(res, '|') + 1;
//         char r_id[10], r_name[100], r_stat[20], r_start[30], r_end[30];

//         char* token = strtok(ptr, "|"); if(token) strcpy(r_id, token);
//         token = strtok(NULL, "|"); if(token) strcpy(r_name, token);
//         token = strtok(NULL, "|"); if(token) strcpy(r_stat, token);
//         token = strtok(NULL, "|"); if(token) strcpy(r_start, token);
//         token = strtok(NULL, "|"); if(token) strcpy(r_end, token);

//         print_header("CHI TIET PHONG DAU GIA");
//         printf(BOLD " Phong: %s (#%s)\n" RESET, r_name, r_id);
        
//         char r_stat_color[10] = RESET;
//         if(strcmp(r_stat, "ACTIVE") == 0) strcpy(r_stat_color, GREEN);
//         else strcpy(r_stat_color, RED);
        
//         printf(" Trang thai: %s%s" RESET " | %s -> %s\n", r_stat_color, r_stat, r_start, r_end);
//         print_divider('-', 95);

//         // --- Header bảng ---
//         printf(BOLD " %-5s %-20s %-12s %-12s %-12s %-12s" RESET "\n", 
//                "ID", "VAT PHAM", "TRANG THAI", "KHOI DIEM", "HIEN TAI", "MUA NGAY");
//         print_divider('.', 95);

//         // --- Parse Items và Lọc ---
//         token = strtok(NULL, ""); 
//         int displayed_count = 0; // Biến đếm số vật phẩm thực sự được hiển thị

//         if (token && strlen(token) > 0) {
//             char* item = strtok(token, ";");
//             while (item) {
//                 int i_id; 
//                 char i_name[100], i_stat[20]; 
//                 double p_start, p_curr, p_buy;
                
//                 sscanf(item, "%d|%[^|]|%[^|]|%lf|%lf|%lf", 
//                        &i_id, i_name, i_stat, &p_start, &p_curr, &p_buy);
                
//                 // === [LOGIC LỌC TẠI ĐÂY] ===
//                 // Nếu trạng thái KHÔNG PHẢI là ACTIVE thì bỏ qua (không in)
//                 if (strcmp(i_stat, "ACTIVE") != 0) {
//                     item = strtok(NULL, ";");
//                     continue; // Nhảy sang vòng lặp tiếp theo
//                 }

//                 // Nếu là ACTIVE thì in ra
//                 printf(" %-5d %-20.20s " GREEN "%-12s" RESET " %-12.0f %-12.0f %-12.0f\n", 
//                        i_id, i_name, i_stat, p_start, p_curr, p_buy);
                
//                 displayed_count++;
//                 item = strtok(NULL, ";");
//             }
//         } 
        
//         // Nếu không có item nào ACTIVE
//         if (displayed_count == 0) {
//             printf(YELLOW " >> Hien chua co vat pham nao dang dau gia (Active).\n" RESET);
//         }
//         print_divider('=', 95);

//         // 3. Menu thao tác
//         printf(BOLD " [4] Dat gia    [5] Mua ngay    [R] Refresh (Tai lai)\n" RESET);
//         printf(BOLD " [6] Roi phong  [0] Quay lai menu chinh\n" RESET);
        
//         // 4. Nhập lệnh
//         char input[10];
//         printf("\n >> Nhap lua chon: ");
//         fgets(input, sizeof(input), stdin);
//         input[strcspn(input, "\n")] = 0; 

//         // 5. Xử lý lệnh
//         if (strcmp(input, "0") == 0) {
//             in_room_menu = 0; 
//         } 
//         else if (strcasecmp(input, "r") == 0) {
//             continue; 
//         }
//         else if (strcmp(input, "4") == 0) {
//             handle_place_bid(); 
//         } 
//         else if (strcmp(input, "5") == 0) {
//             handle_buy_now();
//         } 
//         else if (strcmp(input, "6") == 0) {
//             send_command("LEAVE_ROOM");
//             wait_for_response();
//             g_current_room_id = 0;
//             printf(GREEN "Da roi phong.\n" RESET);
//             in_room_menu = 0; 
//         }
//     }
// }

// // =============================================================
// // MAIN PROGRAM
// // =============================================================

// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         fprintf(stderr, "Usage: %s <SERVER_IP>\n", argv[0]);
//         return 1;
//     }

//     struct sockaddr_in serv_addr;
//     g_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
//     memset(&serv_addr, 0, sizeof(serv_addr));
//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_port = htons(PORT);
//     inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
    
//     if (connect(g_socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
//         perror(RED "Connect failed" RESET);
//         return 1;
//     }
    
//     wait_for_response(); // Hello message

//     while(1) {
//         print_header("HE THONG DAU GIA TRUC TUYEN");
        
//         if(!g_is_logged_in) {
//             printf(BOLD " [1]" RESET " Dang nhap\n");
//             printf(BOLD " [2]" RESET " Dang ky\n");
//             printf(BOLD " [0]" RESET " Thoat\n");
//         } else {
//             printf(BOLD " --- CHUC NANG CHINH ---\n" RESET);
//             printf(BOLD " [1]" RESET " Danh sach phong dau gia\n");
//             printf(BOLD " [2]" RESET " Tim kiem vat pham\n");
//             printf(BOLD " [3]" RESET " Lich su dau gia\n");
            
//             if(g_user_role == 1) {
//                 print_divider('-', 30);
//                 printf(BOLD " [4]" RESET " Quan ly nguoi dung (Admin)\n");
//             }
            
//             print_divider('=', APP_WIDTH);
            
//             if(g_current_room_id > 0) {
//                 printf(YELLOW " Ban dang trong phong #%d\n" RESET, g_current_room_id);
//                 printf(BOLD " [5]" RESET " Chi tiet/Thao tac Phong\n");
//                 printf(BOLD " [6]" RESET " Roi phong\n");
//             }
            
//             printf(BOLD " [9]" RESET " Dang xuat\n");
//             printf(BOLD " [0]" RESET " Thoat\n");
//         }
        
//         int choice = get_input_int("\n >> Chon chuc nang: ");
        
//         if(!g_is_logged_in) {
//             switch(choice) {
//                 case 1: handle_login(); break;
//                 case 2: handle_register(); break;
//                 case 0: close(g_socket_fd); return 0;
//                 default: print_message("ERROR", "Lua chon khong hop le"); wait_enter();
//             }
//         } else {
//             switch(choice) {
//                 case 1: 
//                     handle_room_list(); 
//                     printf(CYAN "\n [1] Tao phong moi  [2] Vao phong  [0] Quay lai\n" RESET);
//                     int sub = get_input_int(" >> ");
//                     if(sub == 1) handle_create_room();
//                     else if(sub == 2) handle_join_room();
//                     break;
//                 case 2: handle_search_items(); break;
//                 case 3: handle_auction_history(); break;
//                 case 4: if(g_user_role==1) handle_admin_user_list(); break;
//                 case 5: if(g_current_room_id > 0) handle_room_detail(); break;
//                 case 6: 
//                     if(g_current_room_id > 0) {
//                         send_command("LEAVE_ROOM");
//                         wait_for_response();
//                         g_current_room_id = 0;
//                         print_message("SUCCESS", "Da roi phong."); wait_enter();
//                     }
//                     break;
//                 case 9: 
//                     send_command("LOGOUT");
//                     g_is_logged_in = 0; g_user_role = 0; g_current_room_id = 0;
//                     print_message("INFO", "Da dang xuat."); wait_enter();
//                     break;
//                 case 0: close(g_socket_fd); return 0;
//                 default: print_message("ERROR", "Chuc nang khong ton tai"); wait_enter();
//             }
//         }
//     }
//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>

// =============================================================
// 1. CẤU HÌNH & HẰNG SỐ [cite: 5, 6, 7]
// =============================================================
#define PORT 8080
#define BUFFER_SIZE 4096
#define APP_WIDTH 100

// ANSI Colors for UI
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"
#define CLEAR   "\033[2J\033[H"

// Global State
int g_socket_fd;
char g_socket_buffer[BUFFER_SIZE];
int g_is_logged_in = 0;
char g_username[50] = "";
int g_user_role = 0; // 0 = user, 1 = admin [cite: 279]
int g_current_room_id = 0;

// =============================================================
// 2. UI UTILITIES
// =============================================================

void clear_screen() { printf(CLEAR); }

void print_divider(char ch, int length) {
    printf(CYAN);
    for (int i = 0; i < length; i++) printf("%c", ch);
    printf(RESET "\n");
}

void print_header(const char* title) {
    clear_screen();
    print_divider('=', APP_WIDTH);
    int padding = (APP_WIDTH - strlen(title)) / 2;
    printf(CYAN BOLD "%*s%s%*s" RESET "\n", padding, "", title, padding, "");
    print_divider('=', APP_WIDTH);
    
    if (g_is_logged_in) {
        printf(GREEN " User: %-15s" RESET, g_username);
        if (g_user_role == 1) printf(YELLOW " [ADMIN]" RESET);
        if (g_current_room_id > 0) printf(BLUE " | Room: #%d" RESET, g_current_room_id);
        printf("\n");
        print_divider('-', APP_WIDTH);
    }
}

void print_message(const char* type, const char* msg) {
    if (strcmp(type, "SUCCESS") == 0) printf(GREEN " [OK] %s" RESET "\n", msg);
    else if (strcmp(type, "ERROR") == 0) printf(RED " [ERR] %s" RESET "\n", msg);
    else if (strcmp(type, "INFO") == 0) printf(BLUE " [INFO] %s" RESET "\n", msg);
    else if (strcmp(type, "WARN") == 0) printf(YELLOW " [WARN] %s" RESET "\n", msg);
    else printf(" %s\n", msg);
}

// Input Helpers
void get_input_string(const char* prompt, char* buffer, int size) {
    printf(BOLD "%s" RESET, prompt);
    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
    } else buffer[0] = '\0';
}

int get_input_int(const char* prompt) {
    char buffer[100];
    get_input_string(prompt, buffer, sizeof(buffer));
    return atoi(buffer);
}

double get_input_double(const char* prompt) {
    char buffer[100];
    get_input_string(prompt, buffer, sizeof(buffer));
    return atof(buffer);
}

void wait_enter() {
    printf("\n" CYAN ">> Nhan Enter de tiep tuc..." RESET);
    getchar();
}

void send_command(const char* cmd) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%s\n", cmd); // Thêm \n để Server nhận diện dòng lệnh
    if (send(g_socket_fd, buffer, strlen(buffer), 0) < 0) perror("Gui lenh that bai");
}

// Chỉ dùng cho các thao tác ĐỒNG BỘ (Ngoài phòng đấu giá)
char* wait_for_response() {
    static char response[BUFFER_SIZE];
    fd_set read_fds;
    struct timeval timeout;
    
    timeout.tv_sec = 3; timeout.tv_usec = 0;
    FD_ZERO(&read_fds);
    FD_SET(g_socket_fd, &read_fds);
    
    int activity = select(g_socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (activity > 0 && FD_ISSET(g_socket_fd, &read_fds)) {
        int bytes = recv(g_socket_fd, g_socket_buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) return NULL;
        
        g_socket_buffer[bytes] = '\0';
        // Chỉ lấy dòng đầu tiên nếu dùng hàm sync này
        char* newline = strchr(g_socket_buffer, '\n');
        if (newline) *newline = '\0';
        strncpy(response, g_socket_buffer, BUFFER_SIZE);
        return response;
    }
    return NULL;
}

void render_room_info(char* response) {

    char* ptr = strchr(response, '|');
    if (!ptr) return;
    ptr++; 

    char r_id[10]="", r_name[100]="", r_stat[20]="", r_start[30]="", r_end[30]="";
    char temp_resp[BUFFER_SIZE];
    strncpy(temp_resp, ptr, sizeof(temp_resp));
    
    char* token = strtok(temp_resp, "|"); if(token) strcpy(r_id, token);
    token = strtok(NULL, "|"); if(token) strcpy(r_name, token);
    token = strtok(NULL, "|"); if(token) strcpy(r_stat, token);
    token = strtok(NULL, "|"); if(token) strcpy(r_start, token);
    token = strtok(NULL, "|"); if(token) strcpy(r_end, token);

    clear_screen();
    print_header("DAU GIA TRUC TUYEN (LIVE)");
    printf(BOLD " Phong: %s (#%s)\n" RESET, r_name, r_id);
    
    char stat_color[10] = RESET;
    if(strcmp(r_stat,"ACTIVE")==0) strcpy(stat_color, GREEN);
    else strcpy(stat_color, RED);
    printf(" Trang thai: %s%s" RESET " | %s -> %s\n", stat_color, r_stat, r_start, r_end);
    print_divider('-', APP_WIDTH);

    printf(BOLD " %-4s %-25s %-10s %-12s %-12s %-12s" RESET "\n", 
           "ID", "VAT PHAM", "STATUS", "KHOI DIEM", "HIEN TAI", "MUA NGAY");
    print_divider('.', APP_WIDTH);

    int pipes = 0; char* item_start = ptr;
    while(*item_start) {
        if(*item_start == '|') pipes++;
        if(pipes == 5) { item_start++; break; }
        item_start++;
    }

    int count = 0;
    if (item_start && strlen(item_start) > 0) {
        char items_buf[BUFFER_SIZE];
        strncpy(items_buf, item_start, sizeof(items_buf));
        char* item = strtok(items_buf, ";");
        while (item) {
            int iid; char iname[100], istat[20]; double p1, p2, p3;
            if(sscanf(item, "%d|%[^|]|%[^|]|%lf|%lf|%lf", &iid, iname, istat, &p1, &p2, &p3) >= 6) {
                char c[10]=RESET;
                if(strcmp(istat,"ACTIVE")==0) strcpy(c, GREEN);
                else if(strcmp(istat,"SOLD")==0) strcpy(c, RED);
                else strcpy(c, YELLOW); 

                printf(" %-4d %-25.25s %s%-10s" RESET " %-12.0f %-12.0f %-12.0f\n", 
                       iid, iname, c, istat, p1, p2, p3);
                count++;
            }
            item = strtok(NULL, ";");
        }
    } 
    if (count == 0) printf(YELLOW " >> Phong chua co vat pham nao.\n" RESET);
    
    print_divider('=', APP_WIDTH);
    printf(BOLD " [4] Dat gia    [5] Mua ngay    [6] Roi phong\n" RESET);
    printf(BOLD " [7] Tao Vat Pham (Owner Only)  [0] Ve Menu chinh\n" RESET);
    printf(CYAN " >> Nhap lua chon: " RESET);
    fflush(stdout);
}


void process_server_msg(char* msg, char* refresh_cmd) {

    msg[strcspn(msg, "\r")] = 0;
    if (strlen(msg) == 0) return;

    if (strncmp(msg, "ROOM_DETAIL", 11) == 0) {
        render_room_info(msg);
    }

    else if (strncmp(msg, "BID_SUCCESS", 11) == 0) {
        printf(GREEN "\n [SUCCESS] Dat gia thanh cong! Dang cap nhat...\n" RESET);
        if(refresh_cmd) send_command(refresh_cmd);
    }
    else if (strncmp(msg, "BUY_NOW_SUCCESS", 15) == 0) {
        printf(GREEN "\n [SUCCESS] Mua ngay thanh cong! Dang cap nhat...\n" RESET);
        if(refresh_cmd) send_command(refresh_cmd);
    }
   else if (strncmp(msg, "CREATE_ITEM_SUCCESS", 19) == 0) { 
        printf(GREEN "\n [SUCCESS] Tao vat pham thanh cong!\n" RESET);
        if(refresh_cmd) send_command(refresh_cmd);
    }
    else if (strncmp(msg, "LEAVE_ROOM_SUCCESS", 18) == 0) {
        printf(GREEN "\n [SUCCESS] Da roi phong.\n" RESET);
    }
    else if (strncmp(msg, "AUCTION_WARNING", 15) == 0) {
        int id, sec; char name[50];
        char temp[512]; strcpy(temp, msg);
        char* t = strtok(temp, "|"); t = strtok(NULL, "|"); id = atoi(t);
        t = strtok(NULL, "|"); strcpy(name, t);
        t = strtok(NULL, "|"); sec = atoi(t);
        printf(RED BOLD "\n [CANH BAO] '%s' (#%d) sap het gio: CON %d GIAY!\n" RESET, name, id, sec);
    }
    else if (strncmp(msg, "ITEM_SOLD", 9) == 0) {
        printf(MAGENTA BOLD "\n [KET THUC] %s\n" RESET, strchr(msg, '|') + 1);
        if(refresh_cmd) send_command(refresh_cmd);
    }
    else if (strncmp(msg, "NEW_BID", 7) == 0) {
        char* content = msg + 7; 
        if(*content == '|' || *content == ' ') content++;
        printf(YELLOW "\n [GIA MOI] %s\n" RESET, content);
        if(refresh_cmd) send_command(refresh_cmd); 
    }
   else if (strncmp(msg, "TIME_EXTENDED", 13) == 0) { 
        printf(RED "\n [GIA HAN] Co dau gia phut chot! Thoi gian duoc cong them.\n" RESET);
        if(refresh_cmd) send_command(refresh_cmd);
    }
    else if (strncmp(msg, "ERROR", 5) == 0 || strstr(msg, "FAIL")) {
        printf(RED "\n %s\n" RESET, msg);
    }
    else if (strncmp(msg, "USER_JOINED", 11) == 0 || strncmp(msg, "USER_LEFT", 9) == 0) {
         printf(BLUE "\n [INFO] %s\n" RESET, strchr(msg, '|') + 1);
    }
    
    fflush(stdout); 
   
}

void handle_room_detail() {
    if (g_current_room_id == 0) return;

    char cmd[50]; snprintf(cmd, sizeof(cmd), "GET_ROOM_DETAIL|%d", g_current_room_id);
    send_command(cmd);

    fd_set read_fds;
    int max_fd, in_room = 1;

    while (in_room) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); 
        FD_SET(g_socket_fd, &read_fds); 
        max_fd = (g_socket_fd > STDIN_FILENO) ? g_socket_fd : STDIN_FILENO;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(g_socket_fd, &read_fds)) {
            char buf[BUFFER_SIZE];
            int bytes = recv(g_socket_fd, buf, sizeof(buf) - 1, 0);
            if (bytes <= 0) { print_message("ERROR", "Mat ket noi server!"); break; }
            buf[bytes] = '\0';

            char* line = strtok(buf, "\n");
            while (line != NULL) {
                // Kiểm tra lệnh rời phòng đặc biệt để thoát vòng lặp
                if (strncmp(line, "LEAVE_ROOM_SUCCESS", 18) == 0) {
                    printf(GREEN "\n [SUCCESS] Da roi phong.\n" RESET);
                    g_current_room_id = 0; 
                    in_room = 0;
                    wait_enter();
                } else {
                    // Xử lý các tin nhắn khác
                    process_server_msg(line, cmd);
                }
                line = strtok(NULL, "\n");
            }
            
            if (in_room) {
                fflush(stdout);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds) && in_room) {
            char input[100];
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = 0;
                
                if (strcmp(input, "0") == 0) in_room = 0;
                else if (strcmp(input, "4") == 0) { 
                    int id = get_input_int(" [BID] ID vat pham: ");
                    double val = get_input_double(" [BID] So tien: ");
                    char bcmd[100]; snprintf(bcmd, sizeof(bcmd), "PLACE_BID|%d|%.0f", id, val);
                    send_command(bcmd);
                }
                else if (strcmp(input, "5") == 0) { 
                    int id = get_input_int(" [BUY] ID vat pham: ");
                    char bcmd[100]; snprintf(bcmd, sizeof(bcmd), "BUY_NOW|%d", id);
                    send_command(bcmd);
                }
               else if (strcmp(input, "6") == 0) { 
                    send_command("LEAVE_ROOM");
                }
               else if (strcmp(input, "7") == 0) { 
                    printf(YELLOW "\n [OWNER] Tao vat pham moi:\n" RESET);
                    char name[100]; get_input_string(" Ten: ", name, sizeof(name));
                    double sp = get_input_double(" Gia khoi diem: ");
                    double bp = get_input_double(" Gia mua ngay (0=bo qua): ");
                    int dur = get_input_int(" Thoi gian (phut): ");
                    char ccmd[256]; snprintf(ccmd, sizeof(ccmd), "CREATE_ITEM|%d|%s|%.0f|%d|%.0f", g_current_room_id, name, sp, dur, bp);
                    send_command(ccmd);
                }
                else {
                    send_command(cmd); 
                }
            }
        }
    }
}

 
void handle_register() {
    print_header("DANG KY");
    char u[50], p[50];
    get_input_string(" Username: ", u, sizeof(u));
    get_input_string(" Password: ", p, sizeof(p));
    char cmd[200]; snprintf(cmd, sizeof(cmd), "REGISTER|%s|%s", u, p);
    send_command(cmd);
    char* res = wait_for_response();
    if(res && strncmp(res, "REGISTER_SUCCESS", 16)==0) print_message("SUCCESS", "Dang ky thanh cong!");
    else print_message("ERROR", res?res:"Loi");
    wait_enter();
}


void handle_login() {
    print_header("DANG NHAP");
    char u[50], p[50];
    get_input_string(" Username: ", u, sizeof(u));
    get_input_string(" Password: ", p, sizeof(p));
    char cmd[200]; snprintf(cmd, sizeof(cmd), "LOGIN|%s|%s", u, p);
    send_command(cmd);
    char* res = wait_for_response();
    if(res && strncmp(res, "LOGIN_SUCCESS", 13)==0) {
        char* t = strtok(res, "|"); t = strtok(NULL, "|");
        t = strtok(NULL, "|"); if(t) strcpy(g_username, t);
        t = strtok(NULL, "|"); if(t) g_user_role = atoi(t);
        g_is_logged_in = 1;
        print_message("SUCCESS", "Dang nhap thanh cong!");
    } else print_message("ERROR", res?res:"Sai thong tin");
    wait_enter();
}


void handle_admin_user_list() {
    print_header("DANH SACH USER (ADMIN)");
    send_command("GET_USER_LIST");
    char* res = wait_for_response();
    if(res && strncmp(res, "USER_LIST", 9)==0) {
        char* ptr = strchr(res, '|') + 1;
        printf(BOLD " %-5s %-20s %-10s %-10s" RESET "\n", "ID", "USER", "STATUS", "ROLE");
        print_divider('-', 60);
        char* u = strtok(ptr, ";");
        while(u) {
            char id[10], name[50], st[5], role[5];
            sscanf(u, "%[^|]|%[^|]|%[^|]|%s", id, name, st, role);
            printf(" %-5s %-20s %s%-10s" RESET " %s\n", id, name, 
                   strcmp(st,"1")==0?GREEN:RED, strcmp(st,"1")==0?"Online":"Offline",
                   strcmp(role,"1")==0?YELLOW"ADMIN":RESET"User");
            u = strtok(NULL, ";");
        }
    } else print_message("ERROR", res?res:"Loi");
    wait_enter();
}


void handle_room_list() {
    print_header("DANH SACH PHONG");
    send_command("GET_ROOM_LIST|ALL|1|50");
    char* res = wait_for_response();
    if(res && strncmp(res, "ROOM_LIST", 9)==0) {
        char* ptr = strchr(res, '|') + 1;
        int count = atoi(ptr);
        printf(GREEN " Tim thay %d phong.\n" RESET, count);
        print_divider('-', APP_WIDTH);
        printf(BOLD " %-4s %-25s %-15s %-10s %-10s" RESET "\n", "ID", "TEN PHONG", "OWNER", "STATUS", "ITEMS");
        print_divider('.', APP_WIDTH);
        ptr = strchr(ptr, '|');
        if(ptr) {
            ptr++;
            char* r = strtok(ptr, ";");
            while(r) {
                int id, ic, pc; char name[50], own[50], st[20], t[50];
                sscanf(r, "%d|%[^|]|%[^|]|%[^|]|%d|%d|%s", &id, name, own, st, &ic, &pc, t);
                printf(" %-4d %-25.25s %-15.15s %-10s %-10d\n", id, name, own, st, ic);
                r = strtok(NULL, ";");
            }
        }
    } else print_message("ERROR", "Khong tai duoc du lieu");
    wait_enter();
}


void handle_create_room() {
    print_header("TAO PHONG");
    char name[100], start[30], end[30];
    get_input_string(" Ten phong: ", name, sizeof(name));
    get_input_string(" Bat dau (YYYY-MM-DD HH:MM:SS): ", start, sizeof(start));
    get_input_string(" Ket thuc (YYYY-MM-DD HH:MM:SS): ", end, sizeof(end));
    char cmd[512]; snprintf(cmd, sizeof(cmd), "CREATE_ROOM|%s|%s|%s", name, start, end);
    send_command(cmd);
    char* res = wait_for_response();
    if(res && strncmp(res, "CREATE_ROOM_SUCCESS", 19)==0) print_message("SUCCESS", "Tao phong thanh cong!");
    else print_message("ERROR", res?res:"Loi");
    wait_enter();
}


void handle_search_items() {
    print_header("TIM KIEM");
    printf(" [1] Ten  [2] Thoi gian  [3] Ca hai\n");
    int c = get_input_int(" >> Chon: ");
    char type[10]="NAME", kw[100]="", t1[30]="", t2[30]="";
    if(c==1 || c==3) get_input_string(" Keyword: ", kw, sizeof(kw));
    if(c==2 || c==3) {
        strcpy(type, c==2?"TIME":"BOTH");
        get_input_string(" Tu (YYYY-MM-DD HH:MM:SS): ", t1, sizeof(t1));
        get_input_string(" Den: ", t2, sizeof(t2));
    }
    char cmd[512]; snprintf(cmd, sizeof(cmd), "SEARCH_ITEMS|%s|%s|%s|%s", type, kw, t1, t2);
    send_command(cmd);
    char* res = wait_for_response();
    if(res && strncmp(res, "SEARCH_RESULT", 13)==0) {
        char* ptr = strchr(res, '|') + 1;
        printf(GREEN " Ket qua: %d item(s)\n" RESET, atoi(ptr));
        print_divider('.', APP_WIDTH);
        ptr = strchr(ptr, '|') + 1;
        char* it = strtok(ptr, ";");
        while(it) {
            int id, rid; char rn[50], in[50], st[20], dum[20]; double p1, p2;
            sscanf(it, "%d|%d|%[^|]|%[^|]|%lf|%lf|%s", &id, &rid, rn, in, &p1, &p2, st);
            printf(" #%d (%s) - %s: %.0f (Phong: %s)\n", id, st, in, p2, rn);
            it = strtok(NULL, ";");
        }
    } else print_message("ERROR", res?res:"Loi tim kiem");
    wait_enter();
}


void handle_history() {
    print_header("LICH SU DAU GIA");
    char cmd[100]; snprintf(cmd, sizeof(cmd), "GET_MY_AUCTION_HISTORY|ALL|1|20");
    send_command(cmd);
    char* res = wait_for_response();
    if(res && strncmp(res, "AUCTION_HISTORY", 15)==0) {
        char* ptr = strchr(res, '|') + 1;
        printf(GREEN " Tong: %d phien tham gia\n" RESET, atoi(ptr));
        print_divider('.', APP_WIDTH);
        ptr = strchr(ptr, '|') + 1; // Data
        if(ptr && strlen(ptr)>0) {
            char* row = strtok(ptr, ";");
            while(row) {
                // Giả sử server trả về: itemId|itemName|myBid|status...
                printf(" - %s\n", row); // In raw cho đơn giản vì format dài
                row = strtok(NULL, ";");
            }
        }
    } else print_message("ERROR", res?res:"Khong co du lieu");
    wait_enter();
}

void handle_join_room_wrapper() {
    int id = get_input_int(" Nhap ID phong: ");
    char cmd[50]; snprintf(cmd, sizeof(cmd), "JOIN_ROOM|%d", id);
    send_command(cmd);
    char* res = wait_for_response();
    if(res && strncmp(res, "JOIN_ROOM_SUCCESS", 17)==0) {
        g_current_room_id = id;
        print_message("SUCCESS", "Vao phong thanh cong!");
        sleep(1);
        handle_room_detail(); 
    } else {
        print_message("ERROR", res?res:"Loi vao phong");
        wait_enter();
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) { fprintf(stderr, "Usage: %s <IP_SERVER>\n", argv[0]); return 1; }

    struct sockaddr_in serv_addr;
    g_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    if (connect(g_socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connect failed"); return 1;
    }
    
    wait_for_response(); // Greeting

    while(1) {
        print_header("HE THONG DAU GIA");
        if(!g_is_logged_in) {
            printf(" [1] Dang nhap\n [2] Dang ky\n [0] Thoat\n");
            int c = get_input_int("\n >> Chon: ");
            switch(c) {
                case 1: handle_login(); break;
                case 2: handle_register(); break;
                case 0: close(g_socket_fd); return 0;
            }
        } else {
            printf(" [1] Danh sach phong      [2] Tim kiem vat pham\n");
            printf(" [3] Tao phong dau gia    [4] Vao phong (ID)\n");
            printf(" [5] Lich su dau gia\n");
            if(g_user_role == 1) printf(YELLOW " [8] Quan ly User (Admin)\n" RESET);
            if(g_current_room_id > 0) printf(GREEN " [9] Vao lai phong #%d (LIVE)\n" RESET, g_current_room_id);
            printf(" [0] Dang xuat\n");
            
            int c = get_input_int("\n >> Chon: ");
            switch(c) {
                case 1: handle_room_list(); break;
                case 2: handle_search_items(); break;
                case 3: handle_create_room(); break;
                case 4: handle_join_room_wrapper(); break;
                case 5: handle_history(); break;
                case 8: if(g_user_role==1) handle_admin_user_list(); break;
                case 9: if(g_current_room_id > 0) handle_room_detail(); break;
                case 0: 
                    send_command("LOGOUT"); g_is_logged_in=0; g_current_room_id=0; g_user_role=0;
                    print_message("INFO", "Da dang xuat"); wait_enter();
                    break;
            }
        }
    }
    return 0;
}