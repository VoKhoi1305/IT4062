/*
 * item_handler.c
 * Triển khai các chức năng quản lý vật phẩm đấu giá
 */

#include "item_handler.h"
#include "room_handler.h"
#include "client_handler.h"
#include "server.h"
#include <ctype.h>

extern const char* ITEMS_FILE;

// Lấy ID vật phẩm lớn nhất
int get_last_item_id() {
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) return 0;
    
    char line[2048];
    int max_id = 0;
    
    while (fgets(line, sizeof(line), file)) {
        int item_id;
        if (sscanf(line, "%d|", &item_id) == 1) {
            if (item_id > max_id) max_id = item_id;
        }
    }
    fclose(file);
    return max_id;
}

// Lấy thông tin vật phẩm theo ID
Item* get_item_by_id(int item_id) {
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) return NULL;
    
    static Item item;
    char line[2048];
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        char item_name[200], description[500], status[20];
        char auction_start[30], auction_end[30], bid_history[2048];
        int iid, room_id, winner_id, extend_count;
        double start_price, current_price, buy_now_price, final_price;
        
        // Parse theo format
        char* tokens[14];
        char* ptr = line;
        int token_count = 0;
        
        for (int i = 0; i < 14 && ptr; i++) {
            tokens[i] = ptr;
            ptr = strchr(ptr, '|');
            if (ptr) {
                *ptr = '\0';
                ptr++;
            }
            token_count++;
        }
        
        if (token_count >= 14) {
            iid = atoi(tokens[0]);
            
            if (iid == item_id) {
                item.item_id = iid;
                item.room_id = atoi(tokens[1]);
                strncpy(item.item_name, tokens[2], sizeof(item.item_name));
                strncpy(item.description, tokens[3], sizeof(item.description));
                item.start_price = atof(tokens[4]);
                item.current_price = atof(tokens[5]);
                item.buy_now_price = atof(tokens[6]);
                strncpy(item.status, tokens[7], sizeof(item.status));
                item.winner_id = atoi(tokens[8]);
                item.final_price = atof(tokens[9]);
                strncpy(item.auction_start, tokens[10], sizeof(item.auction_start));
                strncpy(item.auction_end, tokens[11], sizeof(item.auction_end));
                item.extend_count = atoi(tokens[12]);
                strncpy(item.bid_history, tokens[13], sizeof(item.bid_history));
                
                fclose(file);
                return &item;
            }
        }
    }
    fclose(file);
    return NULL;
}

// Lưu/Cập nhật vật phẩm
int save_item(Item* item) {
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) return 0;
    
    char lines[1000][2048];
    int line_count = 0;
    int found = 0;
    
    // Đọc tất cả các dòng
    while (fgets(lines[line_count], sizeof(lines[0]), file)) {
        lines[line_count][strcspn(lines[line_count], "\n")] = 0;
        
        int iid;
        if (sscanf(lines[line_count], "%d|", &iid) == 1 && iid == item->item_id) {
            // Cập nhật dòng này
            snprintf(lines[line_count], sizeof(lines[0]),
                     "%d|%d|%s|%s|%.0f|%.0f|%.0f|%s|%d|%.0f|%s|%s|%d|%s",
                     item->item_id, item->room_id, item->item_name, item->description,
                     item->start_price, item->current_price, item->buy_now_price,
                     item->status, item->winner_id, item->final_price,
                     item->auction_start, item->auction_end, item->extend_count,
                     item->bid_history);
            found = 1;
        }
        line_count++;
    }
    fclose(file);
    
    // Ghi lại file
    file = fopen(ITEMS_FILE, "w");
    if (file == NULL) return 0;
    
    for (int i = 0; i < line_count; i++) {
        fprintf(file, "%s\n", lines[i]);
    }
    fclose(file);
    
    return found;
}

// Tạo vật phẩm đấu giá
void handle_create_item(Client* client, char* room_id_str, char* item_name, 
                        char* start_price_str, char* duration_str, char* buy_now_price_str) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    int room_id = atoi(room_id_str);
    double start_price = atof(start_price_str);
    int duration = atoi(duration_str);
    double buy_now_price = atof(buy_now_price_str);
    
    // Kiểm tra phòng tồn tại
    Room* room = get_room_by_id(room_id);
    if (room == NULL) {
        send_message(client, "CREATE_ITEM_FAIL|Phong khong ton tai");
        return;
    }
    
    // Kiểm tra quyền sở hữu
    if (room->owner_id != client->user_id) {
        send_message(client, "CREATE_ITEM_FAIL|Ban khong phai chu phong");
        return;
    }
    
    if (start_price <= 0) {
        send_message(client, "CREATE_ITEM_FAIL|Gia khoi diem phai lon hon 0");
        return;
    }
    
    if (buy_now_price > 0 && buy_now_price < start_price) {
        send_message(client, "CREATE_ITEM_FAIL|Gia mua ngay phai lon hon gia khoi diem");
        return;
    }
    
    if (duration <= 0) {
        send_message(client, "CREATE_ITEM_FAIL|Thoi gian dau gia khong hop le");
        return;
    }
    
    // Tạo item mới
    int new_item_id = get_last_item_id() + 1;
    
    FILE* file = fopen(ITEMS_FILE, "a");
    if (file == NULL) {
        send_message(client, "CREATE_ITEM_FAIL|Loi mo file");
        return;
    }
    
    // Format: item_id|room_id|item_name|description|start_price|current_price|
    //         buy_now_price|status|winner_id|final_price|auction_start|auction_end|
    //         extend_count|bid_history
    fprintf(file, "%d|%d|%s||%.0f|%.0f|%.0f|%s|0|0.0|||0|\n",
            new_item_id, room_id, item_name, start_price, start_price,
            buy_now_price, ITEM_STATUS_PENDING);
    fclose(file);
    
    char response[256];
    snprintf(response, sizeof(response), 
             "CREATE_ITEM_SUCCESS|Tao vat pham thanh cong|%d", new_item_id);
    send_message(client, response);
}

// Chuyển chuỗi thành chữ thường
void to_lower(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// Tìm kiếm vật phẩm
void handle_search_items(Client* client, char* search_type, char* keyword, 
                         char* time_from, char* time_to) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) {
        send_message(client, "SEARCH_RESULT|0|");
        return;
    }
    
    char response[BUFFER_SIZE] = "SEARCH_RESULT|";
    char results[BUFFER_SIZE] = "";
    int total_count = 0;
    char line[2048];
    
    // Chuyển keyword thành chữ thường để so sánh
    char keyword_lower[200] = "";
    if (keyword && strlen(keyword) > 0) {
        strncpy(keyword_lower, keyword, sizeof(keyword_lower));
        to_lower(keyword_lower);
    }
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        int item_id, room_id;
        char item_name[200], status[20];
        double start_price, current_price;
        char auction_start[30], auction_end[30];
        
        char* tokens[14];
        char* ptr = line;
        int token_count = 0;
        
        for (int i = 0; i < 14 && ptr; i++) {
            tokens[i] = ptr;
            ptr = strchr(ptr, '|');
            if (ptr) {
                *ptr = '\0';
                ptr++;
            }
            token_count++;
        }
        
        if (token_count < 14) continue;
        
        item_id = atoi(tokens[0]);
        room_id = atoi(tokens[1]);
        strncpy(item_name, tokens[2], sizeof(item_name));
        start_price = atof(tokens[4]);
        current_price = atof(tokens[5]);
        strncpy(status, tokens[7], sizeof(status));
        strncpy(auction_start, tokens[10], sizeof(auction_start));
        strncpy(auction_end, tokens[11], sizeof(auction_end));
        
        // Chỉ lấy items ACTIVE hoặc PENDING
        if (strcmp(status, ITEM_STATUS_ACTIVE) != 0 && 
            strcmp(status, ITEM_STATUS_PENDING) != 0) {
            continue;
        }
        
        int match = 0;
        
        if (strcmp(search_type, "NAME") == 0) {
            char item_name_lower[200];
            strncpy(item_name_lower, item_name, sizeof(item_name_lower));
            to_lower(item_name_lower);
            if (strstr(item_name_lower, keyword_lower) != NULL) {
                match = 1;
            }
        } else if (strcmp(search_type, "TIME") == 0) {
            // TODO: So sánh thời gian
            match = 1; // Tạm thời match tất cả
        } else if (strcmp(search_type, "BOTH") == 0) {
            char item_name_lower[200];
            strncpy(item_name_lower, item_name, sizeof(item_name_lower));
            to_lower(item_name_lower);
            if (strstr(item_name_lower, keyword_lower) != NULL) {
                match = 1; // TODO: Thêm điều kiện thời gian
            }
        }
        
        if (match) {
            total_count++;
            
            // Lấy tên phòng
            Room* room = get_room_by_id(room_id);
            char room_name[100] = "Unknown";
            if (room) {
                strncpy(room_name, room->room_name, sizeof(room_name));
            }
            
            char result_data[512];
            snprintf(result_data, sizeof(result_data),
                     "%d|%d|%s|%s|%.0f|%.0f|%s|%s|%s;",
                     item_id, room_id, room_name, item_name,
                     start_price, current_price, status,
                     auction_start, auction_end);
            strncat(results, result_data, sizeof(results) - strlen(results) - 1);
        }
    }
    fclose(file);
    
    char count_str[20];
    snprintf(count_str, sizeof(count_str), "%d|", total_count);
    strncat(response, count_str, sizeof(response) - strlen(response) - 1);
    strncat(response, results, sizeof(response) - strlen(response) - 1);
    
    send_message(client, response);
}