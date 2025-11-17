/*
 * client_handler.h
 */

#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "server.h" 

void add_client(int socket_fd);
void remove_client(int i);
void send_message(Client* client, const char* message);

// Trả về: 1 (Online) nếu user đang đăng nhập, 0 (Offline) nếu không
int get_user_online_status(const char* username);

#endif // CLIENT_HANDLER_H