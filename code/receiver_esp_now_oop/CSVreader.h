#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>

std::vector<std::vector<int>> readCSV(const char* filename) {
  std::vector<std::vector<int>> data;
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open CSV file");
    return data;
  }

  while (file.available()) {
    std::string line = file.readStringUntil('\n').c_str();
    std::vector<int> row;
    std::stringstream lineStream(line);
    std::string cell;

    while (std::getline(lineStream, cell, ',')) {
      row.push_back(std::stoi(cell));
    }
    data.push_back(row);
  }

  file.close();
  return data;
}