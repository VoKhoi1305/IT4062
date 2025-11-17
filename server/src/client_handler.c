/*
 * src/client_handler.c
 */

#include "client_handler.h"
#include "server.h"

// --- ĐỊNH NGHĨA BIẾN TOÀN CỤC ---
Client* g_clients[MAX_CLIENTS] = {0};
const char* DB_FILE = "data/users.txt";
// ---------------------------------

void add_client(int socket_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i] == NULL) {
            Client* new_client = (Client*)malloc(sizeof(Client));
            new_client->socket_fd = socket_fd;
            new_client->buffer_pos = 0;
            new_client->is_logged_in = 0; // Status 0 (Offline)
            new_client->user_id = 0;      // 0 = Chua xac thuc
            new_client->role = 0;         // Role 0 (User)
            memset(new_client->username, 0, MAX_USERNAME);
            
            g_clients[i] = new_client;
            printf("Client moi (fd=%d) da ket noi, o vi tri %d\n", socket_fd, i);
            send_message(new_client, "Chao mung den voi dau gia");
            return;
        }
    }
    printf("Server day, tu choi ket noi %d\n", socket_fd);
    close(socket_fd);
}

void remove_client(int i) {
    Client* client = g_clients[i];
    if (client == NULL) return;
    
    printf("Client (fd=%d, id=%d, user=%s, role=%d) da ngat ket noi.\n", 
           client->socket_fd, client->user_id, client->username, client->role); // Sửa log
    
    close(client->socket_fd);
    free(client);
    g_clients[i] = NULL;
}

void send_message(Client* client, const char* message) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%s\n", message);
    send(client->socket_fd, buffer, strlen(buffer), 0);
}

int get_user_online_status(const char* username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i] && g_clients[i]->is_logged_in && 
            strcmp(g_clients[i]->username, username) == 0) {
            return 1; // 1 = Online
        }
    }
    return 0; // 0 = Offline
}