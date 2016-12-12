#pragma once

#include <deque>
#include <string>

#include <GL/glew.h>


// OpenGL
bool saveScreenshot(std::string filename, int w, int h);
void showTexture(int windowW, int windowH, GLuint textureId);
std::vector<unsigned char> makeTestTextureVector(int w, int h, int border);

// File
double getMtime(const std::string& path);
std::string joinPath(const std::string& a, const std::string& b);
int getExclusiveLock(const std::string filePath);
void releaseExclusiveLock(int fd);

// Keep track of recent rendering times in order to display average
// frames per second.
class averageSec
{
public:
  averageSec();
  ~averageSec();
  void addSec(double);
  double calcAverage();
private:
  std::deque<double> doubleDeque_;
};

