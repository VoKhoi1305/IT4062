/*
 * db_handler.h
 */

#ifndef DB_HANDLER_H
#define DB_HANDLER_H


int check_user_db(const char* username, const char* password, int* role_output, int* id_output);
int register_user_db(const char* username, const char* password);

#endif // DB_HANDLER_H