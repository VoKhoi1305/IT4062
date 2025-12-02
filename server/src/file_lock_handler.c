/*
 * file_lock_handler.c
 * Triển khai file locking với fcntl
 */

#include "file_lock_handler.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int lock_file_read(FILE* file) {
    if (!file) return -1;
    
    int fd = fileno(file);
    struct flock lock;
    
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;    // Read lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;           // Lock entire file
    
    // Thử lock với timeout
    int attempts = 0;
    while (fcntl(fd, F_SETLK, &lock) == -1) {
        if (errno != EAGAIN && errno != EACCES) {
            perror("fcntl read lock failed");
            return -1;
        }
        
        if (++attempts > 10) {
            fprintf(stderr, "Failed to acquire read lock after 10 attempts\n");
            return -1;
        }
        
        usleep(100000); // Sleep 100ms
    }
    
    return 0;
}

int lock_file_write(FILE* file) {
    if (!file) return -1;
    
    int fd = fileno(file);
    struct flock lock;
    
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;    // Write lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;           // Lock entire file
    
    // Thử lock với timeout
    int attempts = 0;
    while (fcntl(fd, F_SETLK, &lock) == -1) {
        if (errno != EAGAIN && errno != EACCES) {
            perror("fcntl write lock failed");
            return -1;
        }
        
        if (++attempts > 10) {
            fprintf(stderr, "Failed to acquire write lock after 10 attempts\n");
            return -1;
        }
        
        usleep(100000); // Sleep 100ms
    }
    
    return 0;
}

int unlock_file(FILE* file) {
    if (!file) return -1;
    
    int fd = fileno(file);
    struct flock lock;
    
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_UNLCK;    // Unlock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("fcntl unlock failed");
        return -1;
    }
    
    return 0;
}

FILE* safe_fopen_read(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file && lock_file_read(file) != 0) {
        fclose(file);
        return NULL;
    }
    return file;
}

FILE* safe_fopen_write(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file && lock_file_write(file) != 0) {
        fclose(file);
        return NULL;
    }
    return file;
}

FILE* safe_fopen_append(const char* filename) {
    FILE* file = fopen(filename, "a");
    if (file && lock_file_write(file) != 0) {
        fclose(file);
        return NULL;
    }
    return file;
}

void safe_fclose(FILE* file) {
    if (file) {
        unlock_file(file);
        fclose(file);
    }
}