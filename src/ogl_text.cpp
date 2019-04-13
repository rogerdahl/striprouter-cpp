#include <cassert>
#include <iostream>

#include <GL/glew.h>
#include <fmt/format.h>

#include "ogl_text.h"
#include "shader.h"

const int MAX_TEXTURE_SIZE_W_H = 1024 * 1024;
// FT returns some values in 1/64th of pixel size.
const int FT_SIZE_FACTOR = 64;
const int BACKGROUND_PADDING_PIXELS = 0;

OglText::OglText(const std::string& fontPath, int fontH)
  : oglInitialized_(false),
    texWH_(0),
    fontH_(fontH),
    textProgramId_(0),
    textBackgroundProgramId(0),
    face_(0)
{
  createFreeType(fontPath);
}

OglText::~OglText()
{
  int error = FT_Done_FreeType(freetypeLibraryHandle_);
  if (error) {
    fmt::print(stderr, "Error: Unable to release the FreeType2 library\n");
    exit(0);
  }
}

void OglText::openGLInit()
{
  textProgramId_ = createProgram("text.vert", "text.frag");
  textBackgroundProgramId =
      createProgram("text_background.vert", "text_background.frag");
  glGenBuffers(1, &vertexBufId_);
  glGenBuffers(1, &texBufId_);
  glGenTextures(1, &textureId_);
  createFontTexture();
  oglInitialized_ = true;
}

void OglText::openGLFree()
{
  glDeleteBuffers(1, &vertexBufId_);
  glDeleteBuffers(1, &texBufId_);
  glDeleteTextures(1, &textureId_);
}

void OglText::setFontH(int fontH)
{
  if (fontH != fontH_) {
    fontH_ = fontH;
    createFontTexture();
  }
}

void OglText::print(
    glm::mat4x4& projMat, int x, int y, int nLine, const std::string& str,
    bool drawBackground)
{
  assert(oglInitialized_); // Call openGLInit() after creating an OpenGL context
  glDisable(GL_DEPTH_TEST);

  glBindTexture(GL_TEXTURE_2D, textureId_);
  glActiveTexture(GL_TEXTURE0);

  if (drawBackground) {
    glUseProgram(textBackgroundProgramId);
    GLint projectionId =
        glGetUniformLocation(textBackgroundProgramId, "projection");
    glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projMat));
    drawTextBackground(x, y, nLine, str);
  }
  glUseProgram(textProgramId_);
  GLint projectionId = glGetUniformLocation(textProgramId_, "projection");
  glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projMat));
  drawText(x, y, nLine, str);
}

int OglText::calcStringWidth(const std::string& str)
{
  int strW = 0;
  for (const char& ascii : str) {
    auto& c = charMeta_[ascii - 32];
    strW += c.advance;
  }
  return strW;
}

int OglText::getLineHeight()
{
  return lineH_;
}

//
// Private
//

void OglText::createFreeType(const std::string& fontPath)
{
  int error = FT_Init_FreeType(&freetypeLibraryHandle_);
  if (error) {
    fmt::print(stderr, "Error: Could not initialize the FreeType2 library\n");
    exit(0);
  }
  error = FT_New_Face(freetypeLibraryHandle_, fontPath.c_str(), 0, &face_);
  if (error == FT_Err_Unknown_File_Format) {
    fmt::print(stderr, "Error: Unsupported font file format\n");
    exit(0);
  }
  else if (error) {
    fmt::print(
        stderr,
        "Error: The font file could not be opened, could not be read or is "
        "broken\n");
    exit(0);
  }
}

// The texture must be able to hold the entire font at the selected pixel size.
// Power-of-two textures are compatible with more devices, so we just try
// drawing the font on textures of increasing size until we get to one that
// fits.
void OglText::createFontTexture()
{
  int error = FT_Set_Pixel_Sizes(face_, 0, fontH_);
  if (error) {
    fmt::print(
        stderr, "Error: Could not set the pixel size. font_h={}\n", fontH_);
    exit(0);
  }
  std::vector<unsigned char> fontVec;
  bool fitOk = false;
  for (texWH_ = 32; texWH_ <= MAX_TEXTURE_SIZE_W_H; texWH_ *= 2) {
    fontVec.clear();
    fontVec.resize(texWH_ * texWH_);
    charMeta_.clear();
    fitOk = renderFont(fontVec);
    if (fitOk) {
      break;
    }
  }
  if (!fitOk) {
    fmt::print(
        "Error: Unable to fit font into texture. font_h={} "
        "MAX_TEXTURE_SIZE_W_H={}\n",
        fontH_, MAX_TEXTURE_SIZE_W_H);
    exit(0);
  }

  lineH_ = static_cast<int>(face_->size->metrics.height / 64);
  maxAscender_ = static_cast<int>(face_->size->metrics.ascender / 64);

  glBindTexture(GL_TEXTURE_2D, textureId_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(
      GL_TEXTURE_2D, 0, GL_R8, texWH_, texWH_, 0, GL_RED, GL_UNSIGNED_BYTE,
      &fontVec[0]);

  //  fmt::print("Info: New font texture size: {0}x{0}\n", texWH_);
}

bool OglText::renderFont(std::vector<unsigned char>& fontVec)
{
  // Iterate over all the printable ASCII characters, which span from ASCII 32
  // to 126. 32 is space, which typically
  // renders as a 0x0 texture. We include it anyway, so that we can get the
  // advance metadata, which says how wide
  // the space is in the particular font.
  bool fitOk = true;
  int tex_x = 0;
  int tex_y = 0;
  for (int i = 32; i <= 126; ++i) {
    int error = FT_Load_Char(face_, i, FT_LOAD_RENDER); //  | FT_LOAD_MONOCHROME
    if (error) {
      fmt::print(
          stderr,
          "Warn: Skipping character that couldn't be rendered. ASCII={}\n", i);
      continue;
    }

    FT_GlyphSlot slot = face_->glyph;
    FT_Bitmap glyph_bitmap = slot->bitmap;
    assert(glyph_bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

    // Calc position for next character.
    if (tex_x + glyph_bitmap.width >= static_cast<unsigned int>(texWH_)) {
      tex_x = 0;
      tex_y += fontH_;
    }

    if (tex_y + fontH_ >= texWH_) {
      fitOk = false;
      break;
    }

    // Copy character to font vector.
    for (unsigned int y = 0; y < glyph_bitmap.rows; ++y) {
      for (unsigned int x = 0; x < glyph_bitmap.width; ++x) {
        fontVec[tex_x + x + (tex_y + y) * texWH_] =
            glyph_bitmap.buffer[x + y * glyph_bitmap.pitch];
      }
    }

    charMeta_.push_back(
        { tex_x, tex_y, static_cast<int>(glyph_bitmap.width),
          static_cast<int>(glyph_bitmap.rows), -slot->bitmap_top,
          slot->bitmap_left, // Negate Y since FT uses cartesian coords.
          static_cast<int>(slot->advance.x / FT_SIZE_FACTOR) });

    tex_x += glyph_bitmap.width;
  }

  return fitOk;
}

void OglText::drawText(int x, int y, int nLine, const std::string& str)
{
  std::vector<GLfloat> triVec;
  std::vector<GLfloat> texVec;

  int screen_x = x;
  int screen_y = y + nLine * lineH_;

  for (const char& ascii : str) {
    auto& c = charMeta_[ascii - 32];

    float x1 = screen_x + c.left_offset;
    float y1 = screen_y + c.top_offset + maxAscender_;
    float x2 = x1 + c.tex_w;
    float y2 = y1 + c.tex_h;

    triVec.insert(triVec.end(), { x1, y1, 0.0f });
    triVec.insert(triVec.end(), { x1, y2, 0.0f });
    triVec.insert(triVec.end(), { x2, y2, 0.0f });

    triVec.insert(triVec.end(), { x1, y1, 0.0f });
    triVec.insert(triVec.end(), { x2, y2, 0.0f });
    triVec.insert(triVec.end(), { x2, y1, 0.0f });

    float tx1 = c.tex_x;
    float ty1 = c.tex_y;
    float tx2 = tx1 + c.tex_w;
    float ty2 = ty1 + c.tex_h;

    texVec.insert(texVec.end(), { tx1, ty1 });
    texVec.insert(texVec.end(), { tx1, ty2 });
    texVec.insert(texVec.end(), { tx2, ty2 });

    texVec.insert(texVec.end(), { tx1, ty1 });
    texVec.insert(texVec.end(), { tx2, ty2 });
    texVec.insert(texVec.end(), { tx2, ty1 });

    screen_x += c.advance;
  }

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
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
  glBindBuffer(GL_ARRAY_BUFFER, texBufId_);
  glBufferData(
      GL_ARRAY_BUFFER, texVec.size() * sizeof(GLfloat), &texVec[0],
      GL_STATIC_DRAW);
  glVertexAttribPointer(
      1, // attribute
      2, // size
      GL_FLOAT, // type
      GL_FALSE, // normalized?
      0, // stride
      (void*)0 // array buffer offset
  );

  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void OglText::drawTextBackground(int x, int y, int nLine, const std::string& str)
{
  auto strW = calcStringWidth(str);

  float x1 = x;
  float y1 = y + nLine * lineH_;
  float x2 = x1 + strW;
  float y2 = y1 + lineH_;

  // Widen the band a bit in the Y dir since that looks nicer.
  y1 -= BACKGROUND_PADDING_PIXELS;
  y2 += BACKGROUND_PADDING_PIXELS;

  std::vector<GLfloat> triVec;

  triVec.insert(triVec.end(), { x1, y1, 0.0f });
  triVec.insert(triVec.end(), { x1, y2, 0.0f });
  triVec.insert(triVec.end(), { x2, y2, 0.0f });

  triVec.insert(triVec.end(), { x1, y1, 0.0f });
  triVec.insert(triVec.end(), { x2, y2, 0.0f });
  triVec.insert(triVec.end(), { x2, y1, 0.0f });

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
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
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}
