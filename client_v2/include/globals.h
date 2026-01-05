#ifndef GLOBALS_H
#define GLOBALS_H

#include <gtk/gtk.h>
#include <pthread.h>

// =============================================================
// CONSTANTS
// =============================================================

#define PORT 8888
#define BUFFER_SIZE 8192
#define REFRESH_INTERVAL 5000  // milliseconds

// =============================================================
// GLOBAL WIDGETS
// =============================================================

extern GtkWidget *g_main_window;
extern GtkWidget *g_stack;
extern GtkWidget *g_status_bar;

// Login/Register widgets
extern GtkWidget *g_login_username;
extern GtkWidget *g_login_password;
extern GtkWidget *g_register_username;
extern GtkWidget *g_register_password;

// Room list widgets
extern GtkWidget *g_room_list_view;
extern GtkListStore *g_room_list_store;
extern GtkWidget *g_admin_button;

// Room detail widgets
extern GtkWidget *g_room_detail_view;
extern GtkListStore *g_room_detail_store;
extern GtkWidget *g_room_info_label;
extern GtkWidget *g_notification_bar;

// Room detail buttons (to show/hide based on role)
extern GtkWidget *g_bid_button;
extern GtkWidget *g_buy_button;
extern GtkWidget *g_create_item_button;
extern GtkWidget *g_delete_item_button;
extern int g_is_room_owner;

// User info widget
extern GtkWidget *g_user_info_label;

// Dialog widgets (for populating from receiver thread)
extern GtkListStore *g_search_result_store;
extern GtkListStore *g_history_store;
extern GtkListStore *g_admin_user_store;

// =============================================================
// GLOBAL VARIABLES
// =============================================================

extern int g_socket_fd;
extern int g_is_logged_in;
extern int g_user_role;
extern char g_username[50];
extern int g_current_room_id;
extern char g_current_room_name[100];

// Thread control
extern pthread_t g_receiver_thread;
extern pthread_mutex_t g_socket_mutex;
extern int g_thread_running;

// Join room protection flag
extern volatile int g_joining_room;

// Item timing data
extern GHashTable *g_item_timers;
extern guint g_countdown_timer_id;

#endif // GLOBALS_H
