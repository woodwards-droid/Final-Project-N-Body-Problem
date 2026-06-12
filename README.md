# N-body gravity simulations

CS 257 final project. Four standalone programs built on [raylib](https://www.raylib.com):

| Program | What it shows |
|---|---|
| `nbody_c` | Three bodies in C, with red arrows showing the net force on each |
| `nbody_sim` | The same three-body system in C++ |
| `nbody_sim_fixed` | A six-planet solar system with orbit trails and a leapfrog integrator |
| `solar_system_sim` | Nine planets propagated from Keplerian orbital elements, with pan and zoom |

CMake downloads and builds raylib automatically the first time you build.
There is nothing to install beyond a compiler and CMake. The first build
takes a minute or two; rebuilds take seconds.

## Build on Windows

Install [Visual Studio 2022 Community](https://visualstudio.microsoft.com/downloads/)
with the **Desktop development with C++** workload. That includes the CL
compiler and CMake.

Open the **x64 Native Tools Command Prompt for VS 2022** from the Start menu,
`cd` into this folder, and run:

```
cmake -S . -B build
cmake --build build
```

## Run on Windows

```
build\nbody_c.exe
build\nbody_sim.exe
build\nbody_sim_fixed.exe
build\solar_system_sim.exe
```

## Build and run on macOS / Linux

```
cmake -S . -B build
cmake --build build
./build/nbody_sim_fixed
```

On Linux (including WSL), install the build tools and X11 dev packages first:

```
sudo apt install build-essential cmake libx11-dev libxrandr-dev libxi-dev \
    libxcursor-dev libxinerama-dev libgl1-mesa-dev libglu1-mesa-dev libasound2-dev
```

On Linux, if no display is available (for example over plain ssh), the first
three programs run a terminal-only simulation instead of opening a window.

## Controls

- `R` resets the simulation (all programs)
- `L` adds the asteroid Ceres (`nbody_sim_fixed`)
- Right-drag pans and the mouse wheel zooms (`solar_system_sim`)
