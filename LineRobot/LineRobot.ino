#define IN1 8
#define IN2 9
#define IN3 10
#define IN4 11
#define ENA 5
#define ENB 6

#define LEFT 2
#define CENTER 3
#define RIGHT 4

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>
#include <EEPROM.h>

#define IR_PIN 12

// переменные которые изменяемые
int speedForward = 125;
int OUT_TurnSpeed = 150;
int SpotTurnSpeed = 205;

int SpotBackSpeed = 120;
int SpotBackDuration = 100;

int SpotTurnDuration = 370;

int IN_TurnSpeed = 40;


int roundTripDistance = 500; // расстояние в мс
int roundTripSpeed = 80;    // скорость проезда

bool manualMode = false;
bool roundTripMode = false;  // флаг режима "туда-обратно"

int* varPointers[] = {&speedForward, &OUT_TurnSpeed, &SpotTurnSpeed, &SpotBackSpeed, &SpotBackDuration, &SpotTurnDuration, &IN_TurnSpeed, &roundTripDistance, &roundTripSpeed};
String varNames[] = {"ForwardSpeed","OUT_TurnSpeed","SpotTurnSpeed","SpotBackSpeed","SpotBackDuration","SpotTurnDuration","IN_TurnSpeed","RoundTripDist","RoundTripSpeed"};
const int VAR_COUNT = 9;

int state = 0;
int selectedVar = 0;
int inputValue = 0;

int lastDirection = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int LCD_COLS = 16;
const int LCD_ROWS = 2;

String current_text = "";

void printToLCD(String message) {

  if (message == current_text) return;

  current_text = message;
  lcd.clear();

  int msgLength = message.length();
  int maxLength = LCD_COLS * LCD_ROWS;

  if (msgLength > maxLength) {
    message = message.substring(0, maxLength);
    msgLength = maxLength;
  }

  for (int i = 0; i < msgLength; i++) {
    int row = i / LCD_COLS;
    int col = i % LCD_COLS;
    lcd.setCursor(col, row);
    lcd.print(message[i]);
  }
}

void setForward(int l, int r) {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, l);
  analogWrite(ENB, r);
}

void setBackward(int l, int r) {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, l);
  analogWrite(ENB, r);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void turnLeftOnSpot(int speedVal) {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, speedVal);
  analogWrite(ENB, speedVal);
}

void turnRightOnSpot(int speedVal) {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, speedVal);
  analogWrite(ENB, speedVal);
}

int getDigit(uint32_t code){

  if(code==0xAD52FF00) return 0;
  if(code==0xE916FF00) return 1;
  if(code==0xE619FF00) return 2;
  if(code==0xF20DFF00) return 3;
  if(code==0xF30CFF00) return 4;
  if(code==0xE718FF00) return 5;
  if(code==0xA15EFF00) return 6;
  if(code==0xF708FF00) return 7;
  if(code==0xE31CFF00) return 8;
  if(code==0xA55AFF00) return 9;

  return -1;
}

void saveAllVariables(){

  int addr = 0;

  for(int i=0;i<VAR_COUNT;i++){
    EEPROM.put(addr, *varPointers[i]);
    addr += sizeof(int);
  }

  printToLCD("Data Saved");
  delay(1000);
}

void loadAllVariables(){

  int addr = 0;

  for(int i=0;i<VAR_COUNT;i++){
    int value;
    EEPROM.get(addr, value);
    *varPointers[i] = value;
    addr += sizeof(int);
  }

  printToLCD("Data loaded");
  delay(1000);
}

void executeRoundTrip() {
  printToLCD("Going forward...");
  setForward(roundTripSpeed, roundTripSpeed);
  delay(roundTripDistance);
  
  stopMotors();
  delay(500);
  
  printToLCD("Turning around...");
  turnRightOnSpot(SpotTurnSpeed);
  delay(SpotTurnDuration * 2);
  
  stopMotors();
  delay(500);
  
  // Возврат назад
  printToLCD("Going back...");
  setForward(roundTripSpeed, roundTripSpeed);
  delay(roundTripDistance);
  
  stopMotors();
  delay(500);
  
  // Разворот в исходное положение
  printToLCD("Final turn...");
  turnRightOnSpot(SpotTurnSpeed);
  delay(SpotTurnDuration * 2);
  
  stopMotors();
  printToLCD("Trip complete");
  delay(1000);
}

void handleInput(uint32_t code){

  if(code==0xBB44FF00 && state!=0){ // ← отмена
    state=0;
    roundTripMode = false;
    stopMotors();
    printToLCD("Cancelled");
    delay(500);
    return;
  }

  if(state==0){

    if(code==0xBF40FF00){ // OK
      state=1;
      selectedVar=0;
      printToLCD("Select:"+varNames[selectedVar]);
    }
    if(code==0xBD42FF00){ // *
        manualMode = !manualMode;

        if(manualMode){
          printToLCD("Manual mode");
        }else{
          printToLCD("Auto mode");
        }

        delay(500);
        return;
    }
  }

  else if(state==1){

    if(code==0xB54AFF00){ // #
      saveAllVariables();
      printToLCD("Data saved");
      delay(700);
      return;
    }

    if(code==0xB946FF00){ // ↑
      selectedVar--;
      if(selectedVar<0) selectedVar=VAR_COUNT-1;

      printToLCD("Select:"+varNames[selectedVar]);
    }

    if(code==0xEA15FF00){ // ↓
      selectedVar++;
      if(selectedVar>=VAR_COUNT) selectedVar=0;

      printToLCD("Select:"+varNames[selectedVar]);
    }

    if(code==0xE916FF00){ // 1
      roundTripMode = true;
      state = 0; // выход из меню
      return;
    }

    if(code==0xBF40FF00){ // OK
      state=2;
      inputValue=0;

    int currentVal = *varPointers[selectedVar];
    printToLCD("Value:" + String(currentVal));

    }

  }

  else if(state==2){

    int digit=getDigit(code);

    if(digit!=-1){
      inputValue=inputValue*10+digit;
      printToLCD("Value:"+String(inputValue));
    }

    if(code==0xBF40FF00 || code==0xB54AFF00){

      *varPointers[selectedVar]=inputValue;

      printToLCD(varNames[selectedVar]+"="+String(inputValue));
      delay(700);

      state=0;
    }
    if(code==0xBD42FF00){ // *
    inputValue = inputValue / 10;
    printToLCD("Value:"+String(inputValue));
    return;
}
  }
}

void handleManualControl(uint32_t code){

  if(code==0xB946FF00){ // ↑
    setForward(speedForward,speedForward);
  }

  else if(code==0xEA15FF00){ // ↓
    setBackward(SpotBackSpeed,SpotBackSpeed);
  }

  else if(code==0xBB44FF00){ // ←
    turnLeftOnSpot(SpotTurnSpeed);
  }

  else if(code==0xBC43FF00){ // →
    turnRightOnSpot(SpotTurnSpeed);
  }

  else if(code==0){ 
    stopMotors();
  }
}

void setup() {

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(LEFT, INPUT);
  pinMode(CENTER, INPUT);
  pinMode(RIGHT, INPUT);

  lcd.init();
  lcd.backlight();

  IrReceiver.begin(IR_PIN);

  loadAllVariables();

  printToLCD("Robot Ready");
}

void loop() {

  if(IrReceiver.decode()){
    uint32_t code=IrReceiver.decodedIRData.decodedRawData;
    IrReceiver.resume();
    handleInput(code);
  }

  // туда-обратно
  if(roundTripMode){
    executeRoundTrip();
    roundTripMode = false;
    return;
  }

  if(state!=0){
    stopMotors();
    return;
  }

  if(manualMode && state==0){
    handleManualControl(IrReceiver.decodedIRData.decodedRawData);
    return;
  }
  int L = digitalRead(LEFT);
  int C = digitalRead(CENTER);
  int R = digitalRead(RIGHT);

  if (C == 1) {
    setForward(speedForward, speedForward);
  }
  else if (L == 1) {

    setForward(IN_TurnSpeed, OUT_TurnSpeed);
    lastDirection = -1;
  }
  else if (R == 1) {

    setForward(OUT_TurnSpeed, IN_TurnSpeed);
    lastDirection = 1;
  }

  else {

    stopMotors();

    setBackward(SpotBackSpeed,SpotBackSpeed); 
    delay(SpotBackDuration); 

    stopMotors();

    if (lastDirection == -1) {
      turnLeftOnSpot(SpotTurnSpeed);
    }
    else if (lastDirection == 1) {
      turnRightOnSpot(SpotTurnSpeed);
    }

    delay(SpotTurnDuration);
  }
}