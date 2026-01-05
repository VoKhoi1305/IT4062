#include "../include/network.h"
#include "../include/response_handlers.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <stdio.h>

// =============================================================
// NETWORK FUNCTIONS
// =============================================================

int connect_to_server(const char* server_ip) {
    struct sockaddr_in serv_addr;
    
    g_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_socket_fd < 0) {
        return -1;
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
        return -1;
    }
    
    if (connect(g_socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
        return -1;
    }
    
    return 0;
}

void send_command(const char* cmd) {
    pthread_mutex_lock(&g_socket_mutex);
    if (g_socket_fd >= 0) {
        char buffer[BUFFER_SIZE];
        snprintf(buffer, BUFFER_SIZE, "%s\\n", cmd);
        send(g_socket_fd, buffer, strlen(buffer), 0);
    }
    pthread_mutex_unlock(&g_socket_mutex);
}

char* wait_for_response_sync() {
    static char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    
    pthread_mutex_lock(&g_socket_mutex);
    if (g_socket_fd < 0) {
        pthread_mutex_unlock(&g_socket_mutex);
        return NULL;
    }
    
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    
    FD_ZERO(&read_fds);
    FD_SET(g_socket_fd, &read_fds);
    
    int activity = select(g_socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (activity > 0 && FD_ISSET(g_socket_fd, &read_fds)) {
        int bytes = recv(g_socket_fd, buffer, BUFFER_SIZE - 1, 0);
        pthread_mutex_unlock(&g_socket_mutex);
        
        if (bytes <= 0) return NULL;
        
        buffer[bytes] = '\0';
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        strncpy(response, buffer, BUFFER_SIZE);
        return response;
    }
    
    pthread_mutex_unlock(&g_socket_mutex);
    return NULL;
}

void* receiver_thread_func(void* arg) {
    char buffer[BUFFER_SIZE];
    int buffer_pos = 0;
    
    while (g_thread_running) {
        pthread_mutex_lock(&g_socket_mutex);
        
        if (g_socket_fd < 0) {
            pthread_mutex_unlock(&g_socket_mutex);
            break;
        }
        
        fd_set read_fds;
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        FD_ZERO(&read_fds);
        FD_SET(g_socket_fd, &read_fds);
        
        int activity = select(g_socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity > 0 && FD_ISSET(g_socket_fd, &read_fds)) {
            int bytes = recv(g_socket_fd, buffer + buffer_pos, 
                           BUFFER_SIZE - buffer_pos - 1, 0);
            
            if (bytes <= 0) {
                pthread_mutex_unlock(&g_socket_mutex);
                break;
            }
            
            buffer_pos += bytes;
            buffer[buffer_pos] = '\0';
            
            char* line_start = buffer;
            char* newline;
            
            while ((newline = strchr(line_start, '\n')) != NULL) {
                *newline = '\0';
                
                // Process the complete line - handle different message types
                handle_server_message(line_start);
                
                line_start = newline + 1;
            }
            
            // Move remaining data to start of buffer
            int remaining = buffer_pos - (line_start - buffer);
            if (remaining > 0) {
                memmove(buffer, line_start, remaining);
            }
            buffer_pos = remaining;
        }
        
        pthread_mutex_unlock(&g_socket_mutex);
    }
    
    return NULL;
}

void close_connection() {
    if (g_socket_fd >= 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
    }
}

void start_receiver_thread() {
    if (!g_thread_running) {
        g_thread_running = 1;
        pthread_create(&g_receiver_thread, NULL, receiver_thread_func, NULL);
    }
}

void stop_receiver_thread() {
    if (g_thread_running) {
        g_thread_running = 0;
        pthread_join(g_receiver_thread, NULL);
    }
}
