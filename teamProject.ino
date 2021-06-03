#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define STATE_PENDING 0
#define STATE_INPUTWORD 1

#define KEYUP 0
#define KEYDOWN 1

#define PN532_IRQ   2 // NFC IRQ 핀
#define PN532_RESET 3 // NFC RESET 핀 (아무 글자 없는 핀)

int buzzerPin = 20; // 피에조 부저 핀
int buttonPin[26] = { 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
  31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 
  41, 42, 43, 44, 45, 46 }; // 버튼핀 A부터 Z까지 26개 - 21번핀~46번핀
int enterPin = 47; // 엔터 핀

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

int nfcList[6][4] = {
  { 183, 8, 198, 95 },    // B7 08 C6 5F
  { 140, 236, 227, 46 },  // 8C EC E3 2E
  { 103, 9, 148, 95 },    // 67 09 94 5F
  { 119, 221, 212, 96 },  // 77 DD D4 60
  { 215, 165, 161, 96 },  // D7 A5 A1 60
  { 135, 2, 134, 95 },    // 87 02 86 5F
};
char nfcWords[6][10] = {
  "cat",
  "cow",
  "dog",
  "frog"
  "rabbit",
  "lion",
};

int nfc_state = STATE_PENDING;
int key_state = KEYUP;
int key_should_reset = 0;
int nfcNumber;
char inputWord[10];
int inputLocation;


void setup(void) {
  // nfc 초기화하는 부분
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

  // 버튼 입력모드로 설정
  for (int i = 0; i < sizeof(buttonPin)/2; i++)
    pinMode(buttonPin[i], INPUT);
  pinMode(enterPin, INPUT);
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0 };
  uint8_t uidLength;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (nfc_state == STATE_PENDING && success) {
    playNFCSound();
    nfcNumber = getNFCNumber(uid);
    Serial.print("NFC Number: "); Serial.println(nfcNumber);
    nfc_state = STATE_INPUTWORD;
    resetInputState();
  }
  if (nfc_state == STATE_INPUTWORD) {
    char inputChar = ' ';
    
    key_should_reset = 1; // 키 안누른 상태
    for (int i = 0; i < sizeof(buttonPin)/2 + 1; i++) { // +1 한 이유는 enterPin까지 한번에 처리하기 위함. 바로 다음핀이기 때문.
      if (digitalRead(buttonPin[0] + i) == HIGH) {
        key_should_reset = 0; // 키 누른 상태
        if (key_state == KEYDOWN) return;
        inputChar = 'a' + i;
        Serial.print("Entered ");Serial.println(inputChar);
        key_state = KEYDOWN;
      }
    }
    if (key_should_reset) { // 키 누른 상태면 아직 초기화 안함
      inputChar = ' ';
      // Serial.println("Entered NOTHING");
      key_state = KEYUP;
      return; // loop 함수 처음부터 다시 시작
    }

    // a~z 사이 글자가 입력된 경우
    if (inputChar >= 97 && inputChar <= 122) {
      inputWord[inputLocation] = inputChar;
      inputLocation++;
      Serial.println(inputChar);
      Serial.println(inputWord);
    }

    // 엔터키가 입력된 경우
    if (inputChar == '{'){ // 엔터키는 '{'로 대응시켰음. 아스키코드로 'z' 다음 문자가 '{'임.
      inputWord[inputLocation] = '\0';
      Serial.println(inputWord);
      if (checkAnswer(inputWord))
        Serial.println("Wrong Answer");
      else
        Serial.println("Right Answer");
      nfc_state = STATE_PENDING;
    }
    delay(200); // 부하를 줄이기 위해 0.2초 지연 추가
  }
}

// 몇번째 NFC인지 확인
int getNFCNumber(uint8_t* id){
  int nfc_number = -1; // 목록에 없는 경우에는 "-1"을 반환함
  for (int i=0; i<6; i++){
    for (int j=0; j<4;j++){
      if (id[j] != nfcList[i][j]) {
        nfc_number = -1;
        break;
      }
      else
        nfc_number = i;
    }
    if (nfc_number != -1)
      break;
  }
  return nfc_number;
}

// 입력 상태 초기화
void resetInputState() {
  memset(inputWord, 0, sizeof(inputWord));
  inputLocation = 0;
}

// 정답이 맞는지 확인
int checkAnswer(char* input) {
  if (strlen(input) != strlen(nfcWords[nfcNumber])) // 글자 수 틀렸나 체크
    return 1;
  for (int i=0; i<sizeof(nfcWords[nfcNumber]); i++){ // 단어 틀렸나 체크
    if (input[i] != nfcWords[nfcNumber][i])
      return 1;
  }
  return 0;
}

// 정답일 때 소리
void playCorrectSound() {
  tone(buzzerPin, 262, 500); // 도
  delay(200);
  tone(buzzerPin, 330, 500); // 미
  delay(200);
  tone(buzzerPin, 392, 500); // 솔
  delay(200);
  tone(buzzerPin, 523, 500); // 높은 도
}

// 오답일 때 소리
void playWrongSound() {
  tone(buzzerPin, 150, 400);
  delay(100);
  tone(buzzerPin, 150, 400);
}

// NFC 태그됐을 때 소리
void playNFCSound() {
  tone(buzzerPin, 294, 200); // 레
}