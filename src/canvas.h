#pragma once
#include "types.h"
#include <vector>

class Canvas {
public:
    Canvas(int width = 32, int height = 32);

    int getWidth()  const { return width_; }
    int getHeight() const { return height_; }

    Color getPixel(int x, int y) const;
    void  setPixel(int x, int y, const Color& c);
    bool  inBounds(int x, int y) const;

    void drawLine(int x0, int y0, int x1, int y1, const Color& c);
    void drawRect(int x0, int y0, int x1, int y1, const Color& c);
    void drawCircle(int cx, int cy, int radius, const Color& c);
    void floodFill(int x, int y, const Color& newColor);

    void clear(const Color& c = {255, 255, 255, 255});

    std::vector<Color> snapshot() const;
    void restore(const std::vector<Color>& snap);

    void resize(int newW, int newH);

    const std::vector<Color>& pixels() const { return pixels_; }

    static std::vector<Point> linePoints(int x0, int y0, int x1, int y1);
    static std::vector<Point> rectPoints(int x0, int y0, int x1, int y1);
    static std::vector<Point> circlePoints(int cx, int cy, int radius);

private:
    int width_, height_;
    std::vector<Color> pixels_;
};
