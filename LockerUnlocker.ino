#include <BfButton.h>

#include <SoftwareSerial.h>
#include <PN532_SWHSU.h>
#include <PN532.h>

// NFC Stuff
SoftwareSerial SWSerial( 3, 2 ); // TX, RX
PN532_SWHSU pn532swhsu( SWSerial );
PN532 nfc( pn532swhsu );
String tagId = "None", dispTag = "None";
byte nuidPICC[4];

// Gear Stuff
int btnPin = 4; // Press to calibrate
int DT = 5; // Output B
int CLK = 6; // Output A

// Motor Stuff
const int in2 = 8;
const int in1 = 7;
const int enA = A1;

BfButton BTN(BfButton::STANDALONE_DIGITAL, btnPin, true, LOW);

float counter = 0;
float angle = 0;
float aState;
float aLastState;

float lockValue = 0;
bool currentlyOpening = false;
int lockStage = 0;

void pressHandler (BfButton *BTN, BfButton::press_pattern_t pattern) {
  switch (pattern) {
    case BfButton::SINGLE_PRESS:
      break;
    case BfButton::DOUBLE_PRESS:
      Serial.println("Resetting lock");
      lockStage = 0;
      break;
    case BfButton::LONG_PRESS:
      Serial.println("Setting current position as 0");
      counter = 0;
      angle = 0;
      lockValue = 0;
      break;
  }
}


void setup() {
  Serial.begin(9600);
  Serial.println("Code Starting: Welcome To Locker-Unlocker");
  nfc.begin();
  nfc.SAMConfig();

  pinMode(in2, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(enA, OUTPUT);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  aLastState = digitalRead(CLK);

  BTN.onPress(pressHandler)
  .onDoublePress(pressHandler)
  .onPressFor(pressHandler, 1000); // 1 Second holding
}

void loop() {
  if (currentlyOpening) {
    enterPasskey(10,36,6);
  }
  else {
    readNFC();
  }
  BTN.read();
}

void readNFC()
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success)
  {
    Serial.println("NFC FOUND");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      nuidPICC[i] = uid[i];
    }
    tagId = tagToString(nuidPICC);
    delay(1000);  // 1 second halt
    //Serial.println(tagId);
    if (tagId == tagId) { //Replace second tagId with list of valid card UIDs
        currentlyOpening = true;
    }  
  }
}

String tagToString(byte id[4])
{
  String tagId = "";
  for (byte i = 0; i < 4; i++)
  {
    if (i < 3) tagId += String(id[i]) + ".";
    else tagId += String(id[i]);
  }
  return tagId;
}

void checkEncoder() {
  // Rotary Encoder Reader
  aState = digitalRead(CLK);

  if (aState != aLastState) {
    if (digitalRead(DT) != aState) {
      counter --;
      angle --;
    }
    else {
      counter ++;
      angle ++;
    }
    if (counter < 0) {
      counter = 89;
    }
    if (counter >= 90) {
      counter = 0;
    }
    lockValue = (counter / 90.0) * 40.0;
    Serial.println(lockValue);
    //delay(10);
  }
  aLastState = aState;
}

float margin = 1;
void enterPasskey(float first, float second, float third) {
  checkEncoder();
  if (lockStage == 0) { // TODO Make lock rotate 3 times to reset lock
    int lockResetCounter = 0;
    
    lockStage = 1;
    editMotor(255, "CCW"); //CW = Clockwise direction
  }
  if (lockStage == 1) {
    if (lockValue < first-margin or lockValue > first+margin) {
      ;
    }
    else {
      editMotor(0, "CW");
      delay(5000);
      editMotor(255, "CW"); //CCW = Counter-Clockwise direction
      lockStage = 2;
    }
  }
  if (lockStage == 2) {
    if (lockValue < second-margin or lockValue > second+margin) {
      ;
    }
    else {
      editMotor(0, "CCW");
      delay(5000);
      editMotor(255, "CCW"); //CW = Clockwise direction
      lockStage = 3;
      
    }
  }
  if (lockStage == 3) {
    if (lockValue < third-margin or lockValue > third+margin) {
      ;
    }
    else {
      editMotor(0, "CW");
      lockStage = 4;
    }
  }
  if (lockStage == 4) {
    currentlyOpening = false;
    lockStage = 0;
    // Add code to push lock open
  }
}

void editMotor(int motorSpeed, String motorDirection) {
  analogWrite(enA, motorSpeed);
  if (motorDirection == "CW") {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }
  else if (motorDirection == "CCW") {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }
}
