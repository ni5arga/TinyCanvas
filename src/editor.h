#pragma once
#include "types.h"
#include "canvas.h"
#include <SDL2/SDL.h>
#include <vector>
#include <string>

class Editor {
public:
    Editor(int canvasW = 32, int canvasH = 32);
    ~Editor();

    bool init();
    void run();

private:
    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture*  canvasTex_ = nullptr;

    Canvas canvas_;

    Tool  currentTool_ = Tool::Pencil;
    Color fgColor_     = {0, 0, 0, 255};
    Color bgColor_     = {255, 255, 255, 255};

    float zoom_       = 12.0f;
    float targetZoom_ = 12.0f;
    float panX_       = 0;
    float panY_       = 0;
    bool  showGrid_   = true;

    int winW_ = 1280;
    int winH_ = 800;
    bool fullscreen_ = false;

    bool  lmbDown_      = false;
    bool  mmbDown_      = false;
    bool  rmbPan_       = false;
    int   lastMouseX_   = 0;
    int   lastMouseY_   = 0;
    int   mouseX_       = 0;
    int   mouseY_       = 0;
    int   cursorCX_     = -1;
    int   cursorCY_     = -1;
    Point dragStart_    = {0, 0};
    Point lastDraw_     = {0, 0};
    bool  dragging_     = false;
    bool  strokeActive_ = false;

    int   hoverToolIdx_    = -1;
    int   hoverSwatchIdx_  = -1;
    bool  hoverGrid_       = false;
    bool  hoverFgBg_       = false;

    Uint64 lastFrameTime_ = 0;
    float  deltaTime_     = 0.016f;

    std::vector<std::vector<Color>> undoStack_;
    std::vector<std::vector<Color>> redoStack_;
    static const int MAX_UNDO = 100;

    static const int TOOLBAR_H      = 48;
    static const int PALETTE_H      = 68;
    static const int STATUS_H       = 26;
    static const int TOOL_BTN_SIZE  = 36;
    static const int TOOL_BTN_PAD   = 4;
    static const int SWATCH_SIZE    = 26;
    static const int SWATCH_PAD     = 3;

    void handleEvent(const SDL_Event& e);
    void handleKeyDown(const SDL_KeyboardEvent& key);
    void handleMouseDown(int x, int y, uint8_t button);
    void handleMouseUp(int x, int y, uint8_t button);
    void handleMouseMotion(int x, int y);
    void handleMouseWheel(int scrollY, int mouseX, int mouseY);
    void handleWindowEvent(const SDL_WindowEvent& we);

    bool handleToolbarClick(int x, int y, uint8_t button);
    bool handlePaletteClick(int x, int y, uint8_t button);

    void updateHover(int x, int y);

    void applyTool(int cx, int cy, bool newStroke);
    void finishShape(int cx, int cy);

    void fitCanvasInView();
    void toggleFullscreen();
    void centerCanvas();
    void updateSmoothZoom();

    void pushUndo();
    void undo();
    void redo();

    void saveFile(const std::string& path = "artwork.bmp");
    void loadFile(const std::string& path = "artwork.bmp");

    void render();
    void renderCanvas();
    void renderGrid();
    void renderShapePreview();
    void renderCursor();
    void renderToolbar();
    void renderPalette();
    void renderStatusBar();
    void renderTooltip(int x, int y, const char* text);

    void fillRect(int x, int y, int w, int h, const Color& c);
    void outlineRect(int x, int y, int w, int h, const Color& c);
    void drawText(int x, int y, const char* text, const Color& c, int scale = 1);
    int  textWidth(const char* text, int scale = 1) const;

    void  canvasOrigin(float& ox, float& oy) const;
    Point screenToCanvas(int sx, int sy) const;
    int   canvasAreaTop()    const { return TOOLBAR_H; }
    int   canvasAreaBottom() const { return winH_ - PALETTE_H - STATUS_H; }
    int   canvasAreaHeight() const { return canvasAreaBottom() - canvasAreaTop(); }
    bool  inCanvasArea(int y) const { return y > canvasAreaTop() && y < canvasAreaBottom(); }
};
