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
    char auction_start[30];
    char auction_end[30];
    int extend_count;
    int duration;           
    char created_at[30];    
    char bid_history[2048];
} Item;


void handle_create_item(Client* client, char* room_id_str, char* item_name, 
                        char* start_price_str, char* duration_str, char* buy_now_price_str);
void handle_search_items(Client* client, char* search_type, char* keyword, 
                         char* time_from, char* time_to);

                         
int get_last_item_id();
Item* get_item_by_id(int item_id);
int save_item(Item* item);

#endif // ITEM_HANDLER_H