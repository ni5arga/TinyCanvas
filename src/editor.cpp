#include "editor.h"
#include "font.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

Editor::Editor(int canvasW, int canvasH) : canvas_(canvasW, canvasH) {}

Editor::~Editor() {
    if (canvasTex_) SDL_DestroyTexture(canvasTex_);
    if (renderer_)  SDL_DestroyRenderer(renderer_);
    if (window_)    SDL_DestroyWindow(window_);
    SDL_Quit();
}
bool Editor::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
        winW_ = std::max(960, (int)(dm.w * 0.75));
        winH_ = std::max(720, (int)(dm.h * 0.75));
    }

    window_ = SDL_CreateWindow("TinyCanvas",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winW_, winH_,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_) return false;

    SDL_SetWindowMinimumSize(window_, 640, 480);

    renderer_ = SDL_CreateRenderer(window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) return false;

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    canvas_.clear({255, 255, 255, 255});
    fitCanvasInView();

    lastFrameTime_ = SDL_GetPerformanceCounter();
    return true;
}


void Editor::run() {
    bool running = true;
    while (running) {

        Uint64 now = SDL_GetPerformanceCounter();
        deltaTime_ = (float)(now - lastFrameTime_) / SDL_GetPerformanceFrequency();
        deltaTime_ = std::min(deltaTime_, 0.05f);
        lastFrameTime_ = now;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            handleEvent(e);
        }

        SDL_GetWindowSize(window_, &winW_, &winH_);

        updateSmoothZoom();

        updateHover(mouseX_, mouseY_);

        render();
    }
}


void Editor::handleEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_KEYDOWN:
        handleKeyDown(e.key);
        break;
    case SDL_MOUSEBUTTONDOWN:
        handleMouseDown(e.button.x, e.button.y, e.button.button);
        break;
    case SDL_MOUSEBUTTONUP:
        handleMouseUp(e.button.x, e.button.y, e.button.button);
        break;
    case SDL_MOUSEMOTION:
        mouseX_ = e.motion.x;
        mouseY_ = e.motion.y;
        handleMouseMotion(e.motion.x, e.motion.y);
        break;
    case SDL_MOUSEWHEEL:
        {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            handleMouseWheel(e.wheel.y, mx, my);
        }
        break;
    case SDL_WINDOWEVENT:
        handleWindowEvent(e.window);
        break;
    }
}


void Editor::handleWindowEvent(const SDL_WindowEvent& we) {
    switch (we.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED:
    case SDL_WINDOWEVENT_RESIZED:
        winW_ = we.data1;
        winH_ = we.data2;
        break;
    }
}


void Editor::handleKeyDown(const SDL_KeyboardEvent& key) {
    bool mod = (key.keysym.mod & (KMOD_GUI | KMOD_CTRL)) != 0;
    bool shift = (key.keysym.mod & KMOD_SHIFT) != 0;

    if (mod) {
        switch (key.keysym.sym) {
        case SDLK_z:
            if (shift) redo(); else undo();
            return;
        case SDLK_s:
            saveFile();
            return;
        case SDLK_o:
            loadFile();
            return;
        case SDLK_n:
            pushUndo();
            canvas_.clear({255, 255, 255, 255});
            return;
        case SDLK_0:
            fitCanvasInView();
            return;
        }
    }

    switch (key.keysym.sym) {
    case SDLK_p: currentTool_ = Tool::Pencil;      break;
    case SDLK_e: currentTool_ = Tool::Eraser;       break;
    case SDLK_l: currentTool_ = Tool::Line;         break;
    case SDLK_r: currentTool_ = Tool::Rectangle;    break;
    case SDLK_c: currentTool_ = Tool::Circle;       break;
    case SDLK_f: currentTool_ = Tool::Fill;         break;
    case SDLK_i: currentTool_ = Tool::ColorPicker;  break;
    case SDLK_g: showGrid_ = !showGrid_;            break;
    case SDLK_x: std::swap(fgColor_, bgColor_);     break;
    case SDLK_EQUALS: case SDLK_PLUS:
        targetZoom_ = std::min(targetZoom_ * 1.25f, 128.0f);
        break;
    case SDLK_MINUS:
        targetZoom_ = std::max(targetZoom_ / 1.25f, 1.0f);
        break;
    case SDLK_F11:
    case SDLK_RETURN:
        if (key.keysym.sym == SDLK_RETURN && !mod) break;
        toggleFullscreen();
        break;
    case SDLK_SPACE:
        fitCanvasInView();
        break;
    }
}

void Editor::canvasOrigin(float& ox, float& oy) const {
    int areaH = canvasAreaHeight();
    ox = panX_ + (winW_ - canvas_.getWidth() * zoom_) / 2.0f;
    oy = panY_ + TOOLBAR_H + (areaH - canvas_.getHeight() * zoom_) / 2.0f;
}

Point Editor::screenToCanvas(int sx, int sy) const {
    float ox, oy;
    canvasOrigin(ox, oy);
    int cx = (int)std::floor((sx - ox) / zoom_);
    int cy = (int)std::floor((sy - oy) / zoom_);
    return {cx, cy};
}

void Editor::handleMouseDown(int x, int y, uint8_t button) {
    if (button == SDL_BUTTON_LEFT) {
        // Try UI areas first
        if (handleToolbarClick(x, y, button)) return;
        if (handlePaletteClick(x, y, button)) return;
        if (inCanvasArea(y)) {
            lmbDown_ = true;
            Point cp = screenToCanvas(x, y);

            if (currentTool_ == Tool::Line ||
                currentTool_ == Tool::Rectangle ||
                currentTool_ == Tool::Circle)
            {
                pushUndo();
                dragStart_ = cp;
                dragging_  = true;
            } else {
                strokeActive_ = true;
                lastDraw_ = cp;
                applyTool(cp.x, cp.y, true);
            }
        }
    } else if (button == SDL_BUTTON_MIDDLE) {
        mmbDown_ = true;
        lastMouseX_ = x;
        lastMouseY_ = y;
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL));
    } else if (button == SDL_BUTTON_RIGHT) {
        if (handlePaletteClick(x, y, button)) return;

        if (inCanvasArea(y)) {
            rmbPan_ = true;
            lastMouseX_ = x;
            lastMouseY_ = y;
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL));
        }
    }
}

void Editor::handleMouseUp(int x, int y, uint8_t button) {
    if (button == SDL_BUTTON_LEFT) {
        if (dragging_) {
            Point cp = screenToCanvas(x, y);
            finishShape(cp.x, cp.y);
            dragging_ = false;
        }
        lmbDown_ = false;
        strokeActive_ = false;
    } else if (button == SDL_BUTTON_MIDDLE) {
        mmbDown_ = false;
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
    } else if (button == SDL_BUTTON_RIGHT) {
        rmbPan_ = false;
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
    }
}

void Editor::handleMouseMotion(int x, int y) {
    if (inCanvasArea(y)) {
        Point cp = screenToCanvas(x, y);
        cursorCX_ = cp.x;
        cursorCY_ = cp.y;
    } else {
        cursorCX_ = cursorCY_ = -1;
    }
    if (mmbDown_ || rmbPan_) {
        panX_ += x - lastMouseX_;
        panY_ += y - lastMouseY_;
        lastMouseX_ = x;
        lastMouseY_ = y;
        return;
    }
    if (lmbDown_ && strokeActive_) {
        Point cp = screenToCanvas(x, y);
        auto pts = Canvas::linePoints(lastDraw_.x, lastDraw_.y, cp.x, cp.y);
        for (auto& p : pts)
            applyTool(p.x, p.y, false);
        lastDraw_ = cp;
    }
}

void Editor::handleMouseWheel(int scrollY, int mouseX, int mouseY) {
    if (!inCanvasArea(mouseY)) {
        return;
    }
    float factor = (scrollY > 0) ? 1.15f : 1.0f / 1.15f;
    float newTarget = targetZoom_ * factor;
    newTarget = std::max(1.0f, std::min(newTarget, 128.0f));
    float ox, oy;
    canvasOrigin(ox, oy);
    float cx = (mouseX - ox) / zoom_;
    float cy = (mouseY - oy) / zoom_;

    targetZoom_ = newTarget;
    float newOx = mouseX - cx * targetZoom_;
    float newOy = mouseY - cy * targetZoom_;
    int areaH = canvasAreaHeight();
    float defOx = (winW_ - canvas_.getWidth() * targetZoom_) / 2.0f;
    float defOy = TOOLBAR_H + (areaH - canvas_.getHeight() * targetZoom_) / 2.0f;
    panX_ = newOx - defOx;
    panY_ = newOy - defOy;
}

void Editor::updateSmoothZoom() {
    if (std::abs(zoom_ - targetZoom_) < 0.01f) {
        zoom_ = targetZoom_;
        return;
    }

    float speed = 12.0f;
    zoom_ += (targetZoom_ - zoom_) * std::min(1.0f, speed * deltaTime_);
}

void Editor::fitCanvasInView() {
    int areaH = canvasAreaHeight();
    float zx = (float)(winW_ - 40) / canvas_.getWidth();
    float zy = (float)(areaH - 20) / canvas_.getHeight();
    targetZoom_ = std::floor(std::min(zx, zy));
    targetZoom_ = std::max(2.0f, std::min(targetZoom_, 64.0f));
    zoom_ = targetZoom_;
    panX_ = panY_ = 0;
}

void Editor::toggleFullscreen() {
    fullscreen_ = !fullscreen_;
    if (fullscreen_) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window_, 0);
    }
    SDL_GetWindowSize(window_, &winW_, &winH_);
}

void Editor::centerCanvas() {
    panX_ = panY_ = 0;
}

void Editor::updateHover(int x, int y) {
    hoverToolIdx_   = -1;
    hoverSwatchIdx_ = -1;
    hoverGrid_      = false;
    hoverFgBg_      = false;
    if (y >= 0 && y < TOOLBAR_H) {
        int toolCount = (int)Tool::COUNT;
        for (int i = 0; i < toolCount; i++) {
            int bx = TOOL_BTN_PAD + i * (TOOL_BTN_SIZE + TOOL_BTN_PAD);
            int by = (TOOLBAR_H - TOOL_BTN_SIZE) / 2;
            if (x >= bx && x < bx + TOOL_BTN_SIZE && y >= by && y < by + TOOL_BTN_SIZE) {
                hoverToolIdx_ = i;
                return;
            }
        }
        int gx = TOOL_BTN_PAD + toolCount * (TOOL_BTN_SIZE + TOOL_BTN_PAD) + 10;
        int gy = (TOOLBAR_H - TOOL_BTN_SIZE) / 2;
        if (x >= gx && x < gx + TOOL_BTN_SIZE && y >= gy && y < gy + TOOL_BTN_SIZE) {
            hoverGrid_ = true;
            return;
        }
        int previewX = winW_ - 80;
        int previewY = (TOOLBAR_H - 36) / 2;
        if (x >= previewX && x < previewX + 44 && y >= previewY && y < previewY + 36) {
            hoverFgBg_ = true;
            return;
        }
    }
    int paletteY = winH_ - PALETTE_H - STATUS_H;
    if (y >= paletteY && y < paletteY + PALETTE_H) {
        int colorsPerRow = 12;
        for (int i = 0; i < PALETTE_SIZE; i++) {
            int row = i / colorsPerRow;
            int col = i % colorsPerRow;
            int sx = SWATCH_PAD + col * (SWATCH_SIZE + SWATCH_PAD);
            int sy = paletteY + SWATCH_PAD + row * (SWATCH_SIZE + SWATCH_PAD);
            if (x >= sx && x < sx + SWATCH_SIZE && y >= sy && y < sy + SWATCH_SIZE) {
                hoverSwatchIdx_ = i;
                return;
            }
        }
    }
}

bool Editor::handleToolbarClick(int x, int y, uint8_t button) {
    if (button != SDL_BUTTON_LEFT) return false;
    if (y < 0 || y >= TOOLBAR_H) return false;

    int toolCount = (int)Tool::COUNT;
    for (int i = 0; i < toolCount; i++) {
        int bx = TOOL_BTN_PAD + i * (TOOL_BTN_SIZE + TOOL_BTN_PAD);
        int by = (TOOLBAR_H - TOOL_BTN_SIZE) / 2;
        if (x >= bx && x < bx + TOOL_BTN_SIZE && y >= by && y < by + TOOL_BTN_SIZE) {
            currentTool_ = (Tool)i;
            return true;
        }
    }
    int gx = TOOL_BTN_PAD + toolCount * (TOOL_BTN_SIZE + TOOL_BTN_PAD) + 10;
    int gy = (TOOLBAR_H - TOOL_BTN_SIZE) / 2;
    if (x >= gx && x < gx + TOOL_BTN_SIZE && y >= gy && y < gy + TOOL_BTN_SIZE) {
        showGrid_ = !showGrid_;
        return true;
    }
    int previewX = winW_ - 80;
    int previewY = (TOOLBAR_H - 36) / 2;
    if (x >= previewX && x < previewX + 44 && y >= previewY && y < previewY + 36) {
        std::swap(fgColor_, bgColor_);
        return true;
    }

    return false;
}

bool Editor::handlePaletteClick(int x, int y, uint8_t button) {
    int paletteY = winH_ - PALETTE_H - STATUS_H;
    if (y < paletteY || y >= paletteY + PALETTE_H) return false;

    int colorsPerRow = 12;
    for (int i = 0; i < PALETTE_SIZE; i++) {
        int row = i / colorsPerRow;
        int col = i % colorsPerRow;
        int sx = SWATCH_PAD + col * (SWATCH_SIZE + SWATCH_PAD);
        int sy = paletteY + SWATCH_PAD + row * (SWATCH_SIZE + SWATCH_PAD);
        if (x >= sx && x < sx + SWATCH_SIZE && y >= sy && y < sy + SWATCH_SIZE) {
            if (button == SDL_BUTTON_LEFT)
                fgColor_ = PALETTE[i];
            else if (button == SDL_BUTTON_RIGHT)
                bgColor_ = PALETTE[i];
            return true;
        }
    }
    return false;
}

void Editor::applyTool(int cx, int cy, bool newStroke) {
    if (!canvas_.inBounds(cx, cy)) return;

    switch (currentTool_) {
    case Tool::Pencil:
        if (newStroke) pushUndo();
        canvas_.setPixel(cx, cy, fgColor_);
        break;
    case Tool::Eraser:
        if (newStroke) pushUndo();
        canvas_.setPixel(cx, cy, bgColor_);
        break;
    case Tool::Fill:
        pushUndo();
        canvas_.floodFill(cx, cy, fgColor_);
        break;
    case Tool::ColorPicker:
        fgColor_ = canvas_.getPixel(cx, cy);
        break;
    default:
        break;
    }
}

void Editor::finishShape(int cx, int cy) {
    switch (currentTool_) {
    case Tool::Line:
        canvas_.drawLine(dragStart_.x, dragStart_.y, cx, cy, fgColor_);
        break;
    case Tool::Rectangle:
        canvas_.drawRect(dragStart_.x, dragStart_.y, cx, cy, fgColor_);
        break;
    case Tool::Circle: {
        int dx = cx - dragStart_.x;
        int dy = cy - dragStart_.y;
        int radius = (int)std::round(std::sqrt(dx * dx + dy * dy));
        canvas_.drawCircle(dragStart_.x, dragStart_.y, radius, fgColor_);
        break;
    }
    default:
        break;
    }
}

void Editor::pushUndo() {
    undoStack_.push_back(canvas_.snapshot());
    if ((int)undoStack_.size() > MAX_UNDO)
        undoStack_.erase(undoStack_.begin());
    redoStack_.clear();
}

void Editor::undo() {
    if (undoStack_.empty()) return;
    redoStack_.push_back(canvas_.snapshot());
    canvas_.restore(undoStack_.back());
    undoStack_.pop_back();
}

void Editor::redo() {
    if (redoStack_.empty()) return;
    undoStack_.push_back(canvas_.snapshot());
    canvas_.restore(redoStack_.back());
    redoStack_.pop_back();
}

void Editor::saveFile(const std::string& path) {
    int w = canvas_.getWidth(), h = canvas_.getHeight();
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!s) return;

    for (int y = 0; y < h; y++) {
        uint8_t* row = (uint8_t*)s->pixels + y * s->pitch;
        for (int x = 0; x < w; x++) {
            Color c = canvas_.getPixel(x, y);
            row[x * 4 + 0] = c.r;
            row[x * 4 + 1] = c.g;
            row[x * 4 + 2] = c.b;
            row[x * 4 + 3] = c.a;
        }
    }
    SDL_SaveBMP(s, path.c_str());
    SDL_FreeSurface(s);
    printf("Saved: %s\n", path.c_str());
}

void Editor::loadFile(const std::string& path) {
    SDL_Surface* raw = SDL_LoadBMP(path.c_str());
    if (!raw) { printf("Failed to load: %s\n", path.c_str()); return; }

    SDL_Surface* s = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    if (!s) return;

    pushUndo();
    canvas_ = Canvas(s->w, s->h);
    for (int y = 0; y < s->h; y++) {
        uint8_t* row = (uint8_t*)s->pixels + y * s->pitch;
        for (int x = 0; x < s->w; x++) {
            canvas_.setPixel(x, y, {row[x*4+0], row[x*4+1], row[x*4+2], row[x*4+3]});
        }
    }
    SDL_FreeSurface(s);

    fitCanvasInView();
    printf("Loaded: %s (%dx%d)\n", path.c_str(), canvas_.getWidth(), canvas_.getHeight());
}

void Editor::fillRect(int x, int y, int w, int h, const Color& c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(renderer_, &r);
}

void Editor::outlineRect(int x, int y, int w, int h, const Color& c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderDrawRect(renderer_, &r);
}

void Editor::drawText(int x, int y, const char* text, const Color& c, int scale) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    for (const char* p = text; *p; p++) {
        int ch = (int)(unsigned char)*p;
        if (ch < 32 || ch > 126) ch = '?';
        const uint8_t* glyph = FONT_5X7[ch - 32];
        for (int row = 0; row < FONT_GLYPH_H; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < FONT_GLYPH_W; col++) {
                if (bits & (1 << (4 - col))) {
                    SDL_Rect pr = {x + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer_, &pr);
                }
            }
        }
        x += FONT_CHAR_W * scale;
    }
}

int Editor::textWidth(const char* text, int scale) const {
    return (int)strlen(text) * FONT_CHAR_W * scale;
}

void Editor::renderTooltip(int x, int y, const char* text) {
    int tw = textWidth(text, 1);
    int pad = 4;
    int tx = x, ty = y + 24;
    if (tx + tw + pad * 2 > winW_) tx = winW_ - tw - pad * 2;
    if (ty + FONT_CHAR_H + pad * 2 > winH_) ty = y - FONT_CHAR_H - pad * 2;
    fillRect(tx + 1, ty + 1, tw + pad * 2, FONT_CHAR_H + pad * 2, {0, 0, 0, 120});
    fillRect(tx, ty, tw + pad * 2, FONT_CHAR_H + pad * 2, {50, 50, 55, 240});
    outlineRect(tx, ty, tw + pad * 2, FONT_CHAR_H + pad * 2, {100, 100, 110, 255});
    drawText(tx + pad, ty + pad, text, {230, 230, 230, 255}, 1);
}

void Editor::render() {
    SDL_SetRenderDrawColor(renderer_, 42, 42, 46, 255);
    SDL_RenderClear(renderer_);

    renderCanvas();
    if (showGrid_ && zoom_ >= 4.0f) renderGrid();
    if (dragging_) renderShapePreview();
    renderCursor();
    renderToolbar();
    renderPalette();
    renderStatusBar();
    if (hoverToolIdx_ >= 0) {
        int bx = TOOL_BTN_PAD + hoverToolIdx_ * (TOOL_BTN_SIZE + TOOL_BTN_PAD);
        char tip[64];
        snprintf(tip, sizeof(tip), "%s (%c)", toolName((Tool)hoverToolIdx_), toolKey((Tool)hoverToolIdx_));
        renderTooltip(bx, TOOLBAR_H - 4, tip);
    } else if (hoverGrid_) {
        int toolCount = (int)Tool::COUNT;
        int gx = TOOL_BTN_PAD + toolCount * (TOOL_BTN_SIZE + TOOL_BTN_PAD) + 10;
        renderTooltip(gx, TOOLBAR_H - 4, showGrid_ ? "Grid ON (G)" : "Grid OFF (G)");
    } else if (hoverFgBg_) {
        renderTooltip(winW_ - 120, TOOLBAR_H - 4, "Click to swap (X)");
    } else if (hoverSwatchIdx_ >= 0) {
        int paletteY = winH_ - PALETTE_H - STATUS_H;
        int col = hoverSwatchIdx_ % 12;
        int sx = SWATCH_PAD + col * (SWATCH_SIZE + SWATCH_PAD);
        Color sc = PALETTE[hoverSwatchIdx_];
        char tip[64];
        snprintf(tip, sizeof(tip), "L:FG R:BG  #%02X%02X%02X", sc.r, sc.g, sc.b);
        renderTooltip(sx, paletteY - 8, tip);
    }

    SDL_RenderPresent(renderer_);
}

void Editor::renderCanvas() {
    float ox, oy;
    canvasOrigin(ox, oy);
    int cw = canvas_.getWidth(), ch = canvas_.getHeight();
    int izoom = std::max(1, (int)std::round(zoom_));
    fillRect(0, canvasAreaTop(), winW_, canvasAreaHeight(), {56, 56, 60, 255});
    int shadowOff = 4;
    int bx = (int)ox, by = (int)oy;
    int bw = (int)(cw * zoom_), bh = (int)(ch * zoom_);
    fillRect(bx + shadowOff, by + shadowOff, bw, bh, {0, 0, 0, 60});

    for (int cy = 0; cy < ch; cy++) {
        for (int cx = 0; cx < cw; cx++) {
            int sx = (int)(ox + cx * zoom_);
            int sy = (int)(oy + cy * zoom_);
            int snx = (int)(ox + (cx + 1) * zoom_);
            int sny = (int)(oy + (cy + 1) * zoom_);
            int pw = snx - sx;
            int ph = sny - sy;
            if (sx + pw < 0 || sx > winW_) continue;
            if (sy + ph < canvasAreaTop() || sy > canvasAreaBottom()) continue;

            Color pc = canvas_.getPixel(cx, cy);
            if (pc.a < 255) {
                int checker = ((cx + cy) % 2 == 0) ? 200 : 240;
                fillRect(sx, sy, pw, ph, {(uint8_t)checker, (uint8_t)checker, (uint8_t)checker, 255});
                if (pc.a > 0) {
                    float a = pc.a / 255.0f;
                    uint8_t br = (uint8_t)(pc.r * a + checker * (1 - a));
                    uint8_t bg = (uint8_t)(pc.g * a + checker * (1 - a));
                    uint8_t bb = (uint8_t)(pc.b * a + checker * (1 - a));
                    fillRect(sx, sy, pw, ph, {br, bg, bb, 255});
                }
            } else {
                fillRect(sx, sy, pw, ph, pc);
            }
        }
    }
    outlineRect(bx - 1, by - 1, bw + 2, bh + 2, {130, 130, 135, 255});
}

void Editor::renderGrid() {
    float ox, oy;
    canvasOrigin(ox, oy);
    int cw = canvas_.getWidth(), ch = canvas_.getHeight();

    uint8_t gridAlpha = (zoom_ < 8.0f) ? 20 : 35;
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, gridAlpha);

    for (int x = 0; x <= cw; x++) {
        int sx = (int)(ox + x * zoom_);
        if (sx >= 0 && sx <= winW_)
            SDL_RenderDrawLine(renderer_, sx, std::max((int)oy, canvasAreaTop()),
                               sx, std::min((int)(oy + ch * zoom_), canvasAreaBottom()));
    }
    for (int y = 0; y <= ch; y++) {
        int sy = (int)(oy + y * zoom_);
        if (sy >= canvasAreaTop() && sy <= canvasAreaBottom())
            SDL_RenderDrawLine(renderer_, std::max((int)ox, 0), sy,
                               std::min((int)(ox + cw * zoom_), winW_), sy);
    }
}

void Editor::renderShapePreview() {
    if (!dragging_) return;
    Point end = screenToCanvas(mouseX_, mouseY_);

    std::vector<Point> pts;
    switch (currentTool_) {
    case Tool::Line:
        pts = Canvas::linePoints(dragStart_.x, dragStart_.y, end.x, end.y);
        break;
    case Tool::Rectangle:
        pts = Canvas::rectPoints(dragStart_.x, dragStart_.y, end.x, end.y);
        break;
    case Tool::Circle: {
        int dx = end.x - dragStart_.x;
        int dy = end.y - dragStart_.y;
        int radius = (int)std::round(std::sqrt(dx * dx + dy * dy));
        pts = Canvas::circlePoints(dragStart_.x, dragStart_.y, radius);
        break;
    }
    default: break;
    }

    float ox, oy;
    canvasOrigin(ox, oy);

    for (auto& p : pts) {
        if (!canvas_.inBounds(p.x, p.y)) continue;
        int sx = (int)(ox + p.x * zoom_);
        int sy = (int)(oy + p.y * zoom_);
        int snx = (int)(ox + (p.x + 1) * zoom_);
        int sny = (int)(oy + (p.y + 1) * zoom_);
        fillRect(sx, sy, snx - sx, sny - sy, {fgColor_.r, fgColor_.g, fgColor_.b, 160});
    }
}

void Editor::renderCursor() {
    if (cursorCX_ < 0 || cursorCY_ < 0) return;
    if (!canvas_.inBounds(cursorCX_, cursorCY_)) return;

    float ox, oy;
    canvasOrigin(ox, oy);
    int sx = (int)(ox + cursorCX_ * zoom_);
    int sy = (int)(oy + cursorCY_ * zoom_);
    int snx = (int)(ox + (cursorCX_ + 1) * zoom_);
    int sny = (int)(oy + (cursorCY_ + 1) * zoom_);
    int pw = snx - sx, ph = sny - sy;
    outlineRect(sx, sy, pw, ph, {255, 255, 255, 200});
    outlineRect(sx - 1, sy - 1, pw + 2, ph + 2, {0, 0, 0, 160});
    if (currentTool_ == Tool::Eraser && zoom_ >= 8) {
        int cx = sx + pw / 2, cy = sy + ph / 2;
        SDL_SetRenderDrawColor(renderer_, 255, 80, 80, 200);
        SDL_RenderDrawLine(renderer_, cx - 2, cy - 2, cx + 2, cy + 2);
        SDL_RenderDrawLine(renderer_, cx + 2, cy - 2, cx - 2, cy + 2);
    } else if (currentTool_ == Tool::ColorPicker && zoom_ >= 8) {
        int cx = sx + pw / 2, cy = sy + ph / 2;
        SDL_SetRenderDrawColor(renderer_, 255, 255, 0, 200);
        SDL_RenderDrawLine(renderer_, cx - 4, cy, cx + 4, cy);
        SDL_RenderDrawLine(renderer_, cx, cy - 4, cx, cy + 4);
    }
}

void Editor::renderToolbar() {
    fillRect(0, 0, winW_, TOOLBAR_H, {32, 32, 36, 255});
    fillRect(0, TOOLBAR_H - 1, winW_, 1, {22, 22, 26, 255});

    int toolCount = (int)Tool::COUNT;
    int btnY = (TOOLBAR_H - TOOL_BTN_SIZE) / 2;

    for (int i = 0; i < toolCount; i++) {
        int bx = TOOL_BTN_PAD + i * (TOOL_BTN_SIZE + TOOL_BTN_PAD);
        int by = btnY;

        bool selected = ((Tool)i == currentTool_);
        bool hovered  = (hoverToolIdx_ == i);

        Color btnBg;
        if (selected)     btnBg = {65, 120, 200, 255};
        else if (hovered) btnBg = {75, 75, 80, 255};
        else              btnBg = {52, 52, 56, 255};

        Color border;
        if (selected)     border = {100, 160, 240, 255};
        else if (hovered) border = {110, 110, 115, 255};
        else              border = {72, 72, 76, 255};
        outlineRect(bx, by, TOOL_BTN_SIZE, TOOL_BTN_SIZE, border);

        // Selected indicator bar at bottom
        if (selected) {
            fillRect(bx + 2, by + TOOL_BTN_SIZE - 3, TOOL_BTN_SIZE - 4, 2, {120, 180, 255, 255});
        }
        char letter[2] = {toolKey((Tool)i), 0};
        int tx = bx + (TOOL_BTN_SIZE - FONT_CHAR_W * 2) / 2;
        int ty = by + (TOOL_BTN_SIZE - FONT_GLYPH_H * 2) / 2 - 1;
        Color textCol = selected ? Color{255,255,255,255} : (hovered ? Color{220,220,225,255} : Color{180,180,185,255});
        drawText(tx, ty, letter, textCol, 2);
    }
    int sepX = TOOL_BTN_PAD + toolCount * (TOOL_BTN_SIZE + TOOL_BTN_PAD) + 2;
    fillRect(sepX, btnY + 4, 1, TOOL_BTN_SIZE - 8, {80, 80, 85, 255});

    int gx = sepX + 8;
    int gy = btnY;
    bool gridHov = hoverGrid_;
    Color gridBg;
    if (showGrid_)       gridBg = {65, 120, 200, 255};
    else if (gridHov)    gridBg = {75, 75, 80, 255};
    else                 gridBg = {52, 52, 56, 255};
    fillRect(gx, gy, TOOL_BTN_SIZE, TOOL_BTN_SIZE, gridBg);
    Color gridBorder;
    if (showGrid_)       gridBorder = {100, 160, 240, 255};
    else if (gridHov)    gridBorder = {110, 110, 115, 255};
    else                 gridBorder = {72, 72, 76, 255};
    outlineRect(gx, gy, TOOL_BTN_SIZE, TOOL_BTN_SIZE, gridBorder);

    int gtx = gx + (TOOL_BTN_SIZE - FONT_CHAR_W * 2) / 2;
    int gty = gy + (TOOL_BTN_SIZE - FONT_GLYPH_H * 2) / 2 - 1;
    drawText(gtx, gty, "G", showGrid_ ? Color{255,255,255,255} : Color{180,180,185,255}, 2);
    int previewX = winW_ - 80;
    int previewY = (TOOLBAR_H - 36) / 2;
    fillRect(previewX + 18, previewY + 12, 24, 24, bgColor_);
    outlineRect(previewX + 18, previewY + 12, 24, 24, {100, 100, 105, 255});
    fillRect(previewX, previewY, 24, 24, fgColor_);
    outlineRect(previewX, previewY, 24, 24, {200, 200, 205, 255});
    if (hoverFgBg_) {
        outlineRect(previewX - 2, previewY - 2, 46, 40, {150, 150, 160, 180});
    }
    drawText(previewX + 7, previewY + 26, "X", {140, 140, 145, 255}, 1);
}

void Editor::renderPalette() {
    int paletteY = winH_ - PALETTE_H - STATUS_H;

    fillRect(0, paletteY, winW_, PALETTE_H, {32, 32, 36, 255});
    fillRect(0, paletteY, winW_, 1, {22, 22, 26, 255});

    int colorsPerRow = 12;
    for (int i = 0; i < PALETTE_SIZE; i++) {
        int row = i / colorsPerRow;
        int col = i % colorsPerRow;
        int sx = SWATCH_PAD + col * (SWATCH_SIZE + SWATCH_PAD);
        int sy = paletteY + SWATCH_PAD + 2 + row * (SWATCH_SIZE + SWATCH_PAD);

        bool isFg = (PALETTE[i] == fgColor_);
        bool isBg = (PALETTE[i] == bgColor_);
        bool hov  = (hoverSwatchIdx_ == i);

        fillRect(sx, sy, SWATCH_SIZE, SWATCH_SIZE, PALETTE[i]);

        if (isFg) {
            outlineRect(sx - 2, sy - 2, SWATCH_SIZE + 4, SWATCH_SIZE + 4, {255, 255, 255, 255});
            outlineRect(sx - 1, sy - 1, SWATCH_SIZE + 2, SWATCH_SIZE + 2, {255, 255, 255, 255});
        } else if (isBg) {
            outlineRect(sx - 2, sy - 2, SWATCH_SIZE + 4, SWATCH_SIZE + 4, {200, 200, 200, 180});
            outlineRect(sx - 1, sy - 1, SWATCH_SIZE + 2, SWATCH_SIZE + 2, {180, 180, 180, 150});
        } else if (hov) {
            outlineRect(sx - 1, sy - 1, SWATCH_SIZE + 2, SWATCH_SIZE + 2, {180, 180, 190, 200});
        } else {
            outlineRect(sx, sy, SWATCH_SIZE, SWATCH_SIZE, {55, 55, 60, 255});
        }
    }
    int labelX = SWATCH_PAD + colorsPerRow * (SWATCH_SIZE + SWATCH_PAD) + 12;
    int labelY1 = paletteY + SWATCH_PAD + 4;
    drawText(labelX, labelY1, "FG", {180, 180, 185, 255}, 1);
    fillRect(labelX + 16, labelY1 - 1, 14, 10, fgColor_);
    outlineRect(labelX + 16, labelY1 - 1, 14, 10, {120, 120, 125, 255});

    int labelY2 = labelY1 + 14;
    drawText(labelX, labelY2, "BG", {140, 140, 145, 255}, 1);
    fillRect(labelX + 16, labelY2 - 1, 14, 10, bgColor_);
    outlineRect(labelX + 16, labelY2 - 1, 14, 10, {120, 120, 125, 255});

    int labelY3 = labelY2 + 14;
    drawText(labelX, labelY3, "L:fg R:bg", {100, 100, 105, 255}, 1);
}

void Editor::renderStatusBar() {
    int statusY = winH_ - STATUS_H;

    fillRect(0, statusY, winW_, STATUS_H, {24, 24, 28, 255});
    fillRect(0, statusY, winW_, 1, {18, 18, 22, 255});

    int ty = statusY + (STATUS_H - FONT_GLYPH_H) / 2;
    int x = 8;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", toolName(currentTool_));
    drawText(x, ty, buf, {130, 180, 240, 255}, 1);
    x += textWidth(buf) + 16;
    if (cursorCX_ >= 0 && cursorCY_ >= 0 && canvas_.inBounds(cursorCX_, cursorCY_)) {
        snprintf(buf, sizeof(buf), "(%d, %d)", cursorCX_, cursorCY_);
        drawText(x, ty, buf, {180, 180, 185, 255}, 1);
        x += textWidth(buf) + 16;
        Color cc = canvas_.getPixel(cursorCX_, cursorCY_);
        fillRect(x, ty - 1, 10, 9, cc);
        outlineRect(x, ty - 1, 10, 9, {120, 120, 125, 255});
        x += 14;
        snprintf(buf, sizeof(buf), "#%02X%02X%02X", cc.r, cc.g, cc.b);
        drawText(x, ty, buf, {140, 140, 145, 255}, 1);
        x += textWidth(buf) + 16;
    }
    snprintf(buf, sizeof(buf), "%dx%d  %.0fx  Undo:%d",
             canvas_.getWidth(), canvas_.getHeight(), zoom_, (int)undoStack_.size());
    int rw = textWidth(buf);
    drawText(winW_ - rw - 8, ty, buf, {120, 120, 125, 255}, 1);
}
