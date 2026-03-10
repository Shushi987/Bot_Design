import cv2
from ultralytics import YOLO
model = YOLO('best_ncnn_model', task='detect')
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
print("Bot vision ini􀆟alized. Looking for image cards...")
while cap.isOpened():
  success, frame = cap.read()
   if not success:
   print("Failed to grab frame from camera. Exi􀆟ng...")
   break

results = model.predict(frame, stream=True, verbose=False)

for r in results:
boxes = r.boxes
for box in boxes:
# Get the class ID and confidence score
class_id = int(box.cls[0])
confidence = float(box.conf[0])
# Print what the bot sees to the terminal
print(f"Card Detected! Class: {class_id}, Confidence: {confidence:.2f}")
cap.release()