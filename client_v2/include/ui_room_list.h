#ifndef UI_ROOM_LIST_H
#define UI_ROOM_LIST_H

#include "globals.h"

// =============================================================
// ROOM LIST PAGE FUNCTIONS
// =============================================================

// Create room list page
GtkWidget* create_room_list_page();

// Update room list UI (thread-safe)
gboolean update_room_list_ui(gpointer user_data);

// Refresh room list
void refresh_room_list();

// Button callbacks
void on_create_room_clicked(GtkWidget *widget, gpointer data);
void on_join_room_clicked(GtkWidget *widget, gpointer data);
void on_refresh_rooms_clicked(GtkWidget *widget, gpointer data);
void on_search_items_clicked(GtkWidget *widget, gpointer data);
void on_view_history_clicked(GtkWidget *widget, gpointer data);
void on_logout_clicked(GtkWidget *widget, gpointer data);
void on_admin_panel_clicked(GtkWidget *widget, gpointer data);

#endif // UI_ROOM_LIST_H
