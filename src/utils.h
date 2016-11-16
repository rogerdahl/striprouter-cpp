#pragma once

#include <deque>
#include <string>

#include "int_types.h"

bool save_screenshot(std::string filename, int w, int h);
//double average(std::deque<double>& d);
void showTexture(u32 windowW, u32 windowH, GLuint textureId);
std::vector<u8> makeTestTextureVector(u32 w, u32 h, u32 border);

class averageSec {
public:
  averageSec();
  ~averageSec();
  void addSec(double);
  double calcAverage();
private:
  std::deque<double> doubleDequer_;
};
