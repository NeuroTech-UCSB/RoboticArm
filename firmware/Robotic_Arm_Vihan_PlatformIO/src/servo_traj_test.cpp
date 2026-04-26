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

  Serial.print("L1: "); 
  Serial.println(L1);
  Serial.print("L2: "); 
  Serial.println(L2);
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
    double r_squared = x*x + y*y; 

    Serial.print("r_squared: ");
    Serial.println(r_squared); 

    //check if in the Reachable workspace
    if (r_squared > (L1+L2) * (L1+L2) ) {
      theta1 = 180/PI*atan2(y,x); 
      theta2 = 0;
      Serial.print("theta2: ");
      Serial.println(theta2); 

      Serial.print("theta1: ");
      Serial.println(theta1); 
      Serial.println("outside reachable workspace");
      return;
    }
    if (r_squared < abs(L1-L2) * abs(L1-L2)) {
      theta1 = 180/PI*atan2(y,x); 
      theta2 = -180; 
      Serial.print("theta2: ");
      Serial.println(theta2); 

      Serial.print("theta1: ");
      Serial.println(theta1); 
      Serial.println("inside reachable radius");
      return; 
    }
      
    double cos_theta2 = (-r_squared + (L1 * L1) + (L2 * L2)) / (2.0 * L1 * L2);
    
    //const double link_dependency = L1*L1 - L2 * L2/(2*L1*L2);
    
    // Safety clamp to avoid NaN from acos if target is slightly out of bounds
    if (cos_theta2 > 1.0) cos_theta2 = 1.0;
    if (cos_theta2 < -1.0) cos_theta2 = -1.0;

    //theta2 ranges from -180 to 0 (elbow down configuration)
    theta2 = - (PI - acos(cos_theta2));

    double beta = atan2(L2*sin(theta2), L1 + L2*cos(theta2));
    //theta1 = (float)atan2(y, x) - atan2(L2 * sin(theta2), L1 + L2 * cos_theta2); 

    theta1 = atan2(y,x)-beta; 

    theta1 = theta1 * 180/PI; 
    theta2 = theta2 * 180/PI; 

    Serial.print("cos_theta2: ");
    Serial.println(cos_theta2); 

    Serial.print("theta2: ");
    Serial.println(theta2); 

    Serial.print("theta1: ");
    Serial.println(theta1); 
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
  if (args[0] == nullptr || args[1] == nullptr || args[2] != nullptr){ //|| args[3] != nullptr) {
    Serial.println("Require 2 numeric arguments in mm, <x>, <y>");
    return;
  }

  // Parse servo number (1-4)
  //char *servoEnd = nullptr;
  //long servoNumber = strtol(args[0], &servoEnd, 10);
  //if (*servoEnd != '\0' || servoNumber < 1 || servoNumber > NUM_SERVOS) {
  //  Serial.println("Servo number should be between 1 and 4");
  //  return;
  //}

  // Parse x, y (0-180)
  char *x_str = nullptr;
  long x = strtol(args[0], &x_str, 10);
  //Serial.print(x); 
  /*if (*x_str != '\0' || x < abs(L1-L2) || x > L1+L2) {
    Serial.print("Input should be between ");
    Serial.print(abs(L1-L2));
    Serial.println("mm and ");
    Serial.print(L1 + L2);
    Serial.println("mm."); 
    return;
  }*/

  char *y_str = nullptr;
  long y = strtol(args[1], &y_str, 10);
  /*if (*y_str != '\0' || y < abs(L1-L2) || y > L1+L2) {
    Serial.print("Input should be between ");
    Serial.print(abs(L1-L2));
    Serial.println("mm and ");
    Serial.print(L1 + L2);
    Serial.println("mm."); 
    return;
  }*/

  if ( *y_str != '\0' || *x_str != '\0' ) {
    Serial.println("Please enter two coordinates (x,y) in mm"); 
    return; 
  }

  float epsilon = 3; 
  float r_squared = x*x +y*y;
  float max_radius_sq = (L1+L2)*(L1+L2);
  float min_radius_sq = abs(L1-L2) * abs(L1-L2); 
  if (r_squared >= max_radius_sq || r_squared <= min_radius_sq) {
    Serial.println("Radius of coordinates should be within reachable workspace"); 
    Serial.print("x: ");
    Serial.print(x);
    Serial.print(" y: ");
    Serial.println(y);

    Serial.print("r_squared: ");
    Serial.println(r_squared); 

    Serial.print("max_radius_squared: ");
    Serial.println(max_radius_sq);
    Serial.print("min_radius_squared: ");
    Serial.println(min_radius_sq);

    return; 
  }

  float theta1, theta2;
  goToXY(x,y, theta1, theta2);
  //Serial.print((int)theta1);
  //Serial.printl
  //Serial.print((int)theta2);

  //theta_des = - (theta_real - theta_offset); 
  writeAngle((int)1, (int)theta1);

  int theta_real2 = -(((int)theta2) - SERVO_ZERO_ANGLE[2]);
  writeAngle((int)2, theta_real2 );

  Serial.print("Servo 1: ");
  Serial.println( theta1);
  Serial.print("Servo 2: ");
  Serial.println( theta_real2);
  
  //writeAngle((int)servoNumber, (int)angle);
  ledState = !ledState;
  digitalWrite(LED, ledState);
}

void loop() {
  readCommands();
}