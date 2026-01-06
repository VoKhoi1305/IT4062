// /*
//  * item_handler.c
//  * Triển khai các chức năng quản lý vật phẩm đấu giá
//  */

// #include "item_handler.h"
// #include "room_handler.h"
// #include "client_handler.h"
// #include "server.h"
// #include <ctype.h>

// extern const char* ITEMS_FILE;

// // Lấy ID vật phẩm lớn nhất
// int get_last_item_id() {
//     FILE* file = fopen(ITEMS_FILE, "r");
//     if (file == NULL) return 0;
    
//     char line[2048];
//     int max_id = 0;
    
//     while (fgets(line, sizeof(line), file)) {
//         int item_id;
//         if (sscanf(line, "%d|", &item_id) == 1) {
//             if (item_id > max_id) max_id = item_id;
//         }
//     }
//     fclose(file);
//     return max_id;
// }

// // Lấy thông tin vật phẩm theo ID
// Item* get_item_by_id(int item_id) {
//     FILE* file = fopen(ITEMS_FILE, "r");
//     if (file == NULL) return NULL;
    
//     static Item item;
//     char line[2048];
    
//     while (fgets(line, sizeof(line), file)) {
//         line[strcspn(line, "\n")] = 0;
        
//         char item_name[200], description[500], status[20];
//         char auction_start[30], auction_end[30], bid_history[2048];
//         int iid, room_id, winner_id, extend_count;
//         double start_price, current_price, buy_now_price, final_price;
        
//         // Parse theo format
//         char* tokens[14];
//         char* ptr = line;
//         int token_count = 0;
        
//         for (int i = 0; i < 14 && ptr; i++) {
//             tokens[i] = ptr;
//             ptr = strchr(ptr, '|');
//             if (ptr) {
//                 *ptr = '\0';
//                 ptr++;
//             }
//             token_count++;
//         }
        
//         if (token_count >= 14) {
//             iid = atoi(tokens[0]);
            
//             if (iid == item_id) {
//                 item.item_id = iid;
//                 item.room_id = atoi(tokens[1]);
//                 strncpy(item.item_name, tokens[2], sizeof(item.item_name));
//                 strncpy(item.description, tokens[3], sizeof(item.description));
//                 item.start_price = atof(tokens[4]);
//                 item.current_price = atof(tokens[5]);
//                 item.buy_now_price = atof(tokens[6]);
//                 strncpy(item.status, tokens[7], sizeof(item.status));
//                 item.winner_id = atoi(tokens[8]);
//                 item.final_price = atof(tokens[9]);
//                 strncpy(item.auction_start, tokens[10], sizeof(item.auction_start));
//                 strncpy(item.auction_end, tokens[11], sizeof(item.auction_end));
//                 item.extend_count = atoi(tokens[12]);
//                 strncpy(item.bid_history, tokens[13], sizeof(item.bid_history));
                
//                 fclose(file);
//                 return &item;
//             }
//         }
//     }
//     fclose(file);
//     return NULL;
// }

// // Lưu/Cập nhật vật phẩm
// int save_item(Item* item) {
//     FILE* file = fopen(ITEMS_FILE, "r");
//     if (file == NULL) return 0;
    
//     char lines[1000][2048];
//     int line_count = 0;
//     int found = 0;
    
  
//     while (fgets(lines[line_count], sizeof(lines[0]), file)) {
//         lines[line_count][strcspn(lines[line_count], "\n")] = 0;
        
//         int iid;
//         if (sscanf(lines[line_count], "%d|", &iid) == 1 && iid == item->item_id) {
//             // Cập nhật dòng này
//             snprintf(lines[line_count], sizeof(lines[0]),
//                      "%d|%d|%s|%s|%.0f|%.0f|%.0f|%s|%d|%.0f|%s|%s|%d|%s",
//                      item->item_id, item->room_id, item->item_name, item->description,
//                      item->start_price, item->current_price, item->buy_now_price,
//                      item->status, item->winner_id, item->final_price,
//                      item->auction_start, item->auction_end, item->extend_count,
//                      item->bid_history);
//             found = 1;
//         }
//         line_count++;
//     }
//     fclose(file);
    
//     // Ghi lại file
//     file = fopen(ITEMS_FILE, "w");
//     if (file == NULL) return 0;
    
//     for (int i = 0; i < line_count; i++) {
//         fprintf(file, "%s\n", lines[i]);
//     }
//     fclose(file);
    
//     return found;
// }

// // Tạo vật phẩm đấu giá
// void handle_create_item(Client* client, char* room_id_str, char* item_name, 
//                         char* start_price_str, char* duration_str, char* buy_now_price_str) {
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }
    
//     int room_id = atoi(room_id_str);
//     double start_price = atof(start_price_str);
//     int duration = atoi(duration_str);
//     double buy_now_price = atof(buy_now_price_str);
    
//     // Kiểm tra phòng tồn tại
//     Room* room = get_room_by_id(room_id);
//     if (room == NULL) {
//         send_message(client, "CREATE_ITEM_FAIL|Phong khong ton tai");
//         return;
//     }
    
//     // Kiểm tra quyền sở hữu
//     if (room->owner_id != client->user_id) {
//         send_message(client, "CREATE_ITEM_FAIL|Ban khong phai chu phong");
//         return;
//     }
    
//     if (start_price <= 0) {
//         send_message(client, "CREATE_ITEM_FAIL|Gia khoi diem phai lon hon 0");
//         return;
//     }
    
//     if (buy_now_price > 0 && buy_now_price < start_price) {
//         send_message(client, "CREATE_ITEM_FAIL|Gia mua ngay phai lon hon gia khoi diem");
//         return;
//     }
    
//     if (duration <= 0) {
//         send_message(client, "CREATE_ITEM_FAIL|Thoi gian dau gia khong hop le");
//         return;
//     }
    
//     // Tạo item mới
//     int new_item_id = get_last_item_id() + 1;
    
//     FILE* file = fopen(ITEMS_FILE, "a");
//     if (file == NULL) {
//         send_message(client, "CREATE_ITEM_FAIL|Loi mo file");
//         return;
//     }
    
//     // Format: item_id|room_id|item_name|description|start_price|current_price|
//     //         buy_now_price|status|winner_id|final_price|auction_start|auction_end|
//     //         extend_count|bid_history
//     fprintf(file, "%d|%d|%s||%.0f|%.0f|%.0f|%s|0|0.0|||0|\n",
//             new_item_id, room_id, item_name, start_price, start_price,
//             buy_now_price, ITEM_STATUS_PENDING);
//     fclose(file);
    
//     char response[256];
//     snprintf(response, sizeof(response), 
//              "CREATE_ITEM_SUCCESS|Tao vat pham thanh cong|%d", new_item_id);
//     send_message(client, response);
// }

// // Chuyển chuỗi thành chữ thường
// void to_lower(char* str) {
//     for (int i = 0; str[i]; i++) {
//         str[i] = tolower((unsigned char)str[i]);
//     }
// }

// // Tìm kiếm vật phẩm
// void handle_search_items(Client* client, char* search_type, char* keyword, 
//                          char* time_from, char* time_to) {
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }
    
//     FILE* file = fopen(ITEMS_FILE, "r");
//     if (file == NULL) {
//         send_message(client, "SEARCH_RESULT|0|");
//         return;
//     }
    
//     char response[BUFFER_SIZE] = "SEARCH_RESULT|";
//     char results[BUFFER_SIZE] = "";
//     int total_count = 0;
//     char line[2048];
    
//     char keyword_lower[200] = "";
//     if (keyword && strlen(keyword) > 0) {
//         strncpy(keyword_lower, keyword, sizeof(keyword_lower));
//         to_lower(keyword_lower);
//     }
    
//     while (fgets(line, sizeof(line), file)) {
//         line[strcspn(line, "\n")] = 0;
        
//         int item_id, room_id;
//         char item_name[200], status[20];
//         double start_price, current_price;
//         char auction_start[30], auction_end[30];
        
//         char* tokens[14];
//         char* ptr = line;
//         int token_count = 0;
        
//         for (int i = 0; i < 14 && ptr; i++) {
//             tokens[i] = ptr;
//             ptr = strchr(ptr, '|');
//             if (ptr) {
//                 *ptr = '\0';
//                 ptr++;
//             }
//             token_count++;
//         }
        
//         if (token_count < 14) continue;
        
//         item_id = atoi(tokens[0]);
//         room_id = atoi(tokens[1]);
//         strncpy(item_name, tokens[2], sizeof(item_name));
//         start_price = atof(tokens[4]);
//         current_price = atof(tokens[5]);
//         strncpy(status, tokens[7], sizeof(status));
//         strncpy(auction_start, tokens[10], sizeof(auction_start));
//         strncpy(auction_end, tokens[11], sizeof(auction_end));
        
//         // Chỉ lấy items ACTIVE hoặc PENDING
//         if (strcmp(status, ITEM_STATUS_ACTIVE) != 0 && 
//             strcmp(status, ITEM_STATUS_PENDING) != 0) {
//             continue;
//         }
        
//         int match = 0;
        
//         if (strcmp(search_type, "NAME") == 0) {
//             char item_name_lower[200];
//             strncpy(item_name_lower, item_name, sizeof(item_name_lower));
//             to_lower(item_name_lower);
//             if (strstr(item_name_lower, keyword_lower) != NULL) {
//                 match = 1;
//             }
//         } else if (strcmp(search_type, "TIME") == 0) {
//             // TODO: So sánh thời gian
//             match = 1; // Tạm thời match tất cả
//         } else if (strcmp(search_type, "BOTH") == 0) {
//             char item_name_lower[200];
//             strncpy(item_name_lower, item_name, sizeof(item_name_lower));
//             to_lower(item_name_lower);
//             if (strstr(item_name_lower, keyword_lower) != NULL) {
//                 match = 1; // TODO: Thêm điều kiện thời gian
//             }
//         }
        
//         if (match) {
//             total_count++;
            
//             // Lấy tên phòng
//             Room* room = get_room_by_id(room_id);
//             char room_name[100] = "Unknown";
//             if (room) {
//                 strncpy(room_name, room->room_name, sizeof(room_name));
//             }
            
//             char result_data[512];
//             snprintf(result_data, sizeof(result_data),
//                      "%d|%d|%s|%s|%.0f|%.0f|%s|%s|%s;",
//                      item_id, room_id, room_name, item_name,
//                      start_price, current_price, status,
//                      auction_start, auction_end);
//             strncat(results, result_data, sizeof(results) - strlen(results) - 1);
//         }
//     }
//     fclose(file);
    
//     char count_str[20];
//     snprintf(count_str, sizeof(count_str), "%d|", total_count);
//     strncat(response, count_str, sizeof(response) - strlen(response) - 1);
//     strncat(response, results, sizeof(response) - strlen(response) - 1);
    
//     send_message(client, response);
// }

/*
 * item_handler.c
 * Triển khai các chức năng quản lý vật phẩm đấu giá
 */

#define _XOPEN_SOURCE
#include "item_handler.h"
#include "room_handler.h"
#include "client_handler.h"
#include "server.h"
#include "timer_handler.h"
#include <ctype.h>
#include <time.h>

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
        
        // Parse theo format mới với 18 trường
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
        
        // Hỗ trợ cả format cũ (16 trường) và mới (18 trường)
        if (token_count >= 16) {
            int iid = atoi(tokens[0]);
            
            if (iid == item_id) {
                memset(&item, 0, sizeof(Item));
                
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
                item.duration = atoi(tokens[13]);
                strncpy(item.created_at, tokens[14], sizeof(item.created_at));
                
                // Format mới có scheduled_start/end
                if (token_count >= 18) {
                    strncpy(item.scheduled_start, tokens[15], sizeof(item.scheduled_start));
                    strncpy(item.scheduled_end, tokens[16], sizeof(item.scheduled_end));
                    strncpy(item.bid_history, tokens[17], sizeof(item.bid_history));
                } else {
                    // Format cũ - không có scheduled_start/end
                    item.scheduled_start[0] = '\0';
                    item.scheduled_end[0] = '\0';
                    strncpy(item.bid_history, tokens[15], sizeof(item.bid_history));
                }
                
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
            // Cập nhật dòng này với format mới bao gồm scheduled_start/end
            snprintf(lines[line_count], sizeof(lines[0]),
                     "%d|%d|%s|%s|%.0f|%.0f|%.0f|%s|%d|%.0f|%s|%s|%d|%d|%s|%s|%s|%s",
                     item->item_id, item->room_id, item->item_name, item->description,
                     item->start_price, item->current_price, item->buy_now_price,
                     item->status, item->winner_id, item->final_price,
                     item->auction_start, item->auction_end, item->extend_count,
                     item->duration, item->created_at, 
                     item->scheduled_start, item->scheduled_end,
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

// Xóa vật phẩm khỏi file
int delete_item_from_file(int item_id) {
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) return 0;
    
    char lines[1000][2048];
    int line_count = 0;
    int found = 0;
    
    // Đọc tất cả các dòng, bỏ qua dòng cần xóa
    while (fgets(lines[line_count], sizeof(lines[0]), file)) {
        lines[line_count][strcspn(lines[line_count], "\n")] = 0;
        
        int iid;
        if (sscanf(lines[line_count], "%d|", &iid) == 1 && iid == item_id) {
            found = 1;
            continue; // Bỏ qua dòng này (không tăng line_count)
        }
        line_count++;
    }
    fclose(file);
    
    if (!found) return 0;
    
    // Ghi lại file không có dòng đã xóa
    file = fopen(ITEMS_FILE, "w");
    if (file == NULL) return 0;
    
    for (int i = 0; i < line_count; i++) {
        fprintf(file, "%s\n", lines[i]);
    }
    fclose(file);
    
    return 1;
}

// Parse thời gian yyyy-mm-dd hh:mm:ss thành timestamp
static time_t parse_time_to_timestamp(const char* time_str) {
    if (!time_str || strlen(time_str) == 0) return -1;
    
    struct tm tm_info = {0};
    if (strptime(time_str, "%Y-%m-%d %H:%M:%S", &tm_info) == NULL) {
        return -1;
    }
    return mktime(&tm_info);
}

// Kiểm tra xung đột khung giờ trong cùng phòng
int check_time_slot_conflict(int room_id, const char* new_start, const char* new_end, int exclude_item_id) {
    if (!new_start || !new_end || strlen(new_start) == 0 || strlen(new_end) == 0) {
        return 0; // Không có khung giờ thì không xung đột
    }
    
    time_t new_start_time = parse_time_to_timestamp(new_start);
    time_t new_end_time = parse_time_to_timestamp(new_end);
    
    if (new_start_time < 0 || new_end_time < 0) return 0;
    if (new_start_time >= new_end_time) return 1; // Thời gian không hợp lệ
    
    FILE* file = fopen(ITEMS_FILE, "r");
    if (file == NULL) return 0;
    
    char line[2048];
    int conflict = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
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
        
        if (token_count < 17) continue;
        
        int item_id = atoi(tokens[0]);
        int item_room_id = atoi(tokens[1]);
        char* status = tokens[7];
        char* sched_start = tokens[15];
        char* sched_end = tokens[16];
        
        // Bỏ qua item đang exclude hoặc item đã SOLD/CLOSED
        if (item_id == exclude_item_id) continue;
        if (item_room_id != room_id) continue;
        if (strcmp(status, ITEM_STATUS_SOLD) == 0 || strcmp(status, ITEM_STATUS_CLOSED) == 0) continue;
        
        // Kiểm tra xung đột thời gian
        time_t exist_start = parse_time_to_timestamp(sched_start);
        time_t exist_end = parse_time_to_timestamp(sched_end);
        
        if (exist_start < 0 || exist_end < 0) continue;
        
        // Xung đột nếu: new_start < exist_end AND new_end > exist_start
        if (new_start_time < exist_end && new_end_time > exist_start) {
            printf("Time conflict detected: new [%s-%s] overlaps with item %d [%s-%s]\n",
                   new_start, new_end, item_id, sched_start, sched_end);
            conflict = 1;
            break;
        }
    }
    fclose(file);
    
    return conflict;
}

// Xử lý xóa vật phẩm
void handle_delete_item(Client* client, char* item_id_str) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    if (!item_id_str || strlen(item_id_str) == 0) {
        send_message(client, "DELETE_ITEM_FAIL|Thieu ID vat pham");
        return;
    }
    
    int item_id = atoi(item_id_str);
    Item* item = get_item_by_id(item_id);
    
    if (item == NULL) {
        send_message(client, "DELETE_ITEM_FAIL|Vat pham khong ton tai");
        return;
    }
    
    // Kiểm tra quyền: phải là chủ phòng chứa vật phẩm này
    Room* room = get_room_by_id(item->room_id);
    if (room == NULL) {
        send_message(client, "DELETE_ITEM_FAIL|Phong khong ton tai");
        return;
    }
    
    if (room->owner_id != client->user_id) {
        send_message(client, "DELETE_ITEM_FAIL|Ban khong phai chu phong, khong co quyen xoa");
        return;
    }
    
    // Kiểm tra không thể xóa item đã bán hoặc đã đóng
    if (strcmp(item->status, ITEM_STATUS_SOLD) == 0) {
        send_message(client, "DELETE_ITEM_FAIL|Khong the xoa vat pham da ban");
        return;
    }
    if (strcmp(item->status, ITEM_STATUS_CLOSED) == 0) {
        send_message(client, "DELETE_ITEM_FAIL|Khong the xoa vat pham da dong");
        return;
    }
    
    // Kiểm tra không có ai đặt giá (áp dụng cho cả PENDING và ACTIVE)
    if (strlen(item->bid_history) > 0) {
        send_message(client, "DELETE_ITEM_FAIL|Khong the xoa vat pham da co nguoi dat gia");
        return;
    }
    
    // Nếu item đang ACTIVE, cần remove timer và broadcast
    if (strcmp(item->status, ITEM_STATUS_ACTIVE) == 0) {
        remove_timer(item_id);
    }
    
    // Thực hiện xóa
    char item_name[200];
    strncpy(item_name, item->item_name, sizeof(item_name));
    
    if (delete_item_from_file(item_id)) {
        char response[256];
        snprintf(response, sizeof(response), 
                 "DELETE_ITEM_SUCCESS|Da xoa vat pham '%s' (ID: %d)", item_name, item_id);
        send_message(client, response);
        
        // Broadcast cho những người trong phòng
        char broadcast_msg[256];
        snprintf(broadcast_msg, sizeof(broadcast_msg),
                 "ITEM_DELETED|%d|%s|Vat pham da bi chu phong xoa", item_id, item_name);
        broadcast_to_room(item->room_id, broadcast_msg, client->socket_fd);
        
        printf("Item %d '%s' deleted by user %d\n", item_id, item_name, client->user_id);
    } else {
        send_message(client, "DELETE_ITEM_FAIL|Loi khi xoa vat pham");
    }
}

// Tạo vật phẩm đấu giá với khung giờ
void handle_create_item(Client* client, char* room_id_str, char* item_name, 
                        char* description, char* start_price_str, char* duration_str, 
                        char* buy_now_price_str, char* scheduled_start, char* scheduled_end) {
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }
    
    int room_id = atoi(room_id_str);
    double start_price = atof(start_price_str);
    int duration = atoi(duration_str);
    double buy_now_price = atof(buy_now_price_str);
    
    // Debug: Log parsed values
    DEBUG_LOG("[DEBUG] CREATE_ITEM parsed: room_id=%d, start_price=%.2f (str='%s'), duration=%d, buy_now=%.2f\n",
              room_id, start_price, start_price_str, duration, buy_now_price);
    
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
    
    // Xử lý khung giờ - nếu có scheduled_start/end thì kiểm tra
    char sched_start[30] = "";
    char sched_end[30] = "";
    
    if (scheduled_start && strlen(scheduled_start) > 0) {
        strncpy(sched_start, scheduled_start, sizeof(sched_start) - 1);
    }
    if (scheduled_end && strlen(scheduled_end) > 0) {
        strncpy(sched_end, scheduled_end, sizeof(sched_end) - 1);
    }
    
    // Nếu có khung giờ, kiểm tra xung đột
    if (strlen(sched_start) > 0 && strlen(sched_end) > 0) {
        // Validate format yyyy-mm-dd hh:mm:ss
        struct tm start_tm = {0}, end_tm = {0};
        if (strptime(sched_start, "%Y-%m-%d %H:%M:%S", &start_tm) == NULL ||
            strptime(sched_end, "%Y-%m-%d %H:%M:%S", &end_tm) == NULL) {
            send_message(client, "CREATE_ITEM_FAIL|Khung gio khong hop le (dinh dang yyyy-mm-dd hh:mm:ss)");
            return;
        }
        
        time_t start_time = mktime(&start_tm);
        time_t end_time = mktime(&end_tm);
        
        if (start_time >= end_time) {
            send_message(client, "CREATE_ITEM_FAIL|Gio bat dau phai truoc gio ket thuc");
            return;
        }
        
        // Kiểm tra duration có phù hợp với khung giờ không
        int slot_duration = (int)difftime(end_time, start_time);  // in seconds
        if (duration > slot_duration) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), 
                     "CREATE_ITEM_FAIL|Thoi luong %d giay vuot qua khung gio (%d giay)", 
                     duration, slot_duration);
            send_message(client, err_msg);
            return;
        }
        
        // Kiểm tra xung đột với các item khác trong phòng
        if (check_time_slot_conflict(room_id, sched_start, sched_end, -1)) {
            send_message(client, "CREATE_ITEM_FAIL|Khung gio bi trung voi vat pham khac trong phong");
            return;
        }
    }
    
    // Tạo item mới
    int new_item_id = get_last_item_id() + 1;
    
    // Lấy thời gian hiện tại làm created_at
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char created_at[30];
    strftime(created_at, sizeof(created_at), "%Y-%m-%d %H:%M:%S", tm_info);
    
    FILE* file = fopen(ITEMS_FILE, "a");
    if (file == NULL) {
        send_message(client, "CREATE_ITEM_FAIL|Loi mo file");
        return;
    }
    
    // Format mới: item_id|room_id|item_name|description|start_price|current_price|
    //             buy_now_price|status|winner_id|final_price|auction_start|auction_end|
    //             extend_count|duration|created_at|scheduled_start|scheduled_end|bid_history
    fprintf(file, "%d|%d|%s|%s|%.0f|%.0f|%.0f|%s|0|0.0|||0|%d|%s|%s|%s|\n",
            new_item_id, room_id, item_name, description ? description : "", start_price, start_price,
            buy_now_price, ITEM_STATUS_PENDING, duration, created_at,
            sched_start, sched_end);
    fclose(file);
    
    char response[256];
    if (strlen(sched_start) > 0) {
        snprintf(response, sizeof(response), 
                 "CREATE_ITEM_SUCCESS|Tao vat pham thanh cong (khung %s-%s)|%d", 
                 sched_start, sched_end, new_item_id);
    } else {
        snprintf(response, sizeof(response), 
                 "CREATE_ITEM_SUCCESS|Tao vat pham thanh cong|%d", new_item_id);
    }
    send_message(client, response);
    
    // Broadcast cho những người trong phòng
    char broadcast_msg[512];
    if (strlen(sched_start) > 0) {
        snprintf(broadcast_msg, sizeof(broadcast_msg),
                 "ITEM_CREATED|%d|%s|%.0f|%d|%s|%s|Vat pham moi '%s' da duoc them vao phong (khung gio: %s - %s)",
                 new_item_id, item_name, start_price, duration, sched_start, sched_end, 
                 item_name, sched_start, sched_end);
    } else {
        snprintf(broadcast_msg, sizeof(broadcast_msg),
                 "ITEM_CREATED|%d|%s|%.0f|%d|||Vat pham moi '%s' da duoc them vao phong",
                 new_item_id, item_name, start_price, duration, item_name);
    }
    broadcast_to_room(room_id, broadcast_msg, client->socket_fd);
    
    printf("Item %d '%s' created in room %d by user %d\n", new_item_id, item_name, room_id, client->user_id);
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
        
        item_id = atoi(tokens[0]);
        room_id = atoi(tokens[1]);
        strncpy(item_name, tokens[2], sizeof(item_name));
        start_price = atof(tokens[4]);
        current_price = atof(tokens[5]);
        strncpy(status, tokens[7], sizeof(status));
        
        // auction_start và auction_end ở vị trí 10 và 11
        if (token_count >= 18) {
            // Format mới: có đủ 18 trường
            strncpy(auction_start, tokens[10], sizeof(auction_start));
            strncpy(auction_end, tokens[11], sizeof(auction_end));
        } else {
            // Format cũ: 14 trường
            strncpy(auction_start, tokens[10], sizeof(auction_start));
            strncpy(auction_end, tokens[11], sizeof(auction_end));
        }
        
        // Hiển thị tất cả items (bao gồm cả CLOSED, SOLD) trong kết quả tìm kiếm
        // User có thể muốn xem lại các vật phẩm đã kết thúc
        
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