#if INCLUDE_LUT_SUPPORT

#include <fstream>
#include <sstream>
#include <string>

#include "LUT.h"

void lut::SoftLUT::initialize(const std::string& fileName) {
  // Load file with LUT constants
  std::ifstream lutFile(fileName);

  if (lutFile.is_open()) {
    std::string line;
    int lineNum = 0;

    // Read line by line
    while (std::getline(lutFile, line)) {
      std::istringstream iss(line);
      float num;
      char comma;
      // Every 3 lines is a new curve
      int curveSelect = lineNum / 3;

      // Num extracts a float from iss
      while (iss >> num) {
        // Decide whether num is a vMem, offset, or slope
        switch (lineNum % 3) {
          case 0:
            vMemss[curveSelect].push_back(num);
            break;
          case 1:
            offsetss[curveSelect].push_back(num);
            break;
          case 2:
            slopess[curveSelect].push_back(num);
            break;
        }

        // Consume a comma from iss before looping for next float
        iss >> comma;
      }
      lineNum++;
    }
  } else {
    throw std::runtime_error("could not open lutFile");
  }
}

double lut::SoftLUT::interpolate(double vMem, lut::CurveSelect curveSelect) {
  std::vector<float> vMems = vMemss[curveSelect];
  std::vector<float> slopes = slopess[curveSelect];
  std::vector<float> offsets = offsetss[curveSelect];

  // Find where vMem indexes in LUT
  int leftBound = 0;
  int rightBound = vMems.size() - 1;
  int checkIndex;
  while (leftBound < rightBound) {
    checkIndex = (rightBound + leftBound) / 2 + 1;
    if (vMem < vMems[checkIndex]) {
      rightBound = checkIndex - 1;
    } else {
      leftBound = checkIndex;
    }
  }

  return slopes[leftBound] * vMem + offsets[leftBound];
}
#endif //INCLUDE_LUT_SUPPORT