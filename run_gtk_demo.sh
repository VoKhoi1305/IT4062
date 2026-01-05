#!/bin/bash

# Script để chạy GTK Client Demo
# Sử dụng: ./run_gtk_demo.sh

echo "=================================="
echo "GTK Client Demo Setup"
echo "=================================="

# Kiểm tra DISPLAY
if [ -z "$DISPLAY" ]; then
    echo "⚠️  WARNING: DISPLAY not set!"
    echo ""
    echo "Giải pháp cho WSL:"
    echo "1. Windows 11: WSLg đã có sẵn X server"
    echo "   Không cần config gì thêm"
    echo ""
    echo "2. Windows 10: Cần cài X server (VcXsrv/Xming)"
    echo "   export DISPLAY=:0"
    echo ""
    read -p "Nhấn Enter để tiếp tục (hoặc Ctrl+C để thoát)..."
fi

# Kiểm tra server
echo ""
echo "1. Kiểm tra server..."
if ! netstat -tuln 2>/dev/null | grep -q ":8080 "; then
    echo "❌ Server chưa chạy trên port 8080"
    echo ""
    echo "Vui lòng start server trong terminal khác:"
    echo "  cd ~/THLTM/Project/server"
    echo "  ./bin/server_app"
    echo ""
    read -p "Nhấn Enter sau khi đã start server..."
    
    # Kiểm tra lại
    if ! netstat -tuln 2>/dev/null | grep -q ":8080 "; then
        echo "❌ Server vẫn chưa chạy. Thoát."
        exit 1
    fi
fi

echo "✅ Server đang chạy trên port 8080"

# Kiểm tra GTK
echo ""
echo "2. Kiểm tra GTK environment..."
if ! pkg-config --exists gtk+-3.0; then
    echo "❌ GTK+ 3.0 chưa được cài đặt"
    echo "Cài đặt: sudo apt install libgtk-3-dev"
    exit 1
fi

echo "✅ GTK+ 3.0 đã cài đặt"

# Kiểm tra client executable
echo ""
echo "3. Kiểm tra client executable..."
cd ~/THLTM/Project/client

if [ ! -f "./client_gtk" ]; then
    echo "❌ client_gtk chưa được compile"
    echo "Compiling..."
    make client_gtk
    
    if [ $? -ne 0 ]; then
        echo "❌ Compile thất bại"
        exit 1
    fi
fi

echo "✅ client_gtk executable ready"

# Chạy client
echo ""
echo "=================================="
echo "Starting GTK Client..."
echo "=================================="
echo ""

# Test với terminal client trước để đảm bảo connection
echo "Testing connection với terminal client..."
timeout 2 ./client 127.0.0.1 <<EOF > /dev/null 2>&1
0
EOF

if [ $? -eq 124 ]; then
    echo "✅ Connection OK"
else
    echo "⚠️  Connection test không chắc chắn, thử tiếp..."
fi

echo ""
echo "Launching GTK client..."
echo "Nếu không hiện cửa sổ, kiểm tra:"
echo "  - DISPLAY environment: $DISPLAY"
echo "  - X server đang chạy (Windows 11 WSLg hoặc VcXsrv/Xming)"
echo ""

./client_gtk 127.0.0.1
