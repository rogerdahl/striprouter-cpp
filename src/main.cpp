// Style: http://geosoft.no/development/cppstyle.html

#include <cstdio>
#include <ctime>
#include <iostream>
//#include <vector>
#include <ctime>

#include <boost/filesystem.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtx/string_cast.hpp>
#include <fmt/format.h>

//#include "shader.h"
#include "utils.h"
#include "ogl_text.h"
#include "gl_error.h"
#include "pcb.h"
#include "parser.h"

using namespace std;
using namespace boost::filesystem;

GLFWwindow *window;

u32 window_w = 800;
u32 window_h = 800;
bool fullScreen = false;
const char *FONT_PATH = "./fonts/LiberationMono-Regular.ttf";
const u32 FONT_SIZE = 18;


int main(int argc, char **argv)
{
  path recDirPath;
  if (argc == 2) {
    recDirPath = argv[1];
  }

  srand((unsigned int) time(NULL));

  // Initialise GLFW OpenGL windowing framework.
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  if (!glfwJoystickPresent(GLFW_JOYSTICK_1)) {
    cerr << "Joystick not found" << endl;
  }

  // Create window and OpenGL context.
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//  glfwWindowHint(GLFW_ALPHA_BITS, 8);
  GLFWmonitor *glfwMonitor = fullScreen ? glfwGetPrimaryMonitor() : NULL;
  window = glfwCreateWindow(window_w, window_h, "Stripboard Autorouter", glfwMonitor, NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to create GLFW window.\n");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Initialize GLEW.
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    glfwTerminate();
    return -1;
  }

  // Clear GL Error
  glGetError();

  // Cause glfwGetKey() to return a GLFW_PRESS on first call even if the key has already been released.
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  // Alpha
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
//  glEnable(GL_DEPTH_TEST);
//  glDepthFunc(GL_LESS);
//  glEnable(GL_CULL_FACE);


  // State is automatically stored here.
  GLuint vertexArrayId;
  glGenVertexArrays(1, &vertexArrayId);
  glBindVertexArray(vertexArrayId);

//  float aspect_ratio = (float) window_w / (float) window_h;

  OglText oglText(window_w, window_h, FONT_PATH, FONT_SIZE);

  averageSec averageRendering;

  // Switch back to regular frame buffer.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, window_w, window_h);

  PcbDraw pcbDraw(window_w, window_h);

  Parser parser;

  std::time_t mtime_prev = 0;

  string line;
  string error;

  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {
//    break; //////////////////////

    std::time_t mtime_cur = boost::filesystem::last_write_time("./circuit.txt");
    if (mtime_cur != mtime_prev) {
      mtime_prev = mtime_cur;
      parser = Parser();
      auto lineErrorPair = parser.parse();
      if (lineErrorPair.second != "Ok") {
        parser = Parser();
      }
      line = lineErrorPair.first;
      error = lineErrorPair.second;
    }

    glDisable(GL_DEPTH_TEST);

    glfwSetTime(0.0);

    // Check that framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      fmt::print(stderr, "Invalid framebuffer\n");
      exit(0);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    pcbDraw.draw(parser);


    // Can be very bad for performance.
    glFinish();

    averageRendering.addSec(glfwGetTime());
    glfwSetTime(0.0);

    auto avgRenderingSec = averageRendering.calcAverage();

    auto nLine = 0;
    oglText.print(0, 0, nLine++, fmt::format("Render: {:.1f}ms", avgRenderingSec * 1000));
    oglText.print(0, 0, nLine++, line);
    oglText.print(0, 0, nLine++, error);

    glfwSwapBuffers(window);

    checkGlError();

    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &vertexArrayId);

  glfwTerminate();

  return 0;
}
