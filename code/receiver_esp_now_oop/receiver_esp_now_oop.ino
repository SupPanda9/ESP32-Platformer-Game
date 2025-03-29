#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <JPEGDecoder.h>
#include <TFT_eSPI.h>
#include "player.h"
#include "background.h"
#include "CSVreader.h"
//#include "star.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2
#define NEUTRAL 'N'
#define PLAYER_WIDTH 17
#define PLAYER_HEIGHT 28
//#define NUMBER_OF_STARS 20


// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  char button[2];
  char x[2];
  char y[2];
} struct_message;

// Forward declaration of the callback function
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

class EspNowReceiver {
public:
  struct_message myData;
  TFT_eSPI tft;
  TFT_eSprite render_ready;
  //TFT_eSprite CSV;
  Player *player;
  Background *background;
  std::vector<std::vector<int>> mapData;
  TFT_eSprite heartSprite;
  //Star *stars[NUMBER_OF_STARS];

  EspNowReceiver() : tft(TFT_eSPI()), render_ready(&tft), heartSprite(&tft) {
    /*for (int i = 0; i < NUMBER_OF_STARS; ++i) {
      stars[i] = new Star(&tft);
    }*/
  }

  void init() {
    // Initialize Serial Monitor
    Serial.begin(115200);

    digitalWrite(22, HIGH); // Touch controller chip select (if used)
    digitalWrite(15, HIGH); // TFT screen chip select
    digitalWrite(5, HIGH); 

    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_WHITE);

    render_ready.createSprite(320,240);
    render_ready.fillSprite(TFT_WHITE);

    heartSprite.createSprite(20,20);
    heartSprite.fillSprite(TFT_WHITE);
    heartSprite.setSwapBytes(true);

    //CSV.createSprite(1280,SCREEN_HEIGHT);
    //CSV.fillSprite(TFT_WHITE);

    if (!SD.begin(5, tft.getSPIinstance())) {
    Serial.println("Card Mount Failed");
    return;
    }
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    Serial.print(F("Receiver initialized : "));
    Serial.println(WiFi.macAddress());

    // Define receive function
    esp_now_register_recv_cb(OnDataRecv);

    background = new Background(&tft, "/whole_level.jpg");

    player = new Player(&tft, SCREEN_WIDTH / 2 - PLAYER_WIDTH/2, SCREEN_HEIGHT / 2 - PLAYER_WIDTH/2, PLAYER_WIDTH, PLAYER_HEIGHT);
    if (!player->loadImage("/player.jpg")) {
      Serial.println("Failed to load player image");
    }

    mapData = readCSV("/platforms.csv");
    //visualizeMap();

    if (!loadHeartImage("/heart.jpg")) {
      Serial.println("Failed to load heart image");
    }

    /*spawnStars();*/
  }

  void updatePlayerMovement() {
    /*if (player) {
      player->move(myData.x[0], myData.y[0]);
      player->draw(&render_ready);
    }*/
    if (player) {
      char xDir = myData.x[0];

      if (!player->getIsMovingLeft() && xDir == 'L') {
        player->mirrorX();
        player->setIsMovingLeft(true);
      }

      if (player->getIsMovingLeft() && xDir == 'R') {
        player->mirrorX();
        player->setIsMovingLeft(false);
      }

      char yDir = myData.y[0];
      char action = myData.button[0];

      // If the background is at the far left edge and the player is within the left movement zone, allow the player to move
      if (background->getX() == 0 && player->getX() <= SCREEN_WIDTH / 2 - PLAYER_WIDTH/2) {
        player->move(xDir, yDir, action);
      }
      
      // If the background is at the far right edge and the player is within the right movement zone, allow the player to move
      else if (background->getX() == 1280 - SCREEN_WIDTH && player->getX() >= SCREEN_WIDTH / 2 - PLAYER_WIDTH/2) {
        player->move(xDir, yDir, action);
      }
      
      // Otherwise, move the background
      else {
        background->move(xDir);
        player->move('N', yDir, action);
        if (player->getX() != SCREEN_WIDTH / 2 - PLAYER_WIDTH/2) 
          player->setX(SCREEN_WIDTH / 2 - PLAYER_WIDTH/2);
      }

      background->draw(&render_ready);
      player->draw(&render_ready);
    }
  }

  void updatePhysics() {
    if (player) {
      player->applyPhysics(mapData, background);

      // Check for collisions with water
      int tileSize = 16;
      int mapOffsetX = background->getX();
      int playerY1 = player->getY() / tileSize + 1;
      int playerY2 = playerY1 + 2;
      Serial.println(playerY1);
      Serial.println(playerY2);

      //in the end areas
      if ((background->getX() == 0 && player->getX() <= SCREEN_WIDTH / 2 - PLAYER_WIDTH/2) || (background->getX() == 1280 - SCREEN_WIDTH && player->getX() >= SCREEN_WIDTH / 2 - PLAYER_WIDTH/2)) {
        int playerX1 = (player->getX() + mapOffsetX) / tileSize;
        int playerX2 = (player->getX() + mapOffsetX + PLAYER_WIDTH - 1) / tileSize;

        for (int mapX = playerX1; mapX <= playerX2; ++mapX) {
          for (int mapY = playerY1; mapY <= playerY2; ++mapY) {
            if (mapY >= 0 && mapY < mapData.size() && mapX >= 0 && mapX < mapData[0].size()) {
              int tile = mapData[mapY][mapX];
              Serial.println("Tile:");Serial.println(tile);
              if (tile == 2) {
                if (!player->getInWaterCooldown()) {
                  player->loseLife();
                  player->setInWaterCooldown(true);
                  player->waterCooldownStartTime = millis();
                  //Serial.println("Lost life!");
                }
                else {
                  unsigned long currentTime = millis();
                  if (currentTime - player->waterCooldownStartTime >= player->waterCooldownDuration) {
                    player->setInWaterCooldown(false);
                  }
                }
                //player->setY(SCREEN_HEIGHT / 2 - PLAYER_HEIGHT / 2); // Reset player position
              }
            }
          }
        }
      }
      else {
        int playerX1 = (background->getX() + SCREEN_WIDTH/2 - PLAYER_WIDTH/2) / tileSize;
        int playerX2 = (background->getX() + SCREEN_WIDTH/2 - PLAYER_WIDTH/2 - 1) / tileSize;

        for (int mapX = playerX1; mapX <= playerX2; ++mapX) {
          for (int mapY = playerY1; mapY <= playerY2; ++mapY) {
            if (mapY >= 0 && mapY < mapData.size() && mapX >= 0 && mapX < mapData[0].size()) {
              int tile = mapData[mapY][mapX];
              Serial.println("Tile:");Serial.println(tile);
              if (tile == 2) {
                if (!player->getInWaterCooldown()) {
                  player->loseLife();
                  player->setInWaterCooldown(true);
                  player->waterCooldownStartTime = millis();
                  //Serial.println("Lost life!");
                }
                else {
                  unsigned long currentTime = millis();
                  if (currentTime - player->waterCooldownStartTime >= player->waterCooldownDuration) {
                    player->setInWaterCooldown(false);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  bool loadHeartImage(const char* filename) {
      File jpegFile = SD.open(filename, FILE_READ);
      if (!jpegFile) {
        Serial.print("ERROR: File \""); Serial.print(filename); Serial.println("\" not found!");
        return false;
      }

      bool decoded = JpegDec.decodeSdFile(jpegFile);
      if (!decoded) {
        Serial.println("Jpeg file format not supported!");
        return false;
      }

      renderHeartImage();
      return true;
  }

  void renderHeartImage() {
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    while (JpegDec.read()) {
      pImg = JpegDec.pImage;
      int mcu_x = JpegDec.MCUx * mcu_w;
      int mcu_y = JpegDec.MCUy * mcu_h;

      heartSprite.pushImage(mcu_x, mcu_y, mcu_w, mcu_h, pImg);
    }
  }

  void render() {
    render_ready.fillSprite(TFT_WHITE);
    background->draw(&render_ready);
    //CSV.pushToSprite(&render_ready, -background->getX(), 0, TFT_BLACK);
    player->draw(&render_ready);
    for (int i = 0; i < player->getLives(); ++i) {
      heartSprite.pushToSprite(&render_ready, 10 + i * (heartSprite.width() + 2), 10, TFT_BLACK);
    }
    /*for (int i = 0; i < 20; ++i) {
      stars[i]->draw(&render_ready);
    }*/
    render_ready.pushSprite(0, 0);
  }

  /*void spawnStars() {
    for (int i = 0; i < 20; ++i) {
      stars[i]->spawn(mapData, background); // Spawn stars in valid map locations
    }
  }*/

  /*void checkStarCollisions(Player& player) {
    for (int i = 0; i < 20; ++i) {
      if (stars[i].checkCollision(player)) {
        stars[i].collect(player); // Collect the star and respawn it
      }
    }
  }*/

  /*void visualizeMap() {
    int tileSize = 16;
    CSV.fillScreen(TFT_WHITE);
    for (int y = 0; y < mapData.size(); y++) {
      for (int x = 0; x < mapData[y].size(); x++) {
        if (mapData[y][x] != -1 && mapData[y][x] != 0 && mapData[y][x] != 1) {
          CSV.fillRect(x * tileSize, y * tileSize, tileSize, tileSize, TFT_RED);
        }
      }
    }
  }*/
};

// Global instance of the receiver
EspNowReceiver receiver;

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&receiver.myData, incomingData, sizeof(receiver.myData));
  if (!(receiver.myData.x[0] == 'N' && receiver.myData.y[0] == 'N' && receiver.myData.button[0] == 'N')) {
    receiver.updatePlayerMovement();
  }
}

void setup() {
  receiver.init();
  receiver.render();
}

void loop() {
  receiver.updatePhysics();
  receiver.render();
  delay(25);
}
