#ifndef NETWORK_H
#define NETWORK_H

#include "globals.h"

// =============================================================
// NETWORK FUNCTIONS
// =============================================================

// Connect to server
int connect_to_server(const char* server_ip);

// Send command to server
void send_command(const char* cmd);

// Wait for synchronous response (with timeout)
char* wait_for_response_sync();

// Receiver thread function
void* receiver_thread_func(void* arg);

// Thread management
void start_receiver_thread();
void stop_receiver_thread();

// Close connection
void close_connection();

#endif // NETWORK_H
