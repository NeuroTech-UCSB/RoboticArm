#include <Arduino.h>
#include <Servo.h>

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
Servo servos[NUM_SERVOS];
#define BASE_SERVO 0
#define CLAW_SERVO 3


const int SERVO_PINS[NUM_SERVOS] = {SERVO1_PIN, SERVO2_PIN, SERVO3_PIN, SERVO4_PIN};

const float SERVO_MAX_ANGLE[NUM_SERVOS] = {180, 135, 180, 120};
//Initial angle should stay around 90 for "mid position"
const float SERVO_INIT_ANGLE[NUM_SERVOS] = {97, 90, 135, 90};
const float SERVO_MIN_ANGLE[NUM_SERVOS] = {0, 45, 90, 40};

float SERVO_ANGLES[NUM_SERVOS] = {SERVO_INIT_ANGLE[0], SERVO_INIT_ANGLE[1], SERVO_INIT_ANGLE[2], SERVO_INIT_ANGLE[3]};


void writeAngle(uint8_t servoIndex  , float angle);
void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.begin(115200);


  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(SERVO_PINS[i]);
  }

  for (int i = 0; i < NUM_SERVOS; i++) {
    writeAngle(i , SERVO_INIT_ANGLE[i]);
  }
  Serial.println("Setup Complete!");
}




void writeAngle(uint8_t servoIndex, float angle) {
  //int servoIndex = servoNumber;

  if (angle < SERVO_MIN_ANGLE[servoIndex]) {
    angle = SERVO_MIN_ANGLE[servoIndex];
  } else if (angle > SERVO_MAX_ANGLE[servoIndex]) {
    angle = SERVO_MAX_ANGLE[servoIndex];
  }

  SERVO_ANGLES[servoIndex] = angle;

  servos[servoIndex].write(angle);
  //Serial.print("Servo ");
  //Serial.print(servoIndex);
  //Serial.print(" angle set to: ");
  //Serial.println(angle);
}


int LED_state = 0;
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
  LED_state = !LED_state;
  digitalWrite(LED, LED_state);
}

void loop() {

  readCommands();

  if ( cmd_state == PUSH ) {
    if (SERVO_ANGLES[SERVO2] > SERVO_MIN_ANGLE[SERVO2] && SERVO_ANGLES[SERVO3] > SERVO_MIN_ANGLE[SERVO3]) {
      writeAngle(SERVO2, SERVO_ANGLES[SERVO2] - 0.1); 
      writeAngle(SERVO3, SERVO_ANGLES[SERVO3] - 0.1); //direction is reversed for servo 3 
      //Serial.println("Pushing");   
    }

    }
  else if ( cmd_state == PULL ) {
      if (SERVO_ANGLES[SERVO2] <= SERVO_MAX_ANGLE[SERVO2] && SERVO_ANGLES[SERVO3] <= SERVO_MAX_ANGLE[SERVO3]) {
        writeAngle(SERVO2, SERVO_ANGLES[SERVO2] + 0.1); 
        writeAngle(SERVO3, SERVO_ANGLES[SERVO3] + 0.1); //direction is reversed for servo 3 
        //Serial.println("Pulling");
      }
  }
  else if ( cmd_state == LEFT ) {
    writeAngle(BASE_SERVO, SERVO_ANGLES[BASE_SERVO] + 0.1);
    //Serial.println("Moving left");

  }
  else if ( cmd_state == RIGHT ) {
    writeAngle(BASE_SERVO, SERVO_ANGLES[BASE_SERVO] - 0.1);
    //Serial.println("Moving right");
  }
  else if (cmd_state == OPEN) {
    writeAngle(CLAW_SERVO, SERVO_MAX_ANGLE[CLAW_SERVO]);
    //Serial.println("Opening claw");
  }
   else if (cmd_state == CLOSE) {
    writeAngle(CLAW_SERVO, SERVO_MIN_ANGLE[CLAW_SERVO]);
    //Serial.println("Closing claw");
  }
  else if ( cmd_state == NEUTRAL ) {
    //Serial.println("neutral position");
  }
  delay(10);

}