import sqlite3
from datetime import datetime
import Levenshtein

DB_PATH = "parking.db"

def init_db():
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS parking_sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            plate_number TEXT NOT NULL,
            entry_time DATETIME NOT NULL,
            entry_image TEXT NOT NULL,
            exit_time DATETIME,
            exit_image TEXT,
            status TEXT NOT NULL
        )
    ''')
    conn.commit()
    conn.close()

def add_entry(plate: str, image_path: str):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    entry_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    cursor.execute('''
        INSERT INTO parking_sessions (plate_number, entry_time, entry_image, status)
        VALUES (?, ?, ?, 'đang đỗ')
    ''', (plate, entry_time, image_path))
    conn.commit()
    conn.close()
    return {"plate": plate, "time": entry_time, "status": "đang đỗ"}

def mark_exit(plate: str, image_path: str):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    
    # Get all currently parked vehicles
    cursor.execute("SELECT id, plate_number FROM parking_sessions WHERE status = 'đang đỗ'")
    parked_vehicles = cursor.fetchall()
    
    if not parked_vehicles:
        conn.close()
        return {"error": "Không có xe nào đang đỗ", "plate_detected": plate}
        
    best_match = None
    min_distance = float('inf')
    
    # Fuzzy match using Levenshtein distance
    for vid, parked_plate in parked_vehicles:
        dist = Levenshtein.distance(plate, parked_plate)
        if dist < min_distance:
            min_distance = dist
            best_match = (vid, parked_plate)
            
    # Allow a maximum distance of 2 characters for OCR errors
    if best_match and min_distance <= 2:
        vid, matched_plate = best_match
        exit_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        cursor.execute('''
            UPDATE parking_sessions 
            SET exit_time = ?, exit_image = ?, status = 'đã ra'
            WHERE id = ?
        ''', (exit_time, image_path, vid))
        conn.commit()
        conn.close()
        return {"matched_plate": matched_plate, "time": exit_time, "status": "đã ra"}
    
    conn.close()
    return {"error": "Không tìm thấy xe khớp trong bãi", "plate_detected": plate}

def get_parked_vehicles():
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute("SELECT id, plate_number, entry_time, entry_image FROM parking_sessions WHERE status = 'đang đỗ' ORDER BY entry_time DESC")
    rows = cursor.fetchall()
    conn.close()
    return [{"id": r[0], "plate_number": r[1], "entry_time": r[2], "entry_image": r[3]} for r in rows]

def get_history(limit=50):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute("SELECT id, plate_number, entry_time, exit_time, status FROM parking_sessions ORDER BY id DESC LIMIT ?", (limit,))
    rows = cursor.fetchall()
    conn.close()
    return [{"id": r[0], "plate_number": r[1], "entry_time": r[2], "exit_time": r[3], "status": r[4]} for r in rows]

def reset_db():
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    # Xóa toàn bộ dữ liệu trong bảng
    cursor.execute("DELETE FROM parking_sessions")
    # Reset lại bộ đếm ID về 1
    try:
        cursor.execute("DELETE FROM sqlite_sequence WHERE name='parking_sessions'")
    except:
        pass
    conn.commit()
    conn.close()

if __name__ == "__main__":
    init_db()
