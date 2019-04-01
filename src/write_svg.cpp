#include "simple_svg_1.0.0.hpp"

#include "fmt/format.h"

#include "write_svg.h"

// Write layout to Scalable Vector Graphics (SVG) files.

// Sizes relative to the distance between two vias.

// The distance between vias on a stripboard is 0.1 inches (2.54 mm).
const double VIA_DISTANCE_INCH = 0.1;
const double VIA_DIAMETER = 0.2;

const double WIRE_WIDTH = 0.3;
const double WIRE_ENDPOINT_DIAMETER = 0.8;
const double WIRE_ENDPOINT_WIDTH = 0.1;

const double CUT_WIDTH = 0.83f;
const double CUT_HEIGHT = 0.08f;

const double CORNER_ALIGNMENT_MARKER_DIAMETER = 0.3;
const double BOARD_MARGIN = 5.0;
const double BOARD_OUTLINE_WIDTH = 0.2;

double TITLE_FONT_SIZE = 1.0;
const char *TITLE_FONT_NAME = "Arial";

const auto BLACK = svg::Color::Black;
const auto WHITE = svg::Color::White;

SvgWriter::SvgWriter(const Layout& _layout) : layout_(_layout)
{
}

SvgPathVec SvgWriter::writeFiles(const std::string circuitFilePath)
{
  auto baseName = circuitFilePath.substr(0, circuitFilePath.find_last_of("."));
  auto wireSvgPath = fmt::format(
      "{}.{}.{}.{}.{}.svg", baseName, layout_.nCompletedRoutes,
      layout_.nFailedRoutes, layout_.cost, "wires");
  auto stripCutSvgPath = fmt::format(
      "{}.{}.{}.{}.{}.svg", baseName, layout_.nCompletedRoutes,
      layout_.nFailedRoutes, layout_.cost, "cuts");
  writeWireSvg(wireSvgPath);
  writeStripCutSvg(stripCutSvgPath);
  SvgPathVec svgPathVec;
  svgPathVec.push_back(wireSvgPath);
  svgPathVec.push_back(stripCutSvgPath);
  return svgPathVec;
}

//
// Private
//

svg::Document SvgWriter::initDoc(
    const std::string& svgPath, const bool drawMirrorImage)
{
  auto physicalWInch = VIA_DISTANCE_INCH * (layout_.gridW + (2.0 * BOARD_MARGIN));
  auto physicalHInch = VIA_DISTANCE_INCH * (layout_.gridH + (2.0 * BOARD_MARGIN));

  auto virtualUpperLeft = svg::Point(-BOARD_MARGIN, -BOARD_MARGIN);
  auto virtualLowerRight =
      svg::Point(layout_.gridW + BOARD_MARGIN, layout_.gridH + BOARD_MARGIN);

  auto physicalWStr = fmt::format("{:f}in", physicalWInch);
  auto physicalHStr = fmt::format("{:f}in", physicalHInch);

  auto origin = drawMirrorImage ? svg::Layout::TopRight : svg::Layout::TopLeft;
  auto originOffset =
      drawMirrorImage ? svg::Point(2.0 * BOARD_MARGIN, 0) : svg::Point(0, 0);

  svg::Dimensions dimensions(
      physicalWStr, physicalHStr, virtualUpperLeft, virtualLowerRight);

  return svg::Document(
      svgPath, svg::Layout(dimensions, origin, 1.0, originOffset));
}

void SvgWriter::writeWireSvg(const std::string wireSvgPath)
{
  auto doc = initDoc(wireSvgPath, /*drawMirrorImage*/ false);
  drawBackground(doc);
  drawBoardOutline(doc);
  drawCorners(doc);
  drawWireSections(doc);
  drawTitle(doc, "Wires, wire side view");
  doc.save();
}

void SvgWriter::writeStripCutSvg(const std::string cutSvgPath)
{
  auto doc = initDoc(cutSvgPath, /*drawMirrorImage*/ true);
  drawBackground(doc);
  drawBoardOutline(doc);
  drawCorners(doc);
  drawStripCuts(doc);
  drawTitleMirror(doc, "Cuts, strip side view (mirror image)");
  doc.save();
}

void SvgWriter::drawBackground(svg::Document& doc)
{
  svg::Polygon background(svg::Fill(WHITE), svg::Stroke(0.0, WHITE));
  background << svg::Point(-BOARD_MARGIN, -BOARD_MARGIN)
             << svg::Point(layout_.gridW + BOARD_MARGIN, -BOARD_MARGIN)
             << svg::Point(layout_.gridW + BOARD_MARGIN, layout_.gridH + BOARD_MARGIN)
             << svg::Point(-BOARD_MARGIN, layout_.gridH + BOARD_MARGIN);
  doc << background;
}

void SvgWriter::drawBoardOutline(svg::Document& doc)
{
  svg::Polygon border(svg::Fill(WHITE), svg::Stroke(BOARD_OUTLINE_WIDTH, BLACK));
  border << svg::Point(-1.0, -1.0) << svg::Point(layout_.gridW, -1.0)
         << svg::Point(layout_.gridW, layout_.gridH)
         << svg::Point(-1.0, layout_.gridH);
  doc << border;
}

void SvgWriter::drawCorners(svg::Document& doc)
{
  doc << svg::Circle(
      svg::Point(0, 0), CORNER_ALIGNMENT_MARKER_DIAMETER, svg::Fill(BLACK),
      svg::Stroke(CORNER_ALIGNMENT_MARKER_DIAMETER, BLACK));
  doc << svg::Circle(
      svg::Point(layout_.gridW - 1, 0), CORNER_ALIGNMENT_MARKER_DIAMETER, svg::Fill(BLACK),
      svg::Stroke(CORNER_ALIGNMENT_MARKER_DIAMETER, BLACK));
  doc << svg::Circle(
      svg::Point(layout_.gridW - 1, layout_.gridH - 1), CORNER_ALIGNMENT_MARKER_DIAMETER,
      svg::Fill(BLACK), svg::Stroke(CORNER_ALIGNMENT_MARKER_DIAMETER, BLACK));
  doc << svg::Circle(
      svg::Point(0, layout_.gridH - 1), CORNER_ALIGNMENT_MARKER_DIAMETER, svg::Fill(BLACK),
      svg::Stroke(CORNER_ALIGNMENT_MARKER_DIAMETER, BLACK));
}

void SvgWriter::drawVias(svg::Document& doc)
{
  for (int y = 0; y < layout_.gridH; ++y) {
    for (int x = 0; x < layout_.gridW; ++x) {
      doc << svg::Circle(
          svg::Point(x, y), VIA_DIAMETER, svg::Fill(BLACK),
          svg::Stroke(VIA_DIAMETER, BLACK));
    }
  }
}

void SvgWriter::drawWireSections(svg::Document& doc)
{
  for (const auto& routeSectionVec : layout_.routeVec) {
    for (auto section : routeSectionVec) {
      const auto& start = section.start.via;
      const auto& end = section.end.via;
      if (start.x() != end.x() && start.y() == end.y()) {
          drawWireEndpoint(doc, start);
          drawWireEndpoint(doc, end);
          doc << svg::Line(
              svg::Point(section.start.via.x(), section.start.via.y()),
              svg::Point(section.end.via.x(), section.end.via.y()),
              svg::Stroke(WIRE_WIDTH, BLACK)
          );
      }
    }
  }
}

void SvgWriter::drawWireEndpoint(svg::Document &doc, const Eigen::Array2i& via) {
    doc << svg::Circle(
            svg::Point(via.x(), via.y()),
            WIRE_ENDPOINT_DIAMETER, svg::Fill(WHITE),
            svg::Stroke(WIRE_ENDPOINT_WIDTH, BLACK)
    );
}

void SvgWriter::drawStripCuts(svg::Document& doc)
{
  for (auto& v : layout_.stripCutVec) {
    auto halfCutW = CUT_WIDTH / 2.0f;
    auto halfCutH = CUT_HEIGHT / 2.0f;
    svg::Point start(v.x() +halfCutW, v.y() - 0.5 - halfCutH);
    doc << svg::Rectangle(
        start, CUT_WIDTH, CUT_HEIGHT, svg::Fill(BLACK), svg::Stroke(VIA_DIAMETER, BLACK));
  }
}

void SvgWriter::drawTitle(svg::Document& doc, const std::string& titleStr)
{
    doc << svg::Text(
            svg::Point(0.0, -TITLE_FONT_SIZE * 2), titleStr, BLACK,
            svg::Font(TITLE_FONT_SIZE, TITLE_FONT_NAME));
}

void SvgWriter::drawTitleMirror(svg::Document& doc, const std::string& titleStr)
{
  doc << svg::Text(
      svg::Point(layout_.gridW, -TITLE_FONT_SIZE * 2), titleStr, BLACK,
      svg::Font(TITLE_FONT_SIZE, TITLE_FONT_NAME));
}
