#include <Arduino.h>
#include "servo_config.h"

#include <ServoEasing.h>

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.begin(115200);


  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(SERVO_PINS[i]);
    //servos[i].setEasingType(EASE_QUADRATIC_IN_OUT);
    servos[i].setEasingType(EASE_LINEAR);
    servos[i].setSpeed(100);
  }

  for (int i = 0; i < NUM_SERVOS; i++) {
    writeAngle(i, SERVO_INIT_ANGLE[i]);
  }
}

/*
void writeAngle(int servoNumber, int angle) {
  int servoIndex = servoNumber - 1;

  if (angle < SERVO_MIN_ANGLE[servoIndex]) {
    angle = SERVO_MIN_ANGLE[servoIndex];
  } else if (angle > SERVO_MAX_ANGLE[servoIndex]) {
    angle = SERVO_MAX_ANGLE[servoIndex];
  }

  //servos[servoIndex].write(angle);
  servos[servoIndex].startEaseTo(angle);

  Serial.print("Servo ");
  Serial.print(servoNumber);
  Serial.print(" angle set to: ");
  Serial.println(angle);
}*/

void readCommands() {
  if (!Serial.available()) return;

  char line[32];
  size_t n = Serial.readBytesUntil('\n', line, sizeof(line) - 1);
  line[n] = '\0';

  // Tokenize by whitespace
  char* args[4] = {0}; 

  char *save = nullptr;
  args[0] = strtok_r(line, " \t\r", &save);
  args[1] = strtok_r(nullptr, " \t\r", &save);
  args[2] = strtok_r(nullptr, " \t\r", &save);

  // Require exactly 2 tokens (servo and angle)
  if (args[0] == nullptr || args[1] == nullptr || args[2] != nullptr) {
    Serial.println("Require two arguments");
    return;
  }

  // Parse servo number (1-4)
  char *servoEnd = nullptr;
  long servoNumber = strtol(args[0], &servoEnd, 10);
  if (*servoEnd != '\0' || servoNumber < 0 || servoNumber > NUM_SERVOS-1) {
    Serial.println("Servo number should be between 0 and 3");
    return;
  }

  // Parse angle (0-180)
  char *angleEnd = nullptr;
  long angle = strtol(args[1], &angleEnd, 10);
  Serial.print("Read angle ");
  Serial.println(angle);
  if (*angleEnd != '\0' || angle < SERVO_MIN_ANGLE[servoNumber] || angle > SERVO_MAX_ANGLE[servoNumber]) {
    Serial.print("Input for servo ");
    Serial.print(servoNumber);
    Serial.print(" should be between ");
    Serial.print( SERVO_MIN_ANGLE[servoNumber]);
    Serial.print("and ");
    Serial.print( SERVO_MAX_ANGLE[servoNumber]);
    return;
  }

  Serial.print("Servo: ");
  Serial.println(servoNumber);
  Serial.print(angle);
  if (servoNumber == 2) {
       int theta_real2 = -(((int)angle) - SERVO_ZERO_ANGLE[2]); 
       Serial.print("Servo2 ");
       Serial.println(theta_real2);
       writeAngle((int)2, theta_real2 );
  } else {
      writeAngle((int)servoNumber, (int)angle);
  }

  digitalWrite(LED, angle > 0 ? HIGH : LOW);
}

void loop() {

  readCommands();
}