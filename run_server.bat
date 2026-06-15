@echo off
echo ==============================================================
echo       KHOI DONG HE THONG QUAN LY BAI GIU XE AI CAMERA
echo ==============================================================

echo [*] Dang kich hoat moi truong ao...
call .venv\Scripts\activate

echo [*] Dang chuyen vao thu muc server...
cd server

echo [*] Dang khoi dong Web Server...
echo [*] Vui long truy cap vao http://localhost:8000 de xem giao dien
python main.py

pause
