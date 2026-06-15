import cv2
import easyocr
import os
from ultralytics import YOLO

# Khởi tạo EasyOCR reader (tiếng Anh để đọc alphanumeric biển số)
# Dùng gpu=True nếu laptop có card đồ họa rời
reader = easyocr.Reader(['en'], gpu=False)

# Tải model YOLOv8 (sẽ tải best.pt - model đã train trên dataset Roboflow)
# Nếu file chưa tồn tại ở đây, cần download từ Roboflow đặt vào thư mục server
MODEL_PATH = "best.pt"
try:
    if os.path.exists(MODEL_PATH):
        model = YOLO(MODEL_PATH)
    else:
        print(f"WARNING: Model {MODEL_PATH} not found. Please download it from Roboflow.")
        model = None
except Exception as e:
    print(f"Error loading YOLO model: {e}")
    model = None

def process_image(image_path: str):
    """
    Nhận đường dẫn ảnh, detect biển số, crop và đọc OCR.
    Trả về: (biển_số_đọc_được, ảnh_crop_path)
    """
    if model is None:
        return "NO_MODEL", None
        
    img = cv2.imread(image_path)
    if img is None:
        return "ERROR_READING_IMG", None

    # Detect
    results = model(img)
    
    # Lấy bounding box
    for r in results:
        boxes = r.boxes
        if len(boxes) > 0:
            valid_boxes = []
            img_area = img.shape[0] * img.shape[1]
            
            # Lọc các box hợp lệ
            for box in boxes:
                conf = box.conf[0].item()
                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)
                box_area = (x2 - x1) * (y2 - y1)
                
                # Bỏ qua box rác: Độ tin cậy thấp hoặc chiếm quá 80% diện tích ảnh (nhận nhầm cả cái xe)
                if conf > 0.4 and (box_area / img_area) < 0.8:
                    valid_boxes.append((conf, x1, y1, x2, y2))
            
            if not valid_boxes:
                continue
                
            # Chọn box có độ tin cậy cao nhất trong số các box hợp lệ
            valid_boxes.sort(key=lambda x: x[0], reverse=True)
            best_box = valid_boxes[0]
            x1, y1, x2, y2 = best_box[1], best_box[2], best_box[3], best_box[4]
            
            # Crop ảnh
            cropped_plate = img[y1:y2, x1:x2]
            
            # Lưu ảnh crop (để debug và hiển thị lên Web)
            crop_path = image_path.replace(".jpg", "_crop.jpg")
            cv2.imwrite(crop_path, cropped_plate)
            
            # Đọc OCR
            allowed_chars = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-.'
            # Sử dụng paragraph=True để EasyOCR tự động gộp dòng thông minh (khắc phục triệt để vụ lộn xộn Y/X)
            ocr_result = reader.readtext(cropped_plate, allowlist=allowed_chars, mag_ratio=2.0, paragraph=True)
            
            if ocr_result:
                # 1. Nối chữ (Lúc này ocr_result đã được paragraph gộp cực chuẩn)
                text = "".join([res[1] for res in ocr_result])
                
                # 2. Làm sạch, chỉ giữ lại chữ và số
                clean_text = "".join(e for e in text if e.isalnum()).upper()
                
                # 3. Sửa lỗi AI nhận diện nhầm nét (VD: G -> 6, 8 -> B)
                chars = list(clean_text)
                if len(chars) >= 6:
                    char_map = {'0':'D','1':'I','2':'Z','4':'A','5':'S','6':'G','8':'B'}
                    num_map = {'A':'4','B':'8','D':'0','G':'6','I':'1','O':'0','S':'5','Z':'2'}
                    
                    # - 2 ký tự đầu luôn là SỐ (Mã tỉnh)
                    for i in [0, 1]:
                        if chars[i] in num_map: chars[i] = num_map[chars[i]]
                        
                    # - Ký tự thứ 3 luôn là CHỮ CÁI (Series biển)
                    if chars[2] in char_map: chars[2] = char_map[chars[2]]
                    
                    # - Phân tích từ ký tự thứ 4 trở đi
                    if len(chars) == 8:
                        # Biển oto hoặc xe máy cũ (VD: 30G12345) -> từ index 3 đến cuối là số
                        for i in range(3, len(chars)):
                            if chars[i] in num_map: chars[i] = num_map[chars[i]]
                    elif len(chars) == 9:
                        # Biển xe máy 5 số (VD: 77G116932 hoặc xe 50cc 60AB43750)
                        # Vị trí index 3 có thể là CHỮ hoặc SỐ -> Giữ nguyên không can thiệp!
                        # Từ index 4 trở đi chắc chắn là SỐ
                        for i in range(4, len(chars)):
                            if chars[i] in num_map: chars[i] = num_map[chars[i]]
                
                final_plate = "".join(chars)
                
                # 5. Tự động format lại cho đẹp mắt trên giao diện
                if len(final_plate) == 8:
                    # Biển ô tô 5 số (VD: 76A22222 -> 76A-222.22)
                    final_plate = f"{final_plate[:3]}-{final_plate[3:6]}.{final_plate[6:]}"
                elif len(final_plate) == 7:
                    # Biển ô tô 4 số (VD: 76A1234 -> 76A-1234)
                    final_plate = f"{final_plate[:3]}-{final_plate[3:]}"
                elif len(final_plate) == 9:
                    # Biển xe máy 5 số (VD: 77G116932 -> 77G1-169.32)
                    final_plate = f"{final_plate[:4]}-{final_plate[4:7]}.{final_plate[7:]}"
                
                return final_plate, crop_path
                
    return "UNKNOWN", None
