#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "utils.h"
#include "shader.h"

using namespace std;

averageSec::averageSec()
    : doubleDequer_(0.0)
{
}

averageSec::~averageSec()
{
}

void averageSec::addSec(double s)
{
  doubleDequer_.push_back(s);
  if (doubleDequer_.size() > 60) {
    doubleDequer_.pop_front();
  }
}

double averageSec::calcAverage()
{
  double sumSec = 0.0;
  for (double s : doubleDequer_) {
    sumSec += s;
  }
  return sumSec / doubleDequer_.size();
}

////

bool save_screenshot(string filename, int w, int h)
{
  //This prevents the images getting padded
  // when the width multiplied by 3 is not a multiple of 4
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  int nSize = w * h * 3;
  // First let's create our buffer, 3 channels per Pixel
  char *dataBuffer = (char *) malloc(nSize * sizeof(char));
  if (!dataBuffer) {
    return false;
  }
  // Let's fetch them from the backbuffer
  // We request the pixels in GL_BGR format, thanks to Berzeger for the tip
  glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, dataBuffer);
  //Now the file creation
  FILE *filePtr = fopen(filename.c_str(), "wb");
  if (!filePtr) {
    return false;
  }
  unsigned char TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned char header[6] = {
      static_cast<u8>(w % 256), static_cast<u8>(w / 256),
      static_cast<u8>(h % 256), static_cast<u8>(h / 256),
      static_cast<u8>(24), static_cast<u8>(0)
  };
  // We write the headers
  fwrite(TGAheader, sizeof(unsigned char), 12, filePtr);
  fwrite(header, sizeof(unsigned char), 6, filePtr);
  // And finally our image data
  fwrite(dataBuffer, sizeof(GLubyte), nSize, filePtr);
  fclose(filePtr);
  free(dataBuffer);
  return true;
}

void showTexture(u32 windowW, u32 windowH, GLuint textureId)
{
  GLuint programId = createProgram("show_texture.vert", "show_texture.frag");
  glUseProgram(programId);

  glDisable(GL_DEPTH_TEST);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glActiveTexture(GL_TEXTURE0);

  auto projection = glm::ortho(0.0f, static_cast<float>(windowW), static_cast<float>(windowH),
                               0.0f, 0.0f, 100.0f);
//    auto projection = glm::ortho(0.0f, static_cast<float>(1024), static_cast<float>(1024),
//                                       0.0f, 0.0f, 100.0f);

  glUseProgram(programId);
  GLint projectionId = glGetUniformLocation(programId, "projection");
  assert(projectionId >= 0);
  glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projection));

  vector<GLfloat> triVec;
  vector<GLfloat> texVec;

  float h = static_cast<float>(windowH);
  float w = static_cast<float>(windowW);

  triVec.insert(triVec.end(), {0.0f, 0.0f, 0.0f});
  triVec.insert(triVec.end(), {0.0f, h, 0.0f});
  triVec.insert(triVec.end(), {w, h, 0.0f});

  triVec.insert(triVec.end(), {0.0f, 0.0f, 0.0f});
  triVec.insert(triVec.end(), {w, h, 0.0f});
  triVec.insert(triVec.end(), {w, 0.0f, 0.0f});

  texVec.insert(texVec.end(), {0, 0});
  texVec.insert(texVec.end(), {0, 1});
  texVec.insert(texVec.end(), {1, 1});

  texVec.insert(texVec.end(), {0, 0});
  texVec.insert(texVec.end(), {1, 1});
  texVec.insert(texVec.end(), {1, 0});

  glEnableVertexAttribArray(0);

  GLuint vertexBufId;
  glGenBuffers(1, &vertexBufId);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_STATIC_DRAW);
  glVertexAttribPointer(
      0,         // attribute
      3,         // size
      GL_FLOAT,  // type
      GL_FALSE,  // normalized?
      0,         // stride
      (void *) 0   // array buffer offset
  );

  glEnableVertexAttribArray(1);

  GLuint texBufId;
  glGenBuffers(1, &texBufId);
  glBindBuffer(GL_ARRAY_BUFFER, texBufId);
  glBufferData(GL_ARRAY_BUFFER, texVec.size() * sizeof(GLfloat), &texVec[0], GL_STATIC_DRAW);

  glVertexAttribPointer(
      1,         // attribute
      2,         // size
      GL_FLOAT,  // type
      GL_TRUE,  // normalized?
      0,         // stride
      (void *) 0   // array buffer offset
  );

  glDrawArrays(GL_TRIANGLES, 0, triVec.size());

  glDeleteBuffers(1, &vertexBufId);
  glDeleteBuffers(1, &texBufId);
}

vector<u8> makeTestTextureVector(u32 w, u32 h, u32 border)
{
  // Make a test texture.
  // Red upper left corner, green border, blue center.
  vector<u8> v(w * h * 4);
  for (u32 y = 0; y < h; ++y) {
    for (u32 x = 0; x < w; ++x) {
      u8 r, g, b;
      if (x < border && y < border) {
        r = 255;
        g = 0;
        b = 0;
      } else if (x < border || x > w - border || y < border || y > h - border) {
        r = 0, g = 255;
        b = 0;
      } else {
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
