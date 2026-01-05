#include "../include/ui_room_list.h"
#include "../include/ui_room_detail.h"
#include "../include/ui_components.h"
#include "../include/network.h"
#include <string.h>

// =============================================================
// ROOM LIST PAGE FUNCTIONS
// =============================================================

void refresh_room_list() {
    send_command("GET_ROOM_LIST|ALL|1|50");
}

typedef struct {
    char data[BUFFER_SIZE];
} RoomListData;

gboolean update_room_list_ui(gpointer user_data) {
    RoomListData *data = (RoomListData*)user_data;
    
    if (!g_room_list_store) {
        free(data);
        return FALSE;
    }
    
    gtk_list_store_clear(g_room_list_store);
    
    char* ptr = strchr(data->data, '|');
    if (!ptr) {
        free(data);
        return FALSE;
    }
    ptr++;
    
    // Skip count
    ptr = strchr(ptr, '|');
    if (!ptr) {
        free(data);
        return FALSE;
    }
    ptr++;
    
    char room_data[BUFFER_SIZE];
    strncpy(room_data, ptr, BUFFER_SIZE);
    
    char* room = strtok(room_data, ";");
    while (room) {
        int id, item_count, participant_count;
        char name[50], owner[50], status[20], created[50];
        
        if (sscanf(room, "%d|%49[^|]|%49[^|]|%19[^|]|%d|%d|%49s", 
                   &id, name, owner, status, &item_count, &participant_count, created) == 7) {
            
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
    return FALSE;
}

void process_room_list_response(char* response) {
    if (!response || strncmp(response, "ROOM_LIST", 9) != 0) return;
    
    RoomListData *data = malloc(sizeof(RoomListData));
    strncpy(data->data, response, BUFFER_SIZE);
    
    g_idle_add(update_room_list_ui, data);
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
    g_admin_button = gtk_button_new_with_label("👤 Admin");
    GtkWidget *logout_button = gtk_button_new_with_label("🚪 Đăng xuất");
    
    // Hide admin button initially
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

// =============================================================
// EVENT HANDLERS
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
            
            update_status_bar("Đang tạo phòng...");
        } else {
            show_message_dialog(GTK_MESSAGE_WARNING, "Cảnh báo", "Vui lòng nhập đầy đủ thông tin!");
        }
    }
    
    gtk_widget_destroy(dialog);
}

void on_join_room_clicked(GtkWidget *widget, gpointer data) {
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
        
        g_joining_room = 1;
        
        char cmd[50];
        snprintf(cmd, sizeof(cmd), "JOIN_ROOM|%d", room_id);
        send_command(cmd);
        
        char* response = wait_for_response_sync();
        if (response && strncmp(response, "JOIN_ROOM_SUCCESS", 17) == 0) {
            g_current_room_id = room_id;
            strncpy(g_current_room_name, room_name, sizeof(g_current_room_name));
            
            g_is_room_owner = (strstr(response, "Chu phong") != NULL) ? 1 : 0;
            
            if (g_is_room_owner) {
                if (g_create_item_button) gtk_widget_show(g_create_item_button);
                if (g_delete_item_button) gtk_widget_show(g_delete_item_button);
                if (g_bid_button) gtk_widget_hide(g_bid_button);
                if (g_buy_button) gtk_widget_hide(g_buy_button);
            } else {
                if (g_create_item_button) gtk_widget_hide(g_create_item_button);
                if (g_delete_item_button) gtk_widget_hide(g_delete_item_button);
                if (g_bid_button) gtk_widget_show(g_bid_button);
                if (g_buy_button) gtk_widget_show(g_buy_button);
            }
            
            gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "room_detail");
            
            char status[128];
            snprintf(status, sizeof(status), "Đã vào phòng: %s", room_name);
            update_status_bar(status);
            
            refresh_room_detail();
        } else {
            g_current_room_id = 0;
            g_is_room_owner = 0;
            memset(g_current_room_name, 0, sizeof(g_current_room_name));
            show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi", response ? response : "Không thể vào phòng!");
        }
        
        g_joining_room = 0;
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
    
    if (g_user_info_label) {
        gtk_widget_hide(g_user_info_label);
    }
    
    if (g_admin_button) {
        gtk_widget_hide(g_admin_button);
    }
    
    stop_receiver_thread();
    
    gtk_stack_set_visible_child_name(GTK_STACK(g_stack), "login");
    update_status_bar("Đã đăng xuất");
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
    if (g_current_room_id > 0) {
        const gchar *current_page = gtk_stack_get_visible_child_name(GTK_STACK(g_stack));
        if (strcmp(current_page, "room_detail") == 0) {
            refresh_room_detail();
        }
    }
    return TRUE;
}
