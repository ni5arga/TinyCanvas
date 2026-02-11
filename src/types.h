#pragma once
#include <cstdint>
#include <vector>

struct Color {
    uint8_t r, g, b, a;
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

struct Point {
    int x, y;
    Point() : x(0), y(0) {}
    Point(int x, int y) : x(x), y(y) {}
};

enum class Tool {
    Pencil,
    Eraser,
    Line,
    Rectangle,
    Circle,
    Fill,
    ColorPicker,
    COUNT
};

inline const char* toolName(Tool t) {
    switch (t) {
        case Tool::Pencil:      return "Pencil";
        case Tool::Eraser:      return "Eraser";
        case Tool::Line:        return "Line";
        case Tool::Rectangle:   return "Rectangle";
        case Tool::Circle:      return "Circle";
        case Tool::Fill:        return "Fill";
        case Tool::ColorPicker: return "Picker";
        default:                return "?";
    }
}

inline char toolKey(Tool t) {
    switch (t) {
        case Tool::Pencil:      return 'P';
        case Tool::Eraser:      return 'E';
        case Tool::Line:        return 'L';
        case Tool::Rectangle:   return 'R';
        case Tool::Circle:      return 'C';
        case Tool::Fill:        return 'F';
        case Tool::ColorPicker: return 'I';
        default:                return '?';
    }
}

const Color PALETTE[] = {
    {  0,   0,   0},       // Black
    {127, 127, 127},       // Dark Gray
    {195, 195, 195},       // Light Gray
    {255, 255, 255},       // White
    {136,   0,  21},       // Dark Red
    {237,  28,  36},       // Red
    {255, 127,  39},       // Orange
    {255, 242,   0},       // Yellow
    {34,  177,  76},       // Green
    {0,   162, 232},       // Light Blue
    {63,   72, 204},       // Blue
    {163,  73, 164},       // Purple

    {185, 122,  87},       // Brown
    {255, 174, 201},       // Pink
    {255, 201,  14},       // Gold
    {153, 217, 234},       // Sky Blue
    {112, 146, 190},       // Steel Blue
    {200, 191, 231},       // Lavender
    {255, 127, 127},       // Light Red
    {127, 255, 127},       // Light Green
    {127, 127, 255},       // Light Purple
    {255, 255, 127},       // Light Yellow
    {127, 255, 255},       // Cyan
    {255, 127, 255},       // Magenta
};

const int PALETTE_SIZE = sizeof(PALETTE) / sizeof(PALETTE[0]);
