#include <Servo.h>

// -------- IR Sensor Array --------
#define S1 A0
#define S2 A1
#define S3 A2
#define S4 A3
#define S5 A4

// -------- Motor Driver Pins --------
#define ENA 3
#define IN1 6
#define IN2 7
#define ENB 5
#define IN3 8
#define IN4 4

// -------- Ultrasonic Sensors --------
// Front sensor
#define TRIG 9
#define ECHO 10

// Left sensor
#define TRIG_LEFT 13
#define ECHO_LEFT A5

// Right sensor
#define TRIG_RIGHT 11
#define ECHO_RIGHT 12  // Using pin 12

// -------- Servo Motor --------
#define SERVO_PIN 1
Servo detectionServo;
const int servoLeft = 20;
const int servoCenter = 90;
const int servoRight = 170;

// -------- Speeds --------
int baseSpeed = 130;
int maxSpeed = 200;
int recoverySpeed = 120;

// -------- Non-Blocking Timer Variables --------
unsigned long lastDistanceCheck = 0;
unsigned long lastSideCheck = 0;
unsigned long lastServoAction = 0;

const int frontCheckInterval = 100;
const int sideCheckInterval = 150;
const int servoUpdateInterval = 800; 

float distFront = 999;
float distLeft = 999;
float distRight = 999;

// -------- PID Constants --------
float Kp = 0.9;  
float Ki = 0.0;
float Kd = 15.0; 

float P = 0, I = 0, D = 0;
float error = 0, previousError = 0;
float lastSeen = 0;
float PID_value = 0;

bool all_black = false;
int allBlackCount = 0;

// -------- FUNCTION PROTOTYPES --------
// (Adding these here ensures the compiler knows about them before they are called)
void stopMotor();
void turn180();
void setMotors(int leftSpeed, int rightSpeed);
void recoverLine();

void setup() {
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);

  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);

  // Serial.begin(9600); // Kept off so Pin 1 Servo works without glitching

  detectionServo.attach(SERVO_PIN);
  detectionServo.write(servoCenter);
  delay(300);
}

// -------- GET ULTRASONIC DISTANCE --------
float getUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 5000); // 5000us timeout (~85cm)
  return (duration > 0) ? (duration * 0.034 / 2.0) : 999;
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. -------- NON-BLOCKING FRONT ULTRASONIC CHECK --------
  if (currentMillis - lastDistanceCheck >= frontCheckInterval) {
    lastDistanceCheck = currentMillis;
    distFront = getUltrasonicDistance(TRIG, ECHO);
  }

  // 2. -------- FRONT OBSTACLE HANDLING (180 TURN) --------
  if (distFront > 0 && distFront < 35) {
    delay(200);
    if (distFront < 25){
      stopMotor();
      delay(200);
      turn180();
      delay(200);
      distFront = 999; 
      return;          
    }
  }

  // 3. -------- NON-BLOCKING SIDE ULTRASONIC CHECK --------
  if (currentMillis - lastSideCheck >= sideCheckInterval) {
    lastSideCheck = currentMillis;
    distLeft = getUltrasonicDistance(TRIG_LEFT, ECHO_LEFT);
    distRight = getUltrasonicDistance(TRIG_RIGHT, ECHO_RIGHT);
  }

  // // 4. -------- SERVO STEERING LOGIC & CAMERA PAUSE --------
  if (currentMillis - lastServoAction > servoUpdateInterval) {
    if (distLeft > 0 && distLeft < 50) {
      detectionServo.write(servoRight);
                           // Halt chassis
      delay(100);                      // 100ms pause to stabilize frame
      lastServoAction = currentMillis; // Reset timer
    } 
    else if (distRight > 0 && distRight < 50) {
      detectionServo.write(servoLeft);
                      // Halt chassis
      delay(100);                      // 100ms pause to stabilize frame
      lastServoAction = currentMillis; // Reset timer
    } 
    else {
      detectionServo.write(servoCenter);
      lastServoAction = currentMillis; // Reset timer
    }
  }

  // 5. -------- SENSOR READING --------
  int s1 = digitalRead(S1);
  int s2 = digitalRead(S2);
  int s3 = digitalRead(S3);
  int s4 = digitalRead(S4);
  int s5 = digitalRead(S5);

  // 6. -------- LINE LOST / RECOVERY --------
  if (s1 == 1 && s2 == 1 && s3 == 1 && s4 == 1 && s5 == 1) {
    recoverLine();
    return;
  }

  // 7. -------- ALL BLACK DETECTION --------
  if (s1 == 0 && s2 == 0 && s3 == 0 && s4 == 0 && s5 == 0) {
    if (!all_black) {
      allBlackCount++;
      all_black = true;
      if (allBlackCount == 2) {
        delay(200);
        stopMotor();
        while (1); // Halt robot
      }
    }
  } else {
    all_black = false;
  }

  // 8. -------- WEIGHTED ERROR CALCULATION --------
  int weights[5] = {-8, -4, 0, 4, 8};
  int sensors[5] = {s1, s2, s3, s4, s5};
  float sum = 0;
  int count = 0;

  for (int i = 0; i < 5; i++) {
    if (sensors[i] == 0) {
      sum += weights[i];
      count++;
    }
  }

  if (count > 0) {
    error = sum / count;
    lastSeen = error;
  } else {
    error = (lastSeen > 0) ? 10 : -10;
  }

  // 9. -------- PID CALCULATION --------
  P = error;
  I = constrain(I + error, -50, 50); // Windup guard
  D = error - previousError;
  PID_value = (Kp * P) + (Ki * I) + (Kd * D);
  previousError = error;

  // 10. -------- MOTOR CONTROL --------
  int leftMotorSpeed = baseSpeed + PID_value;
  int rightMotorSpeed = baseSpeed - PID_value;

  setMotors(constrain(leftMotorSpeed, 0, maxSpeed),
            constrain(rightMotorSpeed, 0, maxSpeed));
}

// -------- 180 TURN --------
void turn180() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, 150);
  analogWrite(ENB, 150);

  delay(700); // Adjusted by iterative observation

  stopMotor();
}

// -------- MOTOR CONTROL --------
void setMotors(int leftSpeed, int rightSpeed) {
  if (leftSpeed >= 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    leftSpeed = -leftSpeed;
  }

  if (rightSpeed >= 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    rightSpeed = -rightSpeed;
  }

  analogWrite(ENA, leftSpeed);
  analogWrite(ENB, rightSpeed);
}

// -------- STOP MOTOR --------
void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// -------- LINE RECOVERY --------
void recoverLine() {
  if (lastSeen < 0) setMotors(-recoverySpeed, 
  recoverySpeed);
  else setMotors(recoverySpeed, -recoverySpeed);

  while (digitalRead(S1) == 1 && digitalRead(S2) == 1 
           && digitalRead(S3) == 1 &&
         digitalRead(S4) == 1 && digitalRead(S5) == 1) {
             // Block until a line is found again
         }
}
