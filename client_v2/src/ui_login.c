#include "../include/ui_login.h"
#include "../include/ui_room_list.h"
#include "../include/network.h"
#include "../include/ui_components.h"
#include <string.h>

// =============================================================
// LOGIN/REGISTER PAGE FUNCTIONS
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
    g_login_username = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_login_username), "Nhập tên đăng nhập");
    
    GtkWidget *password_label = gtk_label_new("Mật khẩu:");
    g_login_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(g_login_password), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_login_password), "Nhập mật khẩu");
    
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_login_username, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_login_password, 1, 1, 1, 1);
    
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
    g_register_username = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_register_username), "Nhập tên đăng nhập");
    
    GtkWidget *password_label = gtk_label_new("Mật khẩu:");
    g_register_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(g_register_password), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_register_password), "Nhập mật khẩu");
    
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_register_username, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_register_password, 1, 1, 1, 1);
    
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

void on_login_clicked(GtkWidget *widget, gpointer data) {
    const char* username = gtk_entry_get_text(GTK_ENTRY(g_login_username));
    const char* password = gtk_entry_get_text(GTK_ENTRY(g_login_password));
    
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
        gtk_entry_set_text(GTK_ENTRY(g_login_password), "");
        
        // Load room list
        refresh_room_list();
    } else {
        show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi đăng nhập", 
                          response ? response : "Sai thông tin đăng nhập!");
    }
}

void on_register_clicked(GtkWidget *widget, gpointer data) {
    const char* username = gtk_entry_get_text(GTK_ENTRY(g_register_username));
    const char* password = gtk_entry_get_text(GTK_ENTRY(g_register_password));
    
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
        gtk_entry_set_text(GTK_ENTRY(g_register_username), "");
        gtk_entry_set_text(GTK_ENTRY(g_register_password), "");
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
