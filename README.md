## Stripboard Autorouter

<aside class="notice">

This may become a PCB autorouter for stripboards some day, but right now, you probably won't find anything useful here.

</aside>

<img align="right" width="50%" src="./screenshot.png">

Currently, the program can parse a circuit description and visualize it. It also converts the circuit description to a table of required connections which would essentially be the input for an autorouter.
 
The circuit description is a text file that can be edited while the program is running. Open the description in a text editor side by side with the renderer and hit save to update the rendered version.

### Circuit description

This is the example circuit description that is rendered in the screenshot.

```
# Package
# pkg <package name> <pin coordinates relative to pin 0>
pkg dip14 0,0 1,0 2,0 3,0 4,0 5,0 6,0 6,-3 5,-3 4,-3 3,-3 2,-3 1,-3 0,-3
pkg header20 0,0 1,0 2,0 3,0 4,0 5,0 6,0 7,0 8,0 9,0 10,0 11,0 12,0 13,0 14,0 15,0 16,0 17,0 18,0 19,0
pkg customFootprint 0,0 3,3 4,3 -2,1

# Components
# com <component name> <package name>
com rpi header20
com 7400 dip14
com custom customFootprint

# Positions (pin 0)
# com <name of component> <absolute position of component pin 0>
pos rpi 4,2
pos 7400 10,15
pos custom 5,20

# Connections
# c <from component name>.<pin index> <to component name>.<pin index>
c 7400.9 rpi.10
c 7400.4 rpi.9
c rpi.8 7400.1
c custom.3 rpi.0
c custom.3 rpi.8
c rpi.19 custom.2

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
