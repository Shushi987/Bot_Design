import cv2
import time
from flask import Flask, render_template, Response, jsonify
from ultralytics import YOLO
app = Flask(__name__)
model = YOLO('custom_ncnn_model', task='detect')
start_time = time.time()
detected_cards = set()
def generate_frames():
   global detected_cards
   cap = cv2.VideoCapture(0)
   cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
   cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
  while cap.isOpened():
    success, frame = cap.read()
    if not success:
       break
    results = model.predict(frame, stream=True, verbose=False)
    for r in results:
     annotated_frame = r.plot()
    for box in r.boxes:
     class_id = int(box.cls[0])
    confidence = float(box.conf[0])
    if confidence > 0.60:
     detected_cards.add(class_id)
    ret, buffer = cv2.imencode('.jpg', annotated_frame)
    frame_bytes = buffer.tobytes()
    yield (b'--frame\r\n'
      b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
@app.route('/')
def index():
 return render_template('index.html')
@app.route('/video_feed')
def video_feed():
 return Response(generate_frames(), mimetype='multipart/x-mixed-replace;
boundary=frame')
@app.route('/get_stats')
def get_stats():
elapsed = int(time.time() - start_time)
minutes, seconds = divmod(elapsed, 60)
time_str = f"{minutes:02d}:{seconds:02d}"
return jsonify({
'cards_detected': len(detected_cards),
'time_elapsed': time_str
})
@app.route('/reset', methods=['POST'])
def reset():
global start_time, detected_cards
start_time = time.time()
detected_cards.clear()
return jsonify({'status': 'Memory wiped. Timer reset.'})
if __name__ == '__main__':
app.run(host='0.0.0.0', port=5000, debug=False)