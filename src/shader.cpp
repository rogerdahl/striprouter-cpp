#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "shader.h"
#include "utils.h"

const std::string SHADERS_DIR_PATH = "./shaders/";

void compileAndCheckShader(const char* path, GLuint shaderId);
void printInfoLog(GLuint shaderId, bool isShader);
std::string loadFile(const char* path);

GLuint createProgram(
    const char* vertexShaderFileName, const char* fragmentShaderFileName)
{
  std::string vertexShaderPath =
      joinPath(SHADERS_DIR_PATH, vertexShaderFileName);
  std::string fragmentShaderPath =
      joinPath(SHADERS_DIR_PATH, fragmentShaderFileName);

  GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
  compileAndCheckShader(vertexShaderPath.c_str(), vertexShaderId);

  GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
  compileAndCheckShader(fragmentShaderPath.c_str(), fragmentShaderId);

  GLuint programId = glCreateProgram();
  glAttachShader(programId, vertexShaderId);
  glAttachShader(programId, fragmentShaderId);
  glLinkProgram(programId);

  GLint isSuccessful;
  glGetProgramiv(programId, GL_LINK_STATUS, &isSuccessful);

  printInfoLog(programId, false);

  if (!isSuccessful) {
    throw std::runtime_error("Shader linking failed");
  }

  glDetachShader(programId, vertexShaderId);
  glDetachShader(programId, fragmentShaderId);
  glDeleteShader(vertexShaderId);
  glDeleteShader(fragmentShaderId);

  return programId;
}

void compileAndCheckShader(const char* path, GLuint shaderId)
{
  //    cout << "Compiling shader: " << path << endl;

  auto shaderStr = loadFile(path);
  auto shaderStrPtr = shaderStr.c_str();
  glShaderSource(shaderId, 1, &shaderStrPtr, NULL);
  glCompileShader(shaderId);

  GLint isSuccessful;

  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isSuccessful);
  if (isSuccessful == GL_FALSE) {
    std::cerr << "Compile failed for shader: " << path << std::endl;
  }

  printInfoLog(shaderId, true);

  if (!isSuccessful) {
    throw std::runtime_error("Shader compile failed");
  }
}

void printInfoLog(GLuint id, bool isShader)
{
  GLint logLen;
  auto glGetiv = isShader ? glGetShaderiv : glGetProgramiv;
  glGetiv(id, GL_INFO_LOG_LENGTH, &logLen);
  if (logLen > 1) {
    std::vector<GLchar> logVec(logLen);
    auto glGetInfoLog = isShader ? glGetShaderInfoLog : glGetProgramInfoLog;
    glGetInfoLog(id, logLen, NULL, &logVec[0]);
    for (auto const& c : logVec) {
      std::cout << c;
    }
    std::cout << std::endl;
  }
}

std::string loadFile(const char* path)
{
  std::ifstream inStream(path, std::ios::in);
  if (inStream.fail()) {
    // raise
  }
  std::stringstream ss;
  ss << inStream.rdbuf();
  return ss.str();
}
