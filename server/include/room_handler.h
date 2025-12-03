/*
 * room_handler.h
 * Quản lý phòng đấu giá
 */

#ifndef ROOM_HANDLER_H
#define ROOM_HANDLER_H

#include "server.h"

// Định nghĩa trạng thái phòng
#define ROOM_STATUS_PENDING "PENDING"
#define ROOM_STATUS_ACTIVE "ACTIVE"
#define ROOM_STATUS_CLOSED "CLOSED"

// Cấu trúc phòng đấu giá
typedef struct {
    int room_id;
    char room_name[100];
    int owner_id;
    char status[20];
    char start_time[30];
    char end_time[30];
} Room;

// Hàm xử lý các lệnh liên quan đến phòng
void handle_create_room(Client* client, char* room_name, char* start_time, char* end_time);
void handle_get_room_list(Client* client, char* status, char* page_str, char* limit_str);
void handle_join_room(Client* client, char* room_id_str);
void handle_leave_room(Client* client);
void handle_get_room_detail(Client* client, char* room_id_str);
void handle_start_auction(Client* client, char* room_id_str); 

// Hàm hỗ trợ
int get_last_room_id();
int count_items_in_room(int room_id);
int count_participants_in_room(int room_id);
int is_room_owner(int user_id, int room_id);
Room* get_room_by_id(int room_id);
void broadcast_to_room(int room_id, const char* message, int exclude_fd);
int update_room_status(int room_id, const char* new_status);  
void check_and_update_room_statuses();  
#endif // ROOM_HANDLER_H