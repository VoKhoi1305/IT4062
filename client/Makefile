# Makefile (Nằm trong thư mục client/)
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = 
TARGET = client
TARGET_GTK = client_gtk

# GTK flags
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)

all: $(TARGET) $(TARGET_GTK)

$(TARGET): client.c
	$(CC) $(CFLAGS) -o $(TARGET) client.c $(LDFLAGS)
	@echo "Bien dich CLIENT (I/O Mux) thanh cong! File thuc thi la: ./client"

$(TARGET_GTK): client_gtk.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $(TARGET_GTK) client_gtk.c $(GTK_LIBS) -lpthread
	@echo "Bien dich CLIENT GTK thanh cong! File thuc thi la: ./client_gtk"

clean:
	rm -f $(TARGET) $(TARGET_GTK)
	@echo "Da don dep client."