#include <Arduino.h>
#include <ServoEasing.hpp>

#define LED 13
#define MAX_TOKENS 8

//SERVO DEFINITIONS
#define SERVO1 0
#define SERVO2 1
#define SERVO3 2
#define SERVO4 3

#define SERVO1_PIN 9 
#define SERVO2_PIN 10
#define SERVO3_PIN 11  
//PLEASE KEEP IN MIND THAT THE rotation OF SERVO 3 is REVERSED, 
//wrt to the coordinate system. if axis of rotation is x-axis, then increasing angle results in CW rotation, rather than counter clockwise rotation
#define SERVO4_PIN 6 
#define NUM_SERVOS 4

#define SERVO_MIN 0
#define SERVO_MAX 180

#define BASE_SERVO 0
#define CLAW_SERVO 3



//DEPENDS ON THE MECHANICAL SYSTEM, MAYBE CHANGE LATER FOR DIFFERENT ROBOTIC ARMS
ServoEasing servos[NUM_SERVOS];
const uint8_t SERVO_PINS[NUM_SERVOS] = {SERVO1_PIN, SERVO2_PIN, SERVO3_PIN, SERVO4_PIN};

const uint8_t SERVO_MAX_ANGLE[NUM_SERVOS] = {180, 135, 175, 150};
//Should stay at around 90 for "mid position"
const uint8_t SERVO_INIT_ANGLE[NUM_SERVOS] = {97, 95, 90, 110};
const uint8_t SERVO_MIN_ANGLE[NUM_SERVOS] = {0, 15, 30, 40};


float SERVO_ANGLES[NUM_SERVOS] = {SERVO_INIT_ANGLE[0], SERVO_INIT_ANGLE[1], SERVO_INIT_ANGLE[2], SERVO_INIT_ANGLE[3]};

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


enum CMD_STATE {
  PUSH,
  PULL,
  LEFT,
  RIGHT,
  OPEN,
  CLOSE,
  NEUTRAL
};
CMD_STATE cmd_state = NEUTRAL;

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

  // Require exactly 1 tokens (command)
  if (args[0] == nullptr || args[1] != nullptr) { //|| args[2] != nullptr) {
    Serial.println("Require only one argument");
    return;
  }

  // Parse commands

  //char * = nullptr;
  char * cmd = args[0]; 
  //long servoNumber = strtol(args[0], &servoEnd, 10);
  if ( strcmp(cmd, "F") == 0 ) {
    cmd_state = PUSH;
  }
  else if ( strcmp(cmd, "B") == 0 ) {
    cmd_state = PULL;
    //Serial.print("PULL start - S2: "); Serial.print(SERVO_ANGLES[SERVO2]);
    //Serial.print(" S3: "); Serial.println(SERVO_ANGLES[SERVO3]);                //debug lines remove later!!!
  }
  else if ( strcmp(cmd, "L") == 0 ) {
    cmd_state = LEFT;
  }
  else if ( strcmp(cmd, "R") == 0 ) {
    cmd_state = RIGHT;
  } else if ( strcmp(cmd, "U") == 0 ) {
    cmd_state = OPEN;
  } else if ( strcmp(cmd, "D") == 0 ) {
    cmd_state = CLOSE;
  }
  else if ( strcmp(cmd, "N") == 0 ) {
    cmd_state = NEUTRAL;
  }
  else {
    Serial.println("Invalid command. Valid commands are: F, B, L, R, U, D, N");
  }

  Serial.print("Received command: ");
  Serial.println(cmd);

  /*
  // Parse angle (0-180)
  char *angleEnd = nullptr;
  long angle = strtol(args[1], &angleEnd, 10);
  if (*angleEnd != '\0' || angle < SERVO_MIN || angle > SERVO_MAX) {
    Serial.println("Input should be between 0 and 180");
    return;
  }*/

//  writeAngle((int)servoNumber, (int)angle);

}
bool Pushing = true;
bool Pulling = false;
bool Rotating = false;
int state = 0;

void loop() {

  readCommands();

  if (cmd_state == PUSH && state == 0) {
    state++;
    Serial.print("State: ");
    Serial.println(state);
    servos[SERVO2].setEaseTo(0);
    servos[SERVO3].setEaseTo(50);
      
    synchronizeAllServosAndStartInterrupt(false);
    updateAndWaitForAllServosToStop();

    servos[SERVO4].easeTo(60);
  }

  else if ( cmd_state == PULL && state ==1) {
    state++;
    Serial.print("State: ");
    Serial.println(state);
    servos[SERVO2].setEaseTo(90);
    servos[SERVO3].setEaseTo(120);
      
    synchronizeAllServosAndStartInterrupt(false);
    updateAndWaitForAllServosToStop();
  }

  else if (cmd_state == LEFT && state == 2) {
    state++;
    Serial.print("State: ");
    Serial.println(state);
    servos[SERVO1].easeTo(180);
  }

  else if (cmd_state == PUSH && state == 3) {
    state++;
    Serial.print("State: ");
    Serial.println(state);
    servos[SERVO2].setEaseTo(0);
    servos[SERVO3].setEaseTo(50);
      
    synchronizeAllServosAndStartInterrupt(false);
    updateAndWaitForAllServosToStop();
    
    servos[SERVO4].easeTo(110);
  }

  delay(10);

}
