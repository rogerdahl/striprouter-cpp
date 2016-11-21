cmake_minimum_required(VERSION 3.5)
project(striprouter)

# https://cmake.org/cmake/help/v3.0/policy/CMP0015.html
cmake_policy(SET "CMP0015" "NEW")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LIBRARIES ${CMAKE_SOURCE_DIR}/libraries/linux)
else ()
  set(LIBRARIES ${CMAKE_SOURCE_DIR}/libraries/win64)
endif ()

# GLFW
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(GLFW_LIBRARIES glfw)
else ()
  set(GLFW_LIBRARIES glfw3.lib)
  set(GLFW_INCLUDE_DIRS ${LIBRARIES}/glfw-3.2.1.bin.WIN64/include)
  set(GLFW_LINK_DIRS ${LIBRARIES}/glfw-3.2.1.bin.WIN64/lib-vc2015)
endif ()

# fmt
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(FMT_LIBRARIES fmt)
  set(FMT_INCLUDE_DIR ${LIBRARIES}/fmt-3.0.0)
  set(FMT_LINK_DIR ${LIBRARIES}/fmt-3.0.0/fmt)
else ()
  set(FMT_LIBRARIES fmt.lib)
  set(FMT_INCLUDE_DIR ${LIBRARIES}/fmt-3.0.1)
  set(FMT_LINK_DIR ${LIBRARIES}/fmt-3.0.1/builds/fmt/Release)
endif ()

# FreeType2
#CPPFLAGS += $(shell freetype-config --cflags)
#LDFLAGS += $(shell freetype-config --libs)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  find_package(Freetype REQUIRED)
  set(FREETYPE_LINK_DIRS "")
else ()
  set(FREETYPE_LIBRARIES freetype.lib)
  set(FREETYPE_INCLUDE_DIRS ${LIBRARIES}/freetype-2.6.3/include)
  set(FREETYPE_LINK_DIRS ${LIBRARIES}/freetype-2.6.3/builds/Release)
endif ()

# Boost
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  # https://cmake.org/cmake/help/v3.0/module/FindBoost.html
  find_package(Boost 1.58.0 REQUIRED)
  set(BOOST_LIBRARIES boost_filesystem boost_system)
else ()
  set(BOOST_LIBRARIES libboost_filesystem-vc140-mt-1_60 libboost_system-vc140-mt-1_60)
  set(BOOST_INCLUDE_DIR ${LIBRARIES}/boost_1_60_0)
  set(BOOST_LINK_DIR ${LIBRARIES}/boost_1_60_0/stage/lib)
endif ()

# glm
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
else ()
  set(GLM_INCLUDE_DIR ${LIBRARIES}/glm-0.9.7.4/glm)
endif ()

# GLEW
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(GLEW_LIBRARIES GLEW)
else ()
  set(GLEW_LIBRARIES glew32.lib)
  set(GLEW_INCLUDE_DIRS ${LIBRARIES}/glew-1.13.0/include)
  set(GLEW_LINK_DIR ${LIBRARIES}/glew-1.13.0/lib/Release/x64)
endif ()

# OpenGL and GLU
find_package(OpenGL REQUIRED)

# X
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(X_LIBRARIES Xrandr X11 Xxf86vm Xinerama Xcursor Xi)
else ()
  set(X_LIBRARIES "")
endif ()

# GCC
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(GCC_LIBRARIES m pthread dl)
else ()
  set(GCC_LIBRARIES "")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wfatal-errors -Wno-deprecated-declarations")
endif ()

# NanoGUI
set(NANO_INCLUDE_DIR
  ${LIBRARIES}/nanogui/include
  ${LIBRARIES}/nanogui/ext/eigen
  ${LIBRARIES}/nanogui/ext/nanovg/src
)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(NANO_LIBRARIES nanogui)
  set(NANO_LINK_DIR ${LIBRARIES}/nanogui/lib)
else ()
  set(NANO_LIBRARIES nanogui.lib)
  set(NANO_LINK_DIR ${LIBRARIES}/nanogui/build/Release)
endif ()

# Eigen
#find_package(Eigen3 REQUIRED)

set(SOURCE_FILES
  ${SOURCE_DIR}/circuit.cpp
  ${SOURCE_DIR}/dijkstra.cpp
  ${SOURCE_DIR}/gl_error.cpp
  ${SOURCE_DIR}/main.cpp
  ${SOURCE_DIR}/ogl_text.cpp
  ${SOURCE_DIR}/parser.cpp
  ${SOURCE_DIR}/render.cpp
  ${SOURCE_DIR}/shader.cpp
  ${SOURCE_DIR}/solution.cpp
  ${SOURCE_DIR}/utils.cpp
  ${SOURCE_DIR}/via.cpp
)

include_directories(
  ${BOOST_INCLUDE_DIR}
  ${EIGEN3_INCLUDE_DIR}
  ${FMT_INCLUDE_DIR}
  ${FREETYPE_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${GLM_INCLUDE_DIR}
  ${NANO_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
)

link_directories(
  ${BOOST_LINK_DIR}
  ${FMT_LINK_DIR}
  ${FREETYPE_LINK_DIRS}
  ${GLEW_LINK_DIR}
  ${GLFW_LINK_DIRS}
  ${NANO_LINK_DIR}
)

link_libraries(
  ${BOOST_LIBRARIES}
  ${FMT_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${GCC_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${NANO_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${X_LIBRARIES}
)

# Suppress GLM warning about having switched from degrees to radians as default
add_definitions(-DGLM_FORCE_RADIANS)

# Copy shaders to binary dir
# Apparently, this can't be done with glob or directory.
set(SHADER_SRC ${SOURCE_DIR}/shaders)
set(SHADER_DST ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
file(MAKE_DIRECTORY ${SHADER_DST})
configure_file(${SHADER_SRC}/text_background.frag ${SHADER_DST} COPYONLY)
configure_file(${SHADER_SRC}/text_background.vert ${SHADER_DST} COPYONLY)
configure_file(${SHADER_SRC}/text.frag ${SHADER_DST} COPYONLY)
configure_file(${SHADER_SRC}/text.vert ${SHADER_DST} COPYONLY)
configure_file(${SHADER_SRC}/fill.frag ${SHADER_DST} COPYONLY)
configure_file(${SHADER_SRC}/fill.vert ${SHADER_DST} COPYONLY)

# Copy fonts to binary dir
set(FONT_SRC ${CMAKE_SOURCE_DIR}/fonts)
set(FONT_DST ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fonts)
file(MAKE_DIRECTORY ${FONT_DST})
configure_file(${FONT_SRC}/LiberationMono-Regular.ttf ${FONT_DST} COPYONLY)

add_executable(striprouter ${SOURCE_FILES})
