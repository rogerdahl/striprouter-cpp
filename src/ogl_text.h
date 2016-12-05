#pragma once

#include <string>
#include <vector>

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
  OglText(const std::string &fontPath, int fontH, int x = 0, int y = 0);
  ~OglText();
  void openGLInit();
  void reset(int windowW, int windowH, int x, int y, int fontH);
  void print(int nLine, const std::string &str);
  int calcStringWidth(const std::string &str);
  int getStringHeight();
private:
  void createFreeType(const std::string &fontPath);
  void createFontTexture();
  bool renderFont(std::vector<unsigned char> &fontVec);
  void drawText(int nLine, const std::string &str);
  void drawTextBackground(int nLine, const std::string &str);

  bool oglInitialized_;

  int texWH_;
  int fontH_;

  int x_;
  int y_;

  GLuint textProgramId_;
  GLuint textBackgroundProgramId;

  int windowW_;
  int windowH_;

  FT_Library freetypeLibraryHandle_;
  FT_Face face_;

  GLuint textureId_;
  GLuint vertexBufId_;
  GLuint texBufId_;

  int lineH_;
  int maxAscender_;

  std::vector<CharMeta> charMeta_;
};
