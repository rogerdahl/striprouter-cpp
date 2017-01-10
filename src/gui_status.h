#pragma once

#include <string>

#include <nanogui/nanogui.h>


class GuiStatus
{
public:
  GuiStatus();
  ~GuiStatus();
  void reset();
  void init(nanogui::Screen* screen);
  void free();
  void refresh();
  void moveToUpperLeft();
  std::string groupThousands(long n);

  float msPerFrame;

  int nCombinationsChecked;
  float nCheckedPerSec;

  int nCurrentCompletedRoutes;
  double nCurrentFailedRoutes;
  double currentCost;

  int nBestCompletedRoutes;
  int nBestFailedRoutes;
  double bestCost;
private:
  nanogui::Screen* screen_;
  nanogui::FormHelper* form_;
  nanogui::Window* formWin_;
  // String representations with thousands separator
  std::string groupedStrCombinationsChecked;
  std::string groupedStrCurrentCompletedRoutes;
  std::string avgStrCurrentFailedRoutes;
  std::string groupedStrCurrentCost;
  std::string groupedStrBestCompletedRoutes;
  std::string groupedStrBestFailedRoutes;
  std::string groupedStrBestCost;
};
