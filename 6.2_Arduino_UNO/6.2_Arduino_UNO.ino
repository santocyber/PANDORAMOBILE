#include <Servo.h>

// === PINOS ===
// Servo
#define SERVO_PIN    9

// Driver 74HC595 + PWM
#define PWM1_PIN     5    // motor direito
#define PWM2_PIN     6    // motor esquerdo
#define SHIFT_CLOCK  2
#define LATCH_PIN    4
#define DATA_PIN     8
#define ENABLE_PIN   7

// Ultrassônico
#define TRIG_PIN    12
#define ECHO_PIN    13

// === COMANDOS ASCII ===
#define CMD_FWD     'f'
#define CMD_BWD     'b'
#define CMD_CCW     'c'
#define CMD_CW      'w'
#define CMD_STOP    's'
#define CMD_SERVO_L 'L'
#define CMD_SERVO_R 'R'
#define CMD_SERVO_A 'A'  // absoluto, ex: "A120\n"

// === CÓDIGOS shiftOut ===
const byte DIR_FORWARD   =  92;
const byte DIR_BACKWARD  = 163;
const byte DIR_STOP      =   0;

// === PARÂMETROS ===
const float MIN_DIST_CM       = 5.0;        // distância mínima de segurança
const unsigned long TIMEOUT_MS = 2000UL;   // 10 s sem comando → STOP

// === ESTADOS GLOBAIS ===
Servo        myServo;
int          servoAngle    = 90;
bool         movingForward = false;
unsigned long lastCmdTime  = 0;

// ------------ PROTÓTIPOS ------------
float  getDistance();
void   Motor(byte dir, byte s1, byte s2);
void   checkSerialCommands();
void   checkCommandTimeout();
void   checkSafetyStop();
void   sendDistance();

// ------------ SETUP ------------
void setup() {
  Serial.begin(115200);

  // Servo
  myServo.attach(SERVO_PIN);
  myServo.write(servoAngle);

  // 74HC595 + PWM
  pinMode(ENABLE_PIN,   OUTPUT);
  pinMode(PWM1_PIN,     OUTPUT);
  pinMode(PWM2_PIN,     OUTPUT);
  pinMode(SHIFT_CLOCK,  OUTPUT);
  pinMode(LATCH_PIN,    OUTPUT);
  pinMode(DATA_PIN,     OUTPUT);

  // Ultrassônico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);  // garante trigger LOW

  // Parada inicial
  Motor(DIR_STOP, 0, 0);
  lastCmdTime = millis();
}

// ------------ LOOP ------------
void loop() {
  // 1) Comandos que chegam
  checkSerialCommands();

  // 2) Timeout sem comando → STOP
  checkCommandTimeout();

  // 3) Se estiver avançando, checa obstáculo
  if (movingForward) {
    checkSafetyStop();
  }

  // 4) Envia DIST a cada 500 ms
  static unsigned long tDist = 0;
  if (millis() - tDist >= 500) {
    sendDistance();
    tDist = millis();
  }
}

// ------------ LEITURA DO SENSOR ------------
float getDistance() {
  // Gera pulso de trigger
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Aguarda echo até 30 ms (máx ≈ 5 m)
  unsigned long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  if (dur == 0) return -1.0;      // sem leitura
  return dur / 58.0;              // converte para cm
}

// ------------ ENVIO PARA 74HC595 + PWM ------------
void Motor(byte dir, byte s1, byte s2) {
  digitalWrite(ENABLE_PIN, LOW);
  analogWrite(PWM1_PIN, s1);
  analogWrite(PWM2_PIN, s2);
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, SHIFT_CLOCK, MSBFIRST, dir);
  digitalWrite(LATCH_PIN, HIGH);
}

// ------------ ENVIA DISTÂNCIA POR SERIAL ------------
void sendDistance() {
  float d = getDistance();
  if (d >= 0) {
    Serial.print("DIST:");
    Serial.print(d, 2);
    Serial.println("cm");
  } else {
    Serial.println("DIST:NODATA");
  }
}

// ------------ TIMEOUT DE COMANDO ------------
void checkCommandTimeout() {
  if (millis() - lastCmdTime > TIMEOUT_MS) {
    Motor(DIR_STOP, 0, 0);
    movingForward = false;
    lastCmdTime = millis();
    Serial.println("[TIMEOUT] STOP");
  }
}

// ------------ PARADA DE SEGURANÇA ------------
void checkSafetyStop() {
  float d = getDistance();
  if (d >= 0 && d < MIN_DIST_CM) {
    Motor(DIR_STOP, 0, 0);
    movingForward = false;
    Serial.println("[SAFETY] STOP");
  }
}

// ------------ TRATAMENTO DE COMANDOS ASCII ------------
void checkSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    lastCmdTime = millis();  // reset timeout

    // Servo absoluto “A<num>”
    if (c == CMD_SERVO_A) {
      int a = Serial.parseInt();
      a = constrain(a, 0, 180);
      servoAngle = a;
      myServo.write(a);
      Serial.print("Servo:");
      Serial.println(a);
      continue;
    }

    switch (c) {
      case CMD_FWD:
        Motor(DIR_FORWARD, 255, 255);
        movingForward = true;
        Serial.println("CMD:FWD");
        break;
      case CMD_BWD:
        Motor(DIR_BACKWARD, 255, 255);
        movingForward = false;
        Serial.println("CMD:BWD");
        break;
      case CMD_CCW:
        Motor(DIR_FORWARD, 255, 0);
        movingForward = false;
        Serial.println("CMD:CCW");
        break;
      case CMD_CW:
        Motor(DIR_FORWARD, 0, 255);
        movingForward = false;
        Serial.println("CMD:CW");
        break;
      case CMD_STOP:
        Motor(DIR_STOP, 0, 0);
        movingForward = false;
        Serial.println("CMD:STOP");
        break;
      case CMD_SERVO_L:
        servoAngle = max(0, servoAngle - 1);
        myServo.write(servoAngle);
        Serial.print("Servo:");
        Serial.println(servoAngle);
        break;
      case CMD_SERVO_R:
        servoAngle = min(180, servoAngle + 1);
        myServo.write(servoAngle);
        Serial.print("Servo:");
        Serial.println(servoAngle);
        break;
      default:
        // ignora bytes não reconhecidos
        break;
    }
  }
}
