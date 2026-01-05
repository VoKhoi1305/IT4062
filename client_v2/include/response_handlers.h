#ifndef RESPONSE_HANDLERS_H
#define RESPONSE_HANDLERS_H

#include "globals.h"

// =============================================================
// RESPONSE HANDLER FUNCTIONS
// =============================================================

// Main dispatcher for server messages
void handle_server_message(char* line);

// Individual response processors
void process_room_list_response(char* response);
void process_room_detail_response(char* response);

// Notification types
typedef struct {
    char message[512];
    GtkMessageType type;
} NotificationData;

// Show notification (thread-safe)
gboolean show_notification_ui(gpointer user_data);

#endif // RESPONSE_HANDLERS_H
