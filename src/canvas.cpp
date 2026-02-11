#include "canvas.h"
#include <algorithm>
#include <cmath>
#include <queue>

Canvas::Canvas(int width, int height)
    : width_(width), height_(height), pixels_(width * height, Color(255, 255, 255, 255)) {}

bool Canvas::inBounds(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

Color Canvas::getPixel(int x, int y) const {
    if (!inBounds(x, y)) return {0, 0, 0, 0};
    return pixels_[y * width_ + x];
}

void Canvas::setPixel(int x, int y, const Color& c) {
    if (inBounds(x, y))
        pixels_[y * width_ + x] = c;
}

void Canvas::clear(const Color& c) {
    std::fill(pixels_.begin(), pixels_.end(), c);
}

std::vector<Color> Canvas::snapshot() const {
    return pixels_;
}

void Canvas::restore(const std::vector<Color>& snap) {
    if ((int)snap.size() == width_ * height_)
        pixels_ = snap;
}

void Canvas::resize(int newW, int newH) {
    std::vector<Color> newPixels(newW * newH, Color(255, 255, 255, 255));
    int copyW = std::min(width_, newW);
    int copyH = std::min(height_, newH);
    for (int y = 0; y < copyH; y++)
        for (int x = 0; x < copyW; x++)
            newPixels[y * newW + x] = pixels_[y * width_ + x];
    width_  = newW;
    height_ = newH;
    pixels_ = std::move(newPixels);
}

std::vector<Point> Canvas::linePoints(int x0, int y0, int x1, int y1) {
    std::vector<Point> pts;
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (true) {
        pts.push_back({x0, y0});
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
    return pts;
}

void Canvas::drawLine(int x0, int y0, int x1, int y1, const Color& c) {
    for (auto& p : linePoints(x0, y0, x1, y1))
        setPixel(p.x, p.y, c);
}

std::vector<Point> Canvas::rectPoints(int x0, int y0, int x1, int y1) {
    std::vector<Point> pts;
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    for (int x = x0; x <= x1; x++) {
        pts.push_back({x, y0});
        pts.push_back({x, y1});
    }
    for (int y = y0 + 1; y < y1; y++) {
        pts.push_back({x0, y});
        pts.push_back({x1, y});
    }
    return pts;
}

void Canvas::drawRect(int x0, int y0, int x1, int y1, const Color& c) {
    for (auto& p : rectPoints(x0, y0, x1, y1))
        setPixel(p.x, p.y, c);
}

std::vector<Point> Canvas::circlePoints(int cx, int cy, int radius) {
    std::vector<Point> pts;
    if (radius <= 0) {
        pts.push_back({cx, cy});
        return pts;
    }
    int x = radius, y = 0;
    int err = 1 - radius;
    while (x >= y) {
        pts.push_back({cx + x, cy + y});
        pts.push_back({cx - x, cy + y});
        pts.push_back({cx + x, cy - y});
        pts.push_back({cx - x, cy - y});
        pts.push_back({cx + y, cy + x});
        pts.push_back({cx - y, cy + x});
        pts.push_back({cx + y, cy - x});
        pts.push_back({cx - y, cy - x});
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
    return pts;
}

void Canvas::drawCircle(int cx, int cy, int radius, const Color& c) {
    for (auto& p : circlePoints(cx, cy, radius))
        setPixel(p.x, p.y, c);
}

void Canvas::floodFill(int x, int y, const Color& newColor) {
    if (!inBounds(x, y)) return;
    Color target = getPixel(x, y);
    if (target == newColor) return;

    std::queue<Point> q;
    q.push({x, y});
    setPixel(x, y, newColor);

    const int dx[] = {0, 0, 1, -1};
    const int dy[] = {1, -1, 0, 0};

    while (!q.empty()) {
        Point p = q.front(); q.pop();
        for (int i = 0; i < 4; i++) {
            int nx = p.x + dx[i], ny = p.y + dy[i];
            if (inBounds(nx, ny) && getPixel(nx, ny) == target) {
                setPixel(nx, ny, newColor);
                q.push({nx, ny});
            }
        }
    }
}
