# TinyCanvas

[![Build](https://github.com/ni5arga/TinyCanvas/actions/workflows/build.yml/badge.svg)](https://github.com/ni5arga/TinyCanvas/actions/workflows/build.yml)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![SDL2](https://img.shields.io/badge/SDL2-2.0.5+-brightgreen.svg)](https://www.libsdl.org/)
[![CMake](https://img.shields.io/badge/CMake-3.16+-064F8C.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A lightweight pixel art editor built with C++17 and SDL2.

## Build

### Prerequisites

- CMake 3.16+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- SDL2 2.0.5+

### Install SDL2

**macOS**
```bash
brew install sdl2
```

**Ubuntu/Debian**
```bash
sudo apt update
sudo apt install libsdl2-dev
```

**Arch Linux**
```bash
sudo pacman -S sdl2
```

**Windows**
```bash
vcpkg install sdl2:x64-windows
```

### Compile & Run

```bash
cd pixel-art-editor
mkdir build && cd build
cmake ..
make -j$(nproc)
./pixel_editor

# Custom canvas size
./pixel_editor 64 64
```

## Controls

### Drawing Tools
| Key | Tool          | Description                               |
|-----|---------------|-------------------------------------------|
| `P` | Pencil        | Freehand pixel drawing                    |
| `E` | Eraser        | Remove pixels with background color       |
| `L` | Line          | Draw straight lines between two points    |
| `R` | Rectangle     | Draw rectangular outlines                 |
| `C` | Circle        | Draw circular outlines from center        |
| `F` | Fill          | Flood fill connected regions              |
| `I` | Color Picker  | Sample color from canvas                  |

### View & Navigation
| Input                  | Action                                    |
|------------------------|-------------------------------------------|
| **Scroll Wheel**       | Zoom in/out (1x - 128x)                   |
| **Middle-Click Drag**  | Pan canvas (works anywhere)               |
| **Right-Click Drag**   | Pan canvas (on canvas area)               |
| `Space` or `Cmd/Ctrl+0`| Fit canvas to view                        |
| `+` / `=`              | Zoom in                                   |
| `-`                    | Zoom out                                  |
| `G`                    | Toggle pixel grid overlay                 |
| `F11` or `Cmd/Ctrl+Enter` | Toggle fullscreen                      |

### Color & Editing
| Input                   | Action                                   |
|-------------------------|------------------------------------------|
| **Left-Click Palette**  | Set foreground color                     |
| **Right-Click Palette** | Set background color                     |
| `X`                     | Swap foreground/background colors        |
| **Left-Click Canvas**   | Draw/apply tool with foreground color    |

### File Operations
| Keys                    | Action                                   |
|-------------------------|------------------------------------------|
| `Cmd/Ctrl + S`          | Save as BMP (artwork.bmp)                |
| `Cmd/Ctrl + O`          | Load BMP file                            |
| `Cmd/Ctrl + N`          | Clear canvas (new artwork)               |

### History
| Keys                    | Action                                   |
|-------------------------|------------------------------------------|
| `Cmd/Ctrl + Z`          | Undo (up to 100 steps)                   |
| `Cmd/Ctrl + Shift + Z`  | Redo                                     |


## Advanced Usage

### Custom Canvas Size

Launch with specific dimensions:
```bash
./pixel_editor 128 64    # 128x64 canvas
./pixel_editor 16 16     # Tiny 16x16 icon
```

### File Format

- Export: Saves to `artwork.bmp` in current directory
- Import: Loads any BMP file and resizes canvas to match
- Format: Standard Windows BMP (RGBA32) with alpha channel

### Performance

- 60 FPS rendering with VSync
- Efficient flood fill with BFS and bounds checking
- Delta time compensation for consistent zoom/pan
- Full undo history under 10MB for typical canvases

## Project Structure

```
pixel-art-editor/
├── src/
│   ├── main.cpp          # Entry point
│   ├── editor.h/cpp      # Main editor logic & rendering
│   ├── canvas.h/cpp      # Canvas operations & drawing algorithms
│   ├── types.h           # Core structs (Color, Point, Tool enum)
│   └── font.h            # 5x7 bitmap font for UI text
├── CMakeLists.txt        # Build configuration
├── .gitignore            # Git ignore rules
└── README.md
```

## Building from Source

### Debug Build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release Build (Optimized)
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./pixel_editor
```

### Clean Build
```bash
rm -rf build/
mkdir build && cd build
cmake ..
make
```

## Troubleshooting

**SDL2 Not Found**

If CMake cannot find SDL2, ensure it's installed and pkg-config can locate it:
```bash
pkg-config --modversion sdl2
```

