#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "globals.h"

// =============================================================
// REUSABLE UI COMPONENTS
// =============================================================

// Create datetime picker widget
GtkWidget* create_datetime_picker(const char* label_text, char* output_buffer, int buffer_size);

// Extract datetime from picker widget
void get_datetime_from_picker(GtkWidget *picker_vbox, char* output_buffer, int buffer_size);

// Show message dialog
void show_message_dialog(GtkMessageType type, const char* title, const char* message);

// Show error dialog (shortcut)
void show_error_dialog(const char* message);

// Show notification in room detail page
void show_notification(const char* message, GtkMessageType type);

// Update status bar
void update_status_bar(const char* message);

// Format countdown timer
void format_countdown(time_t end_time, char* buffer, size_t buffer_size);

// Countdown timer callback
gboolean update_countdown_timer(gpointer user_data);

#endif // UI_COMPONENTS_H
