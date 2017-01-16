#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glew.h>

struct CharMeta
{
  int tex_x;
  int tex_y;
  int tex_w;
  int tex_h;
  int top_offset;
  int left_offset;
  int advance;
};

class OglText
{
  public:
  OglText(const std::string& fontPath, int fontH);
  ~OglText();
  void openGLInit();
  void openGLFree();
  void setFontH(int fontH);
  void print(
      glm::tmat4x4<float>& projMat, int x, int y, int nLine,
      const std::string& str, bool drawBackground = true);
  int calcStringWidth(const std::string& str);
  int getLineHeight();

  private:
  void createFreeType(const std::string& fontPath);
  void createFontTexture();
  bool renderFont(std::vector<unsigned char>& fontVec);
  void drawText(int x, int y, int nLine, const std::string& str);
  void drawTextBackground(int x, int y, int nLine, const std::string& str);

  bool oglInitialized_;

  int texWH_;
  int fontH_;

  GLuint textProgramId_;
  GLuint textBackgroundProgramId;

  FT_Library freetypeLibraryHandle_;
  FT_Face face_;

  GLuint textureId_;
  GLuint vertexBufId_;
  GLuint texBufId_;

  int lineH_;
  int maxAscender_;

  std::vector<CharMeta> charMeta_;
};
