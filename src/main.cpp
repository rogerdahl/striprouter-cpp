#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include <Windows.h>
#undef min
#undef max
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmdparser.hpp>
#include <fmt/format.h>
#include <nanogui/nanogui.h>

#include "circuit_parser.h"
#include "circuit_writer.h"
#include "ga_interface.h"
#include "gl_error.h"
#include "gui.h"
#include "gui_status.h"
#include "icon.h"
#include "ogl_text.h"
#include "render.h"
#include "router.h"
#include "status.h"
#include "utils.h"
#include "via.h"
#include "write_svg.h"

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
void centerBoard();
nanogui::Slider* zoomSlider;

// GUI settings
int windowW = 1920 / 2;
int windowH = 1080 - 200;
bool isShowRatsNestEnabled = true;
bool isShowOnlyFailedEnabled = true;
bool isShowCurrentEnabled = false;
glm::tmat4x4<float> projMat;
nanogui::FormHelper* form;
nanogui::Window* formWin;
nanogui::Button* saveInputLayoutButton;
GuiStatus guiStatus;

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
Pos dragStartPos;
Pos panOffsetScrPos;
Pos dragPin0BoardOffset;
std::string dragComponentName;
OglText dragText(DIAG_FONT_PATH, DRAG_FONT_SIZE);
void handleMouseDragOperations(const IntPos& mouseScrPos);
void renderDragStatus(const IntPos mouseScrPos);

// Shared objects
Layout inputLayout;
Layout currentLayout;
Layout bestLayout;

// Genetic Algorithm
#ifndef NDEBUG
const int N_ORGANISMS_IN_POPULATION = 10;
#else
const int N_ORGANISMS_IN_POPULATION = 1000;
#endif
const double CROSSOVER_RATE = 0.7;
const double MUTATION_RATE = 0.01;
GeneticAlgorithm geneticAlgorithm(
    N_ORGANISMS_IN_POPULATION, CROSSOVER_RATE, MUTATION_RATE);

// Misc
Render render;
void resetInputLayout();
nanogui::Button* saveBestLayoutButton;
std::string CIRCUIT_FILE_PATH = "./circuits/example.circuit";

// Status
Status status;
void printStats();
TrackAverage averageRenderTime(60);
TrackAverage averageFailedRoutes(N_ORGANISMS_IN_POPULATION);

// Run control
ThreadStop threadStopApp;
void runHeadless();
void runGui();
void exitApp();
volatile bool isParserPaused = true;

// Command line args
void parseCommandLineArgs(int argc, char** argv);
bool noGui;
bool useRandomSearch;
bool exitOnFirstComplete;
long exitAfterNumChecks;
long checkpointAtNumChecks;
std::string circuitFilePath;

class Application : public nanogui::Screen
{
  public:
  Application()
    : nanogui::Screen(
          IntPos(windowW, windowH), "Stripboard Autorouter",
          /*resizable*/ true, /*fullscreen*/ false,
          /*colorBits*/ 8,
          /*alphaBits*/ 8, /*depthBits*/ 24, /*stencilBits*/ 8,
          /*nSamples*/ 4, /*glMajor*/ 3, /*glMinor*/ 3)
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

    diagText.openGLInit();
    dragText.openGLInit();
    render.openGLInit();

    guiStatus.init(this);

    form = new nanogui::FormHelper(this);
    formWin = form->addWindow(IntPos(10, 10), "Router");

    form->addGroup("Costs");
    {
      auto w = form->addVariable<int>(
          "Wire",
          [&](int v) {
            auto lock = inputLayout.scopeLock();
            inputLayout.settings.wire_cost = v;
            resetInputLayout();
          },
          [&]() {
            auto lock = inputLayout.scopeLock();
            return inputLayout.settings.wire_cost;
          });
      w->setSpinnable(true);
      w->setMinValue(1);
      w->setValueIncrement(1);
    }
    {
      auto w = form->addVariable<int>(
          "Strip",
          [&](int v) {
            auto lock = inputLayout.scopeLock();
            inputLayout.settings.strip_cost = v;
            resetInputLayout();
          },
          [&]() {
            auto lock = inputLayout.scopeLock();
            return inputLayout.settings.strip_cost;
          });
      w->setSpinnable(true);
      w->setMinValue(1);
      w->setValueIncrement(1);
    }
    {
      auto w = form->addVariable<int>(
          "Via",
          [&](int v) {
            auto lock = inputLayout.scopeLock();
            inputLayout.settings.via_cost = v;
            resetInputLayout();
          },
          [&]() {
            auto lock = inputLayout.scopeLock();
            return inputLayout.settings.via_cost;
          });
      w->setSpinnable(true);
      w->setMinValue(1);
      w->setValueIncrement(1);
    }
    {
      auto w = form->addVariable<int>(
          "Cut",
          [&](int v) {
            auto lock = inputLayout.scopeLock();
            inputLayout.settings.cut_cost = v;
            resetInputLayout();
          },
          [&]() {
            auto lock = inputLayout.scopeLock();
            return inputLayout.settings.cut_cost;
          });
      w->setSpinnable(true);
      w->setMinValue(1);
      w->setValueIncrement(1);
    }
    form->addGroup("Display");
    form->addVariable("Rat's Nest", isShowRatsNestEnabled);
    form->addVariable("Only Failed", isShowOnlyFailedEnabled);
    form->addVariable("Current", isShowCurrentEnabled);
    {
      zoomSlider = new nanogui::Slider(formWin);
      form->addWidget("Zoom", zoomSlider);
      zoomSlider->setRange(std::make_pair(ZOOM_MIN, ZOOM_MAX));
      zoomSlider->setValue(ZOOM_DEF);
      zoomSlider->setFixedWidth(70);
      zoomSlider->setCallback([this](float value) {
        zoomLinear = value;
        auto upperLeftPos =
            boardToScrPos(Pos(0.0f, 0.0f), zoom, panOffsetScrPos);
        auto lowerRightPos = boardToScrPos(
            Pos(inputLayout.gridW, inputLayout.gridH), zoom, panOffsetScrPos);
        auto centerBoardPos =
            upperLeftPos + (lowerRightPos - upperLeftPos) / 2.0f;
        setZoomPan(centerBoardPos);
      });
    }
    form->addGroup("Misc");
    {
      form->addVariable<bool>(
          "Pause",
          [&](bool v) {
            auto lock = inputLayout.scopeLock();
            inputLayout.settings.pause = v;
          },
          [&]() {
            auto lock = inputLayout.scopeLock();
            return inputLayout.settings.pause;
          });
    }
    form->addGroup("Input Layout");
    {
      saveInputLayoutButton = form->addButton("Save to .circuit file", []() {
        CircuitFileWriter fileWriter;
        fileWriter.updateComponentPositions(
            circuitFilePath, inputLayout.circuit);
        saveInputLayoutButton->setEnabled(false);
      });
      saveInputLayoutButton->setEnabled(false);
    }
    form->addGroup("Best Layout");
    {
      saveBestLayoutButton = form->addButton("Save to .svg files", [this]() {
        SvgWriter svgWriter(bestLayout);
        auto svgPathVec = svgWriter.writeFiles(circuitFilePath);
        std::stringstream ss;
        ss << "Wrote .svg (Scalable Vector Graphics) files:\n\n";
        for (auto& p : svgPathVec) {
          ss << p << "\n";
        }
        new nanogui::MessageDialog(
            this, nanogui::MessageDialog::Type::Information, "Saved .svg files",
            ss.str());
      });
      saveBestLayoutButton->setEnabled(true);
    }
    performLayout();
    resizeEvent(IntPos(windowW, windowH));
  }

  ~Application()
  {
  }

  // Could not get NanoGUI mouseDragEvent to trigger so rolling my own.
  // See NanoGUI src/window.cpp for example mouseDragEvent handler.
  virtual bool mouseButtonEvent(
      const IntPos& p, int button, bool down, int modifiers)
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
        componentName = getComponentAtBoardPos(
            inputLayout.circuit,
            getMouseBoardPos(mousePos(), zoom, panOffsetScrPos));
      }
      if (componentName != "") {
        // Start component drag
        isComponentDragActive = true;
        dragComponentName = componentName;
        {
          auto lock = inputLayout.scopeLock();
          auto pin0BoardPos =
              inputLayout.circuit.componentNameToComponentMap[componentName]
                  .pin0AbsPos.cast<float>();
          dragPin0BoardOffset =
              getMouseBoardPos(mousePos(), zoom, panOffsetScrPos)
              - pin0BoardPos;
        }
      }
      else {
        // Start board drag
        isBoardDragActive = true;
        isZoomPanAdjusted = true;
      }
    }
    else {
      // End component drag
      if (isComponentDragActive) {
        isComponentDragActive = false;
        saveInputLayoutButton->setEnabled(true);
        {
          auto lock = inputLayout.scopeLock();
          resetInputLayout();
        }
      }
      // End board drag
      isBoardDragActive = false;
    }
    return true;
  }

  virtual bool scrollEvent(const IntPos& p, const Eigen::Vector2f& rel)
  {
    if (nanogui::Widget::scrollEvent(p, rel)) {
      // Event was handled by NanoGUI.
      return true;
    }
    zoomLinear += rel.y() * ZOOM_MOUSE_WHEEL_STEP;
    // Mouse wheel zoom towards or away from current mouse position
    setZoomPan(getMouseScrPos(mousePos()));
    // zoom towards center of board
    //    auto lock = inputLayout.scopeLock();
    //    Pos centerBoardPos(inputLayout.gridW / 2.0f, inputLayout.gridH /
    //    2.0f);
    //    setZoomPan(boardToScrPos(centerBoardPos, zoom, panOffsetScrPos));
    return true;
  }

  void setZoomPan(Pos scrPos)
  {
    zoomLinear = clamp(zoomLinear, ZOOM_MIN, ZOOM_MAX);
    zoomSlider->setValue(zoomLinear);
    auto beforeMouseBoardPos = screenToBoardPos(scrPos, zoom, panOffsetScrPos);
    zoom = expf(zoomLinear);
    auto afterMouseBoardPos = screenToBoardPos(scrPos, zoom, panOffsetScrPos);
    panOffsetScrPos -=
        boardToScrPos(beforeMouseBoardPos, zoom, panOffsetScrPos)
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

  virtual bool resizeEvent(const IntPos& size)
  {
    windowW = size.x();
    windowH = size.y();
    projMat = glm::ortho(
        0.0f, static_cast<float>(size.x()), static_cast<float>(size.y()), 0.0f,
        0.0f, 100.0f);
    glViewport(0, 0, size.x(), size.y());
    // Move the GUI window to the upper right corner
    auto w = formWin->width();
    formWin->setPosition(IntPos(size.x() - w, 0));
    // Status window
    guiStatus.moveToUpperLeft();
    return false;
  }

  // Update and draw the NanoGUI user interface
  virtual void draw(NVGcontext* ctx)
  {
    // Render
    auto avgRenderingSec = averageRenderTime.calcAverage();
    guiStatus.msPerFrame = avgRenderingSec * 1000;
    // Status
    {
      auto inputLock = inputLayout.scopeLock();
      std::lock_guard<std::mutex> lockStatus(statusMutex);
      auto nowTimestamp = std::chrono::high_resolution_clock::now();
      auto layoutElapsed = nowTimestamp - inputLayout.getBaseTimestamp();
      auto layoutElapsedSec =
          std::chrono::duration_cast<std::chrono::milliseconds>(layoutElapsed)
              .count()
          / 1000.0f;
      guiStatus.nCombinationsChecked = status.nCombinationsChecked;
      if (layoutElapsedSec > 0) {
        guiStatus.nCheckedPerSec =
            status.nCombinationsChecked / layoutElapsedSec;
      }
    }
    // Current
    {
      auto inputLock = inputLayout.scopeLock();
      auto currentLock = currentLayout.scopeLock();
      if (currentLayout.isBasedOn(inputLayout)) {
        guiStatus.nCurrentCompletedRoutes = currentLayout.nCompletedRoutes;
        guiStatus.currentCost = currentLayout.cost;

        averageFailedRoutes.addValue(currentLayout.nFailedRoutes);
        guiStatus.nCurrentFailedRoutes = averageFailedRoutes.calcAverage();
      }
      else {
        guiStatus.nCurrentCompletedRoutes = 0;
        guiStatus.nCurrentFailedRoutes = 0;
        guiStatus.currentCost = 0;
      }
    }
    // Best
    {
      auto inputLock = inputLayout.scopeLock();
      auto bestLock = bestLayout.scopeLock();
      if (bestLayout.isBasedOn(inputLayout)) {
        guiStatus.nBestCompletedRoutes = bestLayout.nCompletedRoutes;
        guiStatus.nBestFailedRoutes = bestLayout.nFailedRoutes;
        guiStatus.bestCost = bestLayout.cost;
      }
      else {
        guiStatus.nBestCompletedRoutes = 0;
        guiStatus.nBestFailedRoutes = 0;
        guiStatus.bestCost = 0;
      }
    }
    guiStatus.refresh();

    nanogui::Screen::draw(ctx);
  }

  // Draw app contents (layouts, routes)
  virtual void drawContents()
  {
    centerBoard();
    double drawStartTime = glfwGetTime();

    glBindVertexArray(vertexArrayId);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    handleMouseDragOperations(mousePos());

    // Select which layout to render and render a copy of it to avoid locking
    // the layout during the render time, which could hold up the router
    // threads.
    {
      Layout layout;
      if (isComponentDragActive) {
        layout = inputLayout.threadSafeCopy();
      }
      else if (isShowCurrentEnabled && currentLayout.isBasedOn(inputLayout)) {
        layout = currentLayout.threadSafeCopy();
      }
      else if (bestLayout.isBasedOn(inputLayout)) {
        layout = bestLayout.threadSafeCopy();
      }
      else {
        layout = inputLayout.threadSafeCopy();
      }

      if (!layout.circuit.hasParserError()) {
        // Render the selected layout
        render.draw(
            layout, projMat, panOffsetScrPos,
            getMouseBoardPos(mousePos(), zoom, panOffsetScrPos), zoom, windowW,
            windowH, isShowRatsNestEnabled || isComponentDragActive,
            isShowOnlyFailedEnabled && !isComponentDragActive);
      }
    }

    // Needed for timing but can be bad for performance.
    glFinish();

    renderDragStatus(mousePos());

    double drawEndTime = glfwGetTime();
    averageRenderTime.addValue(drawEndTime - drawStartTime);

    // Circuit file errors
    {
      auto lock = inputLayout.scopeLock();
      if (inputLayout.circuit.hasParserError()) {
        int nLine = 0;
        diagText.print(projMat, 0, 0, nLine++, "Circuit file parsing errors:");
        for (auto s : inputLayout.circuit.parserErrorVec) {
          diagText.print(projMat, 0, 0, nLine++, s);
        }
      }
    }

    checkGlError();

    //    // Video
    //    // avconv -framerate 60 -i video/frame_%05d.tga -c:v libx264 -pix_fmt
    //    yuv420p -r 60 -crf 16 video.mp4
    //    static int frameIdx = 0;
    //    static int intervalIdx = 0;
    //    if (!(intervalIdx++ % 20)) {
    //      auto tgaFileName = fmt::format("./video/frame_{:05d}.tga",
    //      frameIdx++);
    //      saveScreenshot(tgaFileName, windowW, windowH);
    //      fmt::print("{} {} {}\n", intervalIdx, frameIdx, tgaFileName);
    //    }
  }

  private:
  GLuint vertexArrayId;
};

void handleMouseDragOperations(const IntPos& mousePos)
{
  auto mouseScrPos = getMouseScrPos(mousePos);
  auto lock = inputLayout.scopeLock();

  if (!inputLayout.isReadyForRouting) {
    return;
  }

  if (isComponentDragActive) {
    // Prevent dragging outside of grid
    auto mouseBoardPos = getMouseBoardPos(mousePos, zoom, panOffsetScrPos);
    Via v = (mouseBoardPos - dragPin0BoardOffset + 0.5f).cast<int>();
    auto component =
        inputLayout.circuit.componentNameToComponentMap[dragComponentName];
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
    resetInputLayout();
  }
  // Drag board
  else if (isBoardDragActive) {
    panOffsetScrPos = mouseScrPos - dragStartPos;
  }
}

void renderDragStatus(const IntPos mouseScrPos)
{
  if (isComponentDragActive) {
    auto mouseBoardPos = getMouseBoardPos(mouseScrPos, zoom, panOffsetScrPos);
    Via mouseVia = (mouseBoardPos - dragPin0BoardOffset + 0.5f).cast<int>();
    dragText.print(
        projMat, mouseScrPos.x(), mouseScrPos.y(), 0,
        fmt::format("({},{})", mouseVia.x(), mouseVia.y()));
  }
}

void centerBoard()
{
  if (isZoomPanAdjusted || !inputLayout.isReadyForRouting) {
    return;
  }
  float zoomW =
      (windowW - INITIAL_BORDER_PIXELS) / static_cast<float>(inputLayout.gridW);
  float zoomH =
      (windowH - INITIAL_BORDER_PIXELS) / static_cast<float>(inputLayout.gridH);
  zoom = std::min(zoomW, zoomH);
  float boardScreenW = (inputLayout.gridW - 1) * zoom;
  float boardScreenH = (inputLayout.gridH - 1) * zoom;
  panOffsetScrPos =
      Pos(windowW / 2.0f - boardScreenW / 2.0f,
          windowH / 2.0f - boardScreenH / 2.0f);
  zoomLinear = logf(zoom);
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
    int orderingIdx = -1;
    ConnectionIdxVec connectionIdxVec;
    if (useRandomSearch) {
      for (int i = 0;
           i < static_cast<int>(threadLayout.circuit.connectionVec.size());
           ++i) {
        connectionIdxVec.push_back(i);
      }
      random_shuffle(connectionIdxVec.begin(), connectionIdxVec.end());
    }
    else {
      {
        auto lock = geneticAlgorithm.scopeLock();
        orderingIdx = geneticAlgorithm.reserveOrdering();
        if (orderingIdx != -1) {
          connectionIdxVec = geneticAlgorithm.getOrdering(orderingIdx);
        }
      }
      if (orderingIdx == -1) {
        std::this_thread::sleep_for(10ms);
        continue;
      }
    }
    {
      //  // Testing random costs, to vary how the best path is selected
      //  std::default_random_engine generator;
      //  std::uniform_int_distribution<int> distribution(1,100);
      //  layout_.settings.strip_cost = distribution(generator);
      //  layout_.settings.wire_cost = distribution(generator);
      //  layout_.settings.via_cost = distribution(generator);
      Router router(
          threadLayout, connectionIdxVec, threadStopRouter, inputLayout,
          currentLayout, maxRenderDelay);
      auto isAborted = router.route();
      // Ignore result if the routing was aborted or the input has changed.
      if (isAborted || !threadLayout.isBasedOn(inputLayout)) {
        continue;
      }
    }
    {
      std::lock_guard<std::mutex> lockStatus(statusMutex);
      ++status.nCombinationsChecked;
    }
    if (!useRandomSearch) {
      auto lock = geneticAlgorithm.scopeLock();
      geneticAlgorithm.releaseOrdering(
          orderingIdx, threadLayout.nCompletedRoutes, threadLayout.cost);
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
          threadLayout.nCompletedRoutes > bestLayout.nCompletedRoutes;
      auto hasEqualRoutesAndBetterScore =
          threadLayout.nCompletedRoutes == bestLayout.nCompletedRoutes
          && threadLayout.cost < bestLayout.cost;
      auto isBasedOnOtherLayout = !bestLayout.isBasedOn(threadLayout);
      if (hasMoreCompletedRoutes || hasEqualRoutesAndBetterScore
          || isBasedOnOtherLayout) {
        bestLayout = threadLayout;
      }
    }
    // Print status at interval
    if (checkpointAtNumChecks != -1) {
      std::lock_guard<std::mutex> lockStatus(statusMutex);
      if (!(status.nCombinationsChecked % checkpointAtNumChecks)) {
        printStats();
      }
    }
    // Automatic app exit on first completed layout
    if (exitOnFirstComplete) {
      auto bestLock = bestLayout.scopeLock();
      if (!bestLayout.nFailedRoutes) {
        exitApp();
      }
    }
    // Automatic app exit after given number of checks
    if (exitAfterNumChecks != -1) {
      std::lock_guard<std::mutex> lockStatus(statusMutex);
      if (status.nCombinationsChecked == exitAfterNumChecks) {
        exitApp();
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
    if (isParserPaused) {
      std::this_thread::sleep_for(100ms);
      continue;
    }
    try {
      curMtime = getMtime(circuitFilePath);
    } catch (std::string errorMsg) {
      {
        auto lock = inputLayout.scopeLock();
        inputLayout = Layout();
        inputLayout.circuit.parserErrorVec.push_back(errorMsg);
        resetInputLayout();
      }
      curMtime = 0.0;
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
        resetInputLayout();
      }
    }
    std::this_thread::sleep_for(500ms);
  }
}

void resetInputLayout()
{
  assert(inputLayout.isLocked());
  inputLayout.updateBaseTimestamp();
  status.nCombinationsChecked = 0;
  guiStatus.reset();
  {
    auto lock = geneticAlgorithm.scopeLock();
    geneticAlgorithm.reset(
        static_cast<int>(inputLayout.circuit.connectionVec.size()));
  }
}

int main(int argc, char** argv)
{
  //  fmt::print("GLFW: {}\n", glfwGetVersionString());
  std::srand(std::time(0));

  parseCommandLineArgs(argc, argv);

  launchRouterThreads();
  launchParserThread();

  try {
    if (noGui) {
      runHeadless();
    }
    else {
      runGui();
    }
  } catch (const std::runtime_error& e) {
    auto errorStr = fmt::format("Fatal error: {}", e.what());
    fmt::print(stderr, errorStr + "\n");
#if defined(_WIN32)
    MessageBoxA(nullptr, errorStr.c_str(), NULL, MB_ICONERROR | MB_OK);
#endif
    return -1;
  }

  stopParserThread();
  stopRouterThreads();

  if (noGui || exitOnFirstComplete || exitAfterNumChecks != -1) {
    printStats();
  }

  return 0;
}

void parseCommandLineArgs(int argc, char** argv)
{
  cli::Parser parser(argc, argv);

  parser.set_optional<bool>("n", "nogui", false, "Do not open the GUI window");
  parser.set_optional<bool>(
      "r", "random", false, "Use random search instead of genetic algorithm");
  parser.set_optional<bool>(
      "e", "exitcomplete", false,
      "Print stats and exit when first complete layout is found");
  parser.set_optional<long>(
      "a", "exitafter", -1,
      "Print stats and exit after specified number of checks");
  parser.set_optional<long>("p", "checkpoint", -1, "Print stats at interval");
  parser.set_optional<std::string>(
      "c", "circuit", CIRCUIT_FILE_PATH, "Path to .circuit file");
  // parser.set_required<std::vector<short>>("v", "values", "By using a vector
  // it is possible to receive a multitude of inputs.");

  parser.run_and_exit_if_error();

  noGui = parser.get<bool>("n");
  useRandomSearch = parser.get<bool>("r");
  exitOnFirstComplete = parser.get<bool>("e");
  exitAfterNumChecks = parser.get<long>("a");
  checkpointAtNumChecks = parser.get<long>("p");
  circuitFilePath = parser.get<std::string>("c");
  // auto values = parser.get<std::vector<short>>("v");
}

void runHeadless()
{
  isParserPaused = false;
  while (!threadStopApp.isStopped()) {
    std::this_thread::sleep_for(100ms);
  }
}

void runGui()
{
  nanogui::init();
  setlocale(LC_NUMERIC, "");
  nanogui::ref<Application> app = new Application();
  app->setVisible(true);
  setWindowIcon("./icons/48x48.png");
  isParserPaused = false;

  nanogui::mainloop();

  delete form;
  guiStatus.free();
  nanogui::shutdown();
}

void exitApp()
{
  nanogui::leave();
  threadStopApp.stop();
}

void printStats()
{
  auto inputLock = inputLayout.scopeLock();
  auto bestLock = bestLayout.scopeLock();
  fmt::print(
      "search={} nChecks={:n} Best: nCompletedRoutes={:n} nFailedRoutes={:n} "
      "cost={:n}\n",
      useRandomSearch ? "random" : "GA", status.nCombinationsChecked,
      bestLayout.nCompletedRoutes, bestLayout.nFailedRoutes, bestLayout.cost);
}
