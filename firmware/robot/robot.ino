#include <Servo.h>

// Arduino robotic arm serial controller

// Input line formats from Python:
// 1) Preferred: CMD,<seq>,<cmd>
//    Example: CMD,12,F
// 2) Legacy: <cmd>
//    Example: F

// Response format back to Python:
// - ACK,<seq>,<cmd>,<arduino_us>
// - ERR,<seq>,<reason>

// Easy explanation:
// - Python sends text commands over USB serial.
// - Arduino reads one line at a time.
// - Arduino moves servo(s), then replies ACK or ERR.

// Reference links used for this file:
// - Arduino Servo library reference:
//   https://docs.arduino.cc/libraries/servo/
// - Arduino Serial reference: 
//   https://www.arduino.cc/reference/en/language/functions/communication/serial/

// ===== 1) SERVO SETUP =====
// Update pins to match your robot arm wiring.
Servo baseServo;
Servo shoulderServo;
Servo elbowServo;
Servo gripperServo;

// Current servo angles (degrees). Start at neutral midpoint.
// If movement direction is opposite of your arm setup, reverse +/- in applyCommand().
int basePos = 90;
int shoulderPos = 90;
int elbowPos = 90;
int gripperPos = 90;

// Movement increment per command.
// Bigger STEP = faster/larger jump each command.
const int STEP = 4;

// Serial line buffer for incoming command text.
const int SERIAL_BUF_LEN = 64;
char serialBuf[SERIAL_BUF_LEN];
uint8_t serialIdx = 0;

// ===== 2) HELPERS =====
// Keep angle values inside servo-safe range.
int clampInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

// Write all current angles to servos.
void applyPose() {
  baseServo.write(basePos);
  shoulderServo.write(shoulderPos);
  elbowServo.write(elbowPos);
  gripperServo.write(gripperPos);
}

// ===== 3) COMMAND LOGIC =====
// Apply one command letter. Return true if recognized.
bool applyCommand(char cmd) {
  switch (cmd) {
    case 'F': elbowPos += STEP; break;  // current project: "push" raises elbow angle
    case 'N': break; // neutral: no movement
    default: return false;
  }

  // Clamp all joints after any change.
  basePos = clampInt(basePos, 0, 180);
  shoulderPos = clampInt(shoulderPos, 0, 180);
  elbowPos = clampInt(elbowPos, 0, 180);
  gripperPos = clampInt(gripperPos, 0, 180);
  applyPose();
  return true;
}

// ACK tells Python command was accepted and applied.
void sendAck(long seq, char cmd) {
  // micros() gives Arduino local timestamp in microseconds.
  unsigned long nowUs = micros();
  Serial.print("ACK,");
  Serial.print(seq);
  Serial.print(",");
  Serial.print(cmd);
  Serial.print(",");
  Serial.println(nowUs);
}

// ERR tells Python the line/command was invalid.
void sendErr(long seq, const char* reason) {
  Serial.print("ERR,");
  Serial.print(seq);
  Serial.print(",");
  Serial.println(reason);
}

// Parse one full line (newline removed) and execute.
void processLine(char* line) {
  // Preferred format: CMD,<seq>,<cmd>
  // Backwards compatible format: <cmd>
  if (line[0] == '\0') return;

  if (strncmp(line, "CMD,", 4) != 0) {
    // Legacy one-character command mode.
    char legacyCmd = line[0];
    if (applyCommand(legacyCmd)) {
      sendAck(-1, legacyCmd);
    } else {
      sendErr(-1, "bad_legacy_cmd");
    }
    return;
  }

  // Token 1 = "CMD"
  // strtok edits the line in-place by splitting on commas.
  char* token = strtok(line, ","); // CMD
  // Token 2 = sequence number
  token = strtok(NULL, ",");       // seq
  if (token == NULL) {
    sendErr(-1, "missing_seq");
    return;
  }
  long seq = atol(token);

  // Token 3 = command letter
  token = strtok(NULL, ",");       // cmd
  if (token == NULL || token[0] == '\0') {
    sendErr(seq, "missing_cmd");
    return;
  }
  char cmd = token[0];

  if (!applyCommand(cmd)) {
    sendErr(seq, "unknown_cmd");
    return;
  }

  // If command is valid and applied, acknowledge with same seq.
  sendAck(seq, cmd);
}

// ===== 4) ARDUINO LIFECYCLE =====
void setup() {
  Serial.begin(115200);
  // Baud rate must match Python side.
  // Attach each servo to its PWM pin.
  baseServo.attach(3);
  shoulderServo.attach(5);
  elbowServo.attach(6);
  gripperServo.attach(9);
  // Move to initial pose once at startup.
  applyPose();
}

void loop() {
  // Build a line from serial bytes until newline.
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue; // Ignore Windows CR.

    if (c == '\n') {
      // End of line: null-terminate and process.
      serialBuf[serialIdx] = '\0';
      processLine(serialBuf);
      serialIdx = 0;
      continue;
    }

    if (serialIdx < (SERIAL_BUF_LEN - 1)) {
      serialBuf[serialIdx++] = c;
    } else {
      // Overflow guard: reset partial line and report error.
      // Prevents writing beyond serialBuf memory.
      serialIdx = 0;
      sendErr(-1, "line_too_long");
    }
  }
}
