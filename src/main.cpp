// Style: http://geosoft.no/development/cppstyle.html

#include <cstdio>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <climits>

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <nanogui/nanogui.h>

#include "dijkstra.h"
#include "gl_error.h"
#include "gui.h"
#include "ogl_text.h"
#include "render.h"
#include "status.h"
#include "utils.h"


using namespace std;
using namespace boost::filesystem;
using namespace nanogui;


u32 windowW = 1920 / 2;
u32 windowH = 1080 - 200;
//const u32 windowW = 700;
//const u32 windowH = 700;

const u32 GRID_W = 60;
const u32 GRID_H = 60;

const char *DIAG_FONT_PATH = "./fonts/LiberationMono-Regular.ttf";
const u32 DIAG_FONT_SIZE = 18;
const u32 DRAG_FONT_SIZE = 14;

const float ZOOM_MOUSE_WHEEL_STEP = 0.1f;
const float ZOOM_MIN = -2.0f;
const float ZOOM_MAX = 4.0f;
const float ZOOM_DEF = 0.3f;


averageSec averageRendering;
mutex stopThreadMutex;
static bool showInputBool = false;
static bool showBestBool = false;
std::time_t mtime_prev = 0;
thread routeThread;
float zoomLin = ZOOM_DEF;
float zoom = expf(zoomLin);


Circuit circuit;
OglText oglText(DIAG_FONT_PATH, DIAG_FONT_SIZE);
Render pcbDraw(zoom);
Settings settings;
Solution solution;
Status mystatus;

Circuit bestCircuit;
Solution bestSolution;
Status bestStatus;


void launchRouterThread();
void stopRouterThread();
bool isStopRequested(mutex& stopThreadMutex);
void router();
void resetStatus();


bool dragComponentIsActive = false;
bool dragBoardIsActive = false;
Pos dragStartCoord;
Pos dragBoardOffset;
Pos dragPin0Offset;
string dragComponentName;
Pos mouseScreenPos;
OglText dragText(DIAG_FONT_PATH, DRAG_FONT_SIZE);


class Application : public nanogui::Screen
{
public:
  Application()
    : nanogui::Screen(
    Eigen::Vector2i(windowW, windowH),
    "Stripboard Autorouter",
    true, // resizable
    false, // fullscreen
    8, // colorBits
    8, // alphaBits
    24, // depthBits
    8, // stencilBits
    4, // nSamples
    3, // glMajor
    3 // glMinor
  )
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

    oglText.OpenGLInit();
    pcbDraw.OpenGLInit();
    dragText.OpenGLInit();

    // Main grid window
    auto window = new Window(this, "Router");
    window->setPosition(Vector2i(windowW - 400, windowH - 300));
    GridLayout *layout = new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 15, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    window->setLayout(layout);

//    mProgress = new ProgressBar(window);
    {
      new Label(window, "Wire cost:", "sans-bold");
      auto intBox = new IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Vector2i(100, 20));
      intBox->setValue(settings.wire_cost);
      intBox->setDefaultValue("0");
      intBox->setFontSize(18);
      intBox->setFormat("[1-9][0-9]*");
      intBox->setSpinnable(true);
      intBox->setMinValue(1);
      intBox->setValueIncrement(1);
      intBox->setCallback([intBox](u32 value) {
        {
          lock_guard<mutex> lockSettings(settingsMutex);
          settings.wire_cost = value;
        }
        resetStatus();
      });
    }
    {
      new Label(window, "Strip cost:", "sans-bold");
      auto intBox = new IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Vector2i(100, 20));
      intBox->setValue(settings.strip_cost);
      intBox->setDefaultValue("0");
      intBox->setFontSize(18);
      intBox->setFormat("[1-9][0-9]*");
      intBox->setSpinnable(true);
      intBox->setMinValue(1);
      intBox->setValueIncrement(1);
      intBox->setCallback([intBox](u32 value) {
        {
          lock_guard<mutex> lockSettings(settingsMutex);
          settings.strip_cost = value;
        }
        resetStatus();
      });
    }
    {
      new Label(window, "Via cost:", "sans-bold");
      auto intBox = new IntBox<int>(window);
      intBox->setEditable(true);
      intBox->setFixedSize(Vector2i(100, 20));
      intBox->setValue(settings.via_cost);
      intBox->setDefaultValue("0");
      intBox->setFontSize(18);
      intBox->setFormat("[1-9][0-9]*");
      intBox->setSpinnable(true);
      intBox->setMinValue(1);
      intBox->setValueIncrement(1);
      intBox->setCallback([intBox](u32 value) {
        {
          lock_guard<mutex> lockSettings(settingsMutex);
          settings.via_cost = value;
        }
        resetStatus();
      });
    }
    {
      new Label(window, "Show input:", "sans-bold");
      auto cb = new CheckBox(window, "");
      cb->setFontSize(18);
      cb->setChecked(showInputBool);
      cb->setCallback([cb](bool value) {
        showInputBool = value;
      });
    }
    {
      new Label(window, "Show best:", "sans-bold");
      auto cb = new CheckBox(window, "");
      cb->setFontSize(18);
      cb->setChecked(showBestBool);
      cb->setCallback([cb](bool value) {
        showBestBool = value;
      });
    }
    {
      new Label(window, "Zoom:", "sans-bold");

      Widget *panel = new Widget(window);
      panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 5));

      Slider *slider = new Slider(panel);
      slider->setValue(zoom / 4.0f);
      slider->setFixedWidth(100);

      TextBox *textBox = new TextBox(panel);
      textBox->setFixedSize(Vector2i(50, 20));
      textBox->setValue("50");
      textBox->setUnits("%");
      slider->setCallback([textBox](float value) {
        textBox->setValue(std::to_string((int)(value * 200)));
        ::zoom = value * 4.0f;
      });
//      slider->setFinalCallback([&](float value) {
//        ::zoom = value * 4.0f;
//      });
      textBox->setFixedSize(Vector2i(60,25));
      textBox->setFontSize(18);
      textBox->setAlignment(TextBox::Alignment::Right);
    }
    performLayout();
    launchRouterThread();
  }


  ~Application()
  {
    stopRouterThread();
  }

  // Could not get NanoGUI mouseDragEvent to trigger.
  // See NanoGUI src/window.cpp for example mouseDragEvent handler.
  virtual bool
  mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers)
  {
    if (Widget::mouseButtonEvent(p, button, down, modifiers)) {
      // Event was handled by NanoGUI.
      return true;
    }
    if (down) {
      dragStartCoord = mouseScreenPos - dragBoardOffset;
      string componentName;
      {
        lock_guard<mutex> lockCircuit(circuitMutex);
        componentName = getComponentAtMouseCoordinate(pcbDraw, circuit, mouseScreenPos);
      }
      if (componentName != "") {
        // Start component drag
        dragComponentIsActive = true;
        dragComponentName = componentName;
        auto pin0ScreenCoord = pcbDraw.boardToScreenCoord(
          circuit.componentNameToInfoMap[componentName].pin0AbsCoord.cast<float>()
        );
        dragPin0Offset = mouseScreenPos - pin0ScreenCoord;
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
    if (Widget::scrollEvent(p, rel)) {
      // Event was handled by NanoGUI.
      return true;
    }
    zoomLin += rel.y() * ZOOM_MOUSE_WHEEL_STEP;
    zoomLin = max(zoomLin, ZOOM_MIN);
    zoomLin = min(zoomLin, ZOOM_MAX);
    zoom = expf(zoomLin);
    return true;
  }


  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers)
  {
    if (Screen::keyboardEvent(key, scancode, action, modifiers)) {
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
    Screen::draw(ctx);
  }


  virtual void drawContents()
  {
    mouseScreenPos = mousePos().cast<float>();
    glfwGetWindowSize(glfwWindow(), reinterpret_cast<int*>(&windowW), reinterpret_cast<int*>(&windowH));

    glBindVertexArray(vertexArrayId);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowW, windowH);

    std::time_t mtime_cur = boost::filesystem::last_write_time("./circuit.txt");
    if (mtime_cur != mtime_prev) {
      mtime_prev = mtime_cur;
      {
        lock_guard<mutex> lockCircuit(circuitMutex);
        auto parser = Parser();
        parser.parse(circuit);
      }
      resetStatus();
    }

    // Drag component
    if (dragComponentIsActive) {
      // Block component drag when showing best
      if (!showBestBool) {
        {
          lock_guard<mutex> lockCircuit(circuitMutex);
          setComponentPosition(pcbDraw, circuit, mouseScreenPos - dragPin0Offset, dragComponentName);
        }
        resetStatus();
      }
    }
    // Drag board
    if (dragBoardIsActive) {
      dragBoardOffset = mouseScreenPos;
      dragBoardOffset -= dragStartCoord;
    }

    glfwSetTime(0.0);

    {
      lock_guard<mutex> lockSolution(solutionMutex);
      if (showBestBool) {
        pcbDraw.draw(bestCircuit, bestSolution, windowW, windowH, GRID_W,
                     GRID_H, zoom, dragBoardOffset, showInputBool);
      }
      else {
        pcbDraw.draw(circuit, solution, windowW, windowH, GRID_W, GRID_H, zoom,
                     dragBoardOffset, showInputBool);
      }
    }

    if (dragComponentIsActive) {
      dragText.reset(windowW, windowH,
                    static_cast<u32>(mouseScreenPos.x()),
                    static_cast<u32>(mouseScreenPos.y()) - DRAG_FONT_SIZE,
                    DRAG_FONT_SIZE);
      if (showBestBool) {
        dragText.print(0, fmt::format("Can't edit -- showing best"));
      }
      else {
        auto via = circuit.componentNameToInfoMap[dragComponentName].pin0AbsCoord;
        dragText.print(0, fmt::format("({},{})", via.x(), via.y()));
      }
    }

    // Needed for timing but can be bad for performance.
    glFinish();

    averageRendering.addSec(glfwGetTime());

    auto avgRenderingSec = averageRendering.calcAverage();

    u32 nLine = 0;
    oglText.reset(windowW, windowH, 0, 0, DIAG_FONT_SIZE);
    oglText.print(nLine++, fmt::format("Render: {:.1f}ms", avgRenderingSec * 1000));
    for (auto s : circuit.circuitInfoVec) {
      oglText.print(nLine++, s);
    }
    oglText.print(nLine++, fmt::format("Completed: {:n}", solution.numCompletedRoutes));
    oglText.print(nLine++, fmt::format("Failed: {:n}", solution.numFailedRoutes));
    oglText.print(nLine++, fmt::format("Total cost: {:n}", solution.totalCost));
    if (showBestBool) {
      oglText.print(nLine++, fmt::format("Showing best: {:n}", bestStatus.bestCost));
    }
    else {
      if (mystatus.bestCost != INT_MAX) {
        oglText.print(nLine++, fmt::format("Best Cost: {:n}", mystatus.bestCost));
      }
      else {
        oglText.print(nLine++, fmt::format("Best Cost: <no complete routes>", mystatus.bestCost));
      }
    }
    oglText.print(nLine++, fmt::format("Checked: {:n}", mystatus.nChecked));

    checkGlError();
  }

private:
  nanogui::ProgressBar *mProgress;
  GLuint vertexArrayId;
};


void launchRouterThread()
{
  routeThread = thread(router); // , std::ref(solution)
}


void stopRouterThread()
{
  try {
    lock_guard<mutex> stopThread(stopThreadMutex);
    routeThread.join();
  }
  catch (const std::system_error &e) {
    fmt::print(stderr, "Attempted to stop thread that is not running\n");
  }
}


bool isStopRequested(mutex& stopThreadMutex)
{
  bool lockObtained = stopThreadMutex.try_lock();
  if (lockObtained) {
    stopThreadMutex.unlock();
  }
  return !lockObtained;
}


void router()
{
  while (true) {
    Circuit threadCircuit;
    Settings threadSettings;
    Solution threadSolution;
    {
      lock_guard<mutex> lockCircuit(circuitMutex);
      threadCircuit = ::circuit;
    }
    {
      lock_guard<mutex> lockSettings(settingsMutex);
      threadSettings = ::settings;
    }
    {
      Dijkstra dijkstra(GRID_W, GRID_H);
      dijkstra.route(threadSolution, threadSettings, threadCircuit, stopThreadMutex);
      if (isStopRequested(stopThreadMutex)) {
        break;
      }
    }
    {
      lock_guard<mutex> lockSolution(solutionMutex);
      ::solution = threadSolution;
    }
    {
      lock_guard<mutex> lockStatus(statusMutex);
      ++mystatus.nChecked;
      if (!threadSolution.numFailedRoutes) {
        if (threadSolution.totalCost < mystatus.bestCost) {
          mystatus.bestCost = threadSolution.totalCost;
          bestStatus = mystatus;
          {
            lock_guard<mutex> lockCircuit(circuitMutex);
            bestCircuit = threadCircuit;
          }
          {
            lock_guard<mutex> lockSolution(solutionMutex);
            bestSolution = threadSolution;
          }
        }
      }
    }
  }
}


void resetStatus()
{
  {
    lock_guard<mutex> lockCircuit(circuitMutex);
    bestCircuit = Circuit();
  }
  {
    lock_guard<mutex> lockStatus(statusMutex);
    mystatus = Status();
    bestStatus = Status();
  }
  {
    lock_guard<mutex> lockSolution(solutionMutex);
    bestSolution = Solution();
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
    std::cerr << error_msg << endl;
#endif
    return -1;
  }
  return 0;
}
