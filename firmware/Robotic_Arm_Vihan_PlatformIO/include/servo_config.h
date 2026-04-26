#ifndef SERVO_CONFIG_H
#define SERVO_CONFIG_H

#include "ServoEasing.hpp"

#define LED 13
#define MAX_TOKENS 8

//SERVO DEFINITIONS
#define SERVO1 0
#define SERVO2 1
#define SERVO3 2
#define SEROV4 3

#define SERVO1_PIN 9 
#define SERVO2_PIN 10
#define SERVO3_PIN 11  
//PLEASE KEEP IN MIND THAT THE rotation OF SERVO 3 is REVERSED, 
//wrt to the coordinate system. if axis of rotation is x-axis, then increasing angle results in CW rotation, rather than counter clockwise rotation

#define SERVO4_PIN 6 
#define NUM_SERVOS 4

#define SERVO_MIN 0
#define SERVO_MAX 180

//DEPENDS ON THE MECHANICAL SYSTEM, MAYBE CHANGE LATER FOR DIFFERENT ROBOTIC ARMS
ServoEasing servos[NUM_SERVOS];
#define BASE_SERVO 0
#define CLAW_SERVO 3

//Link Lengths
#define L1 63.5 //From Base to second Link
#define L2 127  //From start of second link to end of grasper
#define EPS 3 //error of 3 mm

const float SERVO_PINS[NUM_SERVOS] = {SERVO1_PIN, SERVO2_PIN, SERVO3_PIN, SERVO4_PIN};

const float SERVO_MAX_ANGLE[NUM_SERVOS] = {180, 180, 20, 180};
//Initial angle should stay around 90 for "mid position"
const float SERVO_INIT_ANGLE[NUM_SERVOS] = {97, 87, 20, 90};
const float SERVO_MIN_ANGLE[NUM_SERVOS] = {0, 0, -160, 30};

//used for IK calculations
const float SERVO_ZERO_ANGLE[NUM_SERVOS] = {0, 0, 20, 0};

float SERVO_ANGLES[NUM_SERVOS] = {SERVO_INIT_ANGLE[0], SERVO_INIT_ANGLE[1], SERVO_INIT_ANGLE[2], SERVO_INIT_ANGLE[3]};

//Pick and hold position
//{x, 15, 100, 70}

void writeAngle(uint8_t servoIndex, float angle) {

  /*if (angle < SERVO_MIN_ANGLE[servoIndex]) {
    angle = SERVO_MIN_ANGLE[servoIndex];
  } else if (angle > SERVO_MAX_ANGLE[servoIndex]) {
    angle = SERVO_MAX_ANGLE[servoIndex];
  }*/

  if (angle < SERVO_MIN) {
    angle = SERVO_MIN;
  } else if (angle > SERVO_MAX) {
    angle = SERVO_MAX;
  }



  SERVO_ANGLES[servoIndex] = angle;

  servos[servoIndex].startEaseTo(angle);
  Serial.print("Servo ");
  Serial.print(servoIndex);
  Serial.print(" angle set to: ");
  Serial.println(angle);
}

#endif 
