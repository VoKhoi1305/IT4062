/*
 * client.c
 * (Không thay đổi)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int g_socket_fd;
char g_socket_buffer[BUFFER_SIZE];
int g_socket_buffer_pos = 0;
char g_keyboard_buffer[BUFFER_SIZE];
int g_keyboard_buffer_pos = 0;

void process_socket_buffer() {
    char* newline_ptr;
    while ((newline_ptr = memchr(g_socket_buffer, '\n', g_socket_buffer_pos)) != NULL) {
        int line_len = newline_ptr - g_socket_buffer;
        char line_buffer[BUFFER_SIZE];
        memcpy(line_buffer, g_socket_buffer, line_len);
        line_buffer[line_len] = '\0';
        printf("\nServer: %s\n", line_buffer);
        int remaining_len = g_socket_buffer_pos - (line_len + 1);
        memmove(g_socket_buffer, newline_ptr + 1, remaining_len);
        g_socket_buffer_pos = remaining_len;
    }
}

void process_keyboard_buffer() {
    char* newline_ptr;
    while ((newline_ptr = memchr(g_keyboard_buffer, '\n', g_keyboard_buffer_pos)) != NULL) {
        int line_len = newline_ptr - g_keyboard_buffer;
        char line_buffer[BUFFER_SIZE];
        memcpy(line_buffer, g_keyboard_buffer, line_len);
        line_buffer[line_len] = '\0';
        int remaining_len = g_keyboard_buffer_pos - (line_len + 1);
        memmove(g_keyboard_buffer, newline_ptr + 1, remaining_len);
        g_keyboard_buffer_pos = remaining_len;
        
        if (line_len > 0) {
            if (strcmp(line_buffer, "QUIT") == 0) {
                 close(g_socket_fd);
                 printf("Da thoat.\n");
                 exit(0);
            }
            char send_buf[BUFFER_SIZE];
            snprintf(send_buf, BUFFER_SIZE, "%s\n", line_buffer);
            send(g_socket_fd, send_buf, strlen(send_buf), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;

    if (argc != 2) {
        fprintf(stderr, "Su dung: %s <IP_SERVER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* server_ip = argv[1];
    g_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton() failed"); exit(EXIT_FAILURE);
    }
    if (connect(g_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed"); exit(EXIT_FAILURE);
    }
    printf("Da ket noi den server tai %s. (Client I/O Mux)\n", server_ip);
    printf("Ban: ");
    fflush(stdout); 

    fd_set read_fds;
    int max_fd;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(g_socket_fd, &read_fds);
        max_fd = (g_socket_fd > STDIN_FILENO) ? g_socket_fd : STDIN_FILENO;
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select() error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            int bytes = read(STDIN_FILENO, g_keyboard_buffer + g_keyboard_buffer_pos, BUFFER_SIZE - g_keyboard_buffer_pos);
            if (bytes <= 0) break;
            g_keyboard_buffer_pos += bytes;
            process_keyboard_buffer();
            printf("Ban: ");
            fflush(stdout);
        }

        if (FD_ISSET(g_socket_fd, &read_fds)) {
            int bytes = recv(g_socket_fd, g_socket_buffer + g_socket_buffer_pos, BUFFER_SIZE - g_socket_buffer_pos, 0);
            if (bytes <= 0) {
                printf("\n*** Server da ngat ket noi. ***\n");
                break;
            }
            g_socket_buffer_pos += bytes;
            process_socket_buffer();
            printf("Ban: ");
            fflush(stdout);
        }
    }
    close(g_socket_fd);
    return 0;
}