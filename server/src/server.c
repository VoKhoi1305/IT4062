
// /*
//  * src/server.c - Main với Timer Integration
//  */

// #include "server.h"
// #include "client_handler.h"
// #include "command_handler.h"
// #include "timer_handler.h"

// int main() {
//     int listening_socket;
//     struct sockaddr_in server_addr;

//     // Khởi tạo timer system
//     init_timer_system();

//     listening_socket = socket(AF_INET, SOCK_STREAM, 0);
//     int opt = 1;
//     setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//     memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = INADDR_ANY;
//     server_addr.sin_port = htons(PORT);

//     if (bind(listening_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         perror("bind() failed"); 
//         exit(EXIT_FAILURE);
//     }
//     if (listen(listening_socket, 5) < 0) {
//         perror("listen() failed"); 
//         exit(EXIT_FAILURE);
//     }

//     fd_set read_fds;
//     int max_fd;
//     struct timeval timeout;

//     while (1) {
//         FD_ZERO(&read_fds);
//         FD_SET(listening_socket, &read_fds);
//         max_fd = listening_socket;

//         for (int i = 0; i < MAX_CLIENTS; i++) {
//             if (g_clients[i]) {
//                 FD_SET(g_clients[i]->socket_fd, &read_fds);
//                 if (g_clients[i]->socket_fd > max_fd) {
//                     max_fd = g_clients[i]->socket_fd;
//                 }
//             }
//         }
        
//         // Set timeout 1 giây để xử lý timer
//         timeout.tv_sec = 1;
//         timeout.tv_usec = 0;
        
//         int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout); 

//         if (activity < 0 && errno != EINTR) {
//             perror("select() error");
//             continue;
//         }
        
//         // Xử lý timer mỗi vòng lặp
//         process_timers();

//         if (activity == 0) {
//             // Timeout - chỉ xử lý timer
//             continue;
//         }


//         if (FD_ISSET(listening_socket, &read_fds)) {
//             int new_socket = accept(listening_socket, NULL, NULL);
//             if (new_socket < 0) {
//                 perror("accept() failed");
//             } else {
//                 add_client(new_socket);
//             }
//         }

//         // Xử lý dữ liệu từ clients
//         for (int i = 0; i < MAX_CLIENTS; i++) {
//             if (g_clients[i] && FD_ISSET(g_clients[i]->socket_fd, &read_fds)) {
//                 Client* client = g_clients[i];
//                 int bytes_recvd = recv(client->socket_fd, 
//                                        client->read_buffer + client->buffer_pos, 
//                                        BUFFER_SIZE - client->buffer_pos, 0);
//                 if (bytes_recvd <= 0) {
//                     remove_client(i);
//                 } else {
//                     printf("[RECV from %d]: %.*s\n", 
//                     client->socket_fd, 
//                     bytes_recvd, 
//                     client->read_buffer + client->buffer_pos);
//                     client->buffer_pos += bytes_recvd;
//                     process_command_buffer(client);
//                 }
//             }
//         }
//     }
    
//     cleanup_timer_system();
//     close(listening_socket);
//     return 0;
// }

#include "server.h"
#include "client_handler.h"
#include "command_handler.h"
#include "timer_handler.h"
#include "room_handler.h"

int main() {
    int listening_socket;
    struct sockaddr_in server_addr;

    // Thiết lập timezone UTC+7 (Múi giờ Việt Nam)
    setenv("TZ", "Asia/Ho_Chi_Minh", 1);
    tzset();
    
    printf("Server starting in timezone: UTC+7 (Asia/Ho_Chi_Minh)\n");
    
    // Khởi tạo timer system
    init_timer_system();

    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listening_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed"); 
        exit(EXIT_FAILURE);
    }
    if (listen(listening_socket, 5) < 0) {
        perror("listen() failed"); 
        exit(EXIT_FAILURE);
    }

    fd_set read_fds;
    int max_fd;
    struct timeval timeout;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(listening_socket, &read_fds);
        max_fd = listening_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (g_clients[i]) {
                FD_SET(g_clients[i]->socket_fd, &read_fds);
                if (g_clients[i]->socket_fd > max_fd) {
                    max_fd = g_clients[i]->socket_fd;
                }
            }
        }
        
        // Set timeout 1 giây để xử lý timer
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout); 

        if (activity < 0 && errno != EINTR) {
            perror("select() error");
            continue;
        }
        

        process_timers();
        check_and_update_room_statuses();

        if (activity == 0) {
            continue;
        }

      
        if (FD_ISSET(listening_socket, &read_fds)) {
            int new_socket = accept(listening_socket, NULL, NULL);
            if (new_socket < 0) {
                perror("accept() failed");
            } else {
                add_client(new_socket);
            }
        }

      
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (g_clients[i] && FD_ISSET(g_clients[i]->socket_fd, &read_fds)) {
                Client* client = g_clients[i];
                int bytes_recvd = recv(client->socket_fd, 
                                       client->read_buffer + client->buffer_pos, 
                                       BUFFER_SIZE - client->buffer_pos, 0);
                if (bytes_recvd <= 0) {
                    remove_client(i);
                } else {
                    printf("[RECV from %d]: %.*s\n", 
                    client->socket_fd, 
                    bytes_recvd, 
                    client->read_buffer + client->buffer_pos);
                    client->buffer_pos += bytes_recvd;
                    process_command_buffer(client);
                }
            }
        }
    }
    
    cleanup_timer_system();
    close(listening_socket);
    return 0;
}