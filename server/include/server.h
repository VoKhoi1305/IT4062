/*
 * server.h
 */

#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 4096
#define MAX_USERNAME 50
#define STARTING_USER_ID 1 // ID bắt đầu (ID mới sẽ là 101)

typedef struct {
    int socket_fd;
    char read_buffer[BUFFER_SIZE];
    int buffer_pos;
    
    // Trạng thái
    int is_logged_in; // 0 = false, 1 = true
    int user_id;      // <-- THÊM DÒNG NÀY (0 = chua xac thuc)
    char username[MAX_USERNAME];
    int role;         // 0 = User, 1 = Admin
    
} Client;

extern Client* g_clients[MAX_CLIENTS];
extern const char* DB_FILE;

#endif // SERVER_H