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
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <fmt/format.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <nanogui/nanogui.h>

#include "file_parser.h"
#include "file_writer.h"
#include "gl_error.h"
#include "gui.h"
#include "ogl_text.h"
#include "render.h"
#include "router.h"
#include "status.h"
#include "utils.h"
#include "via.h"


using namespace std::chrono_literals;


// Fonts
std::string DIAG_FONT_PATH = "./fonts/Roboto-Regular.ttf";
const int DIAG_FONT_SIZE = 12;
const int DRAG_FONT_SIZE = 12;
OglText diagText(DIAG_FONT_PATH, DIAG_FONT_SIZE);

// Zoom / pan
const float ZOOM_MOUSE_WHEEL_STEP = 0.3f;
const float ZOOM_MIN = -1.0f;
const float ZOOM_MAX = 5.0f;
const float ZOOM_DEF = 2.0f;
const int INITIAL_BORDER_PIXELS = 50;
float zoomLinear = ZOOM_DEF;
float zoom = expf(zoomLinear);
bool isZoomPanAdjusted = false;
void setZoomPan(Pos scrPos);

// GUI settings
int windowW = 1920 / 2;
int windowH = 1080 - 200;
bool showRatsNestBool = false;
bool showOnlyFailedBool = false;
bool showBestBool = false;
bool writeChangesToCircuitFileBool = false;
std::string circuitFilePath = "./circuits/example.circuit";
glm::tmat4x4<float> projMat;

// Threads
// TODO: Consider https://github.com/vit-vit/ctpl
#ifndef NDEBUG
const int N_ROUTER_THREADS = 1;
#else
const int N_ROUTER_THREADS = std::thread::hardware_concurrency();
#endif

// Router threads
const std::chrono::duration<double> maxRenderDelay = 30s;
std::vector<std::thread> routerThreadVec(N_ROUTER_THREADS);
ThreadStop threadStopRouter;
void stopRouterThreads();
void routerThread();
void launchRouterThreads();

// CircuitFileParser thread
std::thread parserThreadObj;
ThreadStop threadStopParser;
void stopParserThread();
void parserThread();
void launchParserThread();

// Drag / drop
bool isComponentDragActive = false;
bool isBoardDragActive = false;
bool isDragBlockingActive = false;
Pos dragStartPos;
Pos panOffsetScrPos;
Pos dragPin0BoardOffset;
std::string dragComponentName;
OglText dragText(DIAG_FONT_PATH, DRAG_FONT_SIZE);
void handleMouseDragOperations(const IntPos& mouseScrPos);
void renderDragStatus(const IntPos mouseScrPos);

// Diag
averageSec averageRendering;

// Util
std::string formatTotalCost(int totalCost);

// Shared objects
Layout inputLayout;
Layout bestLayout;
Layout currentLayout;

// Misc
Status status;
Render render;



class Application: public nanogui::Screen
{
public:
  Application()
    : nanogui::Screen(Eigen::Vector2i(windowW, windowH),
                      "Stripboard Autorouter",
    // resizable, fullscreen, colorBits, alphaBits, depthBits, stencilBits, nSamples, glMajor, glMinor
                      true,
                      false,
                      8,
                      8,
                      24,
                      8,
                      4,
                      3,
                      3)
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

    diagText.openGLInit();
    dragText.openGLInit();
    render.openGLInit();

    // Main grid window
    auto window = new nanogui::Window(this, "Router");
    window->setPosition(Eigen::Vector2i(windowW - 400, windowH - 300));
    nanogui::GridLayout *layout =
      new nanogui::GridLayout(nanogui::Orientation::Horizontal,
                              2,
                              nanogui::Alignment::Middle,
                              15,
                              5);
    layout->setColAlignment({nanogui::Alignment::Maximum,
                             nanogui::Alignment::Fill});
    layout->setSpacing(0, 10);
    window->setLayout(layout);

//    mProgress = new ProgressBar(window);
    {
      new nanogui::Label(window, "Wire cost:", "sans-bold");
      auto intBox = new nanogui::IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Eigen::Vector2i(100, 20));
      {
        auto lock = inputLayout.scopeLock();
        intBox->setValue(inputLayout.settings.wire_cost);
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
                              auto lock = inputLayout.scopeLock();
                              inputLayout.settings.wire_cost = value;
                              inputLayout.setOriginal();
                            }
                          });
    }
    {
      new nanogui::Label(window, "Strip cost:", "sans-bold");
      auto intBox = new nanogui::IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Eigen::Vector2i(100, 20));
      {
        auto lock = inputLayout.scopeLock();
        intBox->setValue(inputLayout.settings.strip_cost);
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
                              auto lock = inputLayout.scopeLock();
                              inputLayout.settings.strip_cost = value;
                              inputLayout.setOriginal();
                            }
                          });
    }
    {
      new nanogui::Label(window, "Via cost:", "sans-bold");
      auto intBox = new nanogui::IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Eigen::Vector2i(100, 20));
      {
        auto lock = inputLayout.scopeLock();
        intBox->setValue(inputLayout.settings.via_cost);
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
                              auto lock = inputLayout.scopeLock();
                              inputLayout.settings.via_cost = value;
                              inputLayout.setOriginal();
                            }
                          });
    }
    {
      new nanogui::Label(window, "Rat's nest:", "sans-bold");
      auto cb = new nanogui::CheckBox(window, "");
      cb->setFontSize(18);
      cb->setChecked(showRatsNestBool);
      cb->setCallback([cb](bool value)
                      {
                        showRatsNestBool = value;
                      });
    }
    {
      new nanogui::Label(window, "Only Failed:", "sans-bold");
      auto cb = new nanogui::CheckBox(window, "");
      cb->setFontSize(18);
      cb->setChecked(showOnlyFailedBool);
      cb->setCallback([cb](bool value)
                      {
                        showOnlyFailedBool = value;
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
        auto lock = inputLayout.scopeLock();
        cb->setChecked(inputLayout.settings.pause);
      }
      cb->setCallback([cb](bool value)
                      {
                        {
                          auto lock = inputLayout.scopeLock();
                          inputLayout.settings.pause = value;
                        }
                      });
    }
    {
      new nanogui::Label(window, "Write changes to circuit file:", "sans-bold");
      auto cb = new nanogui::CheckBox(window, "");
      cb->setFontSize(18);
      cb->setChecked(writeChangesToCircuitFileBool);
      cb->setCallback([cb](bool value)
                      {
                        writeChangesToCircuitFileBool = value;
                      });
    }
    {
      new nanogui::Label(window, "Zoom:", "sans-bold");

      nanogui::Widget *panel = new nanogui::Widget(window);
      panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
                                              nanogui::Alignment::Middle,
                                              0,
                                              5));

      nanogui::Slider *slider = new nanogui::Slider(panel);
      slider->setRange(std::make_pair(ZOOM_MIN, ZOOM_MAX));
      slider->setValue(ZOOM_DEF);
      slider->setFixedWidth(100);
      slider->setCallback([slider, this](float value) {
        zoomLinear = value;
        auto upperLeftPos = boardToScrPos(Pos(0.0f, 0.0f), zoom, panOffsetScrPos);
        auto lowerRightPos = boardToScrPos(Pos(inputLayout.gridW, inputLayout.gridH), zoom, panOffsetScrPos);
        auto centerBoardPos = upperLeftPos + (lowerRightPos - upperLeftPos ) / 2.0f;
        setZoomPan(centerBoardPos);
      });
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
  virtual bool
  mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers)
  {
    if (nanogui::Widget::mouseButtonEvent(p, button, down, modifiers)) {
      // Event was handled by NanoGUI.
      return true;
    }
    if (down) {
      dragStartPos = getMouseScrPos(mousePos()) - panOffsetScrPos;
      std::string componentName;
      {
        auto lock = inputLayout.scopeLock();
        componentName =
          getComponentAtBoardPos(inputLayout.circuit, getMouseBoardPos(mousePos(), zoom, panOffsetScrPos));
      }
      if (componentName != "") {
        if (!showBestBool) {
          // Start component drag
          isComponentDragActive = true;
          dragComponentName = componentName;
          {
            auto lock = inputLayout.scopeLock();
            auto pin0BoardPos =
              inputLayout.circuit.componentNameToInfoMap[componentName]
                .pin0AbsPos.cast<float>();
            dragPin0BoardOffset = getMouseBoardPos(mousePos(), zoom, panOffsetScrPos) - pin0BoardPos;
          }
        }
        else {
          isDragBlockingActive = true;
        }
      }
      else {
        // Start stripboard drag
        isBoardDragActive = true;
        isZoomPanAdjusted = true;
      }
    }
    else {
      // End component drag
      if (isComponentDragActive) {
        isComponentDragActive = false;
        inputLayout.setOriginal();
      }
      // End blocked drag
      isDragBlockingActive = false;
      // End board drag
      isBoardDragActive = false;
    }
    return true;
  }

  virtual bool scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel)
  {
    if (nanogui::Widget::scrollEvent(p, rel)) {
      // Event was handled by NanoGUI.
      return true;
    }

    // Mouse wheel zoom towards or away from current mouse position
    zoomLinear += rel.y() * ZOOM_MOUSE_WHEEL_STEP;
    zoomLinear = std::max(zoomLinear, ZOOM_MIN);
    zoomLinear = std::min(zoomLinear, ZOOM_MAX);
    setZoomPan(getMouseScrPos(mousePos()));
    return true;
  }

  void setZoomPan(Pos scrPos)
  {
    auto beforeMouseBoardPos = screenToBoardPos(scrPos, zoom, panOffsetScrPos);
    zoom = expf(zoomLinear);
    auto afterMouseBoardPos = screenToBoardPos(scrPos, zoom, panOffsetScrPos);
    panOffsetScrPos -= boardToScrPos(beforeMouseBoardPos, zoom, panOffsetScrPos)
    - boardToScrPos(afterMouseBoardPos, zoom, panOffsetScrPos);
    isZoomPanAdjusted = true;
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

  // Draw the NanoGUI user interface
  virtual void draw(NVGcontext *ctx)
  {
    nanogui::Screen::draw(ctx);
  }

  // Draw app contents (layouts, routes)
  virtual void drawContents()
  {
    glfwGetWindowSize(glfwWindow(), &windowW, &windowH);
    projMat = glm::ortho(
      0.0f,
      static_cast<float>(windowW),
      static_cast<float>(windowH),
      0.0f,
      0.0f,
      100.0f
    );

    double drawStartTime = glfwGetTime();

    glBindVertexArray(vertexArrayId);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowW, windowH);

    handleMouseDragOperations(mousePos());

    // Render a copy of the layout to avoid locking the layout during the render
    // time, which could hold up the router threads.
    //
    //
    // - When the "Show best" checbox is set, render the best found layout.
    // - Else render the current layout.
    {
      Layout layout;
      // When dragging a component, always render the input layout
      if (isComponentDragActive) {
        layout = inputLayout.threadSafeCopy();
      }
      // When "Show best" is set, render the best layout if it's based on the
      // current input. If not, render a blank screen.
      else if (showBestBool) {
        if (bestLayout.isBasedOn(inputLayout)) {
          layout = bestLayout.threadSafeCopy();
        }
      }
      else {
        // If there is a current layout that is based on the current input
        // layout, show render it.
        auto lock = currentLayout.scopeLock();
        if (currentLayout.isBasedOn(inputLayout) && currentLayout.isReadyForEval) {
          layout = currentLayout;
        }
        // Else drop back to rendering the input layout.
        else {
          layout = inputLayout.threadSafeCopy();
        }
      }
      if (!layout.circuit.hasParserError()) {
        // Render the selected layout
        render.draw(
          layout,
          projMat,
          panOffsetScrPos,
          getMouseBoardPos(mousePos(), zoom, panOffsetScrPos),
          zoom,
          windowW,
          windowH,
          showRatsNestBool,
          showOnlyFailedBool
        );
      }
    }

    // Needed for timing but can be bad for performance.
    glFinish();

    renderDragStatus(mousePos());

    double drawEndTime = glfwGetTime();
    averageRendering.addSec(drawEndTime - drawStartTime);
    auto avgRenderingSec = averageRendering.calcAverage();

    // Print status and diags

    // Run status
    int nLine = 0;
    diagText
      .print(projMat, 0, 0, nLine++, fmt::format("Render: {:.1f}ms", avgRenderingSec * 1000));
    diagText.print(projMat, 0, 0, nLine++,
                  fmt::format("Checked: {:n}", status.nunCombinationsChecked));
    diagText.print(projMat, 0, 0, nLine++,
                  fmt::format("Checked/s: {:.2f}",
                              status.nunCombinationsChecked / drawEndTime));
    ++nLine;
    // Input
    {
      auto lock = inputLayout.scopeLock();
      if (inputLayout.circuit.hasParserError()) {
        diagText.print(projMat, 0, 0, nLine++, "Circuit file parsing errors:");
        for (auto s : inputLayout.circuit.parserErrorVec) {
          diagText.print(projMat, 0, 0, nLine++, s);
        }
        ++nLine;
      }
    }
    // Current
    {
      diagText.print(projMat, 0, 0, nLine++, fmt::format("Current"));
      auto inputLock = inputLayout.scopeLock();
      auto currentLock = currentLayout.scopeLock();
      if (currentLayout.isBasedOn(inputLayout)) {
        diagText.print(projMat,
                       0,
                       0,
                       nLine++,
                       fmt::format("Completed: {:n}",
                                   currentLayout.numCompletedRoutes));
        diagText.print(projMat,
                       0,
                       0,
                       nLine++,
                       fmt::format("Failed: {:n}",
                                   currentLayout.numFailedRoutes));
        diagText.print(projMat,
                       0,
                       0,
                       nLine++,
                       fmt::format("Cost: {}",
                                   formatTotalCost(currentLayout.totalCost)));
      }
      else {
        diagText.print(projMat, 0, 0, nLine++, fmt::format("<none>"));
      }
      ++nLine;
    }
    // Best
    {
      diagText.print(projMat, 0, 0, nLine++, fmt::format("Best"));
      auto inputLock = inputLayout.scopeLock();
      auto bestLock = bestLayout.scopeLock();
      if (bestLayout.isBasedOn(inputLayout)) {
        diagText.print(projMat, 0, 0, nLine++,
                       fmt::format("Completed: {:n}",
                                   bestLayout.numCompletedRoutes));
        diagText.print(projMat, 0, 0, nLine++,
                       fmt::format("Failed: {:n}", bestLayout.numFailedRoutes));
        diagText.print(projMat, 0, 0, nLine++,
                       fmt::format("Cost: {}",
                                   formatTotalCost(bestLayout.totalCost)));
      }
      else {
        diagText.print(projMat, 0, 0, nLine++, fmt::format("<none>"));
      }
      ++nLine;
    }

    checkGlError();
  }

private:
  nanogui::ProgressBar *mProgress;
  GLuint vertexArrayId;
};

void handleMouseDragOperations(const IntPos& mousePos)
{
  auto mouseScrPos = getMouseScrPos(mousePos);
  auto lock = inputLayout.scopeLock();

  if (!inputLayout.isReadyForRouting) {
    return;
  }

  if (!isZoomPanAdjusted) {
    float zoomW = (windowW - INITIAL_BORDER_PIXELS) / static_cast<float>(inputLayout.gridW);
    float zoomH = (windowH - INITIAL_BORDER_PIXELS) / static_cast<float>(inputLayout.gridH);
    zoom = std::min(zoomW, zoomH);
    float boardScreenW = (inputLayout.gridW - 1) * zoom;
    float boardScreenH = (inputLayout.gridH - 1)* zoom;
    panOffsetScrPos = Pos(
      windowW / 2.0f - boardScreenW / 2.0f,
      windowH / 2.0f - boardScreenH / 2.0f
    );
    zoomLinear = logf(zoom);
  }

  if (isComponentDragActive) {
    // Prevent dragging outside of grid
    auto mouseBoardPos = getMouseBoardPos(mousePos, zoom, panOffsetScrPos);
    Via v = (mouseBoardPos - dragPin0BoardOffset + 0.5f).cast<int>();
    auto component =
      inputLayout.circuit.componentNameToInfoMap[dragComponentName];
    auto footprint =
      inputLayout.circuit.calcComponentFootprint(dragComponentName);
    auto startPin0Offset = component.pin0AbsPos - footprint.start;
    auto endPin0Offset = footprint.end - component.pin0AbsPos;
    if (v.x() + footprint.start.x() - startPin0Offset.x() < 0) {
      v.x() = startPin0Offset.x();
    }
    if (v.y() + footprint.start.y() - startPin0Offset.y() < 0) {
      v.y() = startPin0Offset.y();
    }
    if (v.x() + endPin0Offset.x() >= inputLayout.gridW) {
      v.x() = inputLayout.gridH - endPin0Offset.x() - 1;
    }
    if (v.y() + endPin0Offset.y() >= inputLayout.gridH) {
      v.y() = inputLayout.gridH - endPin0Offset.y() - 1;
    }
    setComponentPosition(inputLayout.circuit, v, dragComponentName);

    if (writeChangesToCircuitFileBool) {
      CircuitFileWriter fileWriter;
      fileWriter.updateComponentPosition(circuitFilePath, dragComponentName, v);
    }

    inputLayout.setOriginal();
  }
  // Drag board
  else if (isBoardDragActive) {
    panOffsetScrPos = mouseScrPos - dragStartPos;
  }
}

void renderDragStatus(const IntPos mouseScrPos) {
  if (isDragBlockingActive) {
    dragText.print(projMat, mouseScrPos.x(), mouseScrPos.y(), 0,
             fmt::format("Can't edit -- showing best")
    );
  }
  else if (isComponentDragActive) {
//    auto via = inputLayout.circuit.componentNameToInfoMap[dragComponentName].pin0AbsPos;
    auto mouseBoardPos = getMouseBoardPos(mouseScrPos, zoom, panOffsetScrPos);
    Via mouseVia = (mouseBoardPos - dragPin0BoardOffset + 0.5f).cast<int>();
    dragText.print(projMat, mouseScrPos.x(), mouseScrPos.y(), 0,
                   fmt::format("({},{})", mouseVia.x(), mouseVia.y()));
  }
}


std::string formatTotalCost(int totalCost)
{
  if (totalCost == INT_MAX) {
    return "<no complete layouts>";
  }
  return fmt::format("{:n}", totalCost);
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
  threadStopRouter.stop();
  for (int i = 0; i < N_ROUTER_THREADS; ++i) {
    routerThreadVec[i].join();
  }
}

void routerThread()
{
  while (!threadStopRouter.isStopped()) {
    Layout threadLayout;
    {
      auto lock = inputLayout.scopeLock();
      if (!inputLayout.isReadyForRouting || inputLayout.settings.pause) {
        lock.unlock();
        std::this_thread::sleep_for(10ms);
        continue;
      }
      threadLayout = inputLayout;
    }
    {
      Router
        router(threadLayout, threadStopRouter, inputLayout, currentLayout, maxRenderDelay);
      auto isAborted = router.route();
      // Ignore result if the routing was aborted or the input has changed.
      if (isAborted || !threadLayout.isBasedOn(inputLayout)) {
        continue;
      }
    }
    {
      std::lock_guard<std::mutex> lockStatus(statusMutex);
      ++status.nunCombinationsChecked;
    }
    // Update currentLayout
    {
      auto lock = currentLayout.scopeLock();
      currentLayout = threadLayout;
    }
    // Update bestLayout
    {
      auto inputLock = inputLayout.scopeLock();
      auto bestLock = bestLayout.scopeLock();
      auto hasMoreCompletedRoutes =
        threadLayout.numCompletedRoutes > bestLayout.numCompletedRoutes;
      auto hasEqualRoutesAndBetterScore =
        threadLayout.numCompletedRoutes == bestLayout.numCompletedRoutes
          && threadLayout.totalCost > bestLayout.totalCost;
      auto isBasedOnOtherLayout = !bestLayout.isBasedOn(threadLayout);
      if (hasMoreCompletedRoutes || hasEqualRoutesAndBetterScore
        || isBasedOnOtherLayout) {
        bestLayout = threadLayout;
      }
    }
  }
}

//
// CircuitFileParser thread
//

void launchParserThread()
{
  parserThreadObj = std::thread(parserThread);
}

void stopParserThread()
{
  threadStopParser.stop();
  parserThreadObj.join();
}

void parserThread()
{
  static double prevMtime = 0.0;
  static double curMtime = 0.0;
  while (!threadStopParser.isStopped()) {
    try {
      curMtime = getMtime(circuitFilePath);
    }
    catch (std::string errorMsg) {
      {
        auto lock = inputLayout.scopeLock();
        inputLayout = Layout();
        inputLayout.circuit.parserErrorVec.push_back(errorMsg);
      }
      curMtime = 0.0;
      inputLayout.setOriginal();
      std::this_thread::sleep_for(1s);
      continue;
    }
    if (curMtime != prevMtime) {
      prevMtime = curMtime;
      Layout threadLayout;
      auto parser = CircuitFileParser(threadLayout);
      parser.parse(circuitFilePath);
      {
        auto lock = inputLayout.scopeLock();
        inputLayout = threadLayout;
      }

    }
    std::this_thread::sleep_for(500ms);
  }
}

int main(int argc, char **argv)
{
  if (argc == 2) {
    circuitFilePath = argv[1];
  }
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
    std::string
      error_msg = std::string("Fatal error: ") + std::string(e.what());
#if defined(_WIN32)
    MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
    std::cerr << error_msg << std::endl;
#endif
    return -1;
  }
  return 0;
}
