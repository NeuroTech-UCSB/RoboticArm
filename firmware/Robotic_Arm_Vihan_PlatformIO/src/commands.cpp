#include <Arduino.h>
#include "servo_config.h"

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.begin(115200);

  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(SERVO_PINS[i]);
  }

  for (int i = 0; i < NUM_SERVOS; i++) {
    writeAngle(i + 1, SERVO_INIT_ANGLE[i]);
  }
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
  if ( strcmp(cmd, "push") == 0 ) {
    cmd_state = PUSH;
  }
  else if ( strcmp(cmd, "pull") == 0 ) {
    cmd_state = PULL;
  }
  else if ( strcmp(cmd, "left") == 0 ) {
    cmd_state = LEFT;
  }
  else if ( strcmp(cmd, "right") == 0 ) {
    cmd_state = RIGHT;
  } else if ( strcmp(cmd, "open") == 0 ) {
    cmd_state = OPEN;
  } else if ( strcmp(cmd, "close") == 0 ) {
    cmd_state = CLOSE;
  }
  else if ( strcmp(cmd, "neutral") == 0 ) {
    cmd_state = NEUTRAL;
  }
  else {
    Serial.println("Invalid command. Valid commands are: push, pull, left, right, neutral");
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


}