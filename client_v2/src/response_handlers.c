#include "../include/response_handlers.h"
#include "../include/ui_room_list.h"
#include "../include/ui_room_detail.h"
#include <string.h>
#include <time.h>

// =============================================================
// SERVER MESSAGE HANDLERS
// =============================================================

void handle_server_message(char* message) {
    if (!message) return;
    
    if (strncmp(message, "ROOM_LIST", 9) == 0) {
        process_room_list_response(message);
    } else if (strncmp(message, "ROOM_DETAIL", 11) == 0) {
        process_room_detail_response(message);
    } else if (strncmp(message, "NEW_BID", 7) == 0) {
        char msg[512];
        int item_id;
        char bidder[50];
        double amount;
        char countdown[20];
        if (sscanf(message, "NEW_BID|%d|%49[^|]|%lf|%19s", &item_id, bidder, &amount, countdown) >= 3) {
            snprintf(msg, sizeof(msg), "💰 Đặt giá mới: %s đã đặt %.0f cho vật phẩm #%d (Còn %s)", 
                    bidder, amount, item_id, countdown);
            NotificationData *data = malloc(sizeof(NotificationData));
            strncpy(data->message, msg, sizeof(data->message));
            data->type = GTK_MESSAGE_INFO;
            g_idle_add(show_notification_ui, data);
        }
        if (g_current_room_id > 0) refresh_room_detail();
    } else if (strncmp(message, "BID_SUCCESS", 11) == 0) {
        NotificationData *data = malloc(sizeof(NotificationData));
        strncpy(data->message, "✅ Đặt giá thành công!", sizeof(data->message));
        data->type = GTK_MESSAGE_INFO;
        g_idle_add(show_notification_ui, data);
        if (g_current_room_id > 0) refresh_room_detail();
    } else if (strncmp(message, "BUY_NOW_SUCCESS", 15) == 0) {
        NotificationData *data = malloc(sizeof(NotificationData));
        strncpy(data->message, "✅ Mua ngay thành công! Bạn đã sở hữu vật phẩm này.", sizeof(data->message));
        data->type = GTK_MESSAGE_INFO;
        g_idle_add(show_notification_ui, data);
        if (g_current_room_id > 0) refresh_room_detail();
    } else if (strncmp(message, "ITEM_STARTED", 12) == 0) {
        char msg[512];
        int item_id;
        char item_name[100], duration[20];
        if (sscanf(message, "ITEM_STARTED|%d|%99[^|]|%19s", &item_id, item_name, duration) >= 2) {
            snprintf(msg, sizeof(msg), "🔔 Đấu giá bắt đầu: %s (#%d) - Thời gian: %s", 
                    item_name, item_id, duration);
            NotificationData *data = malloc(sizeof(NotificationData));
            strncpy(data->message, msg, sizeof(data->message));
            data->type = GTK_MESSAGE_WARNING;
            g_idle_add(show_notification_ui, data);
        }
        if (g_current_room_id > 0) refresh_room_detail();
    } else if (strncmp(message, "ITEM_SOLD", 9) == 0) {
        char msg[512];
        int item_id;
        char winner[50];
        double price;
        if (sscanf(message, "ITEM_SOLD|%d|%49[^|]|%lf", &item_id, winner, &price) >= 2) {
            snprintf(msg, sizeof(msg), "🏆 Đấu giá kết thúc: %s đã thắng vật phẩm #%d với giá %.0f", 
                    winner, item_id, price);
            NotificationData *data = malloc(sizeof(NotificationData));
            strncpy(data->message, msg, sizeof(data->message));
            data->type = GTK_MESSAGE_INFO;
            g_idle_add(show_notification_ui, data);
        }
        if (g_current_room_id > 0) refresh_room_detail();
    } else if (strncmp(message, "YOU_WON", 7) == 0) {
        char msg[512];
        int item_id;
        char item_name[100];
        double price;
        if (sscanf(message, "YOU_WON|%d|%99[^|]|%lf", &item_id, item_name, &price) >= 2) {
            snprintf(msg, sizeof(msg), "🎉 Chúc mừng! Bạn đã thắng đấu giá '%s' với giá %.0f!", 
                    item_name, price);
            NotificationData *data = malloc(sizeof(NotificationData));
            strncpy(data->message, msg, sizeof(data->message));
            data->type = GTK_MESSAGE_INFO;
            g_idle_add(show_notification_ui, data);
        }
        if (g_current_room_id > 0) refresh_room_detail();
    } else if (strncmp(message, "AUCTION_WARNING", 15) == 0) {
        char msg[512];
        int item_id, seconds;
        if (sscanf(message, "AUCTION_WARNING|%d|%d", &item_id, &seconds) >= 2) {
            snprintf(msg, sizeof(msg), "⏰ Cảnh báo: Vật phẩm #%d còn %d giây!", item_id, seconds);
            NotificationData *data = malloc(sizeof(NotificationData));
            strncpy(data->message, msg, sizeof(data->message));
            data->type = GTK_MESSAGE_WARNING;
            g_idle_add(show_notification_ui, data);
        }
    } else if (strncmp(message, "TIME_EXTENDED", 13) == 0) {
        char msg[512];
        int item_id;
        char new_end_time[30] = "";
        
        if (sscanf(message, "TIME_EXTENDED|%d|%29s", &item_id, new_end_time) >= 2) {
            if (strlen(new_end_time) > 0 && strcmp(new_end_time, "NULL") != 0) {
                struct tm tm_info;
                memset(&tm_info, 0, sizeof(tm_info));
                if (sscanf(new_end_time, "%d-%d-%d %d:%d:%d",
                           &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
                           &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6) {
                    tm_info.tm_year -= 1900;
                    tm_info.tm_mon -= 1;
                    time_t end_time = mktime(&tm_info);
                    
                    if (!g_item_timers) {
                        g_item_timers = g_hash_table_new(g_direct_hash, g_direct_equal);
                    }
                    g_hash_table_insert(g_item_timers, GINT_TO_POINTER(item_id), GINT_TO_POINTER((int)end_time));
                }
            }
            
            snprintf(msg, sizeof(msg), "⏱️ Thời gian đấu giá vật phẩm #%d đã được gia hạn!", item_id);
            NotificationData *data = malloc(sizeof(NotificationData));
            strncpy(data->message, msg, sizeof(data->message));
            data->type = GTK_MESSAGE_WARNING;
            g_idle_add(show_notification_ui, data);
        }
    } else if (strncmp(message, "ROOM_CLOSED", 11) == 0) {
        NotificationData *data = malloc(sizeof(NotificationData));
        strncpy(data->message, "🚪 Phòng đã đóng cửa. Bạn đã bị thoát ra.", sizeof(data->message));
        data->type = GTK_MESSAGE_WARNING;
        g_idle_add(show_notification_ui, data);
        g_current_room_id = 0;
    } else if (strncmp(message, "KICKED", 6) == 0) {
        NotificationData *data = malloc(sizeof(NotificationData));
        strncpy(data->message, "⛔ Bạn đã bị kick khỏi phòng!", sizeof(data->message));
        data->type = GTK_MESSAGE_ERROR;
        g_idle_add(show_notification_ui, data);
        g_current_room_id = 0;
    } else if (strncmp(message, "CREATE_ITEM_SUCCESS", 19) == 0) {
        NotificationData *data = malloc(sizeof(NotificationData));
        strncpy(data->message, "✅ Tạo vật phẩm thành công!", sizeof(data->message));
        data->type = GTK_MESSAGE_INFO;
        g_idle_add(show_notification_ui, data);
        if (g_current_room_id > 0) refresh_room_detail();
    } else if (strncmp(message, "DELETE_ITEM_SUCCESS", 19) == 0) {
        NotificationData *data = malloc(sizeof(NotificationData));
        strncpy(data->message, "✅ Xóa vật phẩm thành công!", sizeof(data->message));
        data->type = GTK_MESSAGE_INFO;
        g_idle_add(show_notification_ui, data);
        if (g_current_room_id > 0) refresh_room_detail();
    } else if (strncmp(message, "USER_JOINED", 11) == 0) {
        char username[50];
        if (sscanf(message, "USER_JOINED|%49[^|]", username) >= 1) {
            char msg[256];
            snprintf(msg, sizeof(msg), "👋 %s đã vào phòng", username);
            NotificationData *data = malloc(sizeof(NotificationData));
            strncpy(data->message, msg, sizeof(data->message));
            data->type = GTK_MESSAGE_INFO;
            g_idle_add(show_notification_ui, data);
        }
    } else if (strncmp(message, "USER_LEFT", 9) == 0) {
        char username[50];
        if (sscanf(message, "USER_LEFT|%49s", username) >= 1) {
            char msg[256];
            snprintf(msg, sizeof(msg), "👋 %s đã rời phòng", username);
            NotificationData *data = malloc(sizeof(NotificationData));
            strncpy(data->message, msg, sizeof(data->message));
            data->type = GTK_MESSAGE_INFO;
            g_idle_add(show_notification_ui, data);
        }
    } else if (strncmp(message, "USER_LIST", 9) == 0) {
        if (g_admin_user_store) {
            char* ptr = strchr(message, '|');
            if (ptr) {
                ptr++;
                char data_copy[BUFFER_SIZE];
                strncpy(data_copy, ptr, BUFFER_SIZE);
                
                g_idle_add((GSourceFunc)gtk_list_store_clear, g_admin_user_store);
                
                char* user = strtok(data_copy, ";");
                while (user) {
                    int id, status;
                    char username[50], role[10];
                    if (sscanf(user, "%d|%49[^|]|%d|%9s", &id, username, &status, role) >= 4) {
                        GtkTreeIter iter;
                        gtk_list_store_append(g_admin_user_store, &iter);
                        gtk_list_store_set(g_admin_user_store, &iter,
                                          0, id,
                                          1, username,
                                          2, status ? "Online" : "Offline",
                                          3, strcmp(role, "1") == 0 ? "Admin" : "User",
                                          -1);
                    }
                    user = strtok(NULL, ";");
                }
            }
        }
        NotificationData *data = malloc(sizeof(NotificationData));
        snprintf(data->message, sizeof(data->message), "📋 Đã tải danh sách người dùng");
        data->type = GTK_MESSAGE_INFO;
        g_idle_add(show_notification_ui, data);
    } else if (strncmp(message, "CREATE_ROOM_SUCCESS", 19) == 0) {
        NotificationData *data = malloc(sizeof(NotificationData));
        snprintf(data->message, sizeof(data->message), "✅ Tạo phòng thành công!");
        data->type = GTK_MESSAGE_INFO;
        g_idle_add(show_notification_ui, data);
        
        const gchar *current_page = gtk_stack_get_visible_child_name(GTK_STACK(g_stack));
        if (strcmp(current_page, "room_list") == 0) {
            refresh_room_list();
        }
    } else if (strncmp(message, "CREATE_ROOM_FAIL", 16) == 0) {
        NotificationData *data = malloc(sizeof(NotificationData));
        char* msg = strchr(message, '|');
        snprintf(data->message, sizeof(data->message), "❌ %s", 
                msg ? msg+1 : "Tạo phòng thất bại");
        data->type = GTK_MESSAGE_ERROR;
        g_idle_add(show_notification_ui, data);
    } else if (strncmp(message, "SEARCH_RESULT", 13) == 0) {
        if (g_search_result_store) {
            char* ptr = strchr(message, '|');
            if (ptr) {
                ptr++;
                // Skip count
                ptr = strchr(ptr, '|');
                if (ptr) {
                    ptr++;
                    char data_copy[BUFFER_SIZE];
                    strncpy(data_copy, ptr, BUFFER_SIZE);
                    
                    g_idle_add((GSourceFunc)gtk_list_store_clear, g_search_result_store);
                    
                    char* item = strtok(data_copy, ";");
                    while (item) {
                        int item_id, room_id;
                        char room_name[100], item_name[100], status[20];
                        double start_price, current_price;
                        if (sscanf(item, "%d|%d|%99[^|]|%99[^|]|%lf|%lf|%19s", 
                                  &item_id, &room_id, room_name, item_name,
                                  &start_price, &current_price, status) >= 6) {
                            GtkTreeIter iter;
                            gtk_list_store_append(g_search_result_store, &iter);
                            gtk_list_store_set(g_search_result_store, &iter,
                                              0, item_id,
                                              1, item_name,
                                              2, room_name,
                                              3, status,
                                              4, (int)start_price,
                                              5, (int)current_price,
                                              -1);
                        }
                        item = strtok(NULL, ";");
                    }
                }
            }
        }
        NotificationData *data = malloc(sizeof(NotificationData));
        snprintf(data->message, sizeof(data->message), "🔍 Tìm kiếm hoàn tất");
        data->type = GTK_MESSAGE_INFO;
        g_idle_add(show_notification_ui, data);
    } else if (strncmp(message, "AUCTION_HISTORY", 15) == 0) {
        if (g_history_store) {
            char* ptr = strchr(message, '|');
            if (ptr) {
                ptr++;
                // Skip count
                ptr = strchr(ptr, '|');
                if (ptr) {
                    ptr++;
                    char data_copy[BUFFER_SIZE];
                    strncpy(data_copy, ptr, BUFFER_SIZE);
                    
                    g_idle_add((GSourceFunc)gtk_list_store_clear, g_history_store);
                    
                    char* hist = strtok(data_copy, ";");
                    while (hist) {
                        int item_id;
                        char item_name[100], room_name[100], result[20];
                        double my_bid;
                        if (sscanf(hist, "%d|%99[^|]|%99[^|]|%lf|%19s",
                                  &item_id, item_name, room_name, &my_bid, result) >= 4) {
                            GtkTreeIter iter;
                            gtk_list_store_append(g_history_store, &iter);
                            gtk_list_store_set(g_history_store, &iter,
                                              0, item_id,
                                              1, item_name,
                                              2, room_name,
                                              3, (int)my_bid,
                                              4, result,
                                              -1);
                        }
                        hist = strtok(NULL, ";");
                    }
                }
            }
        }
        NotificationData *data = malloc(sizeof(NotificationData));
        snprintf(data->message, sizeof(data->message), "📜 Đã tải lịch sử đấu giá");
        data->type = GTK_MESSAGE_INFO;
        g_idle_add(show_notification_ui, data);
    } else if (strncmp(message, "BID_ERROR", 9) == 0 || 
              strncmp(message, "ERROR", 5) == 0) {
        char* msg_ptr = strchr(message, '|');
        if (msg_ptr) {
            msg_ptr++;
            NotificationData *data = malloc(sizeof(NotificationData));
            snprintf(data->message, sizeof(data->message), "❌ %s", msg_ptr);
            data->type = GTK_MESSAGE_ERROR;
            g_idle_add(show_notification_ui, data);
        }
    }
}
