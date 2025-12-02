/*
 * src/db_handler.c
 */

#include "server.h"
#include "db_handler.h"


static int get_last_user_id() {
    FILE* file = fopen(DB_FILE, "r");
    if (file == NULL) {
        
        return STARTING_USER_ID; 
    }
    
    char line[256];
    int max_id = STARTING_USER_ID; 

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue; 
        char* id_str = strtok(line, "|");
        if (id_str) {
            int current_id = atoi(id_str);
            if (current_id > max_id) {
                max_id = current_id; // Tìm ID lớn nhất
            }
        }
    }
    
    fclose(file);
    return max_id;
}


// Trả về role và id qua con trỏ
int check_user_db(const char* username, const char* password, int* role_output, int* id_output) {
    FILE* file = fopen(DB_FILE, "r");
    if (file == NULL) {
        return 2; 
    }
    char line[256];
    int found = 2; // 2 = Not found

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue; 

        char line_copy[256]; 
        strcpy(line_copy, line);

        char* db_id_str = strtok(line_copy, "|");
        char* db_user = strtok(NULL, "|");
        char* db_pass = strtok(NULL, "|");
        char* db_role_str = strtok(NULL, "|");

        if (db_user && strcmp(db_user, username) == 0) {
            if (db_pass && strcmp(db_pass, password) == 0) {
                found = 0; // Thành công
                *role_output = atoi(db_role_str ? db_role_str : "0");
                *id_output = atoi(db_id_str ? db_id_str : "0");
            } else {
                found = 1; // Sai pass
            }
            break;
        }
    }
    fclose(file);
    return found;
}

int register_user_db(const char* username, const char* password) {
    int temp_role, temp_id;
    
    // Kiểm tra xem user đã tồn tại chưa
    if (check_user_db(username, "", &temp_role, &temp_id) != 2) {
        return 0; // Đã tồn tại 
    }

    // Lấy ID lớn nhất hiện tại
    int last_id = get_last_user_id();
    int new_id = last_id + 1; // ID mới sẽ là +1

    // Mở file để ghi
    FILE* file = fopen(DB_FILE, "a+");
    if (file == NULL) {
        return 0; // Lỗi
    }

    long file_size;
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    if (file_size > 0) {
        fseek(file, -1, SEEK_END);
        if (fgetc(file) != '\n') {
            fprintf(file, "\n");
        }
    }

    // Ghi user mới với ID mới và role 0 (USER)
    fprintf(file, "%d|%s|%s|0\n", new_id, username, password);
    
    fclose(file);
    return 1;
}