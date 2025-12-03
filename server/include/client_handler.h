/*
 * client_handler.h
 */

#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "server.h" 
#define LOG_FILE "data/server.log"

void add_client(int socket_fd);
void remove_client(int i);
void send_message(Client* client, const char* message);
int get_user_online_status(const char* username);
void log_message(Client* client, const char* direction, const char* message);

#endif // CLIENT_HANDLER_H