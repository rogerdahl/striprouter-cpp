## Stripboard Autorouter

<img align="right" width="50%" src="./screenshot.png">

**Note**: Not yet in a usable state.

This is a cross-platform program that, given a description of a circuit, searches for the best layout for the connections on a [stripboard](https://en.wikipedia.org/wiki/Stripboard).

The output is a description of where to place and solder wires and where to cut the existing stripboard traces in order to create the connections described in the circuit.

Costs can be assigned to resources in order to direct the autorouter towards preferred layouts. Resources include stripboard area, solder points, trace cuts and lengths of wire. E.g., setting the cost of solder points high in relation to other resources may generate designs that use fewer solder points at the expense of stripboard area.

The program searches only for solutions that use non-overlapping wires that cross the traces at 90 degree angles, which tends to give clean looking designs, and which allows using only uninsulated wires.   


Currently somewhat working:

* Parsing of circuit description file
* Visualization of circuit
* Simple automatic routing of point-to-point connections
* Visualization of discovered routes
* A basic GUI window

### Circuit description

The circuit description is a text file that can be edited while the program is running. Open the description in a text editor side by side with the renderer and hit save to update the rendered version.

This is the example circuit description that is rendered in the screenshot.

```
# Package
# pkg <package name> <pin coordinates relative to pin 0>
pkg dip14 0,0 1,0 2,0 3,0 4,0 5,0 6,0 6,-3 5,-3 4,-3 3,-3 2,-3 1,-3 0,-3
pkg header20 0,0 1,0 2,0 3,0 4,0 5,0 6,0 7,0 8,0 9,0 10,0 11,0 12,0 13,0 14,0 15,0 16,0 17,0 18,0 19,0

pkg header2x20 0,0 0,-1 1,0 1,-1 2,0 2,-1 3,0 3,-1 4,0 4,-1 5,0 5,-1 6,0 6,-1 7,0 7,-1 8,0 8,-1 9,0 9,-1 10,0 10,-1 11,0 11,-1 12,0 12,-1 13,0 13,-1 14,0 14,-1 15,0 15,-1 16,0 16,-1 17,0 17,-1 18,0 18,-1 19,0 19,-1

#pkg customFootprint 0,0 3,3 4,3 -2,1

pkg vpad4 0,0 0,1 0,2 0,3
pkg hpad4 0,0 1,0 2,0 3,0
pkg hpad6 0,0 1,0 2,0 3,0 4,0 5,0

pkg hpad2x4 0,0 1,0 2,0 3,0 0,-1 1,-1 2,-1 3,-1

# Components
# com <component name> <package name>
com rpi header2x20
com 7400 dip14
#com 7402 dip14

com vcc hpad2x4
com gnd hpad2x4
com chan1 hpad2x4
com chan2 hpad2x4
com chan3 hpad2x4
com chan4 hpad2x4
com pwm hpad2x4

# Positions (pin 0)
# com <name of component> <absolute position of component pin 0>
pos rpi 8,10
pos 7400 15,20
pos vcc 8,14
pos gnd 8,17
pos chan1 8,20
pos chan2 8,24
pos chan3 8,27
pos chan4 8,30
pos pwm 24,19

# TODO: fix seg fault caused by leaving out pos by combining com and pos.
#pos 7402 30,25

# Connections
# c <from component name>.<pin index> <to component name>.<pin index>
#c rpi.2 rpi.4
c vcc.5 rpi.2
c gnd.5 rpi.6
c vcc.6 7400.14
c gnd.6 7400.7

c rpi.33 pwm.1

c pwm.5 7400.1
c pwm.6 7400.4
c pwm.7 7400.13
c pwm.8 7400.10

c 7400.6 chan2.5
c 7400.3 chan1.5
c 7400.11 chan3.5
c 7400.8 chan4.5

c 7400.12 rpi.11
c 7400.9 rpi.16
c 7400.2 rpi.18
c 7400.5 rpi.22
```

### Compiling on Linux

* Tested on Linux Mint 18 64-bit.

#### Packaged dependencies

* $ `sudo apt-get install --yes cmake git build-essential libglm-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libboost-dev libboost-filesystem-dev libglew-dev libfreetype6-dev libglfw3-dev` 

#### StripRouter source

* $ `git clone <copy and paste the "Clone with HTTPS " URL from the top of this page>`

#### fmt

* $ `cd striprouter/libraries/build`
* $ `wget https://github.com/fmtlib/fmt/archive/3.0.0.tar.gz`
* $ `tar xf 3.0.0.tar.gz `
* $ `cd fmt-3.0.0`
* $ `cmake -G 'Unix Makefiles'`
* $ `make`
* $ `cd ..`
* $ `cp -r --parents fmt-3.0.0/fmt ../linux`

### Build

* $` cd striprouter/build`
* $ `cmake -G 'Unix Makefiles' ..`
* $ `make` 

### Run

* $` cd striprouter/bin`
* $ `./striprouter`


---


### Compiling on Windows

* Tested on Windows 10 64-bit.

Include and library files need to be moved into the locations set up in CMakeLists.txt.

#### Visual Studio Community 2015

https://go.microsoft.com/fwlink/?LinkId=691978&clcid=0x409
* Chose the type of installation: Custom
* Unselect everything
* Select: Programming Languages > Visual C++ > Common Tools for Visual C++ 2015

#### CMake

https://cmake.org/download/ > `cmake-3.6.3-win64-x64.msi`

#### FreeType

https://sourceforge.net/projects/freetype/ > ft263.zip (2.6.3)
* Open the CMake GUI and browse to the source.
* Use default native compilers
* Source: ft263/freetype-2.6.3
* Build: ft263/freetype-2.6.3/builds
* If "already exists" error: File > Delete Cache
* Click Configure
* Specify the generator for this project > Visual Studio 14 2015 Win64
* Ignore error: "Could NOT find PkgConfig (missing:  PKG_CONFIG_EXECUTABLE)"
* Click Generate
* Open freetype-2.6.3\builds\freetype.sln
* Solution Configurations > Release
* Build > Build Solution

#### GLFW

http://www.glfw.org/download.html > 64-bit Windows binaries

#### fmt

https://github.com/cppformat/cppformat/releases > 3.0.1
* Open the CMake GUI and browse to the source.
* Use default native compilers
* Source: fmt-3.0.1
* Build: fmt-3.0.1/builds (create the builds dir)
* Click Configure
* Select only: FMT_INSTALL, FMT_USE_CPP11
* Click Generate
* Open libraries\win64\fmt-3.0.1\builds\FMT.sln
* Probably no longer required: Right click Solution "FMT" > Retarget solution
* Solution Configurations > Release
* Build > Build Solution

#### Boost

https://sourceforge.net/projects/boost/ > 1.60.0
`bootstrap.bat`
`b2.exe address-model=64`

#### glm

https://github.com/g-truc/glm/releases > 0.9.7.4

glm is header only. Just move into place.

#### GLEW

https://sourceforge.net/projects/glew/ > 1.13.0

Copy `bin\Release\x64\glew32.dll` to `striprouter\bin`.

`glew32.dll` is the 64-bit DLL despite the conflicting name.

`lib\Release\x64\glew32.lib` is the corresponding 64-bit lib.

#### NanoGUI

git clone --recursive https://github.com/wjakob/nanogui.git

* Open in cmake > Configure
* Select only: NANOGUI_USE_GLAD, USE_MSVC_RUNTIME_LIBRARY_DLL
* Generate
* Open NanoGUI.sln
* Solution Configurations (toolbar) > Release
* Build > Build Solution
