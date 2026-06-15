import os
import shutil
from datetime import datetime
from fastapi import FastAPI, File, UploadFile, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
import database
import recognition

app = FastAPI(title="AI Camera Parking Server")

# Initialize database
database.init_db()

# Mount static files and templates
os.makedirs("images", exist_ok=True)
os.makedirs("static", exist_ok=True)
os.makedirs("templates", exist_ok=True)

app.mount("/images", StaticFiles(directory="images"), name="images")
app.mount("/static", StaticFiles(directory="static"), name="static")
templates = Jinja2Templates(directory="templates")

@app.get("/", response_class=HTMLResponse)
async def get_dashboard(request: Request):
    """Phục vụ trang Dashboard quản lý bãi xe."""
    return templates.TemplateResponse(request=request, name="index.html")

@app.get("/api/vehicles")
async def api_get_vehicles():
    """API cho frontend lấy danh sách xe đang đỗ và lịch sử."""
    parked = database.get_parked_vehicles()
    history = database.get_history(limit=20)
    return {"parked": parked, "history": history}

@app.post("/api/reset")
async def api_reset_data():
    """API để reset lại toàn bộ hệ thống (xóa database và ảnh)."""
    # 1. Reset database
    database.reset_db()
    
    # 2. Xóa toàn bộ ảnh trong thư mục images
    if os.path.exists("images"):
        for filename in os.listdir("images"):
            file_path = os.path.join("images", filename)
            try:
                if os.path.isfile(file_path):
                    os.unlink(file_path)
            except Exception as e:
                pass
                
    return {"status": "success", "message": "Đã khôi phục cài đặt gốc thành công!"}

@app.post("/upload/entry")
async def upload_entry(file: UploadFile = File(...)):
    """API cho ESP32-CAM cổng vào gọi."""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    file_extension = file.filename.split(".")[-1] if "." in file.filename else "jpg"
    image_path = f"images/entry_{timestamp}.{file_extension}"
    
    with open(image_path, "wb") as buffer:
        shutil.copyfileobj(file.file, buffer)
        
    plate, crop_path = recognition.process_image(image_path)
    
    if plate and plate not in ["NO_MODEL", "ERROR_READING_IMG", "UNKNOWN"]:
        result = database.add_entry(plate, image_path)
        return {"status": "success", "plate": plate, "db_result": result}
    
    return {"status": "failed", "reason": "Không đọc được biển số", "plate": plate}

@app.post("/upload/exit")
async def upload_exit(file: UploadFile = File(...)):
    """API cho ESP32-CAM cổng ra gọi."""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    file_extension = file.filename.split(".")[-1] if "." in file.filename else "jpg"
    image_path = f"images/exit_{timestamp}.{file_extension}"
    
    with open(image_path, "wb") as buffer:
        shutil.copyfileobj(file.file, buffer)
        
    plate, crop_path = recognition.process_image(image_path)
    
    if plate and plate not in ["NO_MODEL", "ERROR_READING_IMG", "UNKNOWN"]:
        result = database.mark_exit(plate, image_path)
        return {"status": "success", "plate": plate, "db_result": result}
    
    return {"status": "failed", "reason": "Không đọc được biển số", "plate": plate}

if __name__ == "__main__":
    import uvicorn
    # Chạy server ở port 8000, lắng nghe mọi IP (0.0.0.0) để ESP có thể gửi tới
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
