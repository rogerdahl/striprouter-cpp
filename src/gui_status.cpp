#include <fmt/format.h>

#include "gui_status.h"
#include "via.h"

GuiStatus::GuiStatus()
{
  reset();
}

GuiStatus::~GuiStatus()
{
}

void GuiStatus::reset()
{
  msPerFrame = 0.0f;

  nCombinationsChecked = 0;
  nCheckedPerSec = 0.0f;

  nCurrentCompletedRoutes = 0;
  nCurrentFailedRoutes = 0.0;
  currentCost = 0;

  nBestCompletedRoutes = 0;
  nBestFailedRoutes = 0;
  bestCost = 0;

  groupedStrCombinationsChecked = "";

  groupedStrCurrentCompletedRoutes = "";
  avgStrCurrentFailedRoutes = "";
  groupedStrCurrentCost = "";

  groupedStrBestCompletedRoutes = "";
  groupedStrBestFailedRoutes = "";
  groupedStrBestCost = "";
}

void GuiStatus::init(nanogui::Screen* screen)
{
  screen_ = screen;

  form_ = new nanogui::FormHelper(screen_);
  IntPos p(10, 10);
  formWin_ = form_->addWindow(p, "Status");

  form_->addGroup("Render");
  {
    auto w = form_->addVariable("ms/frame", msPerFrame);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->numberFormat("%.2f");
  }

  form_->addGroup("Total");
  {
    auto w = form_->addVariable("Checked", groupedStrCombinationsChecked);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->setAlignment(nanogui::TextBox::Alignment::Right);
  }
  {
    auto w = form_->addVariable("Checked/s", nCheckedPerSec);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->numberFormat("%.2f");
  }
  form_->addGroup("Current Layout");
  {
    auto w = form_->addVariable("Completed", groupedStrCurrentCompletedRoutes);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->setAlignment(nanogui::TextBox::Alignment::Right);
  }
  {
    auto w = form_->addVariable("Failed Avg", avgStrCurrentFailedRoutes);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->setAlignment(nanogui::TextBox::Alignment::Right);
  }
  {
    auto w = form_->addVariable("Cost", groupedStrCurrentCost);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->setAlignment(nanogui::TextBox::Alignment::Right);
  }
  form_->addGroup("Best Layout");
  {
    auto w = form_->addVariable("Completed", groupedStrBestCompletedRoutes);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->setAlignment(nanogui::TextBox::Alignment::Right);
  }
  {
    auto w = form_->addVariable("Failed", groupedStrBestFailedRoutes);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->setAlignment(nanogui::TextBox::Alignment::Right);
  }
  {
    auto w = form_->addVariable("Cost", groupedStrBestCost);
    w->setEditable(false);
    w->setFixedWidth(70);
    w->setAlignment(nanogui::TextBox::Alignment::Right);
  }
}

void GuiStatus::free()
{
  delete form_;
}

void GuiStatus::refresh()
{
  groupedStrCombinationsChecked = groupThousands(nCombinationsChecked);

  groupedStrCurrentCompletedRoutes = groupThousands(nCurrentCompletedRoutes);
  avgStrCurrentFailedRoutes = fmt::format("{:.2f}", nCurrentFailedRoutes);
  groupedStrCurrentCost = groupThousands(currentCost);

  groupedStrBestCompletedRoutes = groupThousands(nBestCompletedRoutes);
  groupedStrBestFailedRoutes = groupThousands(nBestFailedRoutes);
  groupedStrBestCost = groupThousands(bestCost);

  form_->refresh();
}

std::string GuiStatus::groupThousands(long n)
{
  return fmt::format("{:n}", n);
}

void GuiStatus::moveToUpperLeft()
{
  formWin_->setPosition(IntPos(0, 0));
}
