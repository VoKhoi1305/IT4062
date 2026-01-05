/*
 * CLIENT GTK - Giao diện đồ họa cho hệ thống đấu giá trực tuyến
 * Sử dụng GTK+ 3.0 để tạo GUI cho client kết nối với auction server
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

// =============================================================
// CONFIGURATION & CONSTANTS
// =============================================================
#define PORT 8080
#define BUFFER_SIZE 4096
#define REFRESH_INTERVAL 1000  // ms

// =============================================================
// GLOBAL STATE
// =============================================================
int g_socket_fd = -1;
int g_is_logged_in = 0;
char g_username[50] = "";
int g_user_role = 0;  // 0 = user, 1 = admin
int g_current_room_id = 0;
char g_current_room_name[100] = "";

// GTK Widgets (Global References)
GtkWidget *g_main_window = NULL;
GtkWidget *g_stack = NULL;
GtkWidget *g_status_bar = NULL;

// Login/Register widgets
GtkWidget *g_login_username_entry = NULL;
GtkWidget *g_login_password_entry = NULL;
GtkWidget *g_register_username_entry = NULL;
GtkWidget *g_register_password_entry = NULL;

// Room list widgets
GtkWidget *g_room_list_view = NULL;
GtkListStore *g_room_list_store = NULL;
GtkWidget *g_admin_button = NULL;  // Admin button to show/hide

// Room detail widgets
GtkWidget *g_room_detail_view = NULL;
GtkListStore *g_room_detail_store = NULL;
GtkWidget *g_room_info_label = NULL;
GtkWidget *g_notification_bar = NULL;

// Room detail buttons (to show/hide based on role)
GtkWidget *g_bid_button = NULL;
GtkWidget *g_buy_button = NULL;
GtkWidget *g_create_item_button = NULL;
GtkWidget *g_delete_item_button = NULL;
int g_is_room_owner = 0;  // 1 if current user is room owner, 0 otherwise

// User info widget
GtkWidget *g_user_info_label = NULL;

// Dialog widgets (for populating from receiver thread)
GtkListStore *g_search_result_store = NULL;
GtkListStore *g_history_store = NULL;
GtkListStore *g_admin_user_store = NULL;

// Thread control
pthread_t g_receiver_thread;
pthread_mutex_t g_socket_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_thread_running = 0;

// Join room protection flag
volatile int g_joining_room = 0;

// Item timing data structure for countdown
typedef struct {
    int item_id;
    time_t end_time;
} ItemTimer;

// Global hash table to store item end times (key: item_id, value: end_time)
GHashTable *g_item_timers = NULL;
guint g_countdown_timer_id = 0;

// =============================================================
// NETWORK UTILITIES
// =============================================================

void send_command(const char* cmd) {
    pthread_mutex_lock(&g_socket_mutex);
    if (g_socket_fd >= 0) {
        char buffer[BUFFER_SIZE];
        snprintf(buffer, BUFFER_SIZE, "%s\n", cmd);
        send(g_socket_fd, buffer, strlen(buffer), 0);
    }
    pthread_mutex_unlock(&g_socket_mutex);
}

char* wait_for_response_sync() {
    static char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    
    pthread_mutex_lock(&g_socket_mutex);
    if (g_socket_fd < 0) {
        pthread_mutex_unlock(&g_socket_mutex);
        return NULL;
    }
    
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    
    FD_ZERO(&read_fds);
    FD_SET(g_socket_fd, &read_fds);
    
    int activity = select(g_socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (activity > 0 && FD_ISSET(g_socket_fd, &read_fds)) {
        int bytes = recv(g_socket_fd, buffer, BUFFER_SIZE - 1, 0);
        pthread_mutex_unlock(&g_socket_mutex);
        
        if (bytes <= 0) return NULL;
        
        buffer[bytes] = '\0';
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        strncpy(response, buffer, BUFFER_SIZE);
        return response;
    }
    
    pthread_mutex_unlock(&g_socket_mutex);
    return NULL;
}

// =============================================================
// UI UPDATE FUNCTIONS (thread-safe)
// =============================================================

// Helper function to create date/time picker widget
GtkWidget* create_datetime_picker(const char* label_text, char* output_buffer, int buffer_size) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    GtkWidget *label = gtk_label_new(label_text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    // Create horizontal box for date and time
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    // Date entry
    GtkWidget *date_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(date_entry), "YYYY-MM-DD");
    gtk_entry_set_width_chars(GTK_ENTRY(date_entry), 12);
    
    // Time spinners
    GtkWidget *hour_spin = gtk_spin_button_new_with_range(0, 23, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hour_spin), 0);
    
    GtkWidget *minute_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minute_spin), 0);
    
    GtkWidget *second_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(second_spin), 0);
    
    GtkWidget *colon1 = gtk_label_new(":");
    GtkWidget *colon2 = gtk_label_new(":");
    
    gtk_box_pack_start(GTK_BOX(hbox), date_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hour_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), colon1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), minute_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), colon2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), second_spin, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    // Store widget references in the vbox data
    g_object_set_data(G_OBJECT(vbox), "date_entry", date_entry);
    g_object_set_data(G_OBJECT(vbox), "hour_spin", hour_spin);
    g_object_set_data(G_OBJECT(vbox), "minute_spin", minute_spin);
    g_object_set_data(G_OBJECT(vbox), "second_spin", second_spin);
    
    return vbox;
}

// Helper function to extract datetime string from picker widget
void get_datetime_from_picker(GtkWidget *picker_vbox, char* output_buffer, int buffer_size) {
    GtkWidget *date_entry = g_object_get_data(G_OBJECT(picker_vbox), "date_entry");
    GtkWidget *hour_spin = g_object_get_data(G_OBJECT(picker_vbox), "hour_spin");
    GtkWidget *minute_spin = g_object_get_data(G_OBJECT(picker_vbox), "minute_spin");
    GtkWidget *second_spin = g_object_get_data(G_OBJECT(picker_vbox), "second_spin");
    
    const char* date = gtk_entry_get_text(GTK_ENTRY(date_entry));
    int hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(hour_spin));
    int minute = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(minute_spin));
    int second = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(second_spin));
    
    if (strlen(date) > 0) {
        // Parse and normalize date format (ensure YYYY-MM-DD with leading zeros)
        int year, month, day;
        if (sscanf(date, "%d-%d-%d", &year, &month, &day) == 3) {
            snprintf(output_buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d", 
                    year, month, day, hour, minute, second);
        } else {
            // If parse fails, use original (might be invalid, but let server validate)
            snprintf(output_buffer, buffer_size, "%s %02d:%02d:%02d", date, hour, minute, second);
        }
    } else {
        output_buffer[0] = '\0';
    }
}

// Helper function to format countdown time
void format_countdown(time_t end_time, char* buffer, size_t buffer_size) {
    time_t now = time(NULL);
    int remaining = (int)difftime(end_time, now);
    
    if (remaining <= 0) {
        snprintf(buffer, buffer_size, "⏰ Đã hết hạn");
        return;
    }
    
    int hours = remaining / 3600;
    int minutes = (remaining % 3600) / 60;
    int seconds = remaining % 60;
    
    if (hours > 0) {
        snprintf(buffer, buffer_size, "⏱️ %02d:%02d:%02d", hours, minutes, seconds);
    } else {
        snprintf(buffer, buffer_size, "⏱️ %02d:%02d", minutes, seconds);
    }
}

// Countdown timer callback - updates all item countdowns every second
gboolean update_countdown_timer(gpointer user_data) {
    if (!g_room_detail_store || !g_item_timers) {
        return TRUE; // Keep timer running
    }
    
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_room_detail_store), &iter);
    
    while (valid) {
        int item_id;
        gtk_tree_model_get(GTK_TREE_MODEL(g_room_detail_store), &iter, 0, &item_id, -1);
        
        // Look up end_time for this item
        gpointer end_time_ptr = g_hash_table_lookup(g_item_timers, GINT_TO_POINTER(item_id));
        
        if (end_time_ptr) {
            time_t end_time = GPOINTER_TO_INT(end_time_ptr);
            char countdown_str[50];
            format_countdown(end_time, countdown_str, sizeof(countdown_str));
            
            gtk_list_store_set(g_room_detail_store, &iter, 6, countdown_str, -1);
        }
        
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(g_room_detail_store), &iter);
    }
    
    return TRUE; // Keep timer running
}

void update_status_bar(const char* message) {
    if (g_status_bar) {
        gtk_statusbar_push(GTK_STATUSBAR(g_status_bar), 0, message);
    }
}

void show_message_dialog(GtkMessageType type, const char* title, const char* message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_main_window),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               type,
                                               GTK_BUTTONS_OK,
                                               "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void show_error_dialog(const char* message) {
    show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi", message);
}

void show_success_dialog(const char* message) {
    show_message_dialog(GTK_MESSAGE_INFO, "Thành công", message);
}

void show_notification(const char* message, GtkMessageType type) {
    if (!g_notification_bar) return;
    
    gtk_info_bar_set_message_type(GTK_INFO_BAR(g_notification_bar), type);
    
    GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(g_notification_bar));
    GList *children = gtk_container_get_children(GTK_CONTAINER(content));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_container_add(GTK_CONTAINER(content), label);
    gtk_widget_show_all(g_notification_bar);
    gtk_widget_show(g_notification_bar);
}

// =============================================================
// ROOM LIST FUNCTIONS
// =============================================================

void refresh_room_list() {
    send_command("GET_ROOM_LIST|ALL|1|50");
    // Response will be handled by receiver thread
}

// =============================================================
// ROOM DETAIL FUNCTIONS
// =============================================================

void refresh_room_detail() {
    if (g_current_room_id == 0) return;
    
    char cmd[50];
    snprintf(cmd, sizeof(cmd), "GET_ROOM_DETAIL|%d", g_current_room_id);
    send_command(cmd);
}

typedef struct {
    int room_id;
    char room_name[100];
    char status[20];
    char start_time[30];
    char end_time[30];
    char items_data[BUFFER_SIZE];
} RoomDetailData;

typedef struct {
    char message[512];
    GtkMessageType type;
} NotificationData;

gboolean update_room_detail_ui(gpointer user_data) {
    RoomDetailData *data = (RoomDetailData*)user_data;
    
    // Safety check - ensure widgets exist
    if (!g_room_detail_store || !g_room_info_label) {
        free(data);
        return FALSE;
    }
    
    // Update room info label
    char info[512];
    snprintf(info, sizeof(info), 
             "<b>Phòng:</b> %s (#%d)\n<b>Trạng thái:</b> %s\n<b>Thời gian:</b> %s → %s",
             data->room_name, data->room_id, data->status, data->start_time, data->end_time);
    gtk_label_set_markup(GTK_LABEL(g_room_info_label), info);
    
    // Clear and update items
    gtk_list_store_clear(g_room_detail_store);
    
    // Parse items with color-coding by status
    if (strlen(data->items_data) > 0) {
        char items_buf[BUFFER_SIZE];
        strncpy(items_buf, data->items_data, BUFFER_SIZE);
        
        char* item = strtok(items_buf, ";");
        while (item) {
            int item_id;
            char item_name[100], item_status[20];
            double start_price, current_price, buy_now_price;
            char auction_start[30] = "", auction_end[30] = "";
            char sched_start[30] = "", sched_end[30] = "";
            int duration = 0;
            
            // Parse format: item_id|name|status|start_price|current_price|buy_now|auction_start|auction_end|sched_start|sched_end|duration
            int parsed = sscanf(item, "%d|%99[^|]|%19[^|]|%lf|%lf|%lf|%29[^|]|%29[^|]|%29[^|]|%29[^|]|%d", 
                       &item_id, item_name, item_status, 
                       &start_price, &current_price, &buy_now_price,
                       auction_start, auction_end, sched_start, sched_end, &duration);
            
            if (parsed >= 6) {
                // Show ACTIVE and PENDING items, hide SOLD and CLOSED
                if (strcmp(item_status, "SOLD") == 0 || strcmp(item_status, "CLOSED") == 0) {
                    item = strtok(NULL, ";");
                    continue;
                }
                
                // Parse and store auction_end time (only for ACTIVE items)
                if (strcmp(item_status, "ACTIVE") == 0 && strlen(auction_end) > 0 && strcmp(auction_end, "NULL") != 0) {
                    struct tm tm_info;
                    memset(&tm_info, 0, sizeof(tm_info));
                    if (sscanf(auction_end, "%d-%d-%d %d:%d:%d",
                               &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
                               &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6) {
                        tm_info.tm_year -= 1900;
                        tm_info.tm_mon -= 1;
                        time_t end_time = mktime(&tm_info);
                        
                        // Store in hash table
                        if (!g_item_timers) {
                            g_item_timers = g_hash_table_new(g_direct_hash, g_direct_equal);
                        }
                        g_hash_table_insert(g_item_timers, GINT_TO_POINTER(item_id), GINT_TO_POINTER((int)end_time));
                    }
                }
                
                GtkTreeIter iter;
                gtk_list_store_append(g_room_detail_store, &iter);
                
                // Color-code status: ACTIVE=green, PENDING=yellow, SOLD=red, CLOSED=gray
                char status_display[50];
                if (strcmp(item_status, "ACTIVE") == 0) {
                    snprintf(status_display, sizeof(status_display), "🟢 %s", item_status);
                } else if (strcmp(item_status, "PENDING") == 0) {
                    snprintf(status_display, sizeof(status_display), "🟡 %s", item_status);
                } else if (strcmp(item_status, "SOLD") == 0) {
                    snprintf(status_display, sizeof(status_display), "🔴 %s", item_status);
                } else {
                    snprintf(status_display, sizeof(status_display), "⚪ %s", item_status);
                }
                
                // Format initial countdown
                char countdown_str[50] = "⏱️ --:--";
                if (strcmp(item_status, "ACTIVE") == 0) {
                    // For ACTIVE items, show countdown
                    if (strlen(auction_end) > 0 && strcmp(auction_end, "NULL") != 0) {
                        gpointer end_time_ptr = g_hash_table_lookup(g_item_timers, GINT_TO_POINTER(item_id));
                        if (end_time_ptr) {
                            format_countdown(GPOINTER_TO_INT(end_time_ptr), countdown_str, sizeof(countdown_str));
                        }
                    }
                } else if (strcmp(item_status, "PENDING") == 0) {
                    // For PENDING items, show waiting status with start time (HH:MM only)
                    if (strlen(sched_start) > 0 && strcmp(sched_start, "NULL") != 0 && strlen(sched_start) >= 16) {
                        // Extract time part (HH:MM) from "YYYY-MM-DD HH:MM:SS"
                        char time_only[10];
                        strncpy(time_only, sched_start + 11, 5);  // Skip "YYYY-MM-DD " and take "HH:MM"
                        time_only[5] = '\0';
                        snprintf(countdown_str, sizeof(countdown_str), "⏳ Bắt đầu %s", time_only);
                    } else {
                        snprintf(countdown_str, sizeof(countdown_str), "⏳ Chờ kích hoạt");
                    }
                }
                
                gtk_list_store_set(g_room_detail_store, &iter,
                                  0, item_id,
                                  1, item_name,
                                  2, status_display,
                                  3, (int)start_price,
                                  4, (int)current_price,
                                  5, (int)buy_now_price,
                                  6, countdown_str,
                                  -1);
            }
            
            item = strtok(NULL, ";");
        }
    }
    
    free(data);
    return FALSE;  // Don't repeat
}

typedef struct {
    char data[BUFFER_SIZE];
} RoomListData;

gboolean update_room_list_ui(gpointer user_data) {
    RoomListData *data = (RoomListData*)user_data;
    
    // Safety check - ensure widgets exist
    if (!g_room_list_store) {
        free(data);
        return FALSE;
    }
    
    // Clear existing list
    gtk_list_store_clear(g_room_list_store);
    
    // Parse response: ROOM_LIST|count|data
    char* ptr = strchr(data->data, '|');
    if (!ptr) {
        free(data);
        return FALSE;
    }
    ptr++;
    
    int count = atoi(ptr);
    ptr = strchr(ptr, '|');
    if (!ptr) {
        free(data);
        return FALSE;
    }
    ptr++;
    
    // Parse room data
    char room_data[BUFFER_SIZE];
    strncpy(room_data, ptr, BUFFER_SIZE);
    
    char* room = strtok(room_data, ";");
    while (room) {
        int id, item_count, participant_count;
        char name[50], owner[50], status[20], created[50];
        
        if (sscanf(room, "%d|%49[^|]|%49[^|]|%19[^|]|%d|%d|%49s", 
                   &id, name, owner, status, &item_count, &participant_count, created) >= 6) {
            
            GtkTreeIter iter;
            gtk_list_store_append(g_room_list_store, &iter);
            gtk_list_store_set(g_room_list_store, &iter,
                              0, id,
                              1, name,
                              2, owner,
                              3, status,
                              4, item_count,
                              -1);
        }
        
        room = strtok(NULL, ";");
    }
    
    free(data);
    return FALSE;  // Don't repeat
}

void process_room_list_response(char* response) {
    if (!response || strncmp(response, "ROOM_LIST", 9) != 0) return;
    
    RoomListData *data = malloc(sizeof(RoomListData));
    strncpy(data->data, response, BUFFER_SIZE);
    
    // Schedule UI update in main thread
    g_idle_add(update_room_list_ui, data);
}

gboolean show_notification_ui(gpointer user_data) {
    NotificationData *data = (NotificationData*)user_data;
    show_notification(data->message, data->type);
    free(data);
    return FALSE;
}

void process_room_detail_response(char* response) {
    if (!response || strncmp(response, "ROOM_DETAIL", 11) != 0) return;
    
    char* ptr = strchr(response, '|');
    if (!ptr) return;
    ptr++;
    
    RoomDetailData *data = malloc(sizeof(RoomDetailData));
    
    // Parse: room_id|room_name|status|start|end|items
    char temp[BUFFER_SIZE];
    strncpy(temp, ptr, BUFFER_SIZE);
    
    char* token = strtok(temp, "|");
    if (token) data->room_id = atoi(token);
    
    token = strtok(NULL, "|");
    if (token) strncpy(data->room_name, token, sizeof(data->room_name));
    
    token = strtok(NULL, "|");
    if (token) strncpy(data->status, token, sizeof(data->status));
    
    token = strtok(NULL, "|");
    if (token) strncpy(data->start_time, token, sizeof(data->start_time));
    
    token = strtok(NULL, "|");
    if (token) strncpy(data->end_time, token, sizeof(data->end_time));
    
    // Get remaining data (items)
    token = strtok(NULL, "");
    if (token) strncpy(data->items_data, token, sizeof(data->items_data));
    else data->items_data[0] = '\0';
    
    // Schedule UI update in main thread
    g_idle_add(update_room_detail_ui, data);
}

// =============================================================
// RECEIVER THREAD
// =============================================================

void* receiver_thread_func(void* arg) {
    char buffer[BUFFER_SIZE];
    int buffer_pos = 0;
    
    while (g_thread_running) {
        fd_set read_fds;
        struct timeval timeout;
        
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        pthread_mutex_lock(&g_socket_mutex);
        if (g_socket_fd < 0) {
            pthread_mutex_unlock(&g_socket_mutex);
            break;
        }
        
        FD_ZERO(&read_fds);
        FD_SET(g_socket_fd, &read_fds);
        int sock = g_socket_fd;
        pthread_mutex_unlock(&g_socket_mutex);
        
        int activity = select(sock + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity > 0) {
            pthread_mutex_lock(&g_socket_mutex);
            if (g_socket_fd < 0) {
                pthread_mutex_unlock(&g_socket_mutex);
                break;
            }
            
            int bytes = recv(g_socket_fd, buffer + buffer_pos, BUFFER_SIZE - buffer_pos - 1, 0);
            pthread_mutex_unlock(&g_socket_mutex);
            
            if (bytes <= 0) break;
            
            buffer_pos += bytes;
            buffer[buffer_pos] = '\0';
            
            // Process complete lines
            char* line_start = buffer;
            char* newline;
            
            while ((newline = strchr(line_start, '\n')) != NULL) {
                *newline = '\0';
                
                // Process this line
                if (strncmp(line_start, "ROOM_LIST", 9) == 0) {
                    process_room_list_response(line_start);
                } else if (strncmp(line_start, "ROOM_DETAIL", 11) == 0) {
                    process_room_detail_response(line_start);
                } 
                // Handle all notification message types
                else if (strncmp(line_start, "NEW_BID", 7) == 0) {
                    // Format: NEW_BID|item_id|bidder|amount|countdown
                    char msg[512];
                    int item_id;
                    char bidder[50];
                    double amount;
                    char countdown[20];
                    if (sscanf(line_start, "NEW_BID|%d|%49[^|]|%lf|%19s", &item_id, bidder, &amount, countdown) >= 3) {
                        snprintf(msg, sizeof(msg), "💰 Đặt giá mới: %s đã đặt %.0f cho vật phẩm #%d (Còn %s)", 
                                bidder, amount, item_id, countdown);
                        NotificationData *data = malloc(sizeof(NotificationData));
                        strncpy(data->message, msg, sizeof(data->message));
                        data->type = GTK_MESSAGE_INFO;
                        g_idle_add(show_notification_ui, data);
                    }
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "BID_SUCCESS", 11) == 0) {
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, "✅ Đặt giá thành công!", sizeof(data->message));
                    data->type = GTK_MESSAGE_INFO;
                    g_idle_add(show_notification_ui, data);
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "BUY_NOW_SUCCESS", 15) == 0) {
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, "✅ Mua ngay thành công! Bạn đã sở hữu vật phẩm này.", sizeof(data->message));
                    data->type = GTK_MESSAGE_INFO;
                    g_idle_add(show_notification_ui, data);
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "ITEM_STARTED", 12) == 0) {
                    // Format: ITEM_STARTED|item_id|item_name|start_price|auction_end|duration|sched_start|sched_end|auction_start|auction_end2|countdown
                    char msg[512];
                    int item_id, duration, countdown;
                    char item_name[100], auction_end[30];
                    double start_price;
                    
                    // Try to parse full format with auction_end time
                    char* ptr = line_start + 13; // Skip "ITEM_STARTED|"
                    char temp_buf[1024];
                    strncpy(temp_buf, ptr, sizeof(temp_buf) - 1);
                    temp_buf[sizeof(temp_buf) - 1] = '\0';
                    
                    // Parse: item_id|item_name|start_price|auction_end|duration|...
                    char* tok = strtok(temp_buf, "|");
                    if (tok) item_id = atoi(tok);
                    tok = strtok(NULL, "|");
                    if (tok) strncpy(item_name, tok, sizeof(item_name) - 1);
                    tok = strtok(NULL, "|");
                    if (tok) start_price = atof(tok);
                    tok = strtok(NULL, "|");
                    if (tok) strncpy(auction_end, tok, sizeof(auction_end) - 1);
                    tok = strtok(NULL, "|");
                    if (tok) duration = atoi(tok);
                    
                    // Parse and store auction_end time in hash table
                    if (strlen(auction_end) > 0 && strcmp(auction_end, "NULL") != 0) {
                        struct tm tm_info;
                        memset(&tm_info, 0, sizeof(tm_info));
                        if (sscanf(auction_end, "%d-%d-%d %d:%d:%d",
                                   &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
                                   &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6) {
                            tm_info.tm_year -= 1900;
                            tm_info.tm_mon -= 1;
                            time_t end_time = mktime(&tm_info);
                            
                            // Store in hash table for countdown timer
                            if (!g_item_timers) {
                                g_item_timers = g_hash_table_new(g_direct_hash, g_direct_equal);
                            }
                            g_hash_table_insert(g_item_timers, GINT_TO_POINTER(item_id), GINT_TO_POINTER((int)end_time));
                        }
                    }
                    
                    snprintf(msg, sizeof(msg), "🔔 Đấu giá bắt đầu: %s (#%d) - Thời gian: %d phút", 
                            item_name, item_id, duration);
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, msg, sizeof(data->message));
                    data->type = GTK_MESSAGE_WARNING;
                    g_idle_add(show_notification_ui, data);
                    
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "ITEM_SOLD", 9) == 0) {
                    // Format: ITEM_SOLD|item_id|winner|final_price
                    char msg[512];
                    int item_id;
                    char winner[50];
                    double price;
                    if (sscanf(line_start, "ITEM_SOLD|%d|%49[^|]|%lf", &item_id, winner, &price) >= 2) {
                        snprintf(msg, sizeof(msg), "🏆 Đấu giá kết thúc: %s đã thắng vật phẩm #%d với giá %.0f", 
                                winner, item_id, price);
                        NotificationData *data = malloc(sizeof(NotificationData));
                        strncpy(data->message, msg, sizeof(data->message));
                        data->type = GTK_MESSAGE_INFO;
                        g_idle_add(show_notification_ui, data);
                    }
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "YOU_WON", 7) == 0) {
                    // Format: YOU_WON|item_id|item_name|final_price
                    char msg[512];
                    int item_id;
                    char item_name[100];
                    double price;
                    if (sscanf(line_start, "YOU_WON|%d|%99[^|]|%lf", &item_id, item_name, &price) >= 2) {
                        snprintf(msg, sizeof(msg), "🎉 Chúc mừng! Bạn đã thắng đấu giá '%s' với giá %.0f!", 
                                item_name, price);
                        NotificationData *data = malloc(sizeof(NotificationData));
                        strncpy(data->message, msg, sizeof(data->message));
                        data->type = GTK_MESSAGE_INFO;
                        g_idle_add(show_notification_ui, data);
                    }
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "AUCTION_ENDED", 13) == 0) {
                    // Format: AUCTION_ENDED|item_id|item_name|result|winner_name|final_price|message
                    // result can be: SOLD or CLOSED
                    char msg[512];
                    int item_id;
                    char item_name[100], result[20], winner_name[50], message[256];
                    double final_price = 0;
                    
                    // Parse the auction ended message
                    char* ptr = line_start + 14; // Skip "AUCTION_ENDED|"
                    if (sscanf(ptr, "%d|%99[^|]|%19[^|]|%49[^|]|%lf|%255[^\n]", 
                              &item_id, item_name, result, winner_name, &final_price, message) >= 3) {
                        
                        if (strcmp(result, "SOLD") == 0) {
                            snprintf(msg, sizeof(msg), "🏆 Đấu giá kết thúc: '%s' đã được bán cho %s với giá %.0f VND", 
                                    item_name, winner_name, final_price);
                        } else {
                            snprintf(msg, sizeof(msg), "⏹️ Đấu giá kết thúc: '%s' - Không có người đặt giá", item_name);
                        }
                        
                        NotificationData *data = malloc(sizeof(NotificationData));
                        strncpy(data->message, msg, sizeof(data->message));
                        data->type = GTK_MESSAGE_INFO;
                        g_idle_add(show_notification_ui, data);
                    }
                    
                    // Remove timer for this item
                    if (g_item_timers) {
                        g_hash_table_remove(g_item_timers, GINT_TO_POINTER(item_id));
                    }
                    
                    // Refresh room detail to update item list
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "AUCTION_WARNING", 15) == 0) {
                    // Format: AUCTION_WARNING|item_id|seconds_left
                    char msg[512];
                    int item_id, seconds;
                    if (sscanf(line_start, "AUCTION_WARNING|%d|%d", &item_id, &seconds) >= 2) {
                        snprintf(msg, sizeof(msg), "⏰ Cảnh báo: Vật phẩm #%d còn %d giây!", item_id, seconds);
                        NotificationData *data = malloc(sizeof(NotificationData));
                        strncpy(data->message, msg, sizeof(data->message));
                        data->type = GTK_MESSAGE_WARNING;
                        g_idle_add(show_notification_ui, data);
                    }
                }
                else if (strncmp(line_start, "TIME_EXTENDED", 13) == 0) {
                    // Format: TIME_EXTENDED|item_id|new_end_time
                    char msg[512];
                    int item_id;
                    char new_end_time[30] = "";
                    
                    if (sscanf(line_start, "TIME_EXTENDED|%d|%29s", &item_id, new_end_time) >= 2) {
                        // Parse and update end_time in hash table
                        if (strlen(new_end_time) > 0 && strcmp(new_end_time, "NULL") != 0) {
                            struct tm tm_info;
                            memset(&tm_info, 0, sizeof(tm_info));
                            if (sscanf(new_end_time, "%d-%d-%d %d:%d:%d",
                                       &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
                                       &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6) {
                                tm_info.tm_year -= 1900;
                                tm_info.tm_mon -= 1;
                                time_t end_time = mktime(&tm_info);
                                
                                // Update in hash table
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
                    // No need to refresh_room_detail since timer will update countdown automatically
                }
                else if (strncmp(line_start, "ROOM_CLOSED", 11) == 0) {
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, "🚪 Phòng đã đóng cửa. Bạn đã bị thoát ra.", sizeof(data->message));
                    data->type = GTK_MESSAGE_WARNING;
                    g_idle_add(show_notification_ui, data);
                    g_current_room_id = 0;
                }
                else if (strncmp(line_start, "KICKED", 6) == 0) {
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, "⛔ Bạn đã bị kick khỏi phòng!", sizeof(data->message));
                    data->type = GTK_MESSAGE_ERROR;
                    g_idle_add(show_notification_ui, data);
                    g_current_room_id = 0;
                }
                else if (strncmp(line_start, "CREATE_ITEM_SUCCESS", 19) == 0) {
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, "✅ Tạo vật phẩm thành công!", sizeof(data->message));
                    data->type = GTK_MESSAGE_INFO;
                    g_idle_add(show_notification_ui, data);
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "CREATE_ITEM_FAIL", 16) == 0) {
                    // Format: CREATE_ITEM_FAIL|message
                    char* msg_ptr = strchr(line_start, '|');
                    char error_msg[256];
                    if (msg_ptr) {
                        snprintf(error_msg, sizeof(error_msg), "❌ Tạo vật phẩm thất bại: %s", msg_ptr + 1);
                    } else {
                        snprintf(error_msg, sizeof(error_msg), "❌ Tạo vật phẩm thất bại!");
                    }
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, error_msg, sizeof(data->message));
                    data->type = GTK_MESSAGE_ERROR;
                    g_idle_add(show_notification_ui, data);
                }
                else if (strncmp(line_start, "DELETE_ITEM_SUCCESS", 19) == 0) {
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, "✅ Xóa vật phẩm thành công!", sizeof(data->message));
                    data->type = GTK_MESSAGE_INFO;
                    g_idle_add(show_notification_ui, data);
                    if (g_current_room_id > 0) refresh_room_detail();
                }
                else if (strncmp(line_start, "DELETE_ITEM_FAIL", 16) == 0) {
                    // Format: DELETE_ITEM_FAIL|message
                    char* msg_ptr = strchr(line_start, '|');
                    char error_msg[256];
                    if (msg_ptr) {
                        snprintf(error_msg, sizeof(error_msg), "❌ Xóa thất bại: %s", msg_ptr + 1);
                    } else {
                        snprintf(error_msg, sizeof(error_msg), "❌ Xóa vật phẩm thất bại!");
                    }
                    NotificationData *data = malloc(sizeof(NotificationData));
                    strncpy(data->message, error_msg, sizeof(data->message));
                    data->type = GTK_MESSAGE_ERROR;
                    g_idle_add(show_notification_ui, data);
                }
                else if (strncmp(line_start, "USER_JOINED", 11) == 0) {
                    // Format: USER_JOINED|username|message
                    char username[50];
                    if (sscanf(line_start, "USER_JOINED|%49[^|]", username) >= 1) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "👋 %s đã vào phòng", username);
                        NotificationData *data = malloc(sizeof(NotificationData));
                        strncpy(data->message, msg, sizeof(data->message));
                        data->type = GTK_MESSAGE_INFO;
                        g_idle_add(show_notification_ui, data);
                    }
                }
                else if (strncmp(line_start, "USER_LEFT", 9) == 0) {
                    // Format: USER_LEFT|username
                    char username[50];
                    if (sscanf(line_start, "USER_LEFT|%49s", username) >= 1) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "👋 %s đã rời phòng", username);
                        NotificationData *data = malloc(sizeof(NotificationData));
                        strncpy(data->message, msg, sizeof(data->message));
                        data->type = GTK_MESSAGE_INFO;
                        g_idle_add(show_notification_ui, data);
                    }
                }
                else if (strncmp(line_start, "USER_LIST", 9) == 0) {
                    // Format: USER_LIST|id|username|status|role;id|username|status|role;...
                    if (g_admin_user_store) {
                        char* ptr = strchr(line_start, '|');
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
                }
                else if (strncmp(line_start, "CREATE_ROOM_SUCCESS", 19) == 0) {
                    NotificationData *data = malloc(sizeof(NotificationData));
                    snprintf(data->message, sizeof(data->message), "✅ Tạo phòng thành công!");
                    data->type = GTK_MESSAGE_INFO;
                    g_idle_add(show_notification_ui, data);
                    
                    // Refresh room list
                    const gchar *current_page = gtk_stack_get_visible_child_name(GTK_STACK(g_stack));
                    if (strcmp(current_page, "room_list") == 0) {
                        refresh_room_list();
                    }
                }
                else if (strncmp(line_start, "CREATE_ROOM_FAIL", 16) == 0) {
                    NotificationData *data = malloc(sizeof(NotificationData));
                    char* msg = strchr(line_start, '|');
                    snprintf(data->message, sizeof(data->message), "❌ %s", 
                            msg ? msg+1 : "Tạo phòng thất bại");
                    data->type = GTK_MESSAGE_ERROR;
                    g_idle_add(show_notification_ui, data);
                }
                else if (strncmp(line_start, "SEARCH_RESULT", 13) == 0) {
                    // Format: SEARCH_RESULT|count|item_data;item_data;...
                    if (g_search_result_store) {
                        char* ptr = strchr(line_start, '|');
                        if (ptr) {
                            ptr++;
                            int count = atoi(ptr);
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
                }
                else if (strncmp(line_start, "AUCTION_HISTORY", 15) == 0) {
                    // Format: AUCTION_HISTORY|count|history_data;...
                    if (g_history_store) {
                        char* ptr = strchr(line_start, '|');
                        if (ptr) {
                            ptr++;
                            int count = atoi(ptr);
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
                                    // Format từ server có thể khác, adjust theo thực tế
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
                }
                else if (strncmp(line_start, "BID_ERROR", 9) == 0 || 
                        strncmp(line_start, "ERROR", 5) == 0) {
                    char* msg_ptr = strchr(line_start, '|');
                    if (msg_ptr) {
                        msg_ptr++;
                        NotificationData *data = malloc(sizeof(NotificationData));
                        snprintf(data->message, sizeof(data->message), "❌ %s", msg_ptr);
                        data->type = GTK_MESSAGE_ERROR;
                        g_idle_add(show_notification_ui, data);
                    }
                }
                
                line_start = newline + 1;
            }
            
            // Move remaining data to start of buffer
            int remaining = buffer_pos - (line_start - buffer);
            if (remaining > 0) {
                memmove(buffer, line_start, remaining);
            }
            buffer_pos = remaining;
        }
    }
    
    return NULL;
}

void start_receiver_thread() {
    if (!g_thread_running) {
        g_thread_running = 1;
        pthread_create(&g_receiver_thread, NULL, receiver_thread_func, NULL);
    }
}

void stop_receiver_thread() {
    if (g_thread_running) {
        g_thread_running = 0;
        pthread_join(g_receiver_thread, NULL);
    }
}

// =============================================================
// EVENT HANDLERS - LOGIN/REGISTER
// =============================================================

void on_login_clicked(GtkWidget *widget, gpointer data) {
    const char* username = gtk_entry_get_text(GTK_ENTRY(g_login_username_entry));
    const char* password = gtk_entry_get_text(GTK_ENTRY(g_login_password_entry));
    
    if (strlen(username) == 0 || strlen(password) == 0) {
        show_message_dialog(GTK_MESSAGE_WARNING, "Lỗi", "Vui lòng nhập đầy đủ thông tin!");
        return;
    }
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "LOGIN|%s|%s", username, password);
    send_command(cmd);
    
    char* response = wait_for_response_sync();
    if (response && strncmp(response, "LOGIN_SUCCESS", 13) == 0) {
        // Parse: LOGIN_SUCCESS|msg|username|role
        char* ptr = strtok(response, "|");  // LOGIN_SUCCESS
        ptr = strtok(NULL, "|");  // msg
        ptr = strtok(NULL, "|");  // username
        if (ptr) strncpy(g_username, ptr, sizeof(g_username));
        ptr = strtok(NULL, "|");  // role
        if (ptr) g_user_role = atoi(ptr);
        
        g_is_logged_in = 1;
        
        // Start receiver thread
        start_receiver_thread();
        
        // Switch to room list view
        gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "room_list");
        
        // Update user info label
        if (g_user_info_label) {
            char user_info[256];
            const char* role_str = (g_user_role == 1) ? "Admin" : "User";
            snprintf(user_info, sizeof(user_info), 
                     "<b>👤 %s</b> | <span foreground='blue'>%s</span>", 
                     g_username, role_str);
            gtk_label_set_markup(GTK_LABEL(g_user_info_label), user_info);
            gtk_widget_show(g_user_info_label);
        }
        
        // Show/hide admin button based on role
        if (g_admin_button) {
            if (g_user_role == 1) {
                gtk_widget_show(g_admin_button);
            } else {
                gtk_widget_hide(g_admin_button);
            }
        }
        
        char status[128];
        snprintf(status, sizeof(status), "Đăng nhập thành công! Xin chào %s", g_username);
        update_status_bar(status);
        
        // Clear password
        gtk_entry_set_text(GTK_ENTRY(g_login_password_entry), "");
        
        // Load room list
        refresh_room_list();
    } else {
        show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi đăng nhập", 
                          response ? response : "Sai thông tin đăng nhập!");
    }
}

void on_register_clicked(GtkWidget *widget, gpointer data) {
    const char* username = gtk_entry_get_text(GTK_ENTRY(g_register_username_entry));
    const char* password = gtk_entry_get_text(GTK_ENTRY(g_register_password_entry));
    
    if (strlen(username) == 0 || strlen(password) == 0) {
        show_message_dialog(GTK_MESSAGE_WARNING, "Lỗi", "Vui lòng nhập đầy đủ thông tin!");
        return;
    }
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "REGISTER|%s|%s", username, password);
    send_command(cmd);
    
    char* response = wait_for_response_sync();
    if (response && strncmp(response, "REGISTER_SUCCESS", 16) == 0) {
        show_message_dialog(GTK_MESSAGE_INFO, "Thành công", "Đăng ký thành công! Vui lòng đăng nhập.");
        
        // Switch to login view
        gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "login");
        
        // Clear fields
        gtk_entry_set_text(GTK_ENTRY(g_register_username_entry), "");
        gtk_entry_set_text(GTK_ENTRY(g_register_password_entry), "");
    } else {
        show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi đăng ký", 
                          response ? response : "Đăng ký thất bại!");
    }
}

void on_show_register_clicked(GtkWidget *widget, gpointer data) {
    gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "register");
}

void on_show_login_clicked(GtkWidget *widget, gpointer data) {
    gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "login");
}

// =============================================================
// EVENT HANDLERS - ROOM LIST
// =============================================================

void on_refresh_rooms_clicked(GtkWidget *widget, gpointer data) {
    refresh_room_list();
    update_status_bar("Đã làm mới danh sách phòng");
}

void on_create_room_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Tạo phòng đấu giá",
                                                     GTK_WINDOW(g_main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Tạo", GTK_RESPONSE_ACCEPT,
                                                     "_Hủy", GTK_RESPONSE_CANCEL,
                                                     NULL);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    // Room name
    GtkWidget *name_label = gtk_label_new("Tên phòng:");
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    GtkWidget *name_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), name_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), name_entry, FALSE, FALSE, 0);
    
    // Start time with datetime picker
    GtkWidget *start_picker = create_datetime_picker("Bắt đầu:", NULL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), start_picker, FALSE, FALSE, 5);
    
    // End time with datetime picker
    GtkWidget *end_picker = create_datetime_picker("Kết thúc:", NULL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), end_picker, FALSE, FALSE, 5);
    
    gtk_container_add(GTK_CONTAINER(content), vbox);
    gtk_widget_show_all(dialog);
    
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (result == GTK_RESPONSE_ACCEPT) {
        const char* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        
        char start[30], end[30];
        get_datetime_from_picker(start_picker, start, sizeof(start));
        get_datetime_from_picker(end_picker, end, sizeof(end));
        
        if (strlen(name) > 0 && strlen(start) > 0 && strlen(end) > 0) {
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "CREATE_ROOM|%s|%s|%s", name, start, end);
            send_command(cmd);
            
            // Response will be handled by receiver thread (CREATE_ROOM_SUCCESS/FAIL)
            update_status_bar("Đang tạo phòng...");
        } else {
            show_message_dialog(GTK_MESSAGE_WARNING, "Cảnh báo", "Vui lòng nhập đầy đủ thông tin!");
        }
    }
    
    gtk_widget_destroy(dialog);
}

void on_join_room_clicked(GtkWidget *widget, gpointer data) {
    // Prevent double-join or re-entrant calls
    if (g_joining_room) {
        return;
    }
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_room_list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        int room_id;
        char *room_name;
        
        gtk_tree_model_get(model, &iter, 0, &room_id, 1, &room_name, -1);
        
        g_joining_room = 1;  // Set flag before sending command
        
        char cmd[50];
        snprintf(cmd, sizeof(cmd), "JOIN_ROOM|%d", room_id);
        send_command(cmd);
        
        char* response = wait_for_response_sync();
        if (response && strncmp(response, "JOIN_ROOM_SUCCESS", 17) == 0) {
            g_current_room_id = room_id;
            strncpy(g_current_room_name, room_name, sizeof(g_current_room_name));
            
            // Parse: JOIN_ROOM_SUCCESS|msg|room_id|room_name
            // Check if message contains "Chu phong" to detect room owner
            g_is_room_owner = (strstr(response, "Chu phong") != NULL) ? 1 : 0;
            
            // Show/hide buttons based on role
            if (g_is_room_owner) {
                // Owner: show create/delete item buttons, hide bid/buy buttons
                if (g_create_item_button) gtk_widget_show(g_create_item_button);
                if (g_delete_item_button) gtk_widget_show(g_delete_item_button);
                if (g_bid_button) gtk_widget_hide(g_bid_button);
                if (g_buy_button) gtk_widget_hide(g_buy_button);
            } else {
                // Guest: show bid/buy buttons, hide create/delete item buttons
                if (g_create_item_button) gtk_widget_hide(g_create_item_button);
                if (g_delete_item_button) gtk_widget_hide(g_delete_item_button);
                if (g_bid_button) gtk_widget_show(g_bid_button);
                if (g_buy_button) gtk_widget_show(g_buy_button);
            }
            
            // Switch to room detail view
            gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "room_detail");
            
            char status[128];
            snprintf(status, sizeof(status), "Đã vào phòng: %s", room_name);
            update_status_bar(status);
            
            // Request room detail
            refresh_room_detail();
        } else {
            // Ensure current room ID is reset on failure
            g_current_room_id = 0;
            g_is_room_owner = 0;
            memset(g_current_room_name, 0, sizeof(g_current_room_name));
            show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi", response ? response : "Không thể vào phòng!");
        }
        
        g_joining_room = 0;  // Clear flag after operation completes
        g_free(room_name);
    } else {
        show_message_dialog(GTK_MESSAGE_WARNING, "Cảnh báo", "Vui lòng chọn một phòng!");
    }
}

void on_logout_clicked(GtkWidget *widget, gpointer data) {
    send_command("LOGOUT");
    
    g_is_logged_in = 0;
    g_user_role = 0;
    g_current_room_id = 0;
    memset(g_username, 0, sizeof(g_username));
    
    // Hide user info label
    if (g_user_info_label) {
        gtk_widget_hide(g_user_info_label);
    }
    
    // Hide admin button
    if (g_admin_button) {
        gtk_widget_hide(g_admin_button);
    }
    
    // Stop receiver thread
    stop_receiver_thread();
    
    // Switch back to login
    gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "login");
    update_status_bar("Đã đăng xuất");
    
    // Clear password
    gtk_entry_set_text(GTK_ENTRY(g_login_password_entry), "");
}

// =============================================================
// EVENT HANDLERS - ROOM DETAIL
// =============================================================

void on_place_bid_clicked(GtkWidget *widget, gpointer data) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_room_detail_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_message_dialog(GTK_MESSAGE_WARNING, "Cảnh báo", "Vui lòng chọn một vật phẩm!");
        return;
    }
    
    int item_id;
    gtk_tree_model_get(model, &iter, 0, &item_id, -1);
    
    // Create dialog for bid amount
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Đặt giá",
                                                     GTK_WINDOW(g_main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Đặt giá", GTK_RESPONSE_ACCEPT,
                                                     "_Hủy", GTK_RESPONSE_CANCEL,
                                                     NULL);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    
    GtkWidget *label = gtk_label_new("Số tiền:");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nhập số tiền");
    
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);
    
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (result == GTK_RESPONSE_ACCEPT) {
        const char* amount_str = gtk_entry_get_text(GTK_ENTRY(entry));
        double amount = atof(amount_str);
        
        if (amount > 0) {
            char cmd[100];
            snprintf(cmd, sizeof(cmd), "PLACE_BID|%d|%.0f", item_id, amount);
            send_command(cmd);
            
            update_status_bar("Đã gửi lệnh đặt giá");
        }
    }
    
    gtk_widget_destroy(dialog);
}

void on_buy_now_clicked(GtkWidget *widget, gpointer data) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_room_detail_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_message_dialog(GTK_MESSAGE_WARNING, "Cảnh báo", "Vui lòng chọn một vật phẩm!");
        return;
    }
    
    int item_id;
    char *item_name;
    int buy_now_price;
    
    gtk_tree_model_get(model, &iter, 0, &item_id, 1, &item_name, 5, &buy_now_price, -1);
    
    if (buy_now_price <= 0) {
        show_message_dialog(GTK_MESSAGE_WARNING, "Cảnh báo", "Vật phẩm này không có giá mua ngay!");
        g_free(item_name);
        return;
    }
    
    // Confirm dialog
    char msg[256];
    snprintf(msg, sizeof(msg), "Bạn có chắc muốn mua ngay '%s' với giá %d?", 
             item_name, buy_now_price);
    
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_main_window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               "%s", msg);
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (result == GTK_RESPONSE_YES) {
        char cmd[100];
        snprintf(cmd, sizeof(cmd), "BUY_NOW|%d", item_id);
        send_command(cmd);
        
        update_status_bar("Đã gửi lệnh mua ngay");
    }
    
    g_free(item_name);
}

void on_create_item_clicked(GtkWidget *widget, gpointer data) {
    if (g_current_room_id == 0) {
        show_error_dialog("Bạn chưa tham gia phòng nào!");
        return;
    }
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Tạo vật phẩm đấu giá",
                                                     GTK_WINDOW(g_main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Tạo", GTK_RESPONSE_ACCEPT,
                                                     "_Hủy", GTK_RESPONSE_CANCEL,
                                                     NULL);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    // Item name
    GtkWidget *name_label = gtk_label_new("Tên vật phẩm:");
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "Nhập tên vật phẩm");
    gtk_box_pack_start(GTK_BOX(vbox), name_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), name_entry, FALSE, FALSE, 0);
    
    // Starting price
    GtkWidget *start_price_label = gtk_label_new("Giá khởi điểm:");
    gtk_widget_set_halign(start_price_label, GTK_ALIGN_START);
    GtkWidget *start_price_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(start_price_entry), "VD: 100000");
    gtk_box_pack_start(GTK_BOX(vbox), start_price_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), start_price_entry, FALSE, FALSE, 0);
    
    // Duration (seconds)
    GtkWidget *duration_label = gtk_label_new("Thời lượng (giây):");
    gtk_widget_set_halign(duration_label, GTK_ALIGN_START);
    GtkWidget *duration_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(duration_entry), "VD: 300");
    gtk_box_pack_start(GTK_BOX(vbox), duration_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), duration_entry, FALSE, FALSE, 0);
    
    // Buy now price (optional)
    GtkWidget *buy_now_label = gtk_label_new("Giá mua ngay (tùy chọn):");
    gtk_widget_set_halign(buy_now_label, GTK_ALIGN_START);
    GtkWidget *buy_now_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(buy_now_entry), "VD: 500000 hoặc để trống");
    gtk_box_pack_start(GTK_BOX(vbox), buy_now_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), buy_now_entry, FALSE, FALSE, 0);
    
    // Start time with datetime picker (optional)
    GtkWidget *start_picker = create_datetime_picker("Bắt đầu (tùy chọn):", NULL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), start_picker, FALSE, FALSE, 5);
    
    // End time with datetime picker (optional)
    GtkWidget *end_picker = create_datetime_picker("Kết thúc (tùy chọn):", NULL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), end_picker, FALSE, FALSE, 5);
    
    gtk_container_add(GTK_CONTAINER(content), vbox);
    gtk_widget_show_all(dialog);
    
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (result == GTK_RESPONSE_ACCEPT) {
        const char* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        const char* start_price_str = gtk_entry_get_text(GTK_ENTRY(start_price_entry));
        const char* duration_str = gtk_entry_get_text(GTK_ENTRY(duration_entry));
        const char* buy_now_str = gtk_entry_get_text(GTK_ENTRY(buy_now_entry));
        
        char start_time[30], end_time[30];
        get_datetime_from_picker(start_picker, start_time, sizeof(start_time));
        get_datetime_from_picker(end_picker, end_time, sizeof(end_time));
        
        if (strlen(name) > 0 && strlen(start_price_str) > 0 && strlen(duration_str) > 0) {
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "CREATE_ITEM|%d|%s|%s|%s|%s|%s|%s", 
                    g_current_room_id,
                    name, start_price_str, duration_str, 
                    strlen(buy_now_str) > 0 ? buy_now_str : "0",
                    start_time,
                    end_time);
            
            printf("[DEBUG] Creating item: name=%s, price=%s, duration=%s, start=%s, end=%s\n",
                   name, start_price_str, duration_str, start_time, end_time);
            printf("[DEBUG] Sending command: %s\n", cmd);
            
            send_command(cmd);
            
            update_status_bar("Đã gửi lệnh tạo vật phẩm...");
        } else {
            show_error_dialog("Vui lòng nhập đầy đủ thông tin bắt buộc!");
        }
    }
    
    gtk_widget_destroy(dialog);
}

void on_delete_item_clicked(GtkWidget *widget, gpointer data) {
    if (g_current_room_id == 0) {
        show_error_dialog("Bạn chưa tham gia phòng nào!");
        return;
    }
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_room_detail_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog("Vui lòng chọn một vật phẩm để xóa!");
        return;
    }
    
    int item_id;
    char *item_name;
    gtk_tree_model_get(model, &iter, 0, &item_id, 1, &item_name, -1);
    
    printf("[DEBUG] Deleting item: id=%d, name=%s\n", item_id, item_name);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Bạn có chắc muốn xóa vật phẩm '%s' (ID: %d)?", item_name, item_id);
    
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_main_window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               "%s", msg);
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (result == GTK_RESPONSE_YES) {
        char cmd[100];
        snprintf(cmd, sizeof(cmd), "DELETE_ITEM|%d", item_id);
        printf("[DEBUG] Sending delete command: %s\n", cmd);
        send_command(cmd);
        
        update_status_bar("Đã gửi lệnh xóa vật phẩm...");
    } else {
        printf("[DEBUG] User cancelled delete\n");
    }
    
    g_free(item_name);
}

void on_leave_room_clicked(GtkWidget *widget, gpointer data) {
    send_command("LEAVE_ROOM");
    
    g_current_room_id = 0;
    g_is_room_owner = 0;
    memset(g_current_room_name, 0, sizeof(g_current_room_name));
    
    // Switch back to room list
    gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "room_list");
    update_status_bar("Đã rời phòng");
    
    refresh_room_list();
}

// Helper struct for search callback
typedef struct {
    GtkWidget *keyword_entry;
    GtkWidget *time_from_entry;
    GtkWidget *time_to_entry;
    GtkWidget *name_radio;
    GtkWidget *time_radio;
    GtkWidget *both_radio;
} SearchWidgets;

void on_search_button_clicked(GtkWidget *button, gpointer user_data) {
    SearchWidgets *widgets = (SearchWidgets*)user_data;
    
    const char* keyword = gtk_entry_get_text(GTK_ENTRY(widgets->keyword_entry));
    const char* time_from = gtk_entry_get_text(GTK_ENTRY(widgets->time_from_entry));
    const char* time_to = gtk_entry_get_text(GTK_ENTRY(widgets->time_to_entry));
    
    char type[10] = "NAME";
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->time_radio))) {
        strcpy(type, "TIME");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets->both_radio))) {
        strcpy(type, "BOTH");
    }
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "SEARCH_ITEMS|%s|%s|%s|%s", type, keyword, time_from, time_to);
    send_command(cmd);
    update_status_bar("Đang tìm kiếm...");
}

void on_search_items_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Tìm kiếm vật phẩm",
                                                     GTK_WINDOW(g_main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Đóng", GTK_RESPONSE_CLOSE,
                                                     NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 600);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 10);
    
    // Input section
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    // Search mode
    GtkWidget *mode_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *mode_label = gtk_label_new("Kiểu:");
    GtkWidget *name_radio = gtk_radio_button_new_with_label(NULL, "Tên");
    GtkWidget *time_radio = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(name_radio), "Thời gian");
    GtkWidget *both_radio = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(name_radio), "Kết hợp");
    gtk_box_pack_start(GTK_BOX(mode_box), mode_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mode_box), name_radio, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mode_box), time_radio, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mode_box), both_radio, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(input_box), mode_box, FALSE, FALSE, 0);
    
    // Keyword
    GtkWidget *keyword_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *keyword_label = gtk_label_new("Từ khóa:");
    gtk_widget_set_size_request(keyword_label, 80, -1);
    GtkWidget *keyword_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(keyword_entry), "Nhập từ khóa");
    gtk_box_pack_start(GTK_BOX(keyword_box), keyword_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(keyword_box), keyword_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(input_box), keyword_box, FALSE, FALSE, 0);
    
    // Time range
    GtkWidget *time_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *time_label = gtk_label_new("Từ - Đến:");
    gtk_widget_set_size_request(time_label, 80, -1);
    GtkWidget *time_from_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(time_from_entry), "YYYY-MM-DD");
    GtkWidget *time_to_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(time_to_entry), "YYYY-MM-DD");
    gtk_box_pack_start(GTK_BOX(time_box), time_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), time_from_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), time_to_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(input_box), time_box, FALSE, FALSE, 0);
    
    // Search button
    GtkWidget *search_button = gtk_button_new_with_label("🔍 Tìm kiếm");
    gtk_box_pack_start(GTK_BOX(input_box), search_button, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(main_box), input_box, FALSE, FALSE, 0);
    
    // Results section
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    g_search_result_store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING,
                                                G_TYPE_STRING, G_TYPE_STRING,
                                                G_TYPE_INT, G_TYPE_INT);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_search_result_store));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Vật phẩm", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Phòng", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Trạng thái", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Giá khởi điểm", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Giá hiện tại", renderer, "text", 5, NULL);
    
    gtk_container_add(GTK_CONTAINER(scrolled), tree_view);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(content), main_box);
    
    // Setup search widgets struct
    SearchWidgets *search_widgets = g_malloc(sizeof(SearchWidgets));
    search_widgets->keyword_entry = keyword_entry;
    search_widgets->time_from_entry = time_from_entry;
    search_widgets->time_to_entry = time_to_entry;
    search_widgets->name_radio = name_radio;
    search_widgets->time_radio = time_radio;
    search_widgets->both_radio = both_radio;
    
    // Search button callback
    g_signal_connect(search_button, "clicked",
                    G_CALLBACK(on_search_button_clicked), search_widgets);
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    g_free(search_widgets);
    g_search_result_store = NULL;
    gtk_widget_destroy(dialog);
}

void on_view_history_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Lịch sử đấu giá",
                                                     GTK_WINDOW(g_main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Đóng", GTK_RESPONSE_CLOSE,
                                                     NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 500);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    
    // Filter options
    GtkWidget *filter_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *filter_label = gtk_label_new("Lọc:");
    GtkWidget *all_button = gtk_radio_button_new_with_label(NULL, "Tất cả");
    GtkWidget *won_button = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(all_button), "Đã thắng");
    GtkWidget *lost_button = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(all_button), "Đã thua");
    
    gtk_box_pack_start(GTK_BOX(filter_box), filter_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), all_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), won_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), lost_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(box), filter_box, FALSE, FALSE, 0);
    
    // History list view
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    g_history_store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING,
                                             G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_history_store));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Vật phẩm", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Phòng", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Giá", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Kết quả", renderer, "text", 4, NULL);
    
    gtk_container_add(GTK_CONTAINER(scrolled), tree_view);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);
    
    // Send command to get history
    send_command("GET_MY_AUCTION_HISTORY|ALL|1|50");
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    g_history_store = NULL;
    gtk_widget_destroy(dialog);
}

void on_admin_panel_clicked(GtkWidget *widget, gpointer data) {
    if (g_user_role != 1) {
        show_error_dialog("Chỉ admin mới có quyền truy cập!");
        return;
    }
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Quản lý người dùng (Admin)",
                                                     GTK_WINDOW(g_main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Đóng", GTK_RESPONSE_CLOSE,
                                                     NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 500);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    
    // User list view
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    g_admin_user_store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING,
                                             G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_admin_user_store));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Username", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Trạng thái", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                               -1, "Vai trò", renderer, "text", 3, NULL);
    
    gtk_container_add(GTK_CONTAINER(scrolled), tree_view);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(content), box);
    gtk_widget_show_all(dialog);
    
    // Send command to get user list
    send_command("GET_USER_LIST");
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    g_admin_user_store = NULL;
    gtk_widget_destroy(dialog);
}

gboolean auto_refresh_room(gpointer data) {
    // Auto-refresh is disabled because:
    // 1. Countdown timer already updates every second
    // 2. Server broadcasts all changes (NEW_BID, ITEM_STARTED, AUCTION_ENDED)
    // 3. Receiver thread handles broadcasts and auto-refreshes when needed
    // This prevents unnecessary network traffic and UI reloading
    return TRUE;  // Continue calling (but do nothing)
}

// =============================================================
// UI CREATION FUNCTIONS
// =============================================================

GtkWidget* create_login_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='large' weight='bold'>HỆ THỐNG ĐẤU GIÁ TRỰC TUYẾN</span>");
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 20);
    
    // Login form
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    GtkWidget *username_label = gtk_label_new("Tên đăng nhập:");
    g_login_username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_login_username_entry), "Nhập tên đăng nhập");
    
    GtkWidget *password_label = gtk_label_new("Mật khẩu:");
    g_login_password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(g_login_password_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_login_password_entry), "Nhập mật khẩu");
    
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_login_username_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_login_password_entry, 1, 1, 1, 1);
    
    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 10);
    
    // Buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    
    GtkWidget *login_button = gtk_button_new_with_label("Đăng nhập");
    GtkWidget *register_button = gtk_button_new_with_label("Đăng ký");
    
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_clicked), NULL);
    g_signal_connect(register_button, "clicked", G_CALLBACK(on_show_register_clicked), NULL);
    
    gtk_box_pack_start(GTK_BOX(button_box), login_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), register_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(box), button_box, FALSE, FALSE, 10);
    
    return box;
}

GtkWidget* create_register_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='large' weight='bold'>ĐĂNG KÝ TÀI KHOẢN</span>");
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 20);
    
    // Register form
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    GtkWidget *username_label = gtk_label_new("Tên đăng nhập:");
    g_register_username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_register_username_entry), "Nhập tên đăng nhập");
    
    GtkWidget *password_label = gtk_label_new("Mật khẩu:");
    g_register_password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(g_register_password_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_register_password_entry), "Nhập mật khẩu");
    
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_register_username_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_register_password_entry, 1, 1, 1, 1);
    
    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 10);
    
    // Buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    
    GtkWidget *register_button = gtk_button_new_with_label("Đăng ký");
    GtkWidget *back_button = gtk_button_new_with_label("Quay lại");
    
    g_signal_connect(register_button, "clicked", G_CALLBACK(on_register_clicked), NULL);
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_show_login_clicked), NULL);
    
    gtk_box_pack_start(GTK_BOX(button_box), register_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), back_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(box), button_box, FALSE, FALSE, 10);
    
    return box;
}

GtkWidget* create_room_list_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    
    // Header box with title and user info
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='large' weight='bold'>DANH SÁCH PHÒNG ĐẤU GIÁ</span>");
    gtk_box_pack_start(GTK_BOX(header_box), title, FALSE, FALSE, 0);
    
    // User info label (initially hidden)
    g_user_info_label = gtk_label_new("");
    gtk_label_set_use_markup(GTK_LABEL(g_user_info_label), TRUE);
    gtk_widget_set_no_show_all(g_user_info_label, TRUE);
    gtk_box_pack_end(GTK_BOX(header_box), g_user_info_label, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(box), header_box, FALSE, FALSE, 0);
    
    // Toolbar
    GtkWidget *toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    GtkWidget *refresh_button = gtk_button_new_with_label("🔄 Làm mới");
    GtkWidget *create_button = gtk_button_new_with_label("➕ Tạo phòng");
    GtkWidget *join_button = gtk_button_new_with_label("▶ Vào phòng");
    GtkWidget *search_button = gtk_button_new_with_label("🔍 Tìm kiếm");
    GtkWidget *history_button = gtk_button_new_with_label("📜 Lịch sử");
    g_admin_button = gtk_button_new_with_label("👤 Admin");  // Use global variable
    GtkWidget *logout_button = gtk_button_new_with_label("🚪 Đăng xuất");
    
    // Hide admin button initially (will show after login if admin)
    gtk_widget_set_no_show_all(g_admin_button, TRUE);
    gtk_widget_hide(g_admin_button);
    
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_rooms_clicked), NULL);
    g_signal_connect(create_button, "clicked", G_CALLBACK(on_create_room_clicked), NULL);
    g_signal_connect(join_button, "clicked", G_CALLBACK(on_join_room_clicked), NULL);
    g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_items_clicked), NULL);
    g_signal_connect(history_button, "clicked", G_CALLBACK(on_view_history_clicked), NULL);
    g_signal_connect(g_admin_button, "clicked", G_CALLBACK(on_admin_panel_clicked), NULL);
    g_signal_connect(logout_button, "clicked", G_CALLBACK(on_logout_clicked), NULL);
    
    gtk_box_pack_start(GTK_BOX(toolbar_box), refresh_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), create_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), join_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), search_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), history_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), g_admin_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(toolbar_box), logout_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(box), toolbar_box, FALSE, FALSE, 0);
    
    // Room list
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    // Create list store: ID, Name, Owner, Status, Items
    g_room_list_store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, 
                                           G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    
    g_room_list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_room_list_store));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_list_view),
                                               -1, "ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_list_view),
                                               -1, "Tên phòng", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_list_view),
                                               -1, "Chủ phòng", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_list_view),
                                               -1, "Trạng thái", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_list_view),
                                               -1, "Vật phẩm", renderer, "text", 4, NULL);
    
    gtk_container_add(GTK_CONTAINER(scrolled), g_room_list_view);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
    
    return box;
}

GtkWidget* create_room_detail_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    
    // Notification bar
    g_notification_bar = gtk_info_bar_new();
    gtk_info_bar_set_message_type(GTK_INFO_BAR(g_notification_bar), GTK_MESSAGE_INFO);
    gtk_widget_set_no_show_all(g_notification_bar, TRUE);
    gtk_box_pack_start(GTK_BOX(box), g_notification_bar, FALSE, FALSE, 0);
    
    // Room info
    g_room_info_label = gtk_label_new("");
    gtk_label_set_use_markup(GTK_LABEL(g_room_info_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(g_room_info_label), 0);
    gtk_box_pack_start(GTK_BOX(box), g_room_info_label, FALSE, FALSE, 0);
    
    // Toolbar
    GtkWidget *toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    g_bid_button = gtk_button_new_with_label("💰 Đặt giá");
    g_buy_button = gtk_button_new_with_label("💵 Mua ngay");
    g_create_item_button = gtk_button_new_with_label("➕ Tạo vật phẩm");
    g_delete_item_button = gtk_button_new_with_label("🗑️ Xóa vật phẩm");
    GtkWidget *leave_button = gtk_button_new_with_label("◀ Rời phòng");
    
    g_signal_connect(g_bid_button, "clicked", G_CALLBACK(on_place_bid_clicked), NULL);
    g_signal_connect(g_buy_button, "clicked", G_CALLBACK(on_buy_now_clicked), NULL);
    g_signal_connect(g_create_item_button, "clicked", G_CALLBACK(on_create_item_clicked), NULL);
    g_signal_connect(g_delete_item_button, "clicked", G_CALLBACK(on_delete_item_clicked), NULL);
    g_signal_connect(leave_button, "clicked", G_CALLBACK(on_leave_room_clicked), NULL);
    
    // Initially hide all role-specific buttons (will be shown based on role when joining room)
    gtk_widget_set_no_show_all(g_bid_button, TRUE);
    gtk_widget_set_no_show_all(g_buy_button, TRUE);
    gtk_widget_set_no_show_all(g_create_item_button, TRUE);
    gtk_widget_set_no_show_all(g_delete_item_button, TRUE);
    
    gtk_box_pack_start(GTK_BOX(toolbar_box), g_bid_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), g_buy_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), g_create_item_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), g_delete_item_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(toolbar_box), leave_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(box), toolbar_box, FALSE, FALSE, 0);
    
    // Item list
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    // Create list store: ID, Name, Status, Start Price, Current Price, Buy Now, Countdown
    g_room_detail_store = gtk_list_store_new(7, G_TYPE_INT, G_TYPE_STRING, 
                                             G_TYPE_STRING, G_TYPE_INT, 
                                             G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);
    
    g_room_detail_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_room_detail_store));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_detail_view),
                                               -1, "ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_detail_view),
                                               -1, "Vật phẩm", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_detail_view),
                                               -1, "Trạng thái", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_detail_view),
                                               -1, "Giá khởi điểm", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_detail_view),
                                               -1, "Giá hiện tại", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_detail_view),
                                               -1, "Mua ngay", renderer, "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_room_detail_view),
                                               -1, "⏱️ Thời gian còn lại", renderer, "text", 6, NULL);
    
    gtk_container_add(GTK_CONTAINER(scrolled), g_room_detail_view);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
    
    return box;
}

// =============================================================
// MAIN FUNCTION
// =============================================================

int connect_to_server(const char* server_ip) {
    struct sockaddr_in serv_addr;
    
    g_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_socket_fd < 0) {
        return -1;
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
        return -1;
    }
    
    if (connect(g_socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
        return -1;
    }
    
    // Receive greeting
    wait_for_response_sync();
    
    return 0;
}

void on_window_destroy(GtkWidget *widget, gpointer data) {
    stop_receiver_thread();
    
    if (g_socket_fd >= 0) {
        send_command("LOGOUT");
        close(g_socket_fd);
        g_socket_fd = -1;
    }
    
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Connect to server
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    if (connect_to_server(server_ip) < 0) {
        GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
                                                         GTK_DIALOG_MODAL,
                                                         GTK_MESSAGE_ERROR,
                                                         GTK_BUTTONS_OK,
                                                         "Không thể kết nối đến server %s:%d",
                                                         server_ip, PORT);
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return 1;
    }
    
    // Create main window
    g_main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_main_window), "Hệ thống Đấu giá Trực tuyến");
    gtk_window_set_default_size(GTK_WINDOW(g_main_window), 900, 600);
    gtk_window_set_position(GTK_WINDOW(g_main_window), GTK_WIN_POS_CENTER);
    g_signal_connect(g_main_window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    // Create main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Create stack for different pages
    g_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(g_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    
    // Add pages
    gtk_stack_add_named(GTK_STACK(g_stack), create_login_page(), "login");
    gtk_stack_add_named(GTK_STACK(g_stack), create_register_page(), "register");
    gtk_stack_add_named(GTK_STACK(g_stack), create_room_list_page(), "room_list");
    gtk_stack_add_named(GTK_STACK(g_stack), create_room_detail_page(), "room_detail");
    
    // Set initial page
    gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "login");
    
    gtk_box_pack_start(GTK_BOX(main_box), g_stack, TRUE, TRUE, 0);
    
    // Status bar
    g_status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(main_box), g_status_bar, FALSE, FALSE, 0);
    update_status_bar("Đã kết nối đến server");
    
    gtk_container_add(GTK_CONTAINER(g_main_window), main_box);
    
    // Setup auto-refresh timer for room detail
    g_timeout_add(REFRESH_INTERVAL, auto_refresh_room, NULL);
    
    // Setup countdown timer (updates every second)
    g_countdown_timer_id = g_timeout_add(1000, update_countdown_timer, NULL);
    
    // Show window
    gtk_widget_show_all(g_main_window);
    
    // Start GTK main loop
    gtk_main();
    
    return 0;
}
