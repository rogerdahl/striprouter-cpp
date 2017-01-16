#pragma once

#include <string>
#include <vector>

#include "layout.h"

namespace svg
{
class Document;
}

typedef std::vector<std::string> SvgPathVec;

class SvgWriter
{
  public:
  SvgWriter(const Layout& layout);
  SvgPathVec writeFiles(const std::string circuitFilePath);

  private:
  svg::Document initDoc(const std::string& svgPath, const bool drawMirrorImage);
  void writeWireSvg(const std::string wireSvgPath);
  void writeStripCutSvg(const std::string cutSvgPath);
  void drawBackground(svg::Document& doc);
  void drawBoardOutline(svg::Document& doc);
  void drawCorners(svg::Document& doc);
  void drawVias(svg::Document& doc);
  void drawWireSections(svg::Document& doc);
  void drawStripCuts(svg::Document& doc);
  void drawTitle(svg::Document& doc, const std::string& titleStr);
  void drawTitleMirror(svg::Document& doc, const std::string& titleStr);
  const Layout& layout_;
};
