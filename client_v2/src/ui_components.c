#include "../include/ui_components.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// =============================================================
// REUSABLE UI COMPONENTS
// =============================================================

GtkWidget* create_datetime_picker(const char* label_text, char* output_buffer, int buffer_size) {
    // TODO: Copy từ client_gtk.c lines 143-185
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    GtkWidget *label = gtk_label_new(label_text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget *date_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(date_entry), "YYYY-MM-DD");
    gtk_entry_set_width_chars(GTK_ENTRY(date_entry), 12);
    
    GtkWidget *hour_spin = gtk_spin_button_new_with_range(0, 23, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hour_spin), 0);
    
    GtkWidget *minute_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(minute_spin), 0);
    
    GtkWidget *second_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(second_spin), 0);
    
    GtkWidget *colon1 = gtk_label_new(":");
    GtkWidget *colon2 = gtk_label_new(":");
    
    gtk_box_pack_start(GTK_BOX(hbox), date_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hour_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), colon1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), minute_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), colon2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), second_spin, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    g_object_set_data(G_OBJECT(vbox), "date_entry", date_entry);
    g_object_set_data(G_OBJECT(vbox), "hour_spin", hour_spin);
    g_object_set_data(G_OBJECT(vbox), "minute_spin", minute_spin);
    g_object_set_data(G_OBJECT(vbox), "second_spin", second_spin);
    
    return vbox;
}

void get_datetime_from_picker(GtkWidget *picker_vbox, char* output_buffer, int buffer_size) {
    // TODO: Copy từ client_gtk.c với fix format date
    GtkWidget *date_entry = g_object_get_data(G_OBJECT(picker_vbox), "date_entry");
    GtkWidget *hour_spin = g_object_get_data(G_OBJECT(picker_vbox), "hour_spin");
    GtkWidget *minute_spin = g_object_get_data(G_OBJECT(picker_vbox), "minute_spin");
    GtkWidget *second_spin = g_object_get_data(G_OBJECT(picker_vbox), "second_spin");
    
    const char* date = gtk_entry_get_text(GTK_ENTRY(date_entry));
    int hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(hour_spin));
    int minute = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(minute_spin));
    int second = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(second_spin));
    
    if (strlen(date) > 0) {
        int year, month, day;
        if (sscanf(date, "%d-%d-%d", &year, &month, &day) == 3) {
            snprintf(output_buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d", 
                    year, month, day, hour, minute, second);
        } else {
            snprintf(output_buffer, buffer_size, "%s %02d:%02d:%02d", date, hour, minute, second);
        }
    } else {
        output_buffer[0] = '\0';
    }
}

void show_message_dialog(GtkMessageType type, const char* title, const char* message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_main_window),
                                               GTK_DIALOG_MODAL,
                                               type,
                                               GTK_BUTTONS_OK,
                                               "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void show_error_dialog(const char* message) {
    show_message_dialog(GTK_MESSAGE_ERROR, "Lỗi", message);
}

void show_notification(const char* message, GtkMessageType type) {
    if (!g_notification_bar) return;
    
    gtk_info_bar_set_message_type(GTK_INFO_BAR(g_notification_bar), type);
    
    GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(g_notification_bar));
    GList *children = gtk_container_get_children(GTK_CONTAINER(content));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_container_add(GTK_CONTAINER(content), label);
    gtk_widget_show_all(g_notification_bar);
    gtk_widget_show(g_notification_bar);
}

void update_status_bar(const char* message) {
    if (g_status_bar) {
        gtk_statusbar_push(GTK_STATUSBAR(g_status_bar), 0, message);
    }
}

void format_countdown(time_t end_time, char* buffer, size_t buffer_size) {
    time_t now = time(NULL);
    int remaining = (int)difftime(end_time, now);
    
    if (remaining <= 0) {
        snprintf(buffer, buffer_size, "⏰ Đã hết hạn");
        return;
    }
    
    int hours = remaining / 3600;
    int minutes = (remaining % 3600) / 60;
    int seconds = remaining % 60;
    
    if (hours > 0) {
        snprintf(buffer, buffer_size, "⏱️ %02d:%02d:%02d", hours, minutes, seconds);
    } else {
        snprintf(buffer, buffer_size, "⏱️ %02d:%02d", minutes, seconds);
    }
}

gboolean update_countdown_timer(gpointer user_data) {
    if (!g_room_detail_store || !g_item_timers) {
        return TRUE;
    }
    
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_room_detail_store), &iter);
    
    while (valid) {
        int item_id;
        gtk_tree_model_get(GTK_TREE_MODEL(g_room_detail_store), &iter, 0, &item_id, -1);
        
        gpointer end_time_ptr = g_hash_table_lookup(g_item_timers, GINT_TO_POINTER(item_id));
        
        if (end_time_ptr) {
            time_t end_time = GPOINTER_TO_INT(end_time_ptr);
            char countdown_str[50];
            format_countdown(end_time, countdown_str, sizeof(countdown_str));
            
            gtk_list_store_set(g_room_detail_store, &iter, 6, countdown_str, -1);
        }
        
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(g_room_detail_store), &iter);
    }
    
    return TRUE;
}
