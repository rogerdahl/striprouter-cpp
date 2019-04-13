#include <algorithm>
#include <cctype>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "fmt/format.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "utils.h"

using namespace std::chrono_literals;

//
// OpenGL
//

bool saveScreenshot(std::string filename, int w, int h)
{
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  int nSize = w * h * 3;

  char* dataBuffer = (char*)malloc(nSize * sizeof(char));
  if (!dataBuffer) {
    return false;
  }

  glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, dataBuffer);

  FILE* filePtr = fopen(filename.c_str(), "wb");
  if (!filePtr) {
    return false;
  }
  unsigned char TGAheader[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  unsigned char header[6] = { static_cast<unsigned char>(w % 256),
                              static_cast<unsigned char>(w / 256),
                              static_cast<unsigned char>(h % 256),
                              static_cast<unsigned char>(h / 256),
                              static_cast<unsigned char>(24),
                              static_cast<unsigned char>(0) };

  fwrite(TGAheader, sizeof(unsigned char), 12, filePtr);
  fwrite(header, sizeof(unsigned char), 6, filePtr);

  fwrite(dataBuffer, sizeof(GLubyte), nSize, filePtr);
  fclose(filePtr);
  free(dataBuffer);
  return true;
}

void showTexture(int windowW, int windowH, GLuint textureId)
{
  GLuint programId = createProgram("show_texture.vert", "show_texture.frag");
  glUseProgram(programId);

  glDisable(GL_DEPTH_TEST);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glActiveTexture(GL_TEXTURE0);

  auto projection = glm::ortho(
      0.0f, static_cast<float>(windowW), static_cast<float>(windowH), 0.0f,
      0.0f, 100.0f);
  //    auto projection = glm::ortho(0.0f, static_cast<float>(1024),
  //    static_cast<float>(1024),
  //                                       0.0f, 0.0f, 100.0f);

  glUseProgram(programId);
  GLint projectionId = glGetUniformLocation(programId, "projection");
  assert(projectionId >= 0);
  glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projection));

  std::vector<GLfloat> triVec;
  std::vector<GLfloat> texVec;

  float h = static_cast<float>(windowH);
  float w = static_cast<float>(windowW);

  triVec.insert(triVec.end(), { 0.0f, 0.0f, 0.0f });
  triVec.insert(triVec.end(), { 0.0f, h, 0.0f });
  triVec.insert(triVec.end(), { w, h, 0.0f });

  triVec.insert(triVec.end(), { 0.0f, 0.0f, 0.0f });
  triVec.insert(triVec.end(), { w, h, 0.0f });
  triVec.insert(triVec.end(), { w, 0.0f, 0.0f });

  texVec.insert(texVec.end(), { 0, 0 });
  texVec.insert(texVec.end(), { 0, 1 });
  texVec.insert(texVec.end(), { 1, 1 });

  texVec.insert(texVec.end(), { 0, 0 });
  texVec.insert(texVec.end(), { 1, 1 });
  texVec.insert(texVec.end(), { 1, 0 });

  glEnableVertexAttribArray(0);

  GLuint vertexBufId;
  glGenBuffers(1, &vertexBufId);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId);
  glBufferData(
      GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0],
      GL_STATIC_DRAW);
  glVertexAttribPointer(
      0, // attribute
      3, // size
      GL_FLOAT, // type
      GL_FALSE, // normalized?
      0, // stride
      (void*)0 // array buffer offset
  );

  glEnableVertexAttribArray(1);

  GLuint texBufId;
  glGenBuffers(1, &texBufId);
  glBindBuffer(GL_ARRAY_BUFFER, texBufId);
  glBufferData(
      GL_ARRAY_BUFFER, texVec.size() * sizeof(GLfloat), &texVec[0],
      GL_STATIC_DRAW);

  glVertexAttribPointer(
      1, // attribute
      2, // size
      GL_FLOAT, // type
      GL_TRUE, // normalized?
      0, // stride
      (void*)0 // array buffer offset
  );

  glDrawArrays(GL_TRIANGLES, 0, triVec.size());

  glDeleteBuffers(1, &vertexBufId);
  glDeleteBuffers(1, &texBufId);
}

std::vector<unsigned char> makeTestTextureVector(int w, int h, int border)
{
  // Make a test texture.
  // Red upper left corner, green border, blue center.
  std::vector<unsigned char> v(w * h * 4);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      unsigned char r, g, b;
      if (x < border && y < border) {
        r = 255;
        g = 0;
        b = 0;
      }
      else if (x < border || x > w - border || y < border || y > h - border) {
        r = 0, g = 255;
        b = 0;
      }
      else {
        r = 0;
        g = 0;
        b = 255;
      }
      v[x * 4 + 0 + w * 4 * y] = r;
      v[x * 4 + 1 + w * 4 * y] = g;
      v[x * 4 + 2 + w * 4 * y] = b;
      v[x * 4 + 3 + w * 4 * y] = 255;
    }
  }
  return v;
}

//
// File
//

#if defined(_WIN32)

double getMtime(const std::string& path)
{
  HANDLE hFile = CreateFile(
      path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL, NULL);
  FILETIME lastWriteTime;
  GetFileTime(hFile, NULL, NULL, &lastWriteTime);
  double r = static_cast<double>(lastWriteTime.dwLowDateTime);
  CloseHandle(hFile);
  return r;
}

#else

double getMtime(const std::string& path)
{
  struct stat st = { 0 };
  int ret = lstat(path.c_str(), &st);
  if (ret == -1) {
    throw fmt::format("Unable to stat file: {}", path);
  }
  return st.st_mtim.tv_sec + st.st_mtim.tv_nsec / 1000000000.0;
}

#endif

std::string joinPath(const std::string& a, const std::string& b)
{
  if (a.back() == '/') {
    return a + b;
  }
  else {
    return a + std::string("/") + b;
  }
}

#ifdef _WIN32

// Create a string with last error message
std::string GetLastErrorStdStr()
{
  DWORD error = GetLastError();
  if (error) {
    LPVOID lpMsgBuf;
    DWORD bufLen = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    if (bufLen) {
      LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
      std::string result(lpMsgStr, lpMsgStr + bufLen);
      LocalFree(lpMsgBuf);
      return result;
    }
  }
  return std::string();
}

#endif

ExclusiveFileLock::ExclusiveFileLock(const std::string filePath)
{
  isLocked = false;
  while (true) {
#ifndef _WIN32
    fileHandle_ = open(filePath.c_str(), O_RDWR);
    auto result = flock(fileHandle_, LOCK_EX); // blocks
    if (!result) {
      isLocked = true;
      return;
    }
    auto errStr = fmt::format("{}", result);
#else
    fileHandle_ = CreateFile(
        filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0,
        NULL);
    if (fileHandle_ != INVALID_HANDLE_VALUE) {
      isLocked = true;
      return;
    }
    auto errStr = GetLastErrorStdStr();
#endif
    fmt::print("Retrying exclusive file lock. Error={}\n", errStr);
    std::this_thread::sleep_for(1s);
  }
}

ExclusiveFileLock::~ExclusiveFileLock()
{
  if (isLocked) {
    release();
  }
}

void ExclusiveFileLock::release()
{
  while (true) {
#ifndef _WIN32
    auto result = flock(fileHandle_, LOCK_UN);
    if (!result) {
      isLocked = false;
      return;
    }
    auto errStr = fmt::format("{}", result);
#else
    if (CloseHandle(fileHandle_)) {
      isLocked = false;
      return;
    }
    auto errStr = GetLastErrorStdStr();
#endif
    fmt::print("Retrying release of exclusive file lock. Error={}\n", errStr);
    std::this_thread::sleep_for(1s);
  }
}

//
// Average frames per second
//

TrackAverage::TrackAverage(int maxSize) : maxSize_(maxSize)
{
}

TrackAverage::~TrackAverage()
{
}

void TrackAverage::addValue(double s)
{
  doubleDeque_.push_back(s);
  if (static_cast<int>(doubleDeque_.size()) > maxSize_) {
    doubleDeque_.pop_front();
  }
}

double TrackAverage::calcAverage()
{
  double sum = 0.0;
  for (double v : doubleDeque_) {
    sum += v;
  }
  if (doubleDeque_.size()) {
    return sum / doubleDeque_.size();
  }
  else {
    return 0.0;
  }
}

//
// Misc
//

std::string trim(const std::string& s)
{
  auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c) {
    return std::isspace(c);
  });
  auto wsback = std::find_if_not(
                    s.rbegin(), s.rend(), [](int c) { return std::isspace(c); })
                    .base();
  return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
}
