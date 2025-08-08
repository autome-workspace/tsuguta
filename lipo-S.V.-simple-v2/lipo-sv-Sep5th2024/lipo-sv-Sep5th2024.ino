#include <avr/pgmspace.h>

const int SER = A1;
const int CLK = A2;
const int LATCH = A3;
const int REFERENCE = A6;
const int MODE = A7;

byte b1 = 0b00000000;
byte b2 = 0b00000000;
// 以下b1b2兼用
const byte null = 0b00000000;
// 以下b2用
const byte buzz = 0b00100000;
// 以下b1用
const byte num0 PROGMEM = 0b00111111;
const byte num1 PROGMEM = 0b00000110;
const byte num2 PROGMEM = 0b01011011;
const byte num3 PROGMEM = 0b01001111;
const byte num4 PROGMEM = 0b01100110;
const byte num5 PROGMEM = 0b01101101;
const byte num6 PROGMEM = 0b01111101;
const byte num7 PROGMEM = 0b00000111;
const byte num8 PROGMEM = 0b01111111;
const byte num9 PROGMEM = 0b01101111;
const byte dot  PROGMEM = 0b10000000;
const byte C    PROGMEM = 0b00111001;
const byte E    PROGMEM = 0b01111001;
const byte L    PROGMEM = 0b00111000;
const byte LEDA PROGMEM = 0b00000001;
const byte LEDB PROGMEM = 0b00000010;
const byte LEDC PROGMEM = 0b00000100;
const byte LEDD PROGMEM = 0b00001000;
const byte LEDE PROGMEM = 0b00010000;
const byte LEDF PROGMEM = 0b00100000;
const byte LEDG PROGMEM = 0b01000000;

const byte cell3[]        = {num3, C, E, L+dot};
const byte cell2[]        = {num2, C, E, L+dot};
const byte num[]          = {num0, num1, num2, num3, num4, num5, num6, num7, num8, num9};
const byte blinkPattern[] = {LEDG, LEDB, LEDA, LEDF, LEDE, LEDD, LEDC, dot};

const int attentionVoltageLevel = 3700; // 1Sあたり。単位はmV
const int warningVoltageLevel = 3500; //1Sあたり。単位はmV
const int gain = 1027; //測定電圧の誤差修正用。算出方法: テスターでの測定値 / ゲイン無しでの表示値 * 1000
int cellAmount = 0;
int attentionFlg = 0;
int warningFlg = 0;
int displayCnt = 4;
int displayOffCnt = 0;

void setup() {
  pinMode(SER, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(MODE, INPUT);

  blinkInit();
}

void loop() {
  int batteryVoltage = getVoltage();
  for (int i = 0; i < 500; i++) {
    if (attentionFlg == 1) {
      batteryVoltage = getVoltage();
    }
    cellModeDetector();
    displayVoltage(batteryVoltage);
    checkVoltage(batteryVoltage);
  }
}

void shiftOutData() {
  digitalWrite(LATCH, 0);
  shiftOut(SER, CLK, LSBFIRST, b1);
  shiftOut(SER, CLK, LSBFIRST, b2);
  digitalWrite(LATCH, 1);
}

void blinkInit() {
  b2 = null;
  for (int i = 0; i < 4; i++) {
    b1 = null;
    shiftOutData();
    b2 <<= 1;
  }
  delay(500);
  b2 = 0b00000001;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 1500; j++) {
      b1 = blinkPattern[i];
      shiftOutData();
      b2 <<= 1;
      if (b2 == 0b00010000) {
        b2 = 0b00000001;
      }
    }
  }
  for (int i; i < 2; i++){
    b2 = buzz;
    shiftOutData();
    delay(10);
    b2 = null;
    shiftOutData();
    delay(490);
  }
}

void checkVoltage(int batteryVoltage) {
  int cellVoltage = batteryVoltage / cellAmount;
  if (cellVoltage < attentionVoltageLevel) {
    attentionFlg = 1;
  }else {
    attentionFlg = 0;
  }
  if (cellVoltage < warningVoltageLevel) {
    warningFlg = 1;
  }else {
    warningFlg = 0;
  }
}

void displayOff() {
  b2 = 0b00000001;
  for (int i = 0; i < displayOffCnt; i++) {
    b1 = null;
    shiftOutData();
    b2 <<= 1;
    if (b2 == 0b00010000) {
      b2 = 0b00000001;
    }
  }
}

int getVoltage() {
  long batteryVoltage = (long)analogRead(REFERENCE) * 5 * 6 * 1000 / 1023;
  batteryVoltage = (long)batteryVoltage * gain / 1000;
  return batteryVoltage;
}

void displayVoltage(int batteryVoltage) {
  int voltageDigit[4];
  voltageDigit[0] = batteryVoltage / 10000;
  voltageDigit[1] = batteryVoltage % 10000 / 1000;
  voltageDigit[2] = batteryVoltage % 10000 % 1000 / 100;
  voltageDigit[3] = batteryVoltage % 10000 % 1000 % 100 / 10;
  if (attentionFlg == 1) {
    displayCnt = 3000;
    displayOffCnt = 2000;
  }else {
    displayCnt = 4;
  }
  b2 = 0b00000001;
  for (int i = 0; i < displayCnt; i++) {
    b1 = num[voltageDigit[i%4]];
    if (i%4 == 1) {
      b1 += dot;
    }
    if (i%4 == 0 && voltageDigit[0] == 0) {
      b1 = null;
    }
    if (warningFlg == 1) {
      b2 += buzz;
    }
    shiftOutData();
    if (warningFlg == 1) {
      b2 -= buzz;
    }
    b2 <<= 1;
    if (b2 == 0b00010000) {
      b2 = 0b00000001;
    }
  }
  if (attentionFlg == 1) {
   displayOff();
  }
}

void cellModeDetector() {
  int modeDisplayCnt = 7000;
  if (digitalRead(MODE) == 1 && cellAmount != 3) {
    b2 = 0b00000001;
    for (int i = 0; i < modeDisplayCnt; i++) {
      b1 = cell3[i%4];
      shiftOutData();
      b2 <<= 1;
      if (b2 == 0b00010000) {
        b2 = 0b00000001;
      }
    }
    cellAmount = 3;
  }else if (digitalRead(MODE) == 0 && cellAmount != 2) {
    b2 = 0b00000001;
    for (int i = 0; i < modeDisplayCnt; i++) {
      b1 = cell2[i%4];
      shiftOutData();
      b2 <<= 1;
      if (b2 == 0b00010000) {
        b2 = 0b00000001;
      }
    }
    cellAmount = 2;
  }
}
