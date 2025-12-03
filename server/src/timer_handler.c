// /*
//  * timer_handler.c
//  * Triển khai hệ thống timer cho đấu giá tự động
//  */

// #include "timer_handler.h"
// #include "item_handler.h"
// #include "room_handler.h"
// #include "client_handler.h"
// #include "auction_handler.h"

// #include <stdlib.h>
// #include <string.h>
// extern const char* ITEMS_FILE;
// TimerNode* g_timer_list = NULL;

// void init_timer_system() {
//     g_timer_list = NULL;
//    // printf("Timer system initialized\n");
// }

// void cleanup_timer_system() {
//     TimerNode* current = g_timer_list;
//     while (current) {
//         TimerNode* next = current->next;
//         free(current);
//         current = next;
//     }
//     g_timer_list = NULL;
// }

// void add_timer(int item_id, int room_id, time_t end_time) {
//     TimerNode* new_timer = (TimerNode*)malloc(sizeof(TimerNode));
//     new_timer->item_id = item_id;
//     new_timer->room_id = room_id;
//     new_timer->end_time = end_time;
//     new_timer->warning_sent = 0;
//     new_timer->next = g_timer_list;
//     g_timer_list = new_timer;
    
//     printf("Timer added for item %d, end_time: %ld\n", item_id, end_time);
// }

// void remove_timer(int item_id) {
//     TimerNode** current = &g_timer_list;
//     while (*current) {
//         if ((*current)->item_id == item_id) {
//             TimerNode* to_delete = *current;
//             *current = (*current)->next;
//             free(to_delete);
//             printf("Timer removed for item %d\n", item_id);
//             return;
//         }
//         current = &((*current)->next);
//     }
// }

// void update_timer(int item_id, time_t new_end_time) {
//     TimerNode* timer = find_timer(item_id);
//     if (timer) {
//         timer->end_time = new_end_time;
//         timer->warning_sent = 0; // Reset warning flag
//         printf("Timer updated for item %d, new end_time: %ld\n", item_id, new_end_time);
//     }
// }

// TimerNode* find_timer(int item_id) {
//     TimerNode* current = g_timer_list;
//     while (current) {
//         if (current->item_id == item_id) {
//             return current;
//         }
//         current = current->next;
//     }
//     return NULL;
// }

// void handle_auction_timeout(int item_id) {
//     Item* item = get_item_by_id(item_id);
//     if (!item) return;
    
//     printf("Auction timeout for item %d: %s\n", item_id, item->item_name);
    
//     int winner_id = 0;
//     double final_price = 0.0;
//     get_highest_bidder_from_history(item->bid_history, &winner_id, &final_price);
    
//     if (winner_id > 0) {
//         strncpy(item->status, ITEM_STATUS_SOLD, sizeof(item->status));
//         item->winner_id = winner_id;
//         item->final_price = final_price;
//     } else {
//         strncpy(item->status, ITEM_STATUS_CLOSED, sizeof(item->status));
//     }
    
//     save_item(item);
    
//     char winner_name[MAX_USERNAME] = "NONE";
//     if (winner_id > 0) {
//         FILE* file = fopen(DB_FILE, "r");
//         if (file) {
//             char line[256];
//             while (fgets(line, sizeof(line), file)) {
//                 int uid;
//                 char uname[MAX_USERNAME];
//                 if (sscanf(line, "%d|%49[^|]|", &uid, uname) == 2) {
//                     if (uid == winner_id) {
//                         strncpy(winner_name, uname, sizeof(winner_name));
//                         break;
//                     }
//                 }
//             }
//             fclose(file);
//         }
//     }
    
//     char broadcast_msg[512];
//     snprintf(broadcast_msg, sizeof(broadcast_msg),
//              "AUCTION_ENDED|%d|%s|%s|%.0f|TIMEOUT",
//              item_id, item->item_name, winner_name, final_price);
//     broadcast_to_room(item->room_id, broadcast_msg, -1);
    
//     if (winner_id > 0) {
//         for (int i = 0; i < MAX_CLIENTS; i++) {
//             if (g_clients[i] && g_clients[i]->user_id == winner_id) {
//                 char winner_msg[512];
//                 snprintf(winner_msg, sizeof(winner_msg),
//                          "YOU_WON|%d|%s|%.0f|Chuc mung ban da thang dau gia",
//                          item_id, item->item_name, final_price);
//                 send_message(g_clients[i], winner_msg);
//                 break;
//             }
//         }
//     }
    
//     remove_timer(item_id);
    
//     activate_next_item_in_room(item->room_id);
// }

// void activate_next_item_in_room(int room_id) {
//     FILE* file = fopen(ITEMS_FILE, "r");
//     if (!file) return;
    
//     char line[2048];
//     int next_item_id = -1;
//     time_t earliest_created = 0;
    
//     while (fgets(line, sizeof(line), file)) {
//         line[strcspn(line, "\n")] = 0;
        
//         char* tokens[14];
//         char* ptr = line;
//         for (int i = 0; i < 14 && ptr; i++) {
//             tokens[i] = ptr;
//             ptr = strchr(ptr, '|');
//             if (ptr) {
//                 *ptr = '\0';
//                 ptr++;
//             }
//         }
        
//         int item_id = atoi(tokens[0]);
//         int item_room_id = atoi(tokens[1]);
//         char* status = tokens[7];
        
//         if (item_room_id == room_id && strcmp(status, ITEM_STATUS_PENDING) == 0) {
//             // TODO: So sánh created_at để tìm item sớm nhất
//             next_item_id = item_id;
//             break; // Tạm thời lấy item đầu tiên
//         }
//     }
//     fclose(file);
    
//     if (next_item_id > 0) {
//         Item* item = get_item_by_id(next_item_id);
//         if (item) {
//             // Chuyển sang ACTIVE
//             strncpy(item->status, ITEM_STATUS_ACTIVE, sizeof(item->status));
            
//             // Set thời gian bắt đầu và kết thúc
//             time_t now = time(NULL);
//             struct tm* tm_info = localtime(&now);
//             strftime(item->auction_start, sizeof(item->auction_start), 
//                      "%Y-%m-%d %H:%M:%S", tm_info);
            
//             // Giả sử duration được lưu ở đâu đó, tạm thời dùng 60 phút
//             time_t end_time = now + (60 * 60); // 60 minutes
//             tm_info = localtime(&end_time);
//             strftime(item->auction_end, sizeof(item->auction_end),
//                      "%Y-%m-%d %H:%M:%S", tm_info);
            
//             save_item(item);
            
//             // Thêm timer
//             add_timer(next_item_id, room_id, end_time);
            
//             // Broadcast item mới bắt đầu
//             char msg[512];
//             snprintf(msg, sizeof(msg),
//                      "ITEM_STARTED|%d|%s|%.0f|%s",
//                      item->item_id, item->item_name, 
//                      item->start_price, item->auction_end);
//             broadcast_to_room(room_id, msg, -1);
            
//             printf("Next item activated: %d in room %d\n", next_item_id, room_id);
//         }
//     }
// }

// void check_and_send_warnings() {
//     time_t now = time(NULL);
//     TimerNode* current = g_timer_list;
    
//     while (current) {
//         int remaining = (int)difftime(current->end_time, now);
        
//         // Gửi cảnh báo khi còn 30s và chưa gửi
//         if (remaining <= 30 && remaining > 0 && !current->warning_sent) {
//             Item* item = get_item_by_id(current->item_id);
//             if (item && strcmp(item->status, ITEM_STATUS_ACTIVE) == 0) {
//                 char warning_msg[512];
//                 snprintf(warning_msg, sizeof(warning_msg),
//                          "AUCTION_WARNING|%d|%s|%d",
//                          item->item_id, item->item_name, remaining);
//                 broadcast_to_room(current->room_id, warning_msg, -1);
//                 current->warning_sent = 1;
//                 printf("Warning sent for item %d, %d seconds remaining\n", 
//                        current->item_id, remaining);
//             }
//         }
        
//         current = current->next;
//     }
// }

// void process_timers() {
//     time_t now = time(NULL);
//     TimerNode* current = g_timer_list;
    
//     // Kiểm tra các timer đã hết hạn
//     while (current) {
//         TimerNode* next = current->next;
        
//         if (difftime(now, current->end_time) >= 0) {
//             // Timer hết hạn
//             handle_auction_timeout(current->item_id);
//         }
        
//         current = next;
//     }
    
//     check_and_send_warnings();
// }

/*
 * timer_handler.c
 * Triển khai hệ thống timer cho đấu giá tự động
 */

#include "timer_handler.h"
#include "item_handler.h"
#include "room_handler.h"
#include "client_handler.h"
#include "auction_handler.h"

#include <stdlib.h>
#include <string.h>
extern const char* ITEMS_FILE;
TimerNode* g_timer_list = NULL;

void init_timer_system() {
    g_timer_list = NULL;
   // printf("Timer system initialized\n");
}

void cleanup_timer_system() {
    TimerNode* current = g_timer_list;
    while (current) {
        TimerNode* next = current->next;
        free(current);
        current = next;
    }
    g_timer_list = NULL;
}

void add_timer(int item_id, int room_id, time_t end_time) {
    TimerNode* new_timer = (TimerNode*)malloc(sizeof(TimerNode));
    new_timer->item_id = item_id;
    new_timer->room_id = room_id;
    new_timer->end_time = end_time;
    new_timer->warning_sent = 0;
    new_timer->next = g_timer_list;
    g_timer_list = new_timer;
    
    printf("Timer added for item %d, end_time: %ld\n", item_id, end_time);
}

void remove_timer(int item_id) {
    TimerNode** current = &g_timer_list;
    while (*current) {
        if ((*current)->item_id == item_id) {
            TimerNode* to_delete = *current;
            *current = (*current)->next;
            free(to_delete);
            printf("Timer removed for item %d\n", item_id);
            return;
        }
        current = &((*current)->next);
    }
}

void update_timer(int item_id, time_t new_end_time) {
    TimerNode* timer = find_timer(item_id);
    if (timer) {
        timer->end_time = new_end_time;
        timer->warning_sent = 0; // Reset warning flag
        printf("Timer updated for item %d, new end_time: %ld\n", item_id, new_end_time);
    }
}

TimerNode* find_timer(int item_id) {
    TimerNode* current = g_timer_list;
    while (current) {
        if (current->item_id == item_id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void handle_auction_timeout(int item_id) {
    Item* item = get_item_by_id(item_id);
    if (!item) return;
    
    printf("Auction timeout for item %d: %s\n", item_id, item->item_name);
    
    // Tìm người thắng từ bid_history
    int winner_id = 0;
    double final_price = 0.0;
    get_highest_bidder_from_history(item->bid_history, &winner_id, &final_price);
    
    // Cập nhật item
    if (winner_id > 0) {
        strncpy(item->status, ITEM_STATUS_SOLD, sizeof(item->status));
        item->winner_id = winner_id;
        item->final_price = final_price;
    } else {
        strncpy(item->status, ITEM_STATUS_CLOSED, sizeof(item->status));
    }
    
    save_item(item);
    
    // Lấy username của winner
    char winner_name[MAX_USERNAME] = "NONE";
    if (winner_id > 0) {
        FILE* file = fopen(DB_FILE, "r");
        if (file) {
            char line[256];
            while (fgets(line, sizeof(line), file)) {
                int uid;
                char uname[MAX_USERNAME];
                if (sscanf(line, "%d|%49[^|]|", &uid, uname) == 2) {
                    if (uid == winner_id) {
                        strncpy(winner_name, uname, sizeof(winner_name));
                        break;
                    }
                }
            }
            fclose(file);
        }
    }
    
    // Broadcast kết thúc đấu giá
    char broadcast_msg[512];
    snprintf(broadcast_msg, sizeof(broadcast_msg),
             "AUCTION_ENDED|%d|%s|%s|%.0f|TIMEOUT",
             item_id, item->item_name, winner_name, final_price);
    broadcast_to_room(item->room_id, broadcast_msg, -1);
    
    // Gửi thông báo riêng cho winner
    if (winner_id > 0) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (g_clients[i] && g_clients[i]->user_id == winner_id) {
                char winner_msg[512];
                snprintf(winner_msg, sizeof(winner_msg),
                         "YOU_WON|%d|%s|%.0f|Chuc mung ban da thang dau gia",
                         item_id, item->item_name, final_price);
                send_message(g_clients[i], winner_msg);
                break;
            }
        }
    }
    
    // Xóa timer
    remove_timer(item_id);
    
    // Kích hoạt item tiếp theo
    activate_next_item_in_room(item->room_id);
}

void activate_next_item_in_room(int room_id) {
    FILE* file = fopen(ITEMS_FILE, "r");
    if (!file) return;
    
    char line[2048];
    int next_item_id = -1;
    time_t earliest_created = 0;
    int next_item_duration = 60; // Default 60 phút
    
    // Tìm item PENDING sớm nhất theo created_at
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        char* tokens[17];
        char* ptr = line;
        int token_count = 0;
        for (int i = 0; i < 17 && ptr; i++) {
            tokens[i] = ptr;
            ptr = strchr(ptr, '|');
            if (ptr) {
                *ptr = '\0';
                ptr++;
            }
            token_count++;
        }
        
        if (token_count < 16) continue;
        
        int item_id = atoi(tokens[0]);
        int item_room_id = atoi(tokens[1]);
        char* status = tokens[7];
        int duration = atoi(tokens[13]);
        char* created_at = tokens[14];
        
        if (item_room_id == room_id && strcmp(status, ITEM_STATUS_PENDING) == 0) {
            // Parse created_at để so sánh
            struct tm tm_info;
            memset(&tm_info, 0, sizeof(tm_info));
            if (sscanf(created_at, "%d-%d-%d %d:%d:%d",
                       &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
                       &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6) {
                tm_info.tm_year -= 1900;
                tm_info.tm_mon -= 1;
                time_t created_time = mktime(&tm_info);
                
                // Nếu là item đầu tiên hoặc tạo sớm hơn
                if (next_item_id == -1 || created_time < earliest_created) {
                    next_item_id = item_id;
                    earliest_created = created_time;
                    next_item_duration = (duration > 0) ? duration : 60;
                }
            } else {
                // Nếu không parse được, lấy item đầu tiên
                if (next_item_id == -1) {
                    next_item_id = item_id;
                    next_item_duration = (duration > 0) ? duration : 60;
                }
            }
        }
    }
    fclose(file);
    
    if (next_item_id > 0) {
        Item* item = get_item_by_id(next_item_id);
        if (item) {
            // Chuyển sang ACTIVE
            strncpy(item->status, ITEM_STATUS_ACTIVE, sizeof(item->status));
            
            // Set thời gian bắt đầu và kết thúc
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            strftime(item->auction_start, sizeof(item->auction_start), 
                     "%Y-%m-%d %H:%M:%S", tm_info);
            
            // Sử dụng duration từ item (phút)
            time_t end_time = now + (next_item_duration * 60);
            tm_info = localtime(&end_time);
            strftime(item->auction_end, sizeof(item->auction_end),
                     "%Y-%m-%d %H:%M:%S", tm_info);
            
            save_item(item);
            
            // Thêm timer
            add_timer(next_item_id, room_id, end_time);
            
            // Broadcast item mới bắt đầu
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "ITEM_STARTED|%d|%s|%.0f|%s|%d",
                     item->item_id, item->item_name, 
                     item->start_price, item->auction_end, next_item_duration);
            broadcast_to_room(room_id, msg, -1);
            
            printf("Next item activated: %d in room %d, duration: %d min\n", 
                   next_item_id, room_id, next_item_duration);
        }
    }
}

void check_and_send_warnings() {
    time_t now = time(NULL);
    TimerNode* current = g_timer_list;
    
    while (current) {
        int remaining = (int)difftime(current->end_time, now);
        
        // Gửi cảnh báo khi còn 30s và chưa gửi
        if (remaining <= 30 && remaining > 0 && !current->warning_sent) {
            Item* item = get_item_by_id(current->item_id);
            if (item && strcmp(item->status, ITEM_STATUS_ACTIVE) == 0) {
                char warning_msg[512];
                snprintf(warning_msg, sizeof(warning_msg),
                         "AUCTION_WARNING|%d|%s|%d",
                         item->item_id, item->item_name, remaining);
                broadcast_to_room(current->room_id, warning_msg, -1);
                current->warning_sent = 1;
                printf("Warning sent for item %d, %d seconds remaining\n", 
                       current->item_id, remaining);
            }
        }
        
        current = current->next;
    }
}

void process_timers() {
    time_t now = time(NULL);
    TimerNode* current = g_timer_list;
    
    // Kiểm tra các timer đã hết hạn
    while (current) {
        TimerNode* next = current->next;
        
        if (difftime(now, current->end_time) >= 0) {
            // Timer hết hạn
            handle_auction_timeout(current->item_id);
        }
        
        current = next;
    }
    
    // Kiểm tra và gửi cảnh báo
    check_and_send_warnings();
}