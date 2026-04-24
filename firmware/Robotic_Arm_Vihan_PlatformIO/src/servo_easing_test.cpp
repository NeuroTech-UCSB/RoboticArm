#include <Arduino.h>
#include <ServoEasing.hpp>

#define LED 13
#define MAX_TOKENS 8

//SERVO DEFINITIONS
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
const uint8_t SERVO_PINS[NUM_SERVOS] = {SERVO1_PIN, SERVO2_PIN, SERVO3_PIN, SERVO4_PIN};
const uint8_t SERVO_MAX_ANGLE[NUM_SERVOS] = {180, 135, 175, 150};

//Should stay at around 90 for "mid position"
const uint8_t SERVO_INIT_ANGLE[NUM_SERVOS] = {97, 95, 90, 90};
const uint8_t SERVO_MIN_ANGLE[NUM_SERVOS] = {0, 15, 30, 40};

//Pick and hold position
//{x, 15, 100, 70}


void writeAngle(int servoNumber, int angle);
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
    writeAngle(i + 1, SERVO_INIT_ANGLE[i]);
  }
}

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

  // Require exactly 2 tokens (servo and angle)
  if (args[0] == nullptr || args[1] == nullptr || args[2] != nullptr) {
    Serial.println("Require two arguments");
    return;
  }

  // Parse servo number (1-4)
  char *servoEnd = nullptr;
  long servoNumber = strtol(args[0], &servoEnd, 10);
  if (*servoEnd != '\0' || servoNumber < 1 || servoNumber > NUM_SERVOS) {
    Serial.println("Servo number should be between 1 and 4");
    return;
  }

  // Parse angle (0-180)
  char *angleEnd = nullptr;
  long angle = strtol(args[1], &angleEnd, 10);
  if (*angleEnd != '\0' || angle < SERVO_MIN || angle > SERVO_MAX) {
    Serial.println("Input should be between 0 and 180");
    return;
  }

  writeAngle((int)servoNumber, (int)angle);
  digitalWrite(LED, angle > 0 ? HIGH : LOW);
}

void loop() {

  readCommands();
}