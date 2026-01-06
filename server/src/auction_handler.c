/*
 * auction_handler.c (Updated with time extension)
 * Triển khai các chức năng đấu giá với gia hạn 30s
 */

#include "auction_handler.h"
#include "item_handler.h"
#include "room_handler.h"
#include "client_handler.h"
#include "timer_handler.h"
#include <time.h>

extern const char* ITEMS_FILE;

// Lấy người bid cao nhất từ lịch sử
int get_highest_bidder_from_history(const char* bid_history, int* user_id_out, double* amount_out) {
    if (!bid_history || strlen(bid_history) == 0) {
        return 0;
    }
    
    char history_copy[2048];
    strncpy(history_copy, bid_history, sizeof(history_copy));
    
    int highest_user_id = 0;
    double highest_amount = 0.0;
    
    char* bid_token = strtok(history_copy, ";");
    while (bid_token) {
        int uid;
        double amt;
        char timestamp[30];
        
        if (sscanf(bid_token, "%d:%lf:%s", &uid, &amt, timestamp) >= 2) {
            if (amt > highest_amount) {
                highest_amount = amt;
                highest_user_id = uid;
            }
        }
        bid_token = strtok(NULL, ";");
    }
    
    if (highest_user_id > 0) {
        *user_id_out = highest_user_id;
        *amount_out = highest_amount;
        return 1;
    }
    return 0;
}

// Thêm bid vào lịch sử
int add_bid_to_history(char* bid_history, int user_id, double amount) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char new_bid[100];
    snprintf(new_bid, sizeof(new_bid), "%d:%.0f:%s;", user_id, amount, timestamp);
    
    strncat(bid_history, new_bid, 2048 - strlen(bid_history) - 1);
    return 1;
}

// Parse thời gian từ string
time_t parse_time_string(const char* time_str) {
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

// Tính thời gian giới hạn cuối cùng dựa trên scheduled_end của item
// Trả về 0 nếu không có giới hạn
static time_t get_hard_deadline(Item* item) {
    if (item->scheduled_end[0] == '\0' || strlen(item->scheduled_end) < 3) {
        return 0; // Không có khung giờ cứng
    }
    
    // Parse scheduled_end (HH:MM) và kết hợp với ngày hiện tại hoặc auction_start
    int end_hour, end_min;
    if (sscanf(item->scheduled_end, "%d:%d", &end_hour, &end_min) != 2) {
        return 0;
    }
    
    // Lấy ngày từ auction_start hoặc ngày hiện tại
    time_t base_time;
    if (item->auction_start[0] != '\0') {
        base_time = parse_time_string(item->auction_start);
    } else {
        base_time = time(NULL);
    }
    
    if (base_time == 0) base_time = time(NULL);
    
    struct tm* tm_info = localtime(&base_time);
    tm_info->tm_hour = end_hour;
    tm_info->tm_min = end_min;
    tm_info->tm_sec = 0;
    
    return mktime(tm_info);
}

// Đặt giá với gia hạn 30s (có kiểm tra giới hạn thời gian)
void handle_place_bid(Client* client, char* item_id_str, char* bid_amount_str) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    if (client->current_room_id == 0) {
        send_message(client, "ERROR|Ban chua tham gia phong");
        return;
    }
    
    int item_id = atoi(item_id_str);
    double bid_amount = atof(bid_amount_str);
    
    Item* item = get_item_by_id(item_id);
    if (item == NULL) {
        send_message(client, "BID_FAIL|Vat pham khong ton tai");
        return;
    }
    
    if (item->room_id != client->current_room_id) {
        send_message(client, "BID_FAIL|Vat pham khong o trong phong nay");
        return;
    }
    
    // Lỗi 3: Chủ phòng không được đặt giá
    Room* room = get_room_by_id(item->room_id);
    if (room && room->owner_id == client->user_id) {
        send_message(client, "BID_FAIL|Chu phong khong duoc dat gia");
        return;
    }
    
    if (strcmp(item->status, ITEM_STATUS_ACTIVE) != 0) {
        send_message(client, "BID_FAIL|Vat pham da dong");
        return;
    }
    
    if (bid_amount < item->current_price + 10000) {
        send_message(client, "BID_FAIL|Gia phai lon hon gia hien tai it nhat 10.000d");
        return;
    }
    
    int highest_bidder_id = 0;
    double highest_bid = 0.0;
    if (get_highest_bidder_from_history(item->bid_history, &highest_bidder_id, &highest_bid)) {
        if (highest_bidder_id == client->user_id) {
            send_message(client, "BID_FAIL|Ban dang la nguoi dat gia cao nhat");
            return;
        }
    }
    
    // --- TIME EXTENSION: 30 SECOND RULE ---
    time_t now = time(NULL);
    time_t end_time = parse_time_string(item->auction_end);
    int remaining = (int)difftime(end_time, now);
    
    int time_extended = 0;
    // Rule: If less than 30s remaining, extend to now + 30s
    if (remaining <= 30 && remaining > 0) {
        // Tính thời gian gia hạn
        time_t new_end_time = now + 30;
        
        // Kiểm tra scheduled_end để không vượt quá thời gian lên lịch
        if (item->scheduled_end[0] != '\0') {
            time_t scheduled_end_time = parse_time_string(item->scheduled_end);
            if (scheduled_end_time > 0 && new_end_time > scheduled_end_time) {
                // Giới hạn không vượt quá scheduled_end
                new_end_time = scheduled_end_time;
                printf("Time extension limited by scheduled_end for item %d\n", item_id);
            }
        }
        
        item->extend_count++;
        
        struct tm* tm_info = localtime(&new_end_time);
        strftime(item->auction_end, sizeof(item->auction_end),
                 "%Y-%m-%d %H:%M:%S", tm_info);
        
        update_timer(item_id, new_end_time);
        time_extended = 1;
        
        printf("Time extended for item %d, new end: %s, extend_count: %d\n",
               item_id, item->auction_end, item->extend_count);
    }
    
    // Cập nhật item
    item->current_price = bid_amount;
    add_bid_to_history(item->bid_history, client->user_id, bid_amount);
    save_item(item);
    
    char response[256];
    snprintf(response, sizeof(response), 
             "BID_SUCCESS|Dat gia thanh cong|%.0f", bid_amount);
    send_message(client, response);
    
    // Broadcast bid mới với đầy đủ thông tin thời gian (Lỗi 4)
    char broadcast_msg[1024];
    time_t bid_time = time(NULL);
    struct tm* tm_info = localtime(&bid_time);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Tính thời gian đếm ngược
    time_t current_end = parse_time_string(item->auction_end);
    int countdown = (int)difftime(current_end, bid_time);
    if (countdown < 0) countdown = 0;
    
    snprintf(broadcast_msg, sizeof(broadcast_msg),
             "NEW_BID|%d|%s|%s|%.0f|%s|%s|%s|%s|%s|%d",
             item_id, item->item_name, client->username, bid_amount, timestamp,
             item->auction_start, item->auction_end, 
             item->scheduled_start, item->scheduled_end, countdown);
    broadcast_to_room(client->current_room_id, broadcast_msg, -1);
    
    // Broadcast gia hạn nếu có
    if (time_extended == 1) {
        char extend_msg[512];
        snprintf(extend_msg, sizeof(extend_msg),
                 "TIME_EXTENDED|%d|%s|%s|%d",
                 item_id, item->item_name, item->auction_end, item->extend_count);
        broadcast_to_room(client->current_room_id, extend_msg, -1);
    }
}

// Mua ngay
void handle_buy_now(Client* client, char* item_id_str) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    if (client->current_room_id == 0) {
        send_message(client, "ERROR|Ban chua tham gia phong");
        return;
    }
    
    int item_id = atoi(item_id_str);
    Item* item = get_item_by_id(item_id);
    
    if (item == NULL) {
        send_message(client, "BUY_NOW_FAIL|Vat pham khong ton tai");
        return;
    }
    
    if (item->room_id != client->current_room_id) {
        send_message(client, "BUY_NOW_FAIL|Vat pham khong o trong phong nay");
        return;
    }
    
    // Lỗi 3: Chủ phòng không được mua ngay
    Room* room = get_room_by_id(item->room_id);
    if (room && room->owner_id == client->user_id) {
        send_message(client, "BUY_NOW_FAIL|Chu phong khong duoc mua ngay");
        return;
    }
    
    if (item->buy_now_price <= 0) {
        send_message(client, "BUY_NOW_FAIL|San pham khong co gia mua ngay");
        return;
    }
    
    if (strcmp(item->status, ITEM_STATUS_ACTIVE) != 0 && 
        strcmp(item->status, ITEM_STATUS_PENDING) != 0) {
        send_message(client, "BUY_NOW_FAIL|San pham da duoc ban");
        return;
    }
    
    // Cập nhật item
    strncpy(item->status, ITEM_STATUS_SOLD, sizeof(item->status));
    item->winner_id = client->user_id;
    item->final_price = item->buy_now_price;
    save_item(item);
    
    // Xóa timer nếu có
    remove_timer(item_id);
    
    char response[256];
    snprintf(response, sizeof(response), 
             "BUY_NOW_SUCCESS|Mua thanh cong|%.0f", item->buy_now_price);
    send_message(client, response);
    
    // Broadcast cho những người khác
    char broadcast_msg[512];
    snprintf(broadcast_msg, sizeof(broadcast_msg),
             "ITEM_SOLD|%d|%s|%.0f|Da duoc mua ngay",
             item_id, client->username, item->buy_now_price);
    broadcast_to_room(client->current_room_id, broadcast_msg, -1);
    
    // Gửi thông báo hủy bid cho những người đã bid
    if (strlen(item->bid_history) > 0) {
        char history_copy[2048];
        strncpy(history_copy, item->bid_history, sizeof(history_copy));
        
        char* bid_token = strtok(history_copy, ";");
        while (bid_token) {
            int uid;
            if (sscanf(bid_token, "%d:", &uid) == 1) {
                if (uid != client->user_id) {
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (g_clients[i] && g_clients[i]->user_id == uid) {
                            char cancel_msg[512];
                            snprintf(cancel_msg, sizeof(cancel_msg),
                                     "BID_CANCELED|%d|%s|Da duoc mua ngay boi nguoi khac",
                                     item_id, item->item_name);
                            send_message(g_clients[i], cancel_msg);
                        }
                    }
                }
            }
            bid_token = strtok(NULL, ";");
        }
    }
    
    // Kích hoạt item tiếp theo
    activate_next_item_in_room(client->current_room_id);
}

// Xem lịch sử tham gia đấu giá
void handle_get_my_auction_history(Client* client, char* filter, char* page_str, char* limit_str) {
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
    
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) {
        send_message(client, "AUCTION_HISTORY|0|");
        return;
    }
    
    char response[BUFFER_SIZE] = "AUCTION_HISTORY|";
    char results[BUFFER_SIZE] = "";
    int total_count = 0;
    int current_index = 0;
    int offset = (page - 1) * limit;
    char line[2048];
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        // Hỗ trợ cả format cũ (14 trường) và mới (18 trường)
        char* tokens[19];
        char* ptr = line;
        int token_count = 0;
        for (int i = 0; i < 19 && ptr; i++) {
            tokens[i] = ptr;
            ptr = strchr(ptr, '|');
            if (ptr) {
                *ptr = '\0';
                ptr++;
            }
            token_count++;
        }
        
        if (token_count < 14) continue;
        
        int item_id = atoi(tokens[0]);
        int room_id = atoi(tokens[1]);
        char* item_name = tokens[2];
        char* status = tokens[7];
        int winner_id = atoi(tokens[8]);
        double final_price = atof(tokens[9]);
        char* auction_end = tokens[11];
        
        // bid_history ở index 17 (format mới) hoặc index 13 (format cũ)
        // Kiểm tra format dựa trên số lượng trường
        char* bid_history;
        if (token_count >= 18) {
            bid_history = tokens[17];  // Format mới: index 17
        } else {
            bid_history = tokens[13];  // Format cũ: index 13 (nếu có)
        }
        
        char history_copy[2048];
        strncpy(history_copy, bid_history, sizeof(history_copy));
        
        int user_participated = 0;
        double my_highest_bid = 0.0;
        
        char* bid_token = strtok(history_copy, ";");
        while (bid_token) {
            int uid;
            double amt;
            
            if (sscanf(bid_token, "%d:%lf:", &uid, &amt) >= 2) {
                if (uid == client->user_id) {
                    user_participated = 1;
                    if (amt > my_highest_bid) {
                        my_highest_bid = amt;
                    }
                }
            }
            bid_token = strtok(NULL, ";");
        }
        
        if (!user_participated) continue;
        
        char my_status[20] = "BIDDING";
        if (strcmp(status, ITEM_STATUS_SOLD) == 0 || strcmp(status, ITEM_STATUS_CLOSED) == 0) {
            if (winner_id == client->user_id) {
                strcpy(my_status, "WON");
            } else {
                strcpy(my_status, "LOST");
            }
        }
        
        if (filter && strcmp(filter, "ALL") != 0) {
            if (strcmp(filter, my_status) != 0) continue;
        }
        
        total_count++;
        
        if (current_index >= offset && current_index < offset + limit) {
            Room* room = get_room_by_id(room_id);
            char room_name[100] = "Unknown";
            if (room) {
                strncpy(room_name, room->room_name, sizeof(room_name));
            }
            
            char winner_name[MAX_USERNAME] = "None";
            if (winner_id > 0) {
                FILE* user_file = fopen(DB_FILE, "r");
                if (user_file) {
                    char user_line[256];
                    while (fgets(user_line, sizeof(user_line), user_file)) {
                        int uid;
                        char uname[MAX_USERNAME];
                        if (sscanf(user_line, "%d|%49[^|]|", &uid, uname) == 2) {
                            if (uid == winner_id) {
                                strncpy(winner_name, uname, sizeof(winner_name));
                                break;
                            }
                        }
                    }
                    fclose(user_file);
                }
            }
            
            char history_data[512];
            snprintf(history_data, sizeof(history_data),
                     "%d|%s|%s|%.0f|%.0f|%s|%s|%s;",
                     item_id, room_name, item_name, my_highest_bid, 
                     final_price, winner_name, my_status, auction_end);
            strncat(results, history_data, sizeof(results) - strlen(results) - 1);
        }
        current_index++;
    }
    fclose(file);
    
    char count_str[20];
    snprintf(count_str, sizeof(count_str), "%d|", total_count);
    strncat(response, count_str, sizeof(response) - strlen(response) - 1);
    strncat(response, results, sizeof(response) - strlen(response) - 1);
    
    send_message(client, response);
}