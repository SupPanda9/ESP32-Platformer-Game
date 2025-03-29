#pragma once
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <JPEGDecoder.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MOVE_SPEED 8

class Background {
public:
  int x;
  TFT_eSprite background;

  Background(TFT_eSPI* tft, const char* filename)
    : x(0), background(tft){
    background.createSprite(1280, 240);  // Assuming the background image is 1280x240
    background.setSwapBytes(true);
    loadImage(filename);
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
    uint16_t* pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;

    while (JpegDec.read()) {
      pImg = JpegDec.pImage;
      int mcu_x = JpegDec.MCUx * mcu_w;
      int mcu_y = JpegDec.MCUy * mcu_h;

      background.pushImage(mcu_x, mcu_y, mcu_w, mcu_h, pImg);
    }
  }

  void draw(TFT_eSprite* sprite) {
    background.pushToSprite(sprite,-x, 0);
  }

  int getX(){
    return x;
  }

  void setX(int newX){
    x = newX;
  }

  void move(char dir) {
    if (dir == 'L' && x > 0) {
      x -= MOVE_SPEED;
    } else if (dir == 'R' && x < (1280 - SCREEN_WIDTH)) {
      x += MOVE_SPEED;
    }
  }
};
