/*
 * room_handler.c
 * Triển khai các chức năng quản lý phòng đấu giá
 */

#include "room_handler.h"
#include "client_handler.h"
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
    
    if (strcmp(room->status, ROOM_STATUS_ACTIVE) != 0) {
        send_message(client, "JOIN_ROOM_FAIL|Phong chua bat dau");
        return;
    }
    
    if (room->owner_id == client->user_id) {
        send_message(client, "JOIN_ROOM_FAIL|Owner tu dong trong phong");
        return;
    }
    
    // Tham gia phòng
    client->current_room_id = room_id;
    
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

// Xem chi tiết phòng và danh sách vật phẩm
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
    
    // Đọc danh sách items
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file != NULL) {
        char line[1024];
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;
            
            int item_id, item_room_id;
            char item_name[200], status[20];
            double start_price, current_price, buy_now_price;
            char auction_start[30], auction_end[30];
            
            // Parse format: item_id|room_id|item_name|...|status|...|current_price|...
            char* token = strtok(line, "|");
            if (!token) continue;
            item_id = atoi(token);
            
            token = strtok(NULL, "|");
            if (!token) continue;
            item_room_id = atoi(token);
            
            if (item_room_id == room_id) {
                token = strtok(NULL, "|"); // item_name
                strncpy(item_name, token ? token : "", sizeof(item_name));
                
                token = strtok(NULL, "|"); // description - skip
                token = strtok(NULL, "|"); // start_price
                start_price = token ? atof(token) : 0;
                
                token = strtok(NULL, "|"); // current_price
                current_price = token ? atof(token) : 0;
                
                token = strtok(NULL, "|"); // buy_now_price
                buy_now_price = token ? atof(token) : 0;
                
                token = strtok(NULL, "|"); // status
                strncpy(status, token ? token : "", sizeof(status));
                
                char item_data[512];
                snprintf(item_data, sizeof(item_data), 
                         "%d|%s|%s|%.0f|%.0f|%.0f;",
                         item_id, item_name, status, 
                         start_price, current_price, buy_now_price);
                strncat(response, item_data, sizeof(response) - strlen(response) - 1);
            }
        }
        fclose(file);
    }
    
    send_message(client, response);
}