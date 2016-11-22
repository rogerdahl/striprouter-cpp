## Stripboard Autorouter

<img align="right" width="50%" src="./screenshot.png">

**Note**: Not yet in a usable state.

This is a cross-platform program that, given a description of a circuit, searches for the best way to create the required connections on a [stripboard](https://en.wikipedia.org/wiki/Stripboard).

The output is a description of where to place and solder wires and where to cut the existing stripboard traces in order to create the connections.

Costs can be assigned to resources such as stripboard area and solder points in order to direct the router towards preferred layouts. E.g., setting the cost of solder points high in relation to other resources will typically generate layouts that use fewer solder points at the expense of stripboard area.

The program searches only for routes that use non-overlapping wires that cross the traces at right angles, which tends to give layouts that look clean, and which allows using only uninsulated wires.   

Currently somewhat working:

* Parsing of circuit description file
* Visualization of circuit
* Simple automatic routing of point-to-point connections
* Visualization of discovered routes
* Basic GUI controls

Planned functionality:
 
* Write layout to file
* Print mirror image of layout in 1:1 size for use when soldering  
* Optimize routes by considering routing to strip layer sections that are parts of existing routes connecting to the target
* Use a genetic algorithm for exploring the search space. Even for very simple circuits, it's not possible to consider all possible layouts
* Support components such as resistors and diodes that have variable length connectors
* Automatic optimization of component locations

### Circuit description

The circuit description is a text file that can be edited while the program is running. Open the description in a text editor side by side with the router and hit save to update the circuit and restart the routing.

### Tips and Tricks

* If the router is unable to find routes for all the required connections, the unrouted connections are listed separately in the solution. These can then be added by creating regular point-to-point connections with insulated wire at solder time.
  
* Some designs, such as [Rasperry Pi "Hats"](https://shop.pimoroni.com/collections/hats) require a double row header on the edge of the board. In the case of the Raspberry Pi, this is a 2x20 pin header. In order to connect to the outer header pins, the router must go around the header and use board area on the outer side for wires and traces, which makes it impossible for the header to be at the edge of the board. The more connections are required for the outer row, the further in on the board the header must be located.

  Depending on the physical requirements for the final board, this may be acceptable. If it's not, you can enable the double header to stay at the edge of the board by connecting the outer pins with insulated wire. To do this, you have several options:
  
  1) Leave the outer row connections out of the circuit description altogether and create direct point-to-point connections for them at solder time.
  
  2) Keep the outer row connections in the circuit description and use the resulting routes as hints on how to best create your connections.
  
  3) Represent the double header with two single headers in the circuit description. Put one single header at the actual location of the inner row of the double header and put the other in another location on the board. The opposite side may be best. The router can then route directly to each of the single row headers without using area outside the headers. At solder time, connect the outer row of the double header to the row of points that were used by the router. Using an insulated flat cable can be convenient. Old IDE and floppy cables work well for this.

### Technologies

* C++
* Boost
* fmt
* FreeType2
* GLEW
* glm
* NanoGUI
* OpenGL


### Example circuit description

This is the circuit description used in the screenshot.

```
# Raspberry Pi WS2812B LED level shifter and multiplexer

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
