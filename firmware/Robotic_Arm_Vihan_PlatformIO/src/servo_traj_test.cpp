#include <Arduino.h>
#include "servo_config.h"

#include <ServoEasing.h>

int ledState = LOW; 
void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  ledState = LOW;


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

void goToXY(float x, float y, float &theta1, float &theta2) {
    double rsquared = x*x + y*y; 

    double rsquared_div = rsquared / (2*L1*L2); 

    const double link_dependency = L1*L1 - L2 * L2/(2*L1*L2);
    
    theta2 = acos(rsquared_div - link_dependency);

    theta1 = atan2(y,x) - atan2(L2*sin(theta2), L1 + L2*cos(theta2)); 

    theta1 = theta1 * 180/PI; 
    theta2 = theta2 * 180/PI; 
}


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

  // Require exactly 2 tokens (servo and x and y)
  if (args[0] == nullptr || args[1] == nullptr || args[2] == nullptr || args[3] != nullptr) {
    Serial.println("Require three arguments, <servo>, <x>, <y>");
    return;
  }

  // Parse servo number (1-4)
  char *servoEnd = nullptr;
  long servoNumber = strtol(args[0], &servoEnd, 10);
  if (*servoEnd != '\0' || servoNumber < 1 || servoNumber > NUM_SERVOS) {
    Serial.println("Servo number should be between 1 and 4");
    return;
  }

  // Parse x, y (0-180)
  char *x_str = nullptr;
  long x = strtol(args[1], &x_str, 10);
  //Serial.print(x); 
  if (*x_str != '\0' || x < abs(L1-L2) || x > L1+L2) {
    Serial.print("Input should be between ");
    Serial.print(abs(L1-L2));
    Serial.println("mm and ");
    Serial.print(L1 + L2);
    Serial.println("mm."); 
    return;
  }

  char *y_str = nullptr;
  long y = strtol(args[2], &y_str, 10);
  if (*y_str != '\0' || y < abs(L1-L2) || y > L1+L2) {
    Serial.print("Input should be between ");
    Serial.print(abs(L1-L2));
    Serial.println("mm and ");
    Serial.print(L1 + L2);
    Serial.println("mm."); 
    return;
  }

  float theta1, theta2;
  goToXY(x,y, theta1, theta2);
  Serial.print((int)theta1);
  Serial.print((int)theta2);


  //theta_des = - (theta_real - theta_offset); 
  writeAngle((int)1, (int)theta1);

  int theta_real2 = -(((int)theta2) - SERVO_ZERO_ANGLE[2]);
  writeAngle((int)2, theta_real2 );
  Serial.print("Servo 1:");
  Serial.println( (int)theta1);
  Serial.print("Servo 2:");
  Serial.println( (int)theta2);
  
  //writeAngle((int)servoNumber, (int)angle);
  ledState != ledState;
  digitalWrite(LED, ledState);
}

void loop() {
  readCommands();
}