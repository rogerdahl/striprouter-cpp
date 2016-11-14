#pragma once

#include <vector>
#include <boost/filesystem.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "int_types.h"

struct CharMeta {
  u32 tex_x;
  u32 tex_y;
  u32 tex_w;
  u32 tex_h;
  s32 top_offset;
  s32 left_offset;
  s32 advance;
};

class OglText {
public:
  OglText(u32 window_w, u32 window_h, const boost::filesystem::path &fontPath, u32 font_h);
  ~OglText();
  void print(u32 start_x, u32 start_y, u32 line, const std::string &str);
  u32 calcStringWidth(const std::string &str);
  u32 getStringHeight();
private:
  void createFontTexture();
  bool renderFont(std::vector<u8> &fontVec, FT_Face face);
  void drawTextBackground(u32 start_x, u32 start_y, u32 line, const std::string &str);

  u32 window_w_;
  u32 window_h_;

  GLuint textureId_;
  GLuint textProgramId;
  GLuint textBackgroundProgramId;
  GLuint vertexBufId_;
  GLuint texBufId_;

  u32 lineH_;
  u32 maxAscender_;

  u32 tex_w_h_;
  u32 font_h_;
  std::vector<CharMeta> charMeta_;
  boost::filesystem::path fontPath_;
};
