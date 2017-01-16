#pragma once

#include <algorithm>
#include <deque>
#include <string>

#if defined(_WIN32)
#include <Windows.h>
#else
#include <sys/file.h>
#include <sys/stat.h>
#endif

#include <nanogui/opengl.h>

// OpenGL
bool saveScreenshot(std::string filename, int w, int h);
void showTexture(int windowW, int windowH, GLuint textureId);
std::vector<unsigned char> makeTestTextureVector(int w, int h, int border);

// File
double getMtime(const std::string& path);
std::string joinPath(const std::string& a, const std::string& b);

// File locking
#ifndef _WIN32
typedef int FileHandle;
#else
typedef HANDLE FileHandle;
#endif

class ExclusiveFileLock
{
  public:
  ExclusiveFileLock(const std::string filePath);
  ~ExclusiveFileLock();
  void release();

  private:
  bool isLocked;
  FileHandle fileHandle_;
};

// Keep track of recent values in order to calculate average.
class TrackAverage
{
  public:
  TrackAverage(int maxSize);
  ~TrackAverage();
  void addValue(double);
  double calcAverage();

  private:
  int maxSize_;
  std::deque<double> doubleDeque_;
};

// Misc
template <class T>
const T& clamp(const T& v, const T& minValue, const T& maxValue)
{
  return std::min(maxValue, std::max(minValue, v));
}

std::string trim(const std::string& s);
