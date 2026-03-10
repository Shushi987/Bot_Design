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

// -------- Ultrasonic Sensor --------
#define TRIG 9
#define ECHO 10

// -------- Speeds --------
int baseSpeed = 130;
int maxSpeed = 200;
int recoverySpeed = 120;
int avoidSpeed = 140; // Speed for obstacle maneuver

// -------- Non-Blocking Timer Variables --------
unsigned long lastDistanceCheck = 0;
const int checkInterval = 100; 
float currentDistance = 999;

// -------- PID Constants --------
float Kp = 1.0;  
float Ki = 0.0;
float Kd = 14.0; 

float P=0, I=0, D=0;
float error=0, previousError=0;
float lastSeen = 0;
float PID_value=0;

bool all_black = false;
int allBlackCount = 0;

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

  Serial.begin(9600);
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. -------- NON-BLOCKING ULTRASONIC CHECK --------
  if (currentMillis - lastDistanceCheck >= checkInterval) {
    lastDistanceCheck = currentMillis;
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    long duration = pulseIn(ECHO, HIGH, 10000); 
    currentDistance = (duration > 0) ? (duration * 0.034 / 2) : 999;
  }

  // 2. -------- OBSTACLE HANDLING (WALL FOLLOW BYPASS) --------
  if (currentDistance > 0 && currentDistance < 15) {
    avoidObstacle(); 
    currentDistance = 999; 
    return;
  }

  // 3. -------- SENSOR READING --------
  int s1 = digitalRead(S1);
  int s2 = digitalRead(S2);
  int s3 = digitalRead(S3);
  int s4 = digitalRead(S4);
  int s5 = digitalRead(S5);

  // 4. -------- LINE LOST / RECOVERY --------
  if (s1 == 1 && s2 == 1 && s3 == 1 && s4 == 1 && s5 == 1) {
    recoverLine();
    return;
  }

  // 5. -------- ALL BLACK DETECTION --------
  if (s1 == 0 && s2 == 0 && s3 == 0 && s4 == 0 && s5 == 0) {
    if (!all_black) {
      allBlackCount++;
      all_black = true;
      if (allBlackCount == 2) {
        delay(200);
        stopMotor();
        while (1); 
      }
    }
  } else {
    all_black = false;
  }

  // 6. -------- WEIGHTED ERROR CALCULATION --------
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

  // 7. -------- PID CALCULATION --------
  P = error;
  I = constrain(I + error, -50, 50);
  D = error - previousError;
  PID_value = (Kp * P) + (Ki * I) + (Kd * D);
  previousError = error;

  // 8. -------- MOTOR CONTROL --------
  int leftMotorSpeed  = baseSpeed + PID_value;
  int rightMotorSpeed = baseSpeed - PID_value;
  setMotors(constrain(leftMotorSpeed, 0, maxSpeed), constrain(rightMotorSpeed, 0, maxSpeed));
}

// 9. -------- OBSTACLE AVOIDANCE MANEUVER --------
void avoidObstacle() {
  stopMotor();
  delay(500);

  // Turn Left 90 (Adjust delay for your motors)
  setMotors(-avoidSpeed, avoidSpeed);
  delay(450); 
  
  // Move Forward (Clear the side)
  setMotors(avoidSpeed, avoidSpeed);
  delay(700);

  // Turn Right 90
  setMotors(avoidSpeed, -avoidSpeed);
  delay(500);

  // Move Forward (Pass the obstacle)
  setMotors(avoidSpeed, avoidSpeed);
  delay(1100);

  // Turn Right 90 (Angle back toward line)
  setMotors(avoidSpeed, -avoidSpeed);
  delay(500);

  // Move Forward UNTIL sensors see the line
  setMotors(avoidSpeed, avoidSpeed);
  
  unsigned long startTime = millis();
  // Wait until S2, S3, or S4 hits black (0)
  while(digitalRead(S2) == 1 && digitalRead(S3) == 1 && digitalRead(S4) == 1) {
    if(millis() - startTime > 4000) break; // Safety timeout: 4 seconds
  }

  // Small corrective turn to align with the path
  setMotors(-avoidSpeed, avoidSpeed);
  delay(150);
  
  stopMotor();
  delay(200);
}

void setMotors(int leftSpeed, int rightSpeed) {
  if (leftSpeed >= 0) {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    leftSpeed = -leftSpeed;
  }
  if (rightSpeed >= 0) {
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    rightSpeed = -rightSpeed;
  }
  analogWrite(ENA, leftSpeed);
  analogWrite(ENB, rightSpeed);
}

void stopMotor() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}

void recoverLine() {
  if (lastSeen < 0) setMotors(-recoverySpeed, recoverySpeed);
  else setMotors(recoverySpeed, -recoverySpeed);
  while (digitalRead(S1) == 1 && digitalRead(S2) == 1 && digitalRead(S3) == 1 &&
         digitalRead(S4) == 1 && digitalRead(S5) == 1);
}