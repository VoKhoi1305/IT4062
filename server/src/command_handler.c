// // /*
// //  * src/command_handler.c
// //  */

// // #include "command_handler.h"
// // #include "db_handler.h"
// // #include "client_handler.h"
// // #include "server.h"

// // // --- Hàm 'private' (static) cho các lệnh admin ---

// // // static void handle_kick(Client* admin, char* user_to_kick) {
// // //     if (user_to_kick == NULL) {
// // //         send_message(admin, "ADMIN_FAIL|Sai cu phap: KICK|username");
// // //         return;
// // //     }
    
// // //     if (strcmp(admin->username, user_to_kick) == 0) {
// // //         send_message(admin, "ADMIN_FAIL|Ban khong a tu kick minh");
// // //         return;
// // //     }

// // //     for (int i = 0; i < MAX_CLIENTS; i++) {
// // //         if (g_clients[i] && g_clients[i]->is_logged_in && 
// // //             strcmp(g_clients[i]->username, user_to_kick) == 0) {
            
// // //             send_message(g_clients[i], "YOU_GOT_KICKED|Ban da bi Admin kick");
// // //             remove_client(i);
            
// // //             char msg[256];
// // //             snprintf(msg, sizeof(msg), "ADMIN_OK|Da kick user %s", user_to_kick);
// // //             send_message(admin, msg);
// // //             return;
// // //         }
// // //     }
    
// // //     send_message(admin, "ADMIN_FAIL|Khong tim thay user dang online");
// // // }

// // static void handle_get_user_list(Client* admin) {
// //     char long_msg[BUFFER_SIZE] = "USER_LIST|";
// //     char entry[256];
    
// //     FILE* file = fopen(DB_FILE, "r");
// //     if (file == NULL) {
// //         send_message(admin, "ADMIN_FAIL|Loi doc database");
// //         return;
// //     }

// //     char line[256];
// //     while (fgets(line, sizeof(line), file)) {
// //         line[strcspn(line, "\n")] = 0;
// //         if (strlen(line) == 0) continue;

// //         char temp_line[256];
// //         strcpy(temp_line, line);

// //         char* id_str = strtok(temp_line, "|");
// //         char* user = strtok(NULL, "|");
// //         char* pass = strtok(NULL, "|");
// //         char* role_str = strtok(NULL, "|");
        
// //         if (id_str && user && role_str) {
// //             int status = get_user_online_status(user); // 1 = online, 0 = offline
            
// //             // Định dạng: id|user|status|role;
// //             snprintf(entry, sizeof(entry), "%s|%s|%d|%s;", id_str, user, status, role_str);
// //             strncat(long_msg, entry, sizeof(long_msg) - strlen(long_msg) - 1);
// //         }
// //     }
// //     fclose(file);
    
// //     send_message(admin, long_msg);
// // }

// // // --- Hàm Xử lý Lệnh Chính ---

// // static void parse_and_execute(Client* client, char* line) {
// //     char* command = strtok(line, "|");
// //     if (command == NULL) return;

// //     // --- 1. LỆNH CÔNG CỘNG (Chưa đăng nhập) ---
// //     if (strcmp(command, "LOGIN") == 0) {
// //         char* user = strtok(NULL, "|");
// //         char* pass = strtok(NULL, "|");
// //         if (user && pass) {
// //             if (get_user_online_status(user)) {
// //                 send_message(client, "LOGIN_FAIL_LOGGED_IN|nguoi dung da dang nhap");
// //             } else {
// //                 int found_role = 0;
// //                 int found_id = 0; // Biến nhận ID
                
// //                 int result = check_user_db(user, pass, &found_role, &found_id); 
                
// //                 if (result == 0) { // Thành công
// //                     client->is_logged_in = 1;
// //                     client->role = found_role;
// //                     client->user_id = found_id; // LƯU ID
// //                     strncpy(client->username, user, MAX_USERNAME);
                    
// //                     char msg[256];
// //                     snprintf(msg, sizeof(msg), "LOGIN_SUCCESS|dang nhap thanh cong|%s|%d", user, client->role);
// //                     send_message(client, msg);
// //                 } else {
// //                     send_message(client, "LOGIN_FAIL_WRONG_PASS|sai mat khau hoac tai khoan");
// //                 }
// //             }
// //         } else {
// //             send_message(client, "ERROR|Sai cu phap. LOGIN|user|pass");
// //         }
// //         return;
// //     }
// //     else if (strcmp(command, "REGISTER") == 0) {
// //         char* user = strtok(NULL, "|");
// //         char* pass = strtok(NULL, "|");
// //         if (user && pass) {
// //             if (register_user_db(user, pass)) {
// //                 send_message(client, "REGISTER_SUCCESS|Dang ky thanh cong. Hay dang nhap.");
// //             } else {
// //                 send_message(client, "REGISTER_FAIL|Nguoi dung da ton tai");
// //             }
// //         } else {
// //             send_message(client, "ERROR|Sai cu phap. REGISTER|user|pass");
// //         }
// //         return;
// //     }
// //     // --- 2. KIỂM TRA ĐĂNG NHẬP ---
// //     if (!client->is_logged_in) {
// //         send_message(client, "ERROR|Ban phai dang nhap truoc");
// //         return;
// //     }

// //     // --- 3. LỆNH ĐĂNG XUẤT ---
// //     if (strcmp(command, "LOGOUT") == 0) {
// //         client->is_logged_in = 0;
// //         client->role = 0;
// //         client->user_id = 0; // Reset ID
// //         memset(client->username, 0, MAX_USERNAME);
// //         send_message(client, "LOGOUT_SUCCESS|dang xuat thanh cong");
// //         return;
// //     } 

// //     // --- 4. KIỂM TRA QUYỀN ADMIN ---
// //     if (client->role != 1) { // Phải là Admin (role == 1)
// //         send_message(client, "ERROR|Ban khong co quyen (Admin only)");
// //         return;
// //     }
// //     // --- 5. LỆNH ADMIN ---
// //     if (strcmp(command, "GET_USER_LIST") == 0) {
// //         handle_get_user_list(client);
// //         return;
// //     }
    
// //     // if (strcmp(command, "KICK") == 0) {
// //     //     char* user_to_kick = strtok(NULL, "|");
// //     //     handle_kick(client, user_to_kick);
// //     //     return;
// //     // }

// //     send_message(client, "ERROR|Lenh khong hop le");
// // }

// // // --- Hàm Xử lý Buffer ---
// // void process_command_buffer(Client* client) {
// //     char* newline_ptr;
// //     while ((newline_ptr = memchr(client->read_buffer, '\n', client->buffer_pos)) != NULL) {
// //         int line_len = newline_ptr - client->read_buffer;
// //         char line_buffer[BUFFER_SIZE];
// //         memcpy(line_buffer, client->read_buffer, line_len);
// //         line_buffer[line_len] = '\0';
// //         int remaining_len = client->buffer_pos - (line_len + 1);
// //         memmove(client->read_buffer, newline_ptr + 1, remaining_len);
// //         client->buffer_pos = remaining_len;
// //         if (line_len > 0) {
// //             parse_and_execute(client, line_buffer);
// //         }
// //     }
// // }

// /*
//  * src/command_handler.c (Final - Full Integration)
//  */

// #include "command_handler.h"
// #include "db_handler.h"
// #include "client_handler.h"
// #include "room_handler.h"
// #include "item_handler.h"
// #include "auction_handler.h"
// #include "timer_handler.h"
// #include "server.h"

// static void handle_get_user_list(Client* admin) {
//     char long_msg[BUFFER_SIZE] = "USER_LIST|";
//     char entry[256];
    
//     FILE* file = fopen(DB_FILE, "r");
//     if (file == NULL) {
//         send_message(admin, "ADMIN_FAIL|Loi doc database");
//         return;
//     }

//     char line[256];
//     while (fgets(line, sizeof(line), file)) {
//         line[strcspn(line, "\n")] = 0;
//         if (strlen(line) == 0) continue;

//         char temp_line[256];
//         strcpy(temp_line, line);

//         char* id_str = strtok(temp_line, "|");
//         char* user = strtok(NULL, "|");
//         char* pass = strtok(NULL, "|");
//         char* role_str = strtok(NULL, "|");
        
//         if (id_str && user && role_str) {
//             int status = get_user_online_status(user);
//             snprintf(entry, sizeof(entry), "%s|%s|%d|%s;", id_str, user, status, role_str);
//             strncat(long_msg, entry, sizeof(long_msg) - strlen(long_msg) - 1);
//         }
//     }
//     fclose(file);
    
//     send_message(admin, long_msg);
// }

// static void parse_and_execute(Client* client, char* line) {
//     char line_copy[BUFFER_SIZE];
//     strncpy(line_copy, line, BUFFER_SIZE);
    
//     char* command = strtok(line, "|");
//     if (command == NULL) return;

//     // === LỆNH CÔNG KHAI ===
//     if (strcmp(command, "LOGIN") == 0) {
//         char* user = strtok(NULL, "|");
//         char* pass = strtok(NULL, "|");
//         if (user && pass) {
//             if (get_user_online_status(user)) {
//                 send_message(client, "LOGIN_FAIL_LOGGED_IN|nguoi dung da dang nhap");
//             } else {
//                 int found_role = 0;
//                 int found_id = 0;
                
//                 int result = check_user_db(user, pass, &found_role, &found_id); 
                
//                 if (result == 0) {
//                     client->is_logged_in = 1;
//                     client->role = found_role;
//                     client->user_id = found_id;
//                     strncpy(client->username, user, MAX_USERNAME);
                    
//                     char msg[256];
//                     snprintf(msg, sizeof(msg), "LOGIN_SUCCESS|dang nhap thanh cong|%s|%d", 
//                              user, client->role);
//                     send_message(client, msg);
//                 } else {
//                     send_message(client, "LOGIN_FAIL_WRONG_PASS|sai mat khau hoac tai khoan");
//                 }
//             }
//         } else {
//             send_message(client, "ERROR|Sai cu phap. LOGIN|user|pass");
//         }
//         return;
//     }
//     else if (strcmp(command, "REGISTER") == 0) {
//         char* user = strtok(NULL, "|");
//         char* pass = strtok(NULL, "|");
//         if (user && pass) {
//             if (register_user_db(user, pass)) {
//                 send_message(client, "REGISTER_SUCCESS|Dang ky thanh cong. Hay dang nhap.");
//             } else {
//                 send_message(client, "REGISTER_FAIL|Nguoi dung da ton tai");
//             }
//         } else {
//             send_message(client, "ERROR|Sai cu phap. REGISTER|user|pass");
//         }
//         return;
//     }

//     // === CỔNG GÁC ĐĂNG NHẬP ===
//     if (!client->is_logged_in) {
//         send_message(client, "ERROR|Ban phai dang nhap truoc");
//         return;
//     }

//     // === LỆNH USER ===
//     if (strcmp(command, "LOGOUT") == 0) {
//         if (client->current_room_id > 0) {
//             handle_leave_room(client);
//         }
        
//         client->is_logged_in = 0;
//         client->role = 0;
//         client->user_id = 0;
//         client->current_room_id = 0;
//         memset(client->username, 0, MAX_USERNAME);
//         send_message(client, "LOGOUT_SUCCESS|dang xuat thanh cong");
//         return;
//     }
//     else if (strcmp(command, "GET_ROOM_LIST") == 0) {
//         char* status = strtok(NULL, "|");
//         char* page = strtok(NULL, "|");
//         char* limit = strtok(NULL, "|");
//         handle_get_room_list(client, status, page, limit);
//         return;
//     }
//     else if (strcmp(command, "CREATE_ROOM") == 0) {
//         char* room_name = strtok(NULL, "|");
//         char* start_time = strtok(NULL, "|");
//         char* end_time = strtok(NULL, "|");
//         handle_create_room(client, room_name, start_time, end_time);
//         return;
//     }
//     else if (strcmp(command, "JOIN_ROOM") == 0) {
//         char* room_id = strtok(NULL, "|");
//         handle_join_room(client, room_id);
//         return;
//     }
//     else if (strcmp(command, "LEAVE_ROOM") == 0) {
//         handle_leave_room(client);
//         return;
//     }
//     else if (strcmp(command, "GET_ROOM_DETAIL") == 0) {
//         char* room_id = strtok(NULL, "|");
//         handle_get_room_detail(client, room_id);
//         return;
//     }
//     else if (strcmp(command, "CREATE_ITEM") == 0) {
//         char* room_id = strtok(NULL, "|");
//         char* item_name = strtok(NULL, "|");
//         char* start_price = strtok(NULL, "|");
//         char* duration = strtok(NULL, "|");
//         char* buy_now_price = strtok(NULL, "|");
//         handle_create_item(client, room_id, item_name, start_price, duration, buy_now_price);
//         return;
//     }
//     else if (strcmp(command, "SEARCH_ITEMS") == 0) {
//         char* search_type = strtok(NULL, "|");
//         char* keyword = strtok(NULL, "|");
//         char* time_from = strtok(NULL, "|");
//         char* time_to = strtok(NULL, "|");
//         handle_search_items(client, search_type, keyword, time_from, time_to);
//         return;
//     }
//     else if (strcmp(command, "PLACE_BID") == 0) {
//         char* item_id = strtok(NULL, "|");
//         char* bid_amount = strtok(NULL, "|");
//         handle_place_bid(client, item_id, bid_amount);
//         return;
//     }
//     else if (strcmp(command, "BUY_NOW") == 0) {
//         char* item_id = strtok(NULL, "|");
//         handle_buy_now(client, item_id);
//         return;
//     }
//     else if (strcmp(command, "GET_MY_AUCTION_HISTORY") == 0) {
//         char* filter = strtok(NULL, "|");
//         char* page = strtok(NULL, "|");
//         char* limit = strtok(NULL, "|");
//         handle_get_my_auction_history(client, filter, page, limit);
//         return;
//     }

//     // === LỆNH ADMIN ===
//     if (client->role == 1) {
//         if (strcmp(command, "GET_USER_LIST") == 0) {
//             handle_get_user_list(client);
//             return;
//         }
//     } else {
//         if (strcmp(command, "GET_USER_LIST") == 0) {
//             send_message(client, "ERROR|Ban khong co quyen (Admin only)");
//             return;
//         }
//     }

//     send_message(client, "ERROR|Lenh khong hop le");
// }

// void process_command_buffer(Client* client) {
//     char* newline_ptr;
//     while ((newline_ptr = memchr(client->read_buffer, '\n', client->buffer_pos)) != NULL) {
//         int line_len = newline_ptr - client->read_buffer;
//         char line_buffer[BUFFER_SIZE];
//         memcpy(line_buffer, client->read_buffer, line_len);
//         line_buffer[line_len] = '\0';
//         int remaining_len = client->buffer_pos - (line_len + 1);
//         memmove(client->read_buffer, newline_ptr + 1, remaining_len);
//         client->buffer_pos = remaining_len;
//         if (line_len > 0) {
//             parse_and_execute(client, line_buffer);
//         }
//     }
// }

/*
 * src/command_handler.c (Final - Full Integration)
 */

#include "command_handler.h"
#include "db_handler.h"
#include "client_handler.h"
#include "room_handler.h"
#include "item_handler.h"
#include "auction_handler.h"
#include "timer_handler.h"
#include "server.h"

static void handle_get_user_list(Client* admin) {
    char long_msg[BUFFER_SIZE] = "USER_LIST|";
    char entry[256];
    
    FILE* file = fopen(DB_FILE, "r");
    if (file == NULL) {
        send_message(admin, "ADMIN_FAIL|Loi doc database");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        char temp_line[256];
        strcpy(temp_line, line);

        char* id_str = strtok(temp_line, "|");
        char* user = strtok(NULL, "|");
        char* pass = strtok(NULL, "|");
        char* role_str = strtok(NULL, "|");
        
        if (id_str && user && role_str) {
            int status = get_user_online_status(user);
            snprintf(entry, sizeof(entry), "%s|%s|%d|%s;", id_str, user, status, role_str);
            strncat(long_msg, entry, sizeof(long_msg) - strlen(long_msg) - 1);
        }
    }
    fclose(file);
    
    send_message(admin, long_msg);
}

static void parse_and_execute(Client* client, char* line) {
    char line_copy[BUFFER_SIZE];
    strncpy(line_copy, line, BUFFER_SIZE);
    
    char* command = strtok(line, "|");
    if (command == NULL) return;

    // === LỆNH CÔNG KHAI ===
    if (strcmp(command, "LOGIN") == 0) {
        char* user = strtok(NULL, "|");
        char* pass = strtok(NULL, "|");
        if (user && pass) {
            if (get_user_online_status(user)) {
                send_message(client, "LOGIN_FAIL_LOGGED_IN|nguoi dung da dang nhap");
            } else {
                int found_role = 0;
                int found_id = 0;
                
                int result = check_user_db(user, pass, &found_role, &found_id); 
                
                if (result == 0) {
                    client->is_logged_in = 1;
                    client->role = found_role;
                    client->user_id = found_id;
                    strncpy(client->username, user, MAX_USERNAME);
                    
                    char msg[256];
                    snprintf(msg, sizeof(msg), "LOGIN_SUCCESS|dang nhap thanh cong|%s|%d", 
                             user, client->role);
                    send_message(client, msg);
                } else {
                    send_message(client, "LOGIN_FAIL_WRONG_PASS|sai mat khau hoac tai khoan");
                }
            }
        } else {
            send_message(client, "ERROR|Sai cu phap. LOGIN|user|pass");
        }
        return;
    }
    else if (strcmp(command, "REGISTER") == 0) {
        char* user = strtok(NULL, "|");
        char* pass = strtok(NULL, "|");
        if (user && pass) {
            if (register_user_db(user, pass)) {
                send_message(client, "REGISTER_SUCCESS|Dang ky thanh cong. Hay dang nhap.");
            } else {
                send_message(client, "REGISTER_FAIL|Nguoi dung da ton tai");
            }
        } else {
            send_message(client, "ERROR|Sai cu phap. REGISTER|user|pass");
        }
        return;
    }

    // === CỔNG GÁC ĐĂNG NHẬP ===
    if (!client->is_logged_in) {
        send_message(client, "ERROR|Ban phai dang nhap truoc");
        return;
    }

    // === LỆNH USER ===
    if (strcmp(command, "LOGOUT") == 0) {
        if (client->current_room_id > 0) {
            handle_leave_room(client);
        }
        
        client->is_logged_in = 0;
        client->role = 0;
        client->user_id = 0;
        client->current_room_id = 0;
        memset(client->username, 0, MAX_USERNAME);
        send_message(client, "LOGOUT_SUCCESS|dang xuat thanh cong");
        return;
    }
    else if (strcmp(command, "GET_ROOM_LIST") == 0) {
        char* status = strtok(NULL, "|");
        char* page = strtok(NULL, "|");
        char* limit = strtok(NULL, "|");
        handle_get_room_list(client, status, page, limit);
        return;
    }
    else if (strcmp(command, "CREATE_ROOM") == 0) {
        char* room_name = strtok(NULL, "|");
        char* start_time = strtok(NULL, "|");
        char* end_time = strtok(NULL, "|");
        handle_create_room(client, room_name, start_time, end_time);
        return;
    }
    else if (strcmp(command, "JOIN_ROOM") == 0) {
        char* room_id = strtok(NULL, "|");
        handle_join_room(client, room_id);
        return;
    }
    else if (strcmp(command, "LEAVE_ROOM") == 0) {
        handle_leave_room(client);
        return;
    }
    else if (strcmp(command, "GET_ROOM_DETAIL") == 0) {
        char* room_id = strtok(NULL, "|");
        handle_get_room_detail(client, room_id);
        return;
    }
    else if (strcmp(command, "START_AUCTION") == 0) {
        char* room_id = strtok(NULL, "|");
        handle_start_auction(client, room_id);
        return;
    }
    else if (strcmp(command, "CREATE_ITEM") == 0) {
        char* room_id = strtok(NULL, "|");
        char* item_name = strtok(NULL, "|");
        char* description = strtok(NULL, "|");      // Description parameter
        char* start_price = strtok(NULL, "|");
        char* duration = strtok(NULL, "|");
        char* buy_now_price = strtok(NULL, "|");
        char* scheduled_start = strtok(NULL, "|");  // Khung giờ bắt đầu (HH:MM)
        char* scheduled_end = strtok(NULL, "|");    // Khung giờ kết thúc (HH:MM)
        handle_create_item(client, room_id, item_name, description, start_price, duration, buy_now_price,
                          scheduled_start, scheduled_end);
        return;
    }
    else if (strcmp(command, "DELETE_ITEM") == 0) {
        char* item_id = strtok(NULL, "|");
        handle_delete_item(client, item_id);
        return;
    }
    else if (strcmp(command, "SEARCH_ITEMS") == 0) {
        char* search_type = strtok(NULL, "|");
        char* keyword = strtok(NULL, "|");
        char* time_from = strtok(NULL, "|");
        char* time_to = strtok(NULL, "|");
        handle_search_items(client, search_type, keyword, time_from, time_to);
        return;
    }
    else if (strcmp(command, "PLACE_BID") == 0) {
        char* item_id = strtok(NULL, "|");
        char* bid_amount = strtok(NULL, "|");
        handle_place_bid(client, item_id, bid_amount);
        return;
    }
    else if (strcmp(command, "BUY_NOW") == 0) {
        char* item_id = strtok(NULL, "|");
        handle_buy_now(client, item_id);
        return;
    }
    else if (strcmp(command, "GET_MY_AUCTION_HISTORY") == 0) {
        char* filter = strtok(NULL, "|");
        char* page = strtok(NULL, "|");
        char* limit = strtok(NULL, "|");
        handle_get_my_auction_history(client, filter, page, limit);
        return;
    }

    // === LỆNH ADMIN ===
    if (client->role == 1) {
        if (strcmp(command, "GET_USER_LIST") == 0) {
            handle_get_user_list(client);
            return;
        }
    } else {
        if (strcmp(command, "GET_USER_LIST") == 0) {
            send_message(client, "ERROR|Ban khong co quyen (Admin only)");
            return;
        }
    }

    send_message(client, "ERROR|Lenh khong hop le");
}

void process_command_buffer(Client* client) {
    char* newline_ptr;
    while ((newline_ptr = memchr(client->read_buffer, '\n', client->buffer_pos)) != NULL) {
        int line_len = newline_ptr - client->read_buffer;
        char line_buffer[BUFFER_SIZE];
        memcpy(line_buffer, client->read_buffer, line_len);
        line_buffer[line_len] = '\0';
        int remaining_len = client->buffer_pos - (line_len + 1);
        memmove(client->read_buffer, newline_ptr + 1, remaining_len);
        client->buffer_pos = remaining_len;
        if (line_len > 0) {
            // Log received message
            log_message(client, "RECV", line_buffer);
            parse_and_execute(client, line_buffer);
        }
    }
}