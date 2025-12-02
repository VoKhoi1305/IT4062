/*
 * file_lock_handler.h
 * Xử lý file locking để đồng bộ truy cập file
 */

#ifndef FILE_LOCK_HANDLER_H
#define FILE_LOCK_HANDLER_H

#include <stdio.h>

// Lock file để đọc
int lock_file_read(FILE* file);

// Lock file để ghi
int lock_file_write(FILE* file);

// Unlock file
int unlock_file(FILE* file);

// Các hàm wrapper an toàn
FILE* safe_fopen_read(const char* filename);
FILE* safe_fopen_write(const char* filename);
FILE* safe_fopen_append(const char* filename);
void safe_fclose(FILE* file);

#endif // FILE_LOCK_HANDLER_H