/*
 * timer_handler.h
 * Quản lý timer cho đấu giá tự động
 */

#ifndef TIMER_HANDLER_H
#define TIMER_HANDLER_H

#include "server.h"
#include <time.h>

// Cấu trúc timer cho mỗi item
typedef struct TimerNode {
    int item_id;
    time_t end_time;
    int warning_sent;           // Đã gửi cảnh báo 30s chưa
    int room_id;
    struct TimerNode* next;
} TimerNode;

// Danh sách timer toàn cục
extern TimerNode* g_timer_list;

// Khởi tạo và dọn dẹp
void init_timer_system();
void cleanup_timer_system();

// Quản lý timer
void add_timer(int item_id, int room_id, time_t end_time);
void remove_timer(int item_id);
void update_timer(int item_id, time_t new_end_time);
TimerNode* find_timer(int item_id);

// Xử lý timer (gọi trong main loop)
void process_timers();

// Xử lý kết thúc đấu giá
void handle_auction_timeout(int item_id);

// Kích hoạt item tiếp theo trong queue
void activate_next_item_in_room(int room_id);

// Kiểm tra và gửi cảnh báo 30s
void check_and_send_warnings();

// Kiểm tra và kích hoạt items theo scheduled_start
void check_scheduled_items();

#endif // TIMER_HANDLER_H