#include "../include/ui_room_detail.h"
#include "../include/ui_components.h"
#include "../include/network.h"
#include <string.h>
#include <time.h>

// =============================================================
// ROOM DETAIL PAGE FUNCTIONS
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
    
    if (!g_room_detail_store || !g_room_info_label) {
        free(data);
        return FALSE;
    }
    
    char info[512];
    snprintf(info, sizeof(info), 
             "<b>Phòng:</b> %s (#%d)\n<b>Trạng thái:</b> %s\n<b>Thời gian:</b> %s → %s",
             data->room_name, data->room_id, data->status, data->start_time, data->end_time);
    gtk_label_set_markup(GTK_LABEL(g_room_info_label), info);
    
    gtk_list_store_clear(g_room_detail_store);
    
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
            
            int parsed = sscanf(item, "%d|%99[^|]|%19[^|]|%lf|%lf|%lf|%29[^|]|%29[^|]|%29[^|]|%29[^|]|%d", 
                       &item_id, item_name, item_status, 
                       &start_price, &current_price, &buy_now_price,
                       auction_start, auction_end, sched_start, sched_end, &duration);
            
            if (parsed >= 6) {
                if (strcmp(item_status, "ACTIVE") != 0) {
                    item = strtok(NULL, ";");
                    continue;
                }
                
                if (strlen(auction_end) > 0 && strcmp(auction_end, "NULL") != 0) {
                    struct tm tm_info;
                    memset(&tm_info, 0, sizeof(tm_info));
                    if (sscanf(auction_end, "%d-%d-%d %d:%d:%d",
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
                
                GtkTreeIter iter;
                gtk_list_store_append(g_room_detail_store, &iter);
                
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
                
                char countdown_str[50] = "⏱️ --:--";
                if (strlen(auction_end) > 0 && strcmp(auction_end, "NULL") != 0) {
                    gpointer end_time_ptr = g_hash_table_lookup(g_item_timers, GINT_TO_POINTER(item_id));
                    if (end_time_ptr) {
                        format_countdown(GPOINTER_TO_INT(end_time_ptr), countdown_str, sizeof(countdown_str));
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
    return FALSE;
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
    
    token = strtok(NULL, "");
    if (token) strncpy(data->items_data, token, sizeof(data->items_data));
    else data->items_data[0] = '\0';
    
    g_idle_add(update_room_detail_ui, data);
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
    
    // Initially hide all role-specific buttons
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
// EVENT HANDLERS
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
            send_command(cmd);
            
            update_status_bar("Đã gửi lệnh tạo vật phẩm");
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
        show_error_dialog("Vui lòng chọn một vật phẩm!");
        return;
    }
    
    int item_id;
    char *item_name;
    gtk_tree_model_get(model, &iter, 0, &item_id, 1, &item_name, -1);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Bạn có chắc muốn xóa vật phẩm '%s'?", item_name);
    
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
        send_command(cmd);
        
        update_status_bar("Đã gửi lệnh xóa vật phẩm");
    }
    
    g_free(item_name);
}

void on_leave_room_clicked(GtkWidget *widget, gpointer data) {
    send_command("LEAVE_ROOM");
    
    g_current_room_id = 0;
    g_is_room_owner = 0;
    memset(g_current_room_name, 0, sizeof(g_current_room_name));
    
    gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "room_list");
    update_status_bar("Đã rời phòng");
    
    refresh_room_list();
}
