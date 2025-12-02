/*
 * auction_handler.h
 * Xử lý các chức năng đấu giá
 */

#ifndef AUCTION_HANDLER_H
#define AUCTION_HANDLER_H

#include "server.h"

// Hàm xử lý đấu giá
void handle_place_bid(Client* client, char* item_id_str, char* bid_amount_str);
void handle_buy_now(Client* client, char* item_id_str);
void handle_get_my_auction_history(Client* client, char* filter, char* page_str, char* limit_str);

// Hàm hỗ trợ bid
int get_highest_bidder_from_history(const char* bid_history, int* user_id_out, double* amount_out);
int add_bid_to_history(char* bid_history, int user_id, double amount);

#endif // AUCTION_HANDLER_H