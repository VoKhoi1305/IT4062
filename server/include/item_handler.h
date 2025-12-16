/*
 * item_handler.h
 * Quản lý vật phẩm đấu giá
 */

#ifndef ITEM_HANDLER_H
#define ITEM_HANDLER_H

#include "server.h"

// Định nghĩa trạng thái vật phẩm
#define ITEM_STATUS_PENDING "PENDING"
#define ITEM_STATUS_ACTIVE "ACTIVE"
#define ITEM_STATUS_SOLD "SOLD"
#define ITEM_STATUS_CLOSED "CLOSED"

// Cấu trúc vật phẩm
typedef struct {
    int item_id;
    int room_id;
    char item_name[200];
    char description[500];
    double start_price;
    double current_price;
    double buy_now_price;
    char status[20];
    int winner_id;
    double final_price;
    char auction_start[30];      // Thời gian bắt đầu thực tế (khi item ACTIVE)
    char auction_end[30];        // Thời gian kết thúc thực tế
    int extend_count;
    int duration;                // Thời lượng đấu giá (phút)
    char created_at[30];    
    char scheduled_start[30];    // Khung giờ bắt đầu đấu giá (VD: 10:00)
    char scheduled_end[30];      // Khung giờ kết thúc đấu giá (VD: 11:00)
    char bid_history[2048];
} Item;


void handle_create_item(Client* client, char* room_id_str, char* item_name, 
                        char* start_price_str, char* duration_str, char* buy_now_price_str,
                        char* scheduled_start, char* scheduled_end);
void handle_delete_item(Client* client, char* item_id_str);
void handle_search_items(Client* client, char* search_type, char* keyword, 
                         char* time_from, char* time_to);

int check_time_slot_conflict(int room_id, const char* start_time, const char* end_time, int exclude_item_id);
int get_last_item_id();
Item* get_item_by_id(int item_id);
int save_item(Item* item);
int delete_item_from_file(int item_id);

#endif // ITEM_HANDLER_H