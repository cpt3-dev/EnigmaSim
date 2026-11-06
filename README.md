# EnigmaSim

A fidelity-first Enigma machine simulator with a hand-written 3D renderer.

EnigmaSim reproduces the rotor stepping rules of the historical Enigma I, M3
and M4 machines and renders the hardware procedurally in real time. The entire
project is written in C++20 with **no third-party libraries**: the OpenGL
loader, the matrix math, the mesh generation, the UI windowing layer and the
test framework are all implemented from scratch.

## Historical context

The Enigma was an electromechanical cipher machine used for military
communications during the Second World War. This project is an engineering and
cryptography study of its rotor mechanism; it contains no political content
and is not affiliated with any organization.

## Features

- Faithful Enigma logic, including the double-stepping anomaly, ring settings
  (`Ringstellung`), plugboard pairing, thin rotors (Beta/Gamma) and M4
  four-rotor layouts.
- Historical rotor and reflector wiring tables transcribed from the original
  machine specifications.
- Procedural geometry generated at runtime: 26-division rotor cylinders,
  independently rotating ring collars that visualize the `Ringstellung`,
  reflector, 26-lamp lampboard, 26-socket plugboard and a bakelite casing.
- Blinn-Phong shading with embedded GLSL 330 core shaders.
- Orbit camera with orbit / pan / zoom, double-click reset and preset views
  (front, top, translucent cutaway).
- MSAA 4x when the driver supports it, with automatic fallback.
- Zero external asset files, zero network access at runtime.

## Requirements

- Windows 7 or later.
- A GPU driver exposing an OpenGL 3.3 Core profile.
- Visual Studio 2019/2022 with the C++ workload, or a MinGW-w64 toolchain.
- Test builds on Linux/macOS only need a C++20 compiler.

## Building

### Windows (MSVC)

Open a *Developer Command Prompt for Visual Studio* in the repository root and
run:

```bat
build.bat
```

The script compiles and runs the test suites, then produces
`build\enigma_sim.exe`.

### Windows (MinGW-w64)

```bash
x86_64-w64-mingw32-g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic \
  -Icore -Iplatform -Igl -Iapp -Imath -Iscene \
  core/enigma.cpp platform/platform_window_win32.cpp gl/gl_loader_wgl.cpp \
  scene/mesh.cpp scene/camera.cpp scene/renderer.cpp app/main.cpp \
  -o build/enigma_sim.exe -luser32 -lgdi32 -lopengl32
```

### Tests only (any platform)

```bash
./build.sh
```

This compiles and runs the cryptographic and math test suites; it does not
require a window, a GPU or Win32 headers.

## Running

Launch `build\enigma_sim.exe`. Controls:

| Input | Action |
| --- | --- |
| Left mouse drag | Orbit the camera |
| Right mouse drag | Pan the camera |
| Mouse wheel | Zoom in / out |
| Double-click (left) | Reset the default view |
| `F` | Front preset view |
| `T` | Top preset view |
| `C` | Cutaway preset view (translucent casing) |
| `1` - `4` | Switch machine configuration preset |
| `Space` | Step the rotors once |
| `Esc` | Quit |

The four built-in presets cover an Enigma I with default settings, an
Enigma I with plugboard pairs and non-default ring settings, an M3, and an M4
with a thin Beta rotor.

## Project layout

```
core/      Enigma machine model and wiring tables (portable C++20)
math/      Vec3 and column-major Mat4 utilities
scene/     Procedural meshes, orbit camera, Blinn-Phong renderer
gl/        Hand-written OpenGL 3.3 loader and RAII object wrappers
platform/  Win32 + wgl window, context, input and timing layer
app/       Application main loop
tests/     Minimal test framework and the test suites
```

Dependency direction is strictly one-way: `core` and `math` are pure C++20 and
never include `windows.h` or any OpenGL header, so they can be reused and
tested anywhere. All operating-system calls live in `platform/`.

## Testing

`build.bat` and `build.sh` both run two suites:

- `enigma_tests` - 14 tests covering literature vectors (including
  `AAAAA -> BDZGO`), hand-derived plugboard/ring vectors, double stepping,
  dual-notch rotors, thin rotors, reflexivity, the no-self-encipher property
  and configuration validation.
- `enigma_math_tests` - 8 tests covering matrix multiplication, inversion,
  perspective projection, `lookAt` and rotation orthonormality.

Both suites are expected to report `0 failed`.

## Implementation notes

- Matrices are stored column-major and uploaded with `transpose = GL_FALSE`,
  matching the OpenGL convention.
- All meshes are built once at startup and kept in persistent VAO/VBO/EBO
  objects; the steady-state frame performs no heap allocation.
- Every magic number is a named constant, and every GL object is owned by an
  RAII wrapper.
- The historically correct UKW-C wiring is used. Some reproduced tables list a
  28-character line for UKW-C (with duplicated `A` and `Z` entries), which is
  not a valid permutation; the wiring file documents this deviation.

## Status

The rotor letter engravings and the keyboard are not rendered yet, as they
require a text-rendering module. Lamplight output (the emissive channel) is in
place but not yet driven by input.
