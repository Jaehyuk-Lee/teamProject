#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define STATE_PENDING 0
#define STATE_INPUTWORD 1

#define ButtonA_Pin 21
#define ButtonB_Pin 22
#define ButtonC_Pin 23
#define ButtonD_Pin 24


#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

int nfc_list[6][4] = { 
  { 183, 8, 198, 95 },    // B7 08 C6 5F
  { 140, 236, 227, 46 },  // 8C EC E3 2E
  { 103, 9, 148, 95 },    // 67 09 94 5F
  { 119, 221, 212, 96 },  // 77 DD D4 60
  { 215, 165, 161, 96 },  // D7 A5 A1 60
  { 135, 2, 134, 95 },    // 87 02 86 5F
};
char nfc_words[6][10] = {
  "cat",
  "cow",
  "dog",
  "frog"
  "rabbit",
  "lion",
};

int state = STATE_PENDING;
int nfcNumber;
char inputWord[10];
int inputLocation;


void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10); // for Leonardo/Micro/Zero

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0 };
  uint8_t uidLength;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (state == STATE_PENDING && success) {

    nfcNumber = getNFCNumber(uid);
    Serial.print("NFC Number: "); Serial.println(nfcNumber);
    state = STATE_INPUTWORD;
    resetInputState();
  }
  if (state == STATE_INPUTWORD) {
    char input = Serial.read();
    if (input >= 97 && input <= 122){ // a~z 사이 글자만 입력 받음
      inputWord[inputLocation] = input;
      inputLocation++;
      Serial.println(input);
    }
    if (input == '0'){ // 임시로 0 입력하면 입력 완료 버튼 누른걸로 취급
      inputWord[inputLocation] = '\0'; 
      Serial.println(inputWord);
      if(checkAnswer(inputWord))
        Serial.println("Wrong Answer");
      else
        Serial.println("Right Answer");
      state = STATE_PENDING;
    }
  }
}

int getNFCNumber(uint8_t* id){
  int nfc_number = -1;
  for (int i=0; i<6; i++){
    for (int j=0; j<4;j++){
      if (id[j] != nfc_list[i][j]) {
        nfc_number = -1;
        break;
      }
      else
        nfc_number = i;
    }
    if(nfc_number != -1)
      break;
  }
  return nfc_number;
}

void resetInputState() {
  memset(inputWord, 0, sizeof(inputWord));
  inputLocation = 0;
}

int checkAnswer(char* input) {
  if(strlen(input) != strlen(nfc_words[nfcNumber])) // 글자 수 틀렸나 체크
    return 1;
  for(int i=0; i<sizeof(nfc_words[nfcNumber]); i++){ // 단어 틀렸나 체크
    if(input[i] != nfc_words[nfcNumber][i])
      return 1;
  }
  return 0;
}
