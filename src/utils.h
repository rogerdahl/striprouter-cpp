#pragma once

#include <deque>
#include <string>


bool save_screenshot(std::string filename, int w, int h);
//double average(std::deque<double>& d);
void showTexture(int windowW, int windowH, GLuint textureId);
std::vector<unsigned char> makeTestTextureVector(int w, int h, int border);

class averageSec
{
public:
  averageSec();
  ~averageSec();
  void addSec(double);
  double calcAverage();
private:
  std::deque<double> doubleDequer_;
};

