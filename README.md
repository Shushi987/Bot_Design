# Bot_Design
This project develops an autonomous robot that follows a black line using IR sensors and a PID control algorithm for stable navigation. It detects obstacles with an ultrasonic sensor and avoids them automatically. A camera mounted on a servo and connected to a Raspberry Pi enables basic object tracking using OpenCV.
Autonomous Vision-Guided Line Follower
A dual-processor autonomous robot that combines PID-based line following and ultrasonic obstacle avoidance (Arduino) with real-time object detection and a web-based monitoring dashboard (Raspberry Pi + YOLO).

Key Features:
Precision Navigation: Uses a 5-channel IR sensor array with a PID control algorithm for smooth line following.

360° Awareness: Triple ultrasonic sensor setup (Front, Left, Right) for intelligent obstacle avoidance and environment scanning.

AI Vision: Real-time object detection using YOLO (NCNN optimized) via a Raspberry Pi Camera.

Live Dashboard: Flask-based web interface to view the live camera feed, mission elapsed time, and a list of detected unique objects/cards.

Fail-safe Recovery: Automated "Line Lost" recovery routine and "All Black" detection for mission completion.

Project Structure:
line_following_obstacle_avoidance.ino: Arduino firmware handling the PID loop, motor movements, and ultrasonic logic.
PID Line Following

Uses 5 IR sensors (CTR5000 array) to detect the line.

Implements weighted error calculation for smoother tracking.

PID controller adjusts motor speeds dynamically.

PID constants used:

Kp = 1.0
Ki = 0.0
Kd = 14.0

Additional capabilities:

Line recovery function if the robot loses the track.

Weighted sensor logic for accurate error estimation.

All-black detection for stopping at the finish line.
PID Line Following

Uses 5 IR sensors (CTR5000 array) to detect the line.

Implements weighted error calculation for smoother tracking.

PID controller adjusts motor speeds dynamically.

PID constants used:

Kp = 1.0
Ki = 0.0
Kd = 14.0

Additional capabilities:

Line recovery function if the robot loses the track.

Weighted sensor logic for accurate error estimation.

All-black detection for stopping at the finish line.

app.py: Python Flask application for the Raspberry Pi. Manages YOLO inference and the web server.

templates/index.html: The frontend dashboard UI with real-time stat polling.

Arduino Setup
Install the Servo library in your Arduino IDE.

Connect the sensors and motor driver according to the pin definitions in bot_controller.ino.

Note: Unplug the RX pin (Right Ultrasonic Echo) when uploading code to avoid serial conflicts.

Upload the code to your Arduino.

models/: (Directory) Contains the NCNN-exported YOLO model.
A Raspberry Pi camera performs real-time object detection using:

YOLO model (Ultralytics)

NCNN optimized inference

OpenCV processing

Capabilities:

Detects predefined card classes

Tracks detected objects

Draws bounding boxes on the video stream

Maintains a unique list of detected objects



A Flask-based web interface allows monitoring of the robot.

Dashboard displays:

📷 Live camera feed

🃏 Number of detected cards

📋 List of detected items

⏱ Mission timer

🔄 Reset memory button

The dashboard can be accessed from any device on the same network.





                +-------------------+
                |   Raspberry Pi 4 |
                |-------------------|
Camera Module → | YOLO Object Detection |
                | Flask Web Server  |
                +---------+---------+
                          |
                    WiFi Network
                          |
                    Web Dashboard
                     (Laptop/Phone)

Arduino Uno
   │
   ├── IR Sensor Array (Line Detection)
   ├── Ultrasonic Sensors
   ├── Motor Driver (L298N)
   ├── Servo Motor
   │
   └── Controls Robot Movement
