# Makefile (Nằm trong thư mục client/)
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = 
TARGET = client

all: $(TARGET)

$(TARGET): client.c
	$(CC) $(CFLAGS) -o $(TARGET) client.c $(LDFLAGS)
	@echo "Bien dich CLIENT (I/O Mux) thanh cong! File thuc thi la: ./client"

clean:
	rm -f $(TARGET)
	@echo "Da don dep client."