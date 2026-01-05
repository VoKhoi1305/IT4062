#include "../include/globals.h"
#include "../include/network.h"
#include "../include/ui_components.h"
#include "../include/ui_login.h"
#include "../include/ui_room_list.h"
#include "../include/ui_room_detail.h"
#include <stdlib.h>

// =============================================================
// WINDOW DESTROY CALLBACK
// =============================================================

void on_window_destroy(GtkWidget *widget, gpointer data) {
    g_thread_running = 0;
    
    if (g_countdown_timer_id > 0) {
        g_source_remove(g_countdown_timer_id);
    }
    
    pthread_join(g_receiver_thread, NULL);
    close_connection();
    
    if (g_item_timers) {
        g_hash_table_destroy(g_item_timers);
    }
    
    gtk_main_quit();
}

// =============================================================
// MAIN FUNCTION
// =============================================================

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
    
    // Start receiver thread
    g_thread_running = 1;
    pthread_create(&g_receiver_thread, NULL, receiver_thread_func, NULL);
    
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
