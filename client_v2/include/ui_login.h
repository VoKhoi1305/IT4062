#ifndef UI_LOGIN_H
#define UI_LOGIN_H

#include "globals.h"

// =============================================================
// LOGIN/REGISTER PAGE FUNCTIONS
// =============================================================

// Create login page
GtkWidget* create_login_page();

// Create register page
GtkWidget* create_register_page();

// Button callbacks
void on_login_clicked(GtkWidget *widget, gpointer data);
void on_show_register_clicked(GtkWidget *widget, gpointer data);
void on_register_clicked(GtkWidget *widget, gpointer data);
void on_show_login_clicked(GtkWidget *widget, gpointer data);

#endif // UI_LOGIN_H
