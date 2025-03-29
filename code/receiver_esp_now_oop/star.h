#pragma once

#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <JPEGDecoder.h>
#include "background.h"

#define STAR_SIZE 20
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

class Star {
public:
  int x, y;
  TFT_eSprite sprite;

  Star() : sprite(nullptr), x(-1), y(-1) {}

  // Constructor with TFT_eSPI parameter
  Star(TFT_eSPI *tft) : sprite(tft), x(-1), y(-1) {
    sprite.createSprite(STAR_SIZE, STAR_SIZE);
    sprite.setSwapBytes(true);
  }

  // Destructor to clean up sprite
  ~Star() {
    if (sprite.created()) {
      sprite.deleteSprite();
    }
  }

  // Delete copy constructor and copy assignment operator
  Star(const Star& other) = delete;
  Star& operator=(const Star& other) = delete;

  // Move constructor
  Star(Star&& other) noexcept : sprite(std::move(other.sprite)), x(other.x), y(other.y) {
    other.x = -1;
    other.y = -1;
  }

  // Move assignment operator
  Star& operator=(Star&& other) noexcept {
    if (this != &other) {
      if (sprite.created()) {
        sprite.deleteSprite();
      }
      sprite = std::move(other.sprite);
      x = other.x;
      y = other.y;
      other.x = -1;
      other.y = -1;
    }
    return *this;
  }

  void spawn(std::vector<std::vector<int>>& mapData, Background* background) {
    // Find a suitable position for the star on the map
    do {
      x = random(0, mapData[0].size());
      y = random(0, 11); //because this is the row where some more significant platform begins
    } while (mapData[y][x] != -1); // Repeat until a valid position is found

    // Convert map coordinates to screen coordinates
    int tileSize = 16;
    int mapOffsetX = background->getX();
    int screenX = x * tileSize - mapOffsetX;
    int screenY = y * tileSize;

    // Draw the star at the calculated position
    loadImage("/star.jpg");
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

      sprite.pushImage(mcu_x, mcu_y, mcu_w, mcu_h, pImg);
    }
  }

  void draw(TFT_eSprite *sprite) {
    this->sprite.pushToSprite(sprite, x, y, TFT_BLACK);
  }

  bool checkCollision(Player& player) {
    // Check collision between the player and the star
    return !(x + STAR_SIZE < player.x || x > player.x + player.width ||
             y + STAR_SIZE < player.y || y > player.y + player.height);
  }

  /*void collect(Player& player) {
    // When the player collects the star, respawn it in a new location
    spawn(mapData, background);
    player.gainXP(20);
  }*/
};