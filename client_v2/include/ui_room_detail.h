#ifndef UI_ROOM_DETAIL_H
#define UI_ROOM_DETAIL_H

#include "globals.h"

// =============================================================
// ROOM DETAIL PAGE FUNCTIONS
// =============================================================

// Create room detail page
GtkWidget* create_room_detail_page();

// Update room detail UI (thread-safe)
gboolean update_room_detail_ui(gpointer user_data);

// Refresh room detail
void refresh_room_detail();

// Auto-refresh callback
gboolean auto_refresh_room(gpointer user_data);

// Button callbacks
void on_leave_room_clicked(GtkWidget *widget, gpointer data);
void on_place_bid_clicked(GtkWidget *widget, gpointer data);
void on_buy_now_clicked(GtkWidget *widget, gpointer data);
void on_create_item_clicked(GtkWidget *widget, gpointer data);
void on_delete_item_clicked(GtkWidget *widget, gpointer data);

#endif // UI_ROOM_DETAIL_H
