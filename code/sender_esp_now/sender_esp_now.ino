#include <esp_now.h>
#include <WiFi.h>

// Define pins
#define A 33
#define B 25
#define C 26
#define D 27
#define JOYCON_X 32
#define JOYCON_Y 35
#define X_THRESHOLD 4000
#define Y_THRESHOLD 4000
#define NEUTRAL_THRESHOLD 1400
#define DEBOUNCE_TIME 16

volatile bool buttonAPressed = false;
volatile bool buttonBPressed = false;
volatile bool buttonCPressed = false;
volatile bool buttonDPressed = false;

// Button press handlers with debounce
void handleAPress() {
  static unsigned long lastAInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastAInterruptTime > DEBOUNCE_TIME) {
    buttonAPressed = true;
  }
  lastAInterruptTime = interruptTime;
}

void handleBPress() {
  static unsigned long lastBInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastBInterruptTime > DEBOUNCE_TIME) {
    buttonBPressed = true;
  }
  lastBInterruptTime = interruptTime;
}

void handleCPress() {
  static unsigned long lastCInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastCInterruptTime > DEBOUNCE_TIME) {
    buttonCPressed = true;
  }
  lastCInterruptTime = interruptTime;
}

void handleDPress() {
  static unsigned long lastDInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastDInterruptTime > DEBOUNCE_TIME) {
    buttonDPressed = true;
  }
  lastDInterruptTime = interruptTime;
}

uint8_t broadcastAddress[] = {0x10, 0x97, 0xBD, 0xE5, 0xA9, 0x38};

// Structure to send data
typedef struct struct_message {
  char button[2];
  char x[2];
  char y[2];
} struct_message;
struct_message myData;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print(F("\r\n Master packet sent:\t"));
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
  
  // Set up button pins
  pinMode(A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(A), handleAPress, FALLING);
  
  pinMode(B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(B), handleBPress, FALLING);
  
  pinMode(C, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(C), handleCPress, FALLING);
  
  pinMode(D, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(D), handleDPress, FALLING);

  // Set up joystick pins
  pinMode(JOYCON_X, INPUT);
  pinMode(JOYCON_Y, INPUT);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println(F("Error initializing ESP-NOW"));
    return;
  }
  Serial.print(F("Transmitter initialized : "));
  Serial.println(WiFi.macAddress());
  
  // Define Send function
  esp_now_register_send_cb(OnDataSent);

  delay(2000);
  
  // Register peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println(F("Failed to add peer"));
    return;
  }
}

void loop() {
  esp_err_t result;
  strcpy(myData.button, "N");
  strcpy(myData.x, "N");
  strcpy(myData.y, "N");

  if (buttonAPressed) {
    Serial.println("A");
    strcpy(myData.button, "A");
    buttonAPressed = false; // Reset the flag
  }
  else if (buttonBPressed) {
    Serial.println("B");
    strcpy(myData.button, "B");
    buttonBPressed = false; // Reset the flag
  }
  else if (buttonCPressed) {
    Serial.println("C");
    strcpy(myData.button, "C");
    buttonCPressed = false; // Reset the flag
  }
  else if (buttonDPressed) {
    Serial.println("D");
    strcpy(myData.button, "D");
    buttonDPressed = false; // Reset the flag
  }

  // Read joystick values
  int joyX = analogRead(JOYCON_X);
  int joyY = analogRead(JOYCON_Y);

  Serial.print("Joystick X: ");
  Serial.println(joyX);
  Serial.print("Joystick Y: ");
  Serial.println(joyY);

  if (joyX > X_THRESHOLD) {
    Serial.println("RIGHT");
    strcpy(myData.x, "R");
  }
  else if (joyX < NEUTRAL_THRESHOLD) {
    Serial.println("LEFT");
    strcpy(myData.x, "L");
  }

  if (joyY > Y_THRESHOLD) {
    Serial.println("UP");
    strcpy(myData.y, "UP");
  } else if (joyY < NEUTRAL_THRESHOLD) {
    Serial.println("DOWN");
    strcpy(myData.y, "D");
  }
  
  if (joyY < Y_THRESHOLD && joyY > NEUTRAL_THRESHOLD && joyX < X_THRESHOLD && joyX > NEUTRAL_THRESHOLD){
    Serial.println("Neutral");
    Serial.println("Neutral");
    strcpy(myData.x, "N");
    strcpy(myData.y, "N");
  }

  result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println(F("Sent with success"));
  }
  else {
    Serial.println(F("Error sending the data"));
  }
  delay(100);
}

