/*
 * db_handler.h
 */

#ifndef DB_HANDLER_H
#define DB_HANDLER_H

// Trả về: 0 (Thành công), 1 (Sai pass), 2 (Không tồn tại)
// Các con trỏ output sẽ được điền nếu thành công
int check_user_db(const char* username, const char* password, int* role_output, int* id_output);

// Trả về: 1 (Thành công), 0 (Thất bại / Đã tồn tại)
int register_user_db(const char* username, const char* password);

#endif // DB_HANDLER_H