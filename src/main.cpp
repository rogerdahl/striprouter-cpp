// Style: http://geosoft.no/development/cppstyle.html
// Format: CLion. Config:
// Settings > Editor > Code Style > C/C++ > Set from > Predefined Style > Qt
// Blank Lines > Around global variable > 0
// Tabs and indents > Set all that are 4 to 2
// Wrapping and Braces > Simple functions in one line > off
// Wrapping and Braces > After function return type > Wrap if long (all)
// Wrapping and Braces > Keep when reformatting > Line breaks > off

#include <chrono>
#include <climits>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <nanogui/nanogui.h>

#include "router.h"
#include "gl_error.h"
#include "gui.h"
#include "status.h"
#include "utils.h"


int windowW = 1920 / 2;
int windowH = 1080 - 200;
//int windowW = 500;
//int windowH = 500;

const int GRID_W = 60;
const int GRID_H = 60;

//const char *DIAG_FONT_PATH = "./fonts/LiberationMono-Regular.ttf";
const char *DIAG_FONT_PATH = "./fonts/Roboto-Black.ttf";
const int DIAG_FONT_SIZE = 12;
const int DRAG_FONT_SIZE = 12;

const float ZOOM_MOUSE_WHEEL_STEP = 0.1f;
const float ZOOM_MIN = -2.0f;
const float ZOOM_MAX = 4.0f;
const float ZOOM_DEF = 0.3f;

averageSec averageRendering;
static bool showInputBool = false;
static bool showBestBool = false;
std::time_t mtime_prev = 0;
float zoomLinear = ZOOM_DEF;
float zoom = expf(zoomLinear);
OglText oglText(DIAG_FONT_PATH, DIAG_FONT_SIZE);
Render render(zoom);
std::string formatTotalCost(int totalCost);

Solution inputSolution(GRID_W, GRID_H);
Solution bestSolution(GRID_W, GRID_H);
Solution currentSolution(GRID_W, GRID_H);

Status status;

// Threads
// TODO: Consider https://github.com/vit-vit/ctpl
#ifndef NDEBUG
const int N_ROUTER_THREADS = 1;
#else
const int N_ROUTER_THREADS = std::thread::hardware_concurrency();
#endif

void resetBestSolution();

// Router threads
std::vector<std::thread> routerThreadVec(N_ROUTER_THREADS);
std::mutex stopRouterThreadMutex;
void stopRouterThreads();
bool isRouterStopRequested();
void routerThread();
void launchRouterThreads();

// Parser thread
std::thread parserThreadObj;
std::mutex stopParserThreadMutex;
void stopParserThread();
bool isParserStopRequested();
void parserThread();
void launchParserThread();

// Drag / drop
bool dragComponentIsActive = false;
bool dragBoardIsActive = false;
Pos dragStartCoord;
Pos dragBoardOffset;
Pos dragPin0BoardOffset;
std::string dragComponentName;
OglText dragText(DIAG_FONT_PATH, DRAG_FONT_SIZE);

Pos mouseScreenPos;
Pos mouseBoardPos;

class Application: public nanogui::Screen
{
public:
  Application()
    : nanogui::Screen(Eigen::Vector2i(windowW, windowH), "Stripboard Autorouter",
    // resizable, fullscreen, colorBits, alphaBits, depthBits, stencilBits, nSamples, glMajor, glMinor
                      true, false, 8, 8, 24, 8, 4, 3, 3)
  {
    GLuint mTextureId;
    glGenTextures(1, &mTextureId);
    glfwMakeContextCurrent(glfwWindow());

    // Initialize GLEW.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
      fmt::print(stderr, "Failed to initialize GLEW\n");
      glfwTerminate();
      return;
    }

    glGetError();
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glGenVertexArrays(1, &vertexArrayId);
    glBindVertexArray(vertexArrayId);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowW, windowH);

    oglText.openGLInit();
    render.openGLInit();
    dragText.openGLInit();

    // Main grid window
    auto window = new nanogui::Window(this, "Router");
    window->setPosition(Vector2i(windowW - 400, windowH - 300));
    nanogui::GridLayout
      *layout = new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2, nanogui::Alignment::Middle, 15, 5);
    layout->setColAlignment({nanogui::Alignment::Maximum, nanogui::Alignment::Fill});
    layout->setSpacing(0, 10);
    window->setLayout(layout);

//    mProgress = new ProgressBar(window);
    {
      new nanogui::Label(window, "Wire cost:", "sans-bold");
      auto intBox = new nanogui::IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Vector2i(100, 20));
      {
        auto lock = inputSolution.scope_lock();
        intBox->setValue(inputSolution.settings.wire_cost);
      }
      intBox->setDefaultValue("0");
      intBox->setFontSize(18);
      intBox->setFormat("[1-9][0-9]*");
      intBox->setSpinnable(true);
      intBox->setMinValue(1);
      intBox->setValueIncrement(1);
      intBox->setCallback([intBox](int value)
                          {
                            {
                              auto lock = inputSolution.scope_lock();
                              inputSolution.settings.wire_cost = value;
                            }
                            resetBestSolution();
                          });
    }
    {
      new nanogui::Label(window, "Strip cost:", "sans-bold");
      auto intBox = new nanogui::IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Vector2i(100, 20));
      {
        auto lock = inputSolution.scope_lock();
        intBox->setValue(inputSolution.settings.strip_cost);
      }
      intBox->setDefaultValue("0");
      intBox->setFontSize(18);
      intBox->setFormat("[1-9][0-9]*");
      intBox->setSpinnable(true);
      intBox->setMinValue(1);
      intBox->setValueIncrement(1);
      intBox->setCallback([intBox](int value)
                          {
                            {
                              auto lock = inputSolution.scope_lock();
                              inputSolution.settings.strip_cost = value;
                            }
                            resetBestSolution();
                          });
    }
    {
      new nanogui::Label(window, "Via cost:", "sans-bold");
      auto intBox = new nanogui::IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Vector2i(100, 20));
      {
        auto lock = inputSolution.scope_lock();
        intBox->setValue(inputSolution.settings.via_cost);
      }
      intBox->setDefaultValue("0");
      intBox->setFontSize(18);
      intBox->setFormat("[1-9][0-9]*");
      intBox->setSpinnable(true);
      intBox->setMinValue(1);
      intBox->setValueIncrement(1);
      intBox->setCallback([intBox](int value)
                          {
                            {
                              auto lock = inputSolution.scope_lock();
                              inputSolution.settings.via_cost = value;
                            }
                            resetBestSolution();
                          });
    }
    {
      new nanogui::Label(window, "Show input:", "sans-bold");
      auto cb = new nanogui::CheckBox(window, "");
      cb->setFontSize(18);
      cb->setChecked(showInputBool);
      cb->setCallback([cb](bool value)
                      {
                        showInputBool = value;
                      });
    }
    {
      new nanogui::Label(window, "Show best:", "sans-bold");
      auto cb = new nanogui::CheckBox(window, "");
      cb->setFontSize(18);
      cb->setChecked(showBestBool);
      cb->setCallback([cb](bool value)
                      {
                        showBestBool = value;
                      });
    }
    {
      new nanogui::Label(window, "Pause:", "sans-bold");
      auto cb = new nanogui::CheckBox(window, "");
      cb->setFontSize(18);
      {
        auto lock = inputSolution.scope_lock();
        cb->setChecked(inputSolution.settings.pause);
      }
      cb->setCallback([cb](bool value)
                      {
                        {
                          auto lock = inputSolution.scope_lock();
                          inputSolution.settings.pause = value;
                        }
                      });
    }
    {
      new nanogui::Label(window, "Zoom:", "sans-bold");

      nanogui::Widget *panel = new nanogui::Widget(window);
      panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 5));

      nanogui::Slider *slider = new nanogui::Slider(panel);
      slider->setValue(zoom / 4.0f);
      slider->setFixedWidth(100);

      nanogui::TextBox *textBox = new nanogui::TextBox(panel);
      textBox->setFixedSize(Vector2i(50, 20));
      textBox->setValue("50");
      textBox->setUnits("%");
      slider->setCallback([textBox](float value)
                          {
                            textBox->setValue(std::to_string((int) (value * 200)));
                            ::zoom = value * 4.0f;
                          });
//      slider->setFinalCallback([&](float value) {
//        ::zoom = value * 4.0f;
//      });
      textBox->setFixedSize(Vector2i(60, 25));
      textBox->setFontSize(18);
      textBox->setAlignment(nanogui::TextBox::Alignment::Right);
    }
    performLayout();
    launchRouterThreads();
    launchParserThread();
  }

  ~Application()
  {
    stopRouterThreads();
    stopParserThread();
  }

  // Could not get NanoGUI mouseDragEvent to trigger so rolling my own.
  // See NanoGUI src/window.cpp for example mouseDragEvent handler.
  virtual bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers)
  {
    if (nanogui::Widget::mouseButtonEvent(p, button, down, modifiers)) {
      // Event was handled by NanoGUI.
      return true;
    }
    if (down) {
      dragStartCoord = mouseScreenPos - dragBoardOffset;
      std::string componentName;
      {
        auto lock = inputSolution.scope_lock();
        componentName = getComponentAtMouseCoordinate(render, inputSolution.circuit, mouseBoardPos);
      }
      if (componentName != "") {
        // Start component drag
        dragComponentIsActive = true;
        dragComponentName = componentName;
        {
          auto lock = inputSolution.scope_lock();
          auto pin0BoardPos = inputSolution.circuit.componentNameToInfoMap[componentName].pin0AbsCoord.cast<float>();
          dragPin0BoardOffset = mouseBoardPos - pin0BoardPos;
        }
      }
      else {
        // Start stripboard drag
        dragBoardIsActive = true;
      }
    }
    else {
      // End any drag
      dragComponentIsActive = false;
      dragBoardIsActive = false;
    }
    return true;
  }

  virtual bool scrollEvent(const Vector2i &p, const Vector2f &rel)
  {
    if (nanogui::Widget::scrollEvent(p, rel)) {
      // Event was handled by NanoGUI.
      return true;
    }
    zoomLinear += rel.y() * ZOOM_MOUSE_WHEEL_STEP;
    zoomLinear = std::max(zoomLinear, ZOOM_MIN);
    zoomLinear = std::min(zoomLinear, ZOOM_MAX);
    zoom = expf(zoomLinear);
    return true;
  }

  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers)
  {
    if (nanogui::Screen::keyboardEvent(key, scancode, action, modifiers)) {
      return true;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      setVisible(false);
      return true;
    }
    return false;
  }

  virtual void draw(NVGcontext *ctx)
  {
    // Draw the user interface
    nanogui::Screen::draw(ctx);
  }

  virtual void drawContents()
  {
    mouseScreenPos = mousePos().cast<float>();
    glfwGetWindowSize(glfwWindow(), reinterpret_cast<int *>(&windowW), reinterpret_cast<int *>(&windowH));
    double drawStartTime = glfwGetTime();

    glBindVertexArray(vertexArrayId);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowW, windowH);

    // render.draw() first because it updates render's internal metrics, which are later used for resolving mouse
    // pos, etc.
    {
      Solution solution(GRID_W, GRID_H);
      if (dragComponentIsActive) {
        auto lock = inputSolution.scope_lock();
        solution = inputSolution;
      }
      else if (showBestBool) {
        auto lock = bestSolution.scope_lock();
        solution = bestSolution;
      }
      else {
        auto lock = currentSolution.scope_lock();
        solution = currentSolution;
      }
      render.draw(solution, windowW, windowH, zoom, dragBoardOffset, mouseScreenPos, showInputBool);
      mouseBoardPos = render.screenToBoardCoord(mouseScreenPos);
    }

    if (!showBestBool) {
      // Drag component
      if (dragComponentIsActive) {
        // Block component drag when showing best
        if (!showBestBool) {
          {
            auto lock = inputSolution.scope_lock();
            setComponentPosition(render, inputSolution.circuit, mouseBoardPos - dragPin0BoardOffset, dragComponentName);
          }
          resetBestSolution();
        }
      }
    }
    // Drag board
    if (dragBoardIsActive) {
      dragBoardOffset = mouseScreenPos;
      dragBoardOffset -= dragStartCoord;
    }

    if (dragComponentIsActive) {
      dragText.reset(windowW,
                     windowH,
                     static_cast<int>(mouseScreenPos.x()),
                     static_cast<int>(mouseScreenPos.y()) - DRAG_FONT_SIZE,
                     DRAG_FONT_SIZE);
      if (showBestBool) {
        dragText.print(0, fmt::format("Can't edit -- showing best"));
      }
      else {
        auto lock = currentSolution.scope_lock();
        auto via = currentSolution.circuit.componentNameToInfoMap[dragComponentName].pin0AbsCoord;
        dragText.print(0, fmt::format("({},{})", via.x(), via.y()));
      }
    }

    // Needed for timing but can be bad for performance.
    glFinish();

    double drawEndTime = glfwGetTime();
    averageRendering.addSec(drawEndTime - drawStartTime);
    auto avgRenderingSec = averageRendering.calcAverage();

    // Diag

    // Run status
    int nLine = 0;
    oglText.reset(windowW, windowH, 0, 0, DIAG_FONT_SIZE);
    oglText.print(nLine++, fmt::format("Render: {:.1f}ms", avgRenderingSec * 1000));
    oglText.print(nLine++, fmt::format("Checked: {:n}", status.nunCombinationsChecked));
    oglText.print(nLine++, fmt::format("Checked/s: {:.2f}", status.nunCombinationsChecked / drawEndTime));
    ++nLine;
    // List any errors from the parser
    {
      auto lock = inputSolution.scope_lock();
      if (inputSolution.circuit.circuitInfoVec.size()) {
        for (auto s : inputSolution.circuit.circuitInfoVec) {
          oglText.print(nLine++, s);
        }
        ++nLine;
      }
    }
    // Current
    {
      auto lock = currentSolution.scope_lock();
      oglText.print(nLine++, fmt::format("Current"));
      oglText.print(nLine++, fmt::format("Completed: {:n}", currentSolution.numCompletedRoutes));
      oglText.print(nLine++, fmt::format("Failed: {:n}", currentSolution.numFailedRoutes));
//      oglText.print(nLine++, fmt::format("Shortcuts: {:n}", currentSolution.numShortcuts));
      oglText.print(nLine++, fmt::format("Cost: {}", formatTotalCost(currentSolution.totalCost)));
      ++nLine;
    }
    // Best
    {
      auto lock = bestSolution.scope_lock();
      oglText.print(nLine++, fmt::format("Best"));
      oglText.print(nLine++, fmt::format("Completed: {:n}", bestSolution.numCompletedRoutes));
      oglText.print(nLine++, fmt::format("Failed: {:n}", bestSolution.numFailedRoutes));
//      oglText.print(nLine++, fmt::format("Shortcuts: {:n}", bestSolution.numShortcuts));
      oglText.print(nLine++, fmt::format("Cost: {}", formatTotalCost(bestSolution.totalCost)));
      ++nLine;
    }

    checkGlError();
  }

private:
  nanogui::ProgressBar *mProgress;
  GLuint vertexArrayId;
};

std::string formatTotalCost(int totalCost)
{
  if (totalCost == INT_MAX) {
    return "<no complete layouts>";
  }
  return fmt::format("{:n}", totalCost);
}

void resetBestSolution()
{
  auto lock = bestSolution.scope_lock();
  bestSolution = Solution(GRID_W, GRID_H);
}

//
// Router thread
//

void launchRouterThreads()
{
  for (int i = 0; i < N_ROUTER_THREADS; ++i) {
    routerThreadVec[i] = std::thread(routerThread);
  }
}

void stopRouterThreads()
{
  try {
    std::lock_guard<std::mutex> stopThread(stopRouterThreadMutex);
    for (int i = 0; i < N_ROUTER_THREADS; ++i) {
      routerThreadVec[i].join();
    }
  }
  catch (const std::system_error &e) {
    fmt::print(stderr, "Attempted to stop thread that is not running\n");
  }
}

bool isRouterStopRequested()
{
  bool lockObtained = stopRouterThreadMutex.try_lock();
  if (lockObtained) {
    stopRouterThreadMutex.unlock();
  }
  return !lockObtained;
}

void routerThread()
{
  while (true) {
    Solution threadSolution(GRID_W, GRID_H);
    {
      auto lock = inputSolution.scope_lock();
      if (!inputSolution.circuit.isReady || inputSolution.settings.pause) {
        lock.unlock();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
        continue;
      }
      threadSolution = inputSolution;
    }
    {
      Router dijkstra(threadSolution);
      dijkstra.route(stopRouterThreadMutex);
      if (isRouterStopRequested()) {
        break;
      }
    }
    {
      std::lock_guard<std::mutex> lockStatus(statusMutex);
      ++status.nunCombinationsChecked;
    }
    {
      auto lock = currentSolution.scope_lock();
      currentSolution = threadSolution;
    }
    if (threadSolution.numCompletedRoutes > bestSolution.numCompletedRoutes) {
      auto lock = bestSolution.scope_lock();
      bestSolution = threadSolution;
    }
    if (threadSolution.numCompletedRoutes == bestSolution.numCompletedRoutes && threadSolution.totalCost > bestSolution.totalCost) {
      auto lock = bestSolution.scope_lock();
      bestSolution = threadSolution;
    }
  }
}

//
// Parser thread
//

void launchParserThread()
{
  parserThreadObj = std::thread(parserThread);
}

void stopParserThread()
{
  try {
    std::lock_guard<std::mutex> stopThread(stopParserThreadMutex);
    parserThreadObj.join();
  }
  catch (const std::system_error &e) {
    fmt::print(stderr, "Attempted to stop thread that is not running\n");
  }
}

bool isParserStopRequested()
{
  bool lockObtained = stopParserThreadMutex.try_lock();
  if (lockObtained) {
    stopParserThreadMutex.unlock();
  }
  return !lockObtained;
}

// Parse circuit.txt file if changed
void parserThread()
{
  while (true) {
    std::time_t mtime_cur = boost::filesystem::last_write_time("./circuit.txt");
    if (mtime_cur != mtime_prev) {
      mtime_prev = mtime_cur;
      {
        auto parser = Parser();
        auto lock = inputSolution.scope_lock();
        parser.parse(inputSolution.circuit);
      }
      resetBestSolution();
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    if (isParserStopRequested()) {
      break;
    }
  }
}

int main(int argc, char **argv)
{
  try {
    nanogui::init();
    {
      setlocale(LC_NUMERIC, "en_US.UTF-8");
      nanogui::ref<Application> app = new Application();
      app->setVisible(true);
      nanogui::mainloop();
    }
    nanogui::shutdown();
  }
  catch (const std::runtime_error &e) {
    std::string error_msg = std::string("Fatal error: ") + std::string(e.what());
#if defined(_WIN32)
    //MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
    std::cerr << error_msg << std::endl;
#endif
    return -1;
  }
  return 0;
}
