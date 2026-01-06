// /*
//  * room_handler.c
//  * Triển khai các chức năng quản lý phòng đấu giá
//  */

// #include "room_handler.h"
// #include "client_handler.h"
// #include <time.h>

// const char* ROOMS_FILE = "data/rooms.txt";
// const char* ITEMS_FILE = "data/items.txt";

// // Lấy ID phòng lớn nhất
// int get_last_room_id() {
//     FILE* file = fopen(ROOMS_FILE, "r");
//     if (file == NULL) return 0;
    
//     char line[512];
//     int max_id = 0;
    
//     while (fgets(line, sizeof(line), file)) {
//         line[strcspn(line, "\n")] = 0;
//         if (strlen(line) == 0) continue;
        
//         int room_id;
//         if (sscanf(line, "%d|", &room_id) == 1) {
//             if (room_id > max_id) max_id = room_id;
//         }
//     }
//     fclose(file);
//     return max_id;
// }

// // Đếm số vật phẩm trong phòng
// int count_items_in_room(int room_id) {
//     FILE* file = fopen(ITEMS_FILE, "r");
//     if (file == NULL) return 0;
    
//     char line[1024];
//     int count = 0;
    
//     while (fgets(line, sizeof(line), file)) {
//         int item_room_id;
//         if (sscanf(line, "%*d|%d|", &item_room_id) == 1) {
//             if (item_room_id == room_id) count++;
//         }
//     }
//     fclose(file);
//     return count;
// }

// // Đếm số người tham gia phòng
// int count_participants_in_room(int room_id) {
//     int count = 0;
//     for (int i = 0; i < MAX_CLIENTS; i++) {
//         if (g_clients[i] && g_clients[i]->current_room_id == room_id) {
//             count++;
//         }
//     }
//     return count;
// }

// // Kiểm tra user có phải owner không
// int is_room_owner(int user_id, int room_id) {
//     FILE* file = fopen(ROOMS_FILE, "r");
//     if (file == NULL) return 0;
    
//     char line[512];
//     while (fgets(line, sizeof(line), file)) {
//         int rid, owner_id;
//         if (sscanf(line, "%d|%*[^|]|%d|", &rid, &owner_id) == 2) {
//             if (rid == room_id && owner_id == user_id) {
//                 fclose(file);
//                 return 1;
//             }
//         }
//     }
//     fclose(file);
//     return 0;
// }

// // Lấy thông tin phòng theo ID
// Room* get_room_by_id(int room_id) {
//     FILE* file = fopen(ROOMS_FILE, "r");
//     if (file == NULL) return NULL;
    
//     static Room room;
//     char line[512];
    
//     while (fgets(line, sizeof(line), file)) {
//         line[strcspn(line, "\n")] = 0;
        
//         char room_name[100], status[20], start_time[30], end_time[30];
//         int rid, owner_id;
        
//         if (sscanf(line, "%d|%99[^|]|%d|%19[^|]|%29[^|]|%29[^|]", 
//                    &rid, room_name, &owner_id, status, start_time, end_time) == 6) {
//             if (rid == room_id) {
//                 room.room_id = rid;
//                 strncpy(room.room_name, room_name, sizeof(room.room_name));
//                 room.owner_id = owner_id;
//                 strncpy(room.status, status, sizeof(room.status));
//                 strncpy(room.start_time, start_time, sizeof(room.start_time));
//                 strncpy(room.end_time, end_time, sizeof(room.end_time));
//                 fclose(file);
//                 return &room;
//             }
//         }
//     }
//     fclose(file);
//     return NULL;
// }

// // Gửi tin nhắn broadcast đến tất cả user trong phòng
// void broadcast_to_room(int room_id, const char* message, int exclude_fd) {
//     for (int i = 0; i < MAX_CLIENTS; i++) {
//         if (g_clients[i] && 
//             g_clients[i]->current_room_id == room_id &&
//             g_clients[i]->socket_fd != exclude_fd) {
//             send_message(g_clients[i], message);
//         }
//     }
// }

// // Tạo phòng đấu giá
// void handle_create_room(Client* client, char* room_name, char* start_time, char* end_time) {
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }
    
//     if (!room_name || strlen(room_name) == 0) {
//         send_message(client, "CREATE_ROOM_FAIL|Ten phong khong duoc rong");
//         return;
//     }
    
//     if (!start_time || !end_time) {
//         send_message(client, "CREATE_ROOM_FAIL|Thoi gian khong hop le");
//         return;
//     }
    
//     // TODO: Validate định dạng thời gian và start_time < end_time
    
//     int new_room_id = get_last_room_id() + 1;
    
//     FILE* file = fopen(ROOMS_FILE, "a");
//     if (file == NULL) {
//         send_message(client, "CREATE_ROOM_FAIL|Loi mo file");
//         return;
//     }
    
//     // Format: room_id|room_name|owner_id|status|start_time|end_time
//     fprintf(file, "%d|%s|%d|%s|%s|%s\n", 
//             new_room_id, room_name, client->user_id, 
//             ROOM_STATUS_ACTIVE, start_time, end_time);
//     fclose(file);
    
//     char response[256];
//     snprintf(response, sizeof(response), 
//              "CREATE_ROOM_SUCCESS|Tao phong thanh cong|%d", new_room_id);
//     send_message(client, response);
// }

// // Lấy danh sách phòng với phân trang
// void handle_get_room_list(Client* client, char* status_filter, char* page_str, char* limit_str) {
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }
    
//     int page = atoi(page_str ? page_str : "1");
//     int limit = atoi(limit_str ? limit_str : "10");
    
//     if (page < 1 || limit < 1) {
//         send_message(client, "ERROR|Tham so khong hop le");
//         return;
//     }
    
//     FILE* file = fopen(ROOMS_FILE, "r");
//     if (file == NULL) {
//         send_message(client, "ROOM_LIST|0|");
//         return;
//     }
    
//     char line[512];
//     char result[BUFFER_SIZE] = "ROOM_LIST|";
//     int total_count = 0;
//     int current_index = 0;
//     int offset = (page - 1) * limit;
//     char temp_results[BUFFER_SIZE] = "";
    
//     while (fgets(line, sizeof(line), file)) {
//         line[strcspn(line, "\n")] = 0;
//         if (strlen(line) == 0) continue;
        
//         int room_id, owner_id;
//         char room_name[100], status[20], start_time[30], end_time[30];
        
//         if (sscanf(line, "%d|%99[^|]|%d|%19[^|]|%29[^|]|%29[^|]", 
//                    &room_id, room_name, &owner_id, status, start_time, end_time) == 6) {
            
//             // Lọc theo status
//             if (status_filter && strcmp(status_filter, "ALL") != 0) {
//                 if (strcmp(status, status_filter) != 0) continue;
//             }
            
//             total_count++;
            
//             // Phân trang
//             if (current_index >= offset && current_index < offset + limit) {
//                 int item_count = count_items_in_room(room_id);
//                 int participant_count = count_participants_in_room(room_id);
                
//                 // Lấy tên owner
//                 char owner_name[MAX_USERNAME] = "Unknown";
//                 FILE* user_file = fopen(DB_FILE, "r");
//                 if (user_file) {
//                     char user_line[256];
//                     while (fgets(user_line, sizeof(user_line), user_file)) {
//                         int uid;
//                         char uname[MAX_USERNAME];
//                         if (sscanf(user_line, "%d|%49[^|]|", &uid, uname) == 2) {
//                             if (uid == owner_id) {
//                                 strncpy(owner_name, uname, sizeof(owner_name));
//                                 break;
//                             }
//                         }
//                     }
//                     fclose(user_file);
//                 }
                
//                 char room_data[512];
//                 snprintf(room_data, sizeof(room_data), 
//                          "%d|%s|%s|%s|%d|%d|%s|%s;",
//                          room_id, room_name, owner_name, status, 
//                          item_count, participant_count, start_time, end_time);
//                 strncat(temp_results, room_data, sizeof(temp_results) - strlen(temp_results) - 1);
//             }
//             current_index++;
//         }
//     }
//     fclose(file);
    
//     char count_str[20];
//     snprintf(count_str, sizeof(count_str), "%d|", total_count);
//     strncat(result, count_str, sizeof(result) - strlen(result) - 1);
//     strncat(result, temp_results, sizeof(result) - strlen(result) - 1);
    
//     send_message(client, result);
// }

// // Tham gia phòng
// void handle_join_room(Client* client, char* room_id_str) {
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }
    
//     // Kiểm tra đã ở phòng khác chưa
//     if (client->current_room_id != 0) {
//         send_message(client, "JOIN_ROOM_FAIL|Ban dang o phong khac. Moi nguoi chi duoc tham gia 1 phong");
//         return;
//     }
    
//     int room_id = atoi(room_id_str);
//     Room* room = get_room_by_id(room_id);
    
//     if (room == NULL) {
//         send_message(client, "JOIN_ROOM_FAIL|Phong khong ton tai");
//         return;
//     }
    
//     if (strcmp(room->status, ROOM_STATUS_ACTIVE) != 0) {
//         send_message(client, "JOIN_ROOM_FAIL|Phong chua bat dau");
//         return;
//     }
    
//     if (room->owner_id == client->user_id) {
//         send_message(client, "JOIN_ROOM_FAIL|Owner tu dong trong phong");
//         return;
//     }
    
//     // Tham gia phòng
//     client->current_room_id = room_id;
    
//     char response[256];
//     snprintf(response, sizeof(response), 
//              "JOIN_ROOM_SUCCESS|Da tham gia phong|%d|%s", 
//              room_id, room->room_name);
//     send_message(client, response);
    
//     // Broadcast cho những người khác
//     char broadcast_msg[256];
//     snprintf(broadcast_msg, sizeof(broadcast_msg), 
//              "USER_JOINED|%s|vua tham gia phong", client->username);
//     broadcast_to_room(room_id, broadcast_msg, client->socket_fd);
// }

// // Rời phòng
// void handle_leave_room(Client* client) {
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }
    
//     if (client->current_room_id == 0) {
//         send_message(client, "ERROR|Ban khong o trong phong nao");
//         return;
//     }
    
//     int room_id = client->current_room_id;
//     client->current_room_id = 0;
    
//     send_message(client, "LEAVE_ROOM_SUCCESS|Da roi phong");
    
//     // Broadcast cho những người còn lại
//     char broadcast_msg[256];
//     snprintf(broadcast_msg, sizeof(broadcast_msg), 
//              "USER_LEFT|%s|da roi phong", client->username);
//     broadcast_to_room(room_id, broadcast_msg, -1);
// }

// // Xem chi tiết phòng và danh sách vật phẩm
// void handle_get_room_detail(Client* client, char* room_id_str) {
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }
    
//     int room_id = atoi(room_id_str);
//     Room* room = get_room_by_id(room_id);
    
//     if (room == NULL) {
//         send_message(client, "ROOM_DETAIL_FAIL|Phong khong ton tai");
//         return;
//     }
    
//     char response[BUFFER_SIZE];
//     snprintf(response, sizeof(response), 
//              "ROOM_DETAIL|%d|%s|%s|%s|%s|",
//              room->room_id, room->room_name, room->status, 
//              room->start_time, room->end_time);
    
//     // Đọc danh sách items
//     FILE* file = fopen(ITEMS_FILE, "r");
//     if (file != NULL) {
//         char line[1024];
//         while (fgets(line, sizeof(line), file)) {
//             line[strcspn(line, "\n")] = 0;
            
//             int item_id, item_room_id;
//             char item_name[200], status[20];
//             double start_price, current_price, buy_now_price;
//             char auction_start[30], auction_end[30];
            
//             // Parse format: item_id|room_id|item_name|...|status|...|current_price|...
//             char* token = strtok(line, "|");
//             if (!token) continue;
//             item_id = atoi(token);
            
//             token = strtok(NULL, "|");
//             if (!token) continue;
//             item_room_id = atoi(token);
            
//             if (item_room_id == room_id) {
//                 token = strtok(NULL, "|"); // item_name
//                 strncpy(item_name, token ? token : "", sizeof(item_name));
                
//                 token = strtok(NULL, "|"); // description - skip
//                 token = strtok(NULL, "|"); // start_price
//                 start_price = token ? atof(token) : 0;
                
//                 token = strtok(NULL, "|"); // current_price
//                 current_price = token ? atof(token) : 0;
                
//                 token = strtok(NULL, "|"); // buy_now_price
//                 buy_now_price = token ? atof(token) : 0;
                
//                 token = strtok(NULL, "|"); // status
//                 strncpy(status, token ? token : "", sizeof(status));
                
//                 char item_data[512];
//                 snprintf(item_data, sizeof(item_data), 
//                          "%d|%s|%s|%.0f|%.0f|%.0f;",
//                          item_id, item_name, status, 
//                          start_price, current_price, buy_now_price);
//                 strncat(response, item_data, sizeof(response) - strlen(response) - 1);
//             }
//         }
//         fclose(file);
//     }
    
//     send_message(client, response);
// }

/*
 * room_handler.c
 * Triển khai các chức năng quản lý phòng đấu giá
 */

#include "room_handler.h"
#include "client_handler.h"
#include "timer_handler.h"
#include "item_handler.h"
#include <time.h>

const char* ROOMS_FILE = "data/rooms.txt";
const char* ITEMS_FILE = "data/items.txt";

// Lấy ID phòng lớn nhất
int get_last_room_id() {
    FILE* file = fopen(ROOMS_FILE, "r");
    if (file == NULL) return 0;
    
    char line[512];
    int max_id = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        
        int room_id;
        if (sscanf(line, "%d|", &room_id) == 1) {
            if (room_id > max_id) max_id = room_id;
        }
    }
    fclose(file);
    return max_id;
}

// Đếm số vật phẩm trong phòng
int count_items_in_room(int room_id) {
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) return 0;
    
    char line[1024];
    int count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        int item_room_id;
        if (sscanf(line, "%*d|%d|", &item_room_id) == 1) {
            if (item_room_id == room_id) count++;
        }
    }
    fclose(file);
    return count;
}

// Helper: Kiểm tra và tự động kích hoạt item đầu tiên nếu phòng ACTIVE mà chưa có item nào đang đấu giá
void auto_activate_first_item_if_needed(int room_id) {
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) return;
    
    char line[2048];
    int has_active_item = 0;
    int has_pending_item = 0;
    
    while (fgets(line, sizeof(line), file)) {
        int item_id, item_room_id;
        char status[20];
        if (sscanf(line, "%d|%d|%*[^|]|%*[^|]|%*[^|]|%*[^|]|%*[^|]|%19[^|]", 
                   &item_id, &item_room_id, status) >= 3) {
            if (item_room_id == room_id) {
                if (strcmp(status, ITEM_STATUS_ACTIVE) == 0) {
                    has_active_item = 1;
                    break;
                }
                if (strcmp(status, ITEM_STATUS_PENDING) == 0) {
                    has_pending_item = 1;
                }
            }
        }
    }
    fclose(file);
    
    // Nếu không có item active nhưng có item pending, kiểm tra scheduled_start trước khi kích hoạt
    if (!has_active_item && has_pending_item) {
        // Kiểm tra xem có item pending nào đã đến scheduled_start không
        file = fopen(ITEMS_FILE, "r");
        if (file == NULL) return;
        
        time_t now = time(NULL);
        int can_activate = 0;
        
        while (fgets(line, sizeof(line), file)) {
            int item_id, item_room_id;
            char status[20], sched_start[30];
            
            // Parse: item_id|room_id|name|desc|start|current|buynow|status|winner|final|auc_start|auc_end|extend|duration|created|sched_start|sched_end|bids
            char* ptr = line;
            char* tokens[18];
            int token_count = 0;
            
            // Tokenize by |
            while (*ptr && token_count < 18) {
                tokens[token_count++] = ptr;
                char* next = strchr(ptr, '|');
                if (next) {
                    *next = '\0';
                    ptr = next + 1;
                } else {
                    break;
                }
            }
            
            if (token_count >= 16) {
                item_id = atoi(tokens[0]);
                item_room_id = atoi(tokens[1]);
                strncpy(status, tokens[7], sizeof(status) - 1);
                strncpy(sched_start, tokens[15], sizeof(sched_start) - 1);
                
                if (item_room_id == room_id && strcmp(status, ITEM_STATUS_PENDING) == 0) {
                    // Nếu không có scheduled_start hoặc scheduled_start đã đến, có thể activate
                    if (strlen(sched_start) == 0 || strcmp(sched_start, "NULL") == 0) {
                        can_activate = 1;
                        break;
                    } else {
                        // Parse scheduled_start và so sánh với hiện tại
                        struct tm start_tm = {0};
                        if (strptime(sched_start, "%Y-%m-%d %H:%M:%S", &start_tm) != NULL) {
                            time_t start_time = mktime(&start_tm);
                            if (now >= start_time) {
                                can_activate = 1;
                                break;
                            }
                        }
                    }
                }
            }
        }
        fclose(file);
        
        // Chỉ activate nếu có item đủ điều kiện
        if (can_activate) {
            activate_next_item_in_room(room_id);
        }
    }
}

// Đếm số người tham gia phòng
int count_participants_in_room(int room_id) {
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i] && g_clients[i]->current_room_id == room_id) {
            count++;
        }
    }
    return count;
}

// Kiểm tra user có phải owner không
int is_room_owner(int user_id, int room_id) {
    FILE* file = fopen(ROOMS_FILE, "r");
    if (file == NULL) return 0;
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        int rid, owner_id;
        if (sscanf(line, "%d|%*[^|]|%d|", &rid, &owner_id) == 2) {
            if (rid == room_id && owner_id == user_id) {
                fclose(file);
                return 1;
            }
        }
    }
    fclose(file);
    return 0;
}

// Lấy thông tin phòng theo ID
Room* get_room_by_id(int room_id) {
    FILE* file = fopen(ROOMS_FILE, "r");
    if (file == NULL) return NULL;
    
    static Room room;
    char line[512];
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        char room_name[100], status[20], start_time[30], end_time[30];
        int rid, owner_id;
        
        if (sscanf(line, "%d|%99[^|]|%d|%19[^|]|%29[^|]|%29[^|]", 
                   &rid, room_name, &owner_id, status, start_time, end_time) == 6) {
            if (rid == room_id) {
                room.room_id = rid;
                strncpy(room.room_name, room_name, sizeof(room.room_name));
                room.owner_id = owner_id;
                strncpy(room.status, status, sizeof(room.status));
                strncpy(room.start_time, start_time, sizeof(room.start_time));
                strncpy(room.end_time, end_time, sizeof(room.end_time));
                fclose(file);
                return &room;
            }
        }
    }
    fclose(file);
    return NULL;
}

// Gửi tin nhắn broadcast đến tất cả user trong phòng
void broadcast_to_room(int room_id, const char* message, int exclude_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i] && 
            g_clients[i]->current_room_id == room_id &&
            g_clients[i]->socket_fd != exclude_fd) {
            send_message(g_clients[i], message);
        }
    }
}

// Tạo phòng đấu giá
void handle_create_room(Client* client, char* room_name, char* start_time, char* end_time) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    if (!room_name || strlen(room_name) == 0) {
        send_message(client, "CREATE_ROOM_FAIL|Ten phong khong duoc rong");
        return;
    }
    
    if (!start_time || !end_time) {
        send_message(client, "CREATE_ROOM_FAIL|Thoi gian khong hop le");
        return;
    }
    
    // TODO: Validate định dạng thời gian và start_time < end_time
    
    int new_room_id = get_last_room_id() + 1;
    
    FILE* file = fopen(ROOMS_FILE, "a");
    if (file == NULL) {
        send_message(client, "CREATE_ROOM_FAIL|Loi mo file");
        return;
    }
    
    // Format: room_id|room_name|owner_id|status|start_time|end_time
    fprintf(file, "%d|%s|%d|%s|%s|%s\n", 
            new_room_id, room_name, client->user_id, 
            ROOM_STATUS_PENDING, start_time, end_time);
    fclose(file);
    
    char response[256];
    snprintf(response, sizeof(response), 
             "CREATE_ROOM_SUCCESS|Tao phong thanh cong|%d", new_room_id);
    send_message(client, response);
}

// Lấy danh sách phòng với phân trang
void handle_get_room_list(Client* client, char* status_filter, char* page_str, char* limit_str) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    int page = atoi(page_str ? page_str : "1");
    int limit = atoi(limit_str ? limit_str : "10");
    
    if (page < 1 || limit < 1) {
        send_message(client, "ERROR|Tham so khong hop le");
        return;
    }
    
    FILE* file = fopen(ROOMS_FILE, "r");
    if (file == NULL) {
        send_message(client, "ROOM_LIST|0|");
        return;
    }
    
    char line[512];
    char result[BUFFER_SIZE] = "ROOM_LIST|";
    int total_count = 0;
    int current_index = 0;
    int offset = (page - 1) * limit;
    char temp_results[BUFFER_SIZE] = "";
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        
        int room_id, owner_id;
        char room_name[100], status[20], start_time[30], end_time[30];
        
        if (sscanf(line, "%d|%99[^|]|%d|%19[^|]|%29[^|]|%29[^|]", 
                   &room_id, room_name, &owner_id, status, start_time, end_time) == 6) {
            
            // Lọc theo status
            if (status_filter && strcmp(status_filter, "ALL") != 0) {
                if (strcmp(status, status_filter) != 0) continue;
            }
            
            total_count++;
            
            // Phân trang
            if (current_index >= offset && current_index < offset + limit) {
                int item_count = count_items_in_room(room_id);
                int participant_count = count_participants_in_room(room_id);
                
                // Lấy tên owner
                char owner_name[MAX_USERNAME] = "Unknown";
                FILE* user_file = fopen(DB_FILE, "r");
                if (user_file) {
                    char user_line[256];
                    while (fgets(user_line, sizeof(user_line), user_file)) {
                        int uid;
                        char uname[MAX_USERNAME];
                        if (sscanf(user_line, "%d|%49[^|]|", &uid, uname) == 2) {
                            if (uid == owner_id) {
                                strncpy(owner_name, uname, sizeof(owner_name));
                                break;
                            }
                        }
                    }
                    fclose(user_file);
                }
                
                char room_data[512];
                snprintf(room_data, sizeof(room_data), 
                         "%d|%s|%s|%s|%d|%d|%s|%s;",
                         room_id, room_name, owner_name, status, 
                         item_count, participant_count, start_time, end_time);
                strncat(temp_results, room_data, sizeof(temp_results) - strlen(temp_results) - 1);
            }
            current_index++;
        }
    }
    fclose(file);
    
    char count_str[20];
    snprintf(count_str, sizeof(count_str), "%d|", total_count);
    strncat(result, count_str, sizeof(result) - strlen(result) - 1);
    strncat(result, temp_results, sizeof(result) - strlen(result) - 1);
    
    send_message(client, result);
}

// Tham gia phòng
void handle_join_room(Client* client, char* room_id_str) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    // Kiểm tra đã ở phòng khác chưa
    if (client->current_room_id != 0) {
        send_message(client, "JOIN_ROOM_FAIL|Ban dang o phong khac. Moi nguoi chi duoc tham gia 1 phong");
        return;
    }
    
    int room_id = atoi(room_id_str);
    Room* room = get_room_by_id(room_id);
    
    if (room == NULL) {
        send_message(client, "JOIN_ROOM_FAIL|Phong khong ton tai");
        return;
    }
    
    // Kiểm tra trạng thái phòng - Owner được vào bất cứ lúc nào
    if (strcmp(room->status, ROOM_STATUS_ACTIVE) != 0 && room->owner_id != client->user_id) {
        send_message(client, "JOIN_ROOM_FAIL|Phong chua bat dau");
        return;
    }
    
    // Owner vào phòng của mình
    if (room->owner_id == client->user_id) {
        client->current_room_id = room_id;
        char response[256];
        snprintf(response, sizeof(response), 
                 "JOIN_ROOM_SUCCESS|Chao mung Chu phong|%d|%s", 
                 room_id, room->room_name);
        send_message(client, response);
        
        // Broadcast cho những người khác trong phòng
        char broadcast_msg[256];
        snprintf(broadcast_msg, sizeof(broadcast_msg), 
                 "USER_JOINED|%s|vua tham gia phong", client->username);
        broadcast_to_room(room_id, broadcast_msg, client->socket_fd);
        
        return;
    }
    
    // Tham gia phòng
    client->current_room_id = room_id;
    
    // Tự động kích hoạt item đầu tiên nếu chưa có item nào đang đấu giá
    auto_activate_first_item_if_needed(room_id);
    
    char response[256];
    snprintf(response, sizeof(response), 
             "JOIN_ROOM_SUCCESS|Da tham gia phong|%d|%s", 
             room_id, room->room_name);
    send_message(client, response);
    
    // Broadcast cho những người khác
    char broadcast_msg[256];
    snprintf(broadcast_msg, sizeof(broadcast_msg), 
             "USER_JOINED|%s|vua tham gia phong", client->username);
    broadcast_to_room(room_id, broadcast_msg, client->socket_fd);
}

// Rời phòng
void handle_leave_room(Client* client) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    if (client->current_room_id == 0) {
        send_message(client, "ERROR|Ban khong o trong phong nao");
        return;
    }
    
    int room_id = client->current_room_id;
    client->current_room_id = 0;
    
    send_message(client, "LEAVE_ROOM_SUCCESS|Da roi phong");
    
    // Broadcast cho những người còn lại
    char broadcast_msg[256];
    snprintf(broadcast_msg, sizeof(broadcast_msg), 
             "USER_LEFT|%s|da roi phong", client->username);
    broadcast_to_room(room_id, broadcast_msg, -1);
}

// // Xem chi tiết phòng và danh sách vật phẩm
// void handle_get_room_detail(Client* client, char* room_id_str) {
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }
    
//     int room_id = atoi(room_id_str);
//     Room* room = get_room_by_id(room_id);
    
//     if (room == NULL) {
//         send_message(client, "ROOM_DETAIL_FAIL|Phong khong ton tai");
//         return;
//     }
    
//     char response[BUFFER_SIZE];
//     snprintf(response, sizeof(response), 
//              "ROOM_DETAIL|%d|%s|%s|%s|%s|",
//              room->room_id, room->room_name, room->status, 
//              room->start_time, room->end_time);
    
//     FILE* file = fopen(ITEMS_FILE, "r");
//     if (file != NULL) {
//         char line[1024];
//         while (fgets(line, sizeof(line), file)) {
//             line[strcspn(line, "\n")] = 0;
            
//             int item_id, item_room_id;
//             char item_name[200], status[20];
//             double start_price, current_price, buy_now_price;
//             char auction_start[30], auction_end[30];
            
//             char* token = strtok(line, "|");
//             if (!token) continue;
//             item_id = atoi(token);
            
//             token = strtok(NULL, "|");
//             if (!token) continue;
//             item_room_id = atoi(token);
            
//             if (item_room_id == room_id) {
//                 token = strtok(NULL, "|"); // item_name
//                 strncpy(item_name, token ? token : "", sizeof(item_name));
                
//                 token = strtok(NULL, "|"); // description - skip
//                 token = strtok(NULL, "|"); // start_price
//                 start_price = token ? atof(token) : 0;
                
//                 token = strtok(NULL, "|"); // current_price
//                 current_price = token ? atof(token) : 0;
                
//                 token = strtok(NULL, "|"); // buy_now_price
//                 buy_now_price = token ? atof(token) : 0;
                
//                 token = strtok(NULL, "|"); // status
//                 strncpy(status, token ? token : "", sizeof(status));
                
//                 char item_data[512];
//                 snprintf(item_data, sizeof(item_data), 
//                          "%d|%s|%s|%.0f|%.0f|%.0f;",
//                          item_id, item_name, status, 
//                          start_price, current_price, buy_now_price);
//                 strncat(response, item_data, sizeof(response) - strlen(response) - 1);
//             }
//         }
//         fclose(file);
//     }
    
//     send_message(client, response);
// }

char* get_token(char** line_ptr) {
    char* start = *line_ptr;
    if (start == NULL) return NULL;
    
    char* end = strchr(start, '|');
    if (end != NULL) {
        *end = '\0';       // Thay thế dấu | bằng ký tự kết thúc chuỗi
        *line_ptr = end + 1; // Di chuyển con trỏ sang token tiếp theo
    } else {
        *line_ptr = NULL;  // Đã hết chuỗi
    }
    return start;
}

void handle_get_room_detail(Client* client, char* room_id_str) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    int room_id = atoi(room_id_str);
    Room* room = get_room_by_id(room_id);
    
    if (room == NULL) {
        send_message(client, "ROOM_DETAIL_FAIL|Phong khong ton tai");
        return;
    }
    
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), 
             "ROOM_DETAIL|%d|%s|%s|%s|%s|",
             room->room_id, room->room_name, room->status, 
             room->start_time, room->end_time);
    
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file != NULL) {
        char line[1024];
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;
            
            // --- SỬA ĐỔI BẮT ĐẦU TỪ ĐÂY ---
            // Dùng con trỏ phụ để duyệt chuỗi bằng get_token
            char* current_ptr = line;
            
            char *token;
            int item_id, item_room_id;
            char item_name[200], status[20];
            double start_price, current_price, buy_now_price;
            
            // 1. Item ID
            token = get_token(&current_ptr);
            if (!token) continue;
            item_id = atoi(token);
            
            // 2. Room ID
            token = get_token(&current_ptr);
            if (!token) continue;
            item_room_id = atoi(token);
            
            if (item_room_id == room_id) {
                // 3. Name
                token = get_token(&current_ptr);
                strncpy(item_name, token ? token : "", sizeof(item_name));
                
                // 4. Description
                token = get_token(&current_ptr);
                char description[200] = "";
                if (token) strncpy(description, token, sizeof(description) - 1);
                
                // 5. Start Price
                token = get_token(&current_ptr);
                start_price = token ? atof(token) : 0;
                
                // 6. Current Price
                token = get_token(&current_ptr);
                current_price = token ? atof(token) : 0;
                
                // 7. Buy Now Price
                token = get_token(&current_ptr);
                buy_now_price = token ? atof(token) : 0;
                
                // 8. Status
                token = get_token(&current_ptr);
                strncpy(status, token ? token : "UNKNOWN", sizeof(status));
                
                // 9. Winner ID
                token = get_token(&current_ptr);
                
                // 10. Final Price
                token = get_token(&current_ptr);
                
                // 11. Auction Start
                token = get_token(&current_ptr);
                char auction_start[30] = "";
                if (token) strncpy(auction_start, token, sizeof(auction_start) - 1);
                
                // 12. Auction End
                token = get_token(&current_ptr);
                char auction_end[30] = "";
                if (token) strncpy(auction_end, token, sizeof(auction_end) - 1);
                
                // 13. Extend Count
                token = get_token(&current_ptr);
                
                // 14. Duration
                token = get_token(&current_ptr);
                int duration = token ? atoi(token) : 0;
                
                // 15. Created At
                token = get_token(&current_ptr);
                
                // 16. Scheduled Start
                token = get_token(&current_ptr);
                char sched_start[30] = "";
                if (token) strncpy(sched_start, token, sizeof(sched_start) - 1);
                
                // 17. Scheduled End
                token = get_token(&current_ptr);
                char sched_end[30] = "";
                if (token) strncpy(sched_end, token, sizeof(sched_end) - 1);
                
                // Tạo chuỗi item data gửi về client với thông tin thời gian
                char item_data[1024];
                snprintf(item_data, sizeof(item_data), 
                         "%d|%s|%s|%s|%.0f|%.0f|%.0f|%s|%s|%s|%s|%d;",
                         item_id, item_name, description, status, 
                         start_price, current_price, buy_now_price,
                         auction_start, auction_end, sched_start, sched_end, duration);
                strncat(response, item_data, sizeof(response) - strlen(response) - 1);
            }
        }
        fclose(file);
    }
    
    send_message(client, response);
}

// Parse thời gian từ string
static time_t parse_room_time(const char* time_str) {
    struct tm tm_info;
    memset(&tm_info, 0, sizeof(tm_info));
    
    if (sscanf(time_str, "%d-%d-%d %d:%d:%d",
               &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
               &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6) {
        tm_info.tm_year -= 1900;
        tm_info.tm_mon -= 1;
        return mktime(&tm_info);
    }
    return 0;
}

// Cập nhật trạng thái phòng
int update_room_status(int room_id, const char* new_status) {
    FILE* file = fopen(ROOMS_FILE, "r");
    if (file == NULL) return 0;
    
    char lines[100][512];
    int line_count = 0;
    int found = 0;
    
    while (fgets(lines[line_count], sizeof(lines[0]), file)) {
        lines[line_count][strcspn(lines[line_count], "\n")] = 0;
        
        int rid;
        if (sscanf(lines[line_count], "%d|", &rid) == 1 && rid == room_id) {
            // Parse và cập nhật dòng này
            char room_name[100], old_status[20], start_time[30], end_time[30];
            int owner_id;
            
            if (sscanf(lines[line_count], "%d|%99[^|]|%d|%19[^|]|%29[^|]|%29[^|]",
                       &rid, room_name, &owner_id, old_status, start_time, end_time) == 6) {
                snprintf(lines[line_count], sizeof(lines[0]),
                         "%d|%s|%d|%s|%s|%s",
                         rid, room_name, owner_id, new_status, start_time, end_time);
                found = 1;
            }
        }
        line_count++;
    }
    fclose(file);
    
    if (found) {
        file = fopen(ROOMS_FILE, "w");
        if (file == NULL) return 0;
        
        for (int i = 0; i < line_count; i++) {
            fprintf(file, "%s\n", lines[i]);
        }
        fclose(file);
    }
    
    return found;
}

void check_and_update_room_statuses() {
    FILE* file = fopen(ROOMS_FILE, "r");
    if (file == NULL) return;
    
    time_t now = time(NULL);
    char line[512];
    
    // Đọc tất cả phòng và kiểm tra
    typedef struct {
        int room_id;
        char new_status[20];
        int need_activate;
    } RoomUpdate;
    
    RoomUpdate updates[100];
    int update_count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        
        int room_id, owner_id;
        char room_name[100], status[20], start_time[30], end_time[30];
        
        if (sscanf(line, "%d|%99[^|]|%d|%19[^|]|%29[^|]|%29[^|]",
                   &room_id, room_name, &owner_id, status, start_time, end_time) == 6) {
            
            time_t start_t = parse_room_time(start_time);
            time_t end_t = parse_room_time(end_time);
            
            // PENDING -> ACTIVE khi đến start_time
            if (strcmp(status, ROOM_STATUS_PENDING) == 0 && start_t > 0 && now >= start_t) {
                updates[update_count].room_id = room_id;
                strcpy(updates[update_count].new_status, ROOM_STATUS_ACTIVE);
                updates[update_count].need_activate = 1;
                update_count++;
                printf("Room %d: PENDING -> ACTIVE (start_time reached)\n", room_id);
            }
            // ACTIVE -> CLOSED khi quá end_time
            else if (strcmp(status, ROOM_STATUS_ACTIVE) == 0 && end_t > 0 && now >= end_t) {
                updates[update_count].room_id = room_id;
                strcpy(updates[update_count].new_status, ROOM_STATUS_CLOSED);
                updates[update_count].need_activate = 0;
                update_count++;
                printf("Room %d: ACTIVE -> CLOSED (end_time reached)\n", room_id);
            }
        }
    }
    fclose(file);
    
    // Áp dụng các cập nhật
    for (int i = 0; i < update_count; i++) {
        update_room_status(updates[i].room_id, updates[i].new_status);
        
        // Kích hoạt item đầu tiên nếu phòng vừa chuyển sang ACTIVE
        if (updates[i].need_activate) {
            activate_next_item_in_room(updates[i].room_id);
        }
        
        // Lỗi 6: Khi phòng CLOSED, thông báo và kick user (trừ owner)
        if (strcmp(updates[i].new_status, ROOM_STATUS_CLOSED) == 0) {
            // Broadcast thông báo phòng đóng
            char msg[256];
            snprintf(msg, sizeof(msg), "ROOM_CLOSED|%d|Phong da dong",
                     updates[i].room_id);
            broadcast_to_room(updates[i].room_id, msg, -1);
            
            // Kick tất cả user ra khỏi phòng (trừ owner)
            Room* closed_room = get_room_by_id(updates[i].room_id);
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (g_clients[j] && g_clients[j]->current_room_id == updates[i].room_id) {
                    if (closed_room && g_clients[j]->user_id != closed_room->owner_id) {
                        g_clients[j]->current_room_id = 0;
                        send_message(g_clients[j], "KICKED|Ban da bi kick khoi phong (phong dong)");
                    }
                }
            }
        } else {
            // Broadcast thông báo thay đổi trạng thái khác
            char msg[256];
            snprintf(msg, sizeof(msg), "ROOM_STATUS_CHANGED|%d|%s",
                     updates[i].room_id, updates[i].new_status);
            broadcast_to_room(updates[i].room_id, msg, -1);
        }
    }
}

// Kích hoạt đấu giá (owner bắt đầu phiên đấu giá)
void handle_start_auction(Client* client, char* room_id_str) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    int room_id = atoi(room_id_str);
    Room* room = get_room_by_id(room_id);
    
    if (room == NULL) {
        send_message(client, "START_AUCTION_FAIL|Phong khong ton tai");
        return;
    }
    
    // Kiểm tra quyền (phải là owner)
    if (room->owner_id != client->user_id) {
        send_message(client, "START_AUCTION_FAIL|Ban khong phai chu phong");
        return;
    }
    
    // Kiểm tra phòng phải ACTIVE
    if (strcmp(room->status, ROOM_STATUS_ACTIVE) != 0) {
        send_message(client, "START_AUCTION_FAIL|Phong chua hoat dong");
        return;
    }
    
    // Kiểm tra xem đã có item ACTIVE chưa
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) {
        send_message(client, "START_AUCTION_FAIL|Loi doc file");
        return;
    }
    
    char line[2048];
    int has_active_item = 0;
    
    while (fgets(line, sizeof(line), file)) {
        char* tokens[17];
        char* ptr = line;
        for (int i = 0; i < 17 && ptr; i++) {
            tokens[i] = ptr;
            ptr = strchr(ptr, '|');
            if (ptr) {
                *ptr = '\0';
                ptr++;
            }
        }
        
        int item_room_id = atoi(tokens[1]);
        char* status = tokens[7];
        
        if (item_room_id == room_id && strcmp(status, ITEM_STATUS_ACTIVE) == 0) {
            has_active_item = 1;
            break;
        }
    }
    fclose(file);
    
    if (has_active_item) {
        send_message(client, "START_AUCTION_FAIL|Da co vat pham dang dau gia");
        return;
    }
    
    // Kích hoạt item đầu tiên
    activate_next_item_in_room(room_id);
    
    send_message(client, "START_AUCTION_SUCCESS|Da kich hoat phien dau gia");
}