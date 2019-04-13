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
  explicit SvgWriter(const Layout& layout);
  SvgPathVec writeFiles(std::string circuitFilePath);

  private:
  svg::Document initDoc(const std::string& svgPath, bool drawMirrorImage);
  void writeWireSvg(std::string wireSvgPath);
  void writeStripCutSvg(std::string cutSvgPath);
  void drawBackground(svg::Document& doc);
  void drawBoardOutline(svg::Document& doc);
  void drawCorners(svg::Document& doc);
  void drawVias(svg::Document& doc);
  void drawWireSections(svg::Document& doc);
  void drawWireEndpoint(svg::Document &doc, const Eigen::Array2i& via);
  void drawStripCuts(svg::Document& doc);
  void drawTitle(svg::Document& doc, const std::string& titleStr);
  void drawTitleMirror(svg::Document& doc, const std::string& titleStr);
  const Layout& layout_;
};
