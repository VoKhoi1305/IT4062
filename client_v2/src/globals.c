#include "../include/globals.h"

// =============================================================
// GLOBAL WIDGETS DEFINITIONS
// =============================================================

GtkWidget *g_main_window = NULL;
GtkWidget *g_stack = NULL;
GtkWidget *g_status_bar = NULL;

// Login/Register widgets
GtkWidget *g_login_username = NULL;
GtkWidget *g_login_password = NULL;
GtkWidget *g_register_username = NULL;
GtkWidget *g_register_password = NULL;

// Room list widgets
GtkWidget *g_room_list_view = NULL;
GtkListStore *g_room_list_store = NULL;
GtkWidget *g_admin_button = NULL;

// Room detail widgets
GtkWidget *g_room_detail_view = NULL;
GtkListStore *g_room_detail_store = NULL;
GtkWidget *g_room_info_label = NULL;
GtkWidget *g_notification_bar = NULL;

// Room detail buttons
GtkWidget *g_bid_button = NULL;
GtkWidget *g_buy_button = NULL;
GtkWidget *g_create_item_button = NULL;
GtkWidget *g_delete_item_button = NULL;
int g_is_room_owner = 0;

// User info widget
GtkWidget *g_user_info_label = NULL;

// Dialog widgets
GtkListStore *g_search_result_store = NULL;
GtkListStore *g_history_store = NULL;
GtkListStore *g_admin_user_store = NULL;

// =============================================================
// GLOBAL VARIABLES DEFINITIONS
// =============================================================

int g_socket_fd = -1;
int g_is_logged_in = 0;
int g_user_role = 0;
char g_username[50] = "";
int g_current_room_id = 0;
char g_current_room_name[100] = "";

// Thread control
pthread_t g_receiver_thread;
pthread_mutex_t g_socket_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_thread_running = 0;

// Join room protection flag
volatile int g_joining_room = 0;

// Item timing data
GHashTable *g_item_timers = NULL;
guint g_countdown_timer_id = 0;
