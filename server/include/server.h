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
#define STARTING_USER_ID 1

typedef struct {
    int socket_fd;
    char read_buffer[BUFFER_SIZE];
    int buffer_pos;
    
    // Trạng thái xác thực
    int is_logged_in; // 0 = false, 1 = true
    int user_id;      // 0 = chưa xác thực
    char username[MAX_USERNAME];
    int role;         // 0 = User, 1 = Admin
    
    // Trạng thái phòng đấu giá
    int current_room_id; // 0 = không ở phòng nào, >0 = ID phòng hiện tại
    
} Client;

extern Client* g_clients[MAX_CLIENTS];
extern const char* DB_FILE;

// =============================================================
// Debug logging (disabled by default)
// Enable by compiling with: -DENABLE_DEBUG_LOG
// =============================================================
#ifdef ENABLE_DEBUG_LOG
#define DEBUG_LOG(...) do { printf(__VA_ARGS__); } while (0)
#else
#define DEBUG_LOG(...) do { } while (0)
#endif

#endif // SERVER_H