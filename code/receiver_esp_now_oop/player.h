#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <vector>
#include "background.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MOVE_SPEED 8
#define GRAVITY 1.5 // Adjust gravity
#define JUMP_FORCE -12 // Adjust jump force
#define MAX_FALL_SPEED 14

class Player {
public:
  int x, y, width, height;
  TFT_eSprite player;
  float yVelocity;
  bool onGround;
  int mapOffsetX;
  int lives;
  bool isMovingLeft;
  bool inWaterCooldown;
  unsigned long waterCooldownStartTime;
  const unsigned long waterCooldownDuration;

  Player(TFT_eSPI *tft, int x, int y, int width, int height)
    : x(x), y(y), width(width), height(height), player(tft),  yVelocity(0), onGround(false), mapOffsetX(0), lives(3), isMovingLeft(0), inWaterCooldown(false), waterCooldownStartTime(0), waterCooldownDuration(1000){
    player.createSprite(width, height);
    player.setSwapBytes(true);
  }

  bool loadImage(const char* filename) {
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

    renderImage();
    return true;
  }

  void renderImage() {
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    while (JpegDec.read()) {
      pImg = JpegDec.pImage;
      int mcu_x = JpegDec.MCUx * mcu_w;
      int mcu_y = JpegDec.MCUy * mcu_h;

      player.pushImage(mcu_x, mcu_y, mcu_w, mcu_h, pImg);
    }
  }

  void draw(TFT_eSprite *sprite) {
    player.pushToSprite(sprite, x, y, TFT_BLACK);
  }

  void move(char xDir, char yDir, char action) {
    if (xDir == 'L') x -= MOVE_SPEED;
    if (xDir == 'R') x += MOVE_SPEED;

    if (action == 'A' && onGround) 
      jump();
    //if (yDir == 'U') y -= 5;
    //if (yDir == 'D') y += 5;

    // Boundaries check
    if (x < 0) x = 0;
    if (x > SCREEN_WIDTH - width) x = SCREEN_WIDTH - width;
    /*if (y < 0) y = 0;
    if (y > SCREEN_HEIGHT - height) y = SCREEN_HEIGHT - height;*/
  }

  void applyPhysics(const std::vector<std::vector<int>>& mapData, Background *background) {
    yVelocity += GRAVITY;

    if (yVelocity > MAX_FALL_SPEED) {
      yVelocity = MAX_FALL_SPEED;
    }

    // Apply vertical movement
    y += yVelocity;
    resolveVerticalCollisions(mapData, background);

    // Apply horizontal movement
    resolveHorizontalCollisions(mapData, background);
  }

  void resolveVerticalCollisions(const std::vector<std::vector<int>>& mapData, Background *background) {
    int tileSize = 16;
    mapOffsetX = background->getX();
    int mapX1 = (x + mapOffsetX) / tileSize;
    int mapX2 = (x + mapOffsetX + width - 1) / tileSize;
    int mapY1 = y / tileSize;
    int mapY2 = (y + height - 1) / tileSize;

    onGround = false;

    for (int mapX = mapX1; mapX <= mapX2; ++mapX) {
      if (mapY2 >= 0 && mapY2 < mapData.size() && mapX >= 0 && mapX < mapData[0].size()) {
        int tile = mapData[mapY2][mapX];
        if (tile != -1) {
          y = mapY2 * tileSize - height;
          yVelocity = 0;
          onGround = true;
        }
      }
      if (mapY1 >= 0 && mapY1 < mapData.size() && mapX >= 0 && mapX < mapData[0].size()) {
        int tile = mapData[mapY1][mapX];
        if (tile != -1 && yVelocity < 0) {
          y = (mapY1 + 1) * tileSize;
          yVelocity = 0;
        }
      }
    }
  }

  void resolveHorizontalCollisions(const std::vector<std::vector<int>>& mapData, Background *background) {
    int tileSize = 16;
    mapOffsetX = background->getX();
    int playerX1 = (x + mapOffsetX) / tileSize;
    int playerX2 = (x + mapOffsetX + width - 1) / tileSize;
    int playerY1 = y / tileSize;
    int playerY2 = (y + height - 1) / tileSize;

    // Check left side
    for (int mapY = playerY1; mapY <= playerY2; ++mapY) {
        if (playerX1 >= 0 && playerX1 < mapData[0].size() && mapY >= 0 && mapY < mapData.size()) {
            int tile = mapData[mapY][playerX1];
            if (tile != -1) {
                if (background->getX() == 0 && x <= SCREEN_WIDTH / 2 - width / 2) {
                    x = (playerX1 + 1) * tileSize - mapOffsetX;
                } else {
                    background->move('R');
                    mapOffsetX = background->getX();
                }
                break;
            }
        }
    }

    // Recalculate the map coordinates after potential movement adjustment
    playerX1 = (x + mapOffsetX) / tileSize;
    playerX2 = (x + mapOffsetX + width - 1) / tileSize;

    // Check right side
    for (int mapY = playerY1; mapY <= playerY2; ++mapY) {
        if (playerX2 >= 0 && playerX2 < mapData[0].size() && mapY >= 0 && mapY < mapData.size()) {
            int tile = mapData[mapY][playerX2];
            if (tile != -1) {
                if (background->getX() == 1280 - SCREEN_WIDTH && x >= SCREEN_WIDTH / 2 - width / 2) {
                    x = playerX2 * tileSize - mapOffsetX - width;
                } else {
                    background->move('L');
                    mapOffsetX = background->getX();
                }
                break;
            }
        }
    }
  }

  void jump() {
    yVelocity = JUMP_FORCE;
    onGround = false;
  }

  int getX(){
    return x;
  }

  void setX(int newX) {
    x = newX;
  }

  int getY(){
    return y;
  }

  void loseLife() {
    lives--;
  }

  int getLives(){
    return lives;
  }

  bool getIsMovingLeft() {
    return isMovingLeft;
  }

  void setIsMovingLeft(bool isMovingLeft) {
    this->isMovingLeft = isMovingLeft;
  }

  bool getInWaterCooldown() {
    return inWaterCooldown;
  }

  void setInWaterCooldown(bool inWaterCooldown) {
    this->inWaterCooldown = inWaterCooldown;
  }

  bool checkCollision(Player &other) {
    return !(x + width < other.x || x > other.x + other.width ||
             y + height < other.y || y > other.y + other.height);
  }

  void mirrorX() {
    uint16_t color;
    for (int16_t i=0; i < width/2; i++) {
      for (int16_t j=0; j < height; j++) {
        color = player.readPixel(i, j);
        player.drawPixel(i, j, player.readPixel(width - 1 - i, j));
        player.drawPixel(width - 1 - i, j, color);
      }
    }
  }
};
