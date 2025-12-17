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

// Room detail widgets
GtkWidget *g_room_detail_view = NULL;
GtkListStore *g_room_detail_store = NULL;
GtkWidget *g_room_info_label = NULL;

// Thread control
pthread_t g_receiver_thread;
pthread_mutex_t g_socket_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_thread_running = 0;

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

void update_status_bar(const char* message) {
    if (g_status_bar) {
        gtk_statusbar_push(GTK_STATUSBAR(g_status_bar), 0, message);
    }
}

void show_message_dialog(GtkMessageType type, const char* title, const char* message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_main_window),
                                               GTK_DIALOG_MODAL,
                                               type,
                                               GTK_BUTTONS_OK,
                                               "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
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

gboolean update_room_detail_ui(gpointer user_data) {
    RoomDetailData *data = (RoomDetailData*)user_data;
    
    // Update room info label
    char info[512];
    snprintf(info, sizeof(info), 
             "<b>Phòng:</b> %s (#%d)\n<b>Trạng thái:</b> %s\n<b>Thời gian:</b> %s → %s",
             data->room_name, data->room_id, data->status, data->start_time, data->end_time);
    gtk_label_set_markup(GTK_LABEL(g_room_info_label), info);
    
    // Clear and update items
    gtk_list_store_clear(g_room_detail_store);
    
    // Parse items
    if (strlen(data->items_data) > 0) {
        char items_buf[BUFFER_SIZE];
        strncpy(items_buf, data->items_data, BUFFER_SIZE);
        
        char* item = strtok(items_buf, ";");
        while (item) {
            int item_id;
            char item_name[100], item_status[20];
            double start_price, current_price, buy_now_price;
            
            if (sscanf(item, "%d|%99[^|]|%19[^|]|%lf|%lf|%lf", 
                       &item_id, item_name, item_status, 
                       &start_price, &current_price, &buy_now_price) >= 6) {
                
                GtkTreeIter iter;
                gtk_list_store_append(g_room_detail_store, &iter);
                gtk_list_store_set(g_room_detail_store, &iter,
                                  0, item_id,
                                  1, item_name,
                                  2, item_status,
                                  3, (int)start_price,
                                  4, (int)current_price,
                                  5, (int)buy_now_price,
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
                } else if (strncmp(line_start, "NEW_BID", 7) == 0 ||
                          strncmp(line_start, "BID_SUCCESS", 11) == 0 ||
                          strncmp(line_start, "BUY_NOW_SUCCESS", 15) == 0 ||
                          strncmp(line_start, "ITEM_STARTED", 12) == 0 ||
                          strncmp(line_start, "ITEM_SOLD", 9) == 0) {
                    // Refresh room detail on important events
                    if (g_current_room_id > 0) {
                        refresh_room_detail();
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
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    
    // Room name
    GtkWidget *name_label = gtk_label_new("Tên phòng:");
    GtkWidget *name_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);
    
    // Start time
    GtkWidget *start_label = gtk_label_new("Bắt đầu:");
    GtkWidget *start_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(start_entry), "YYYY-MM-DD HH:MM:SS");
    gtk_grid_attach(GTK_GRID(grid), start_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), start_entry, 1, 1, 1, 1);
    
    // End time
    GtkWidget *end_label = gtk_label_new("Kết thúc:");
    GtkWidget *end_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(end_entry), "YYYY-MM-DD HH:MM:SS");
    gtk_grid_attach(GTK_GRID(grid), end_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), end_entry, 1, 2, 1, 1);
    
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);
    
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (result == GTK_RESPONSE_ACCEPT) {
        const char* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        const char* start = gtk_entry_get_text(GTK_ENTRY(start_entry));
        const char* end = gtk_entry_get_text(GTK_ENTRY(end_entry));
        
        if (strlen(name) > 0 && strlen(start) > 0 && strlen(end) > 0) {
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "CREATE_ROOM|%s|%s|%s", name, start, end);
            send_command(cmd);
            
            char* response = wait_for_response_sync();
            if (response && strncmp(response, "CREATE_ROOM_SUCCESS", 19) == 0) {
                show_message_dialog(GTK_MESSAGE_INFO, "Thành công", "Tạo phòng thành công!");
                refresh_room_list();
            } else {
                show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi", response ? response : "Tạo phòng thất bại!");
            }
        }
    }
    
    gtk_widget_destroy(dialog);
}

void on_join_room_clicked(GtkWidget *widget, gpointer data) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_room_list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        int room_id;
        char *room_name;
        
        gtk_tree_model_get(model, &iter, 0, &room_id, 1, &room_name, -1);
        
        char cmd[50];
        snprintf(cmd, sizeof(cmd), "JOIN_ROOM|%d", room_id);
        send_command(cmd);
        
        char* response = wait_for_response_sync();
        if (response && strncmp(response, "JOIN_ROOM_SUCCESS", 17) == 0) {
            g_current_room_id = room_id;
            strncpy(g_current_room_name, room_name, sizeof(g_current_room_name));
            
            // Switch to room detail view
            gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "room_detail");
            
            char status[128];
            snprintf(status, sizeof(status), "Đã vào phòng: %s", room_name);
            update_status_bar(status);
            
            // Request room detail
            refresh_room_detail();
        } else {
            show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi", response ? response : "Không thể vào phòng!");
        }
        
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

void on_leave_room_clicked(GtkWidget *widget, gpointer data) {
    send_command("LEAVE_ROOM");
    
    g_current_room_id = 0;
    memset(g_current_room_name, 0, sizeof(g_current_room_name));
    
    // Switch back to room list
    gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "room_list");
    update_status_bar("Đã rời phòng");
    
    refresh_room_list();
}

gboolean auto_refresh_room(gpointer data) {
    if (g_current_room_id > 0) {
        const gchar *current_page = gtk_stack_get_visible_child_name(GTK_STACK(g_stack));
        if (strcmp(current_page, "room_detail") == 0) {
            refresh_room_detail();
        }
    }
    return TRUE;  // Continue calling
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
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='large' weight='bold'>DANH SÁCH PHÒNG ĐẤU GIÁ</span>");
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    
    // Toolbar
    GtkWidget *toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    GtkWidget *refresh_button = gtk_button_new_with_label("🔄 Làm mới");
    GtkWidget *create_button = gtk_button_new_with_label("➕ Tạo phòng");
    GtkWidget *join_button = gtk_button_new_with_label("▶ Vào phòng");
    GtkWidget *logout_button = gtk_button_new_with_label("🚪 Đăng xuất");
    
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_rooms_clicked), NULL);
    g_signal_connect(create_button, "clicked", G_CALLBACK(on_create_room_clicked), NULL);
    g_signal_connect(join_button, "clicked", G_CALLBACK(on_join_room_clicked), NULL);
    g_signal_connect(logout_button, "clicked", G_CALLBACK(on_logout_clicked), NULL);
    
    gtk_box_pack_start(GTK_BOX(toolbar_box), refresh_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), create_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), join_button, FALSE, FALSE, 0);
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
    
    // Room info
    g_room_info_label = gtk_label_new("");
    gtk_label_set_use_markup(GTK_LABEL(g_room_info_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(g_room_info_label), 0);
    gtk_box_pack_start(GTK_BOX(box), g_room_info_label, FALSE, FALSE, 0);
    
    // Toolbar
    GtkWidget *toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    GtkWidget *bid_button = gtk_button_new_with_label("💰 Đặt giá");
    GtkWidget *buy_button = gtk_button_new_with_label("💵 Mua ngay");
    GtkWidget *leave_button = gtk_button_new_with_label("◀ Rời phòng");
    
    g_signal_connect(bid_button, "clicked", G_CALLBACK(on_place_bid_clicked), NULL);
    g_signal_connect(buy_button, "clicked", G_CALLBACK(on_buy_now_clicked), NULL);
    g_signal_connect(leave_button, "clicked", G_CALLBACK(on_leave_room_clicked), NULL);
    
    gtk_box_pack_start(GTK_BOX(toolbar_box), bid_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), buy_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(toolbar_box), leave_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(box), toolbar_box, FALSE, FALSE, 0);
    
    // Item list
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    // Create list store: ID, Name, Status, Start Price, Current Price, Buy Now
    g_room_detail_store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, 
                                             G_TYPE_STRING, G_TYPE_INT, 
                                             G_TYPE_INT, G_TYPE_INT);
    
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
    
    // Show window
    gtk_widget_show_all(g_main_window);
    
    // Start GTK main loop
    gtk_main();
    
    return 0;
}
