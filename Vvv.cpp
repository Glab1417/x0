// MyBmp.h
#pragma once

#include <cstdint>
#include <string>
#include <vector>

class MyBmp {
public:
    // BMP 24-bit Pixel (BGR)
    struct Pixel {
        uint8_t b, g, r;
    };

    MyBmp() = default;
    MyBmp(int height, int width) { resize(height, width); }

    void resize(int height, int width);

    int width() const;
    int height() const;

    // Safe pixel operations
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    Pixel getPixel(int x, int y) const;

    // BMP I/O (24-bit uncompressed)
    int load(const std::string& path);
    int save(const std::string& path) const;

    // Public for simplicity (you can access img.data[y][x])
    std::vector<std::vector<Pixel>> data;
};

// MyBmp.cpp
#include "MyBmp.h"

#include <fstream>
#include <cmath>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType;      // 'BM' = 0x4D42
    uint32_t bfSize;      // file size
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;   // offset to pixel data
};

struct BMPInfoHeader {
    uint32_t biSize;          // 40
    int32_t  biWidth;
    int32_t  biHeight;        // positive = bottom-up
    uint16_t biPlanes;        // 1
    uint16_t biBitCount;      // 24
    uint32_t biCompression;   // 0 (BI_RGB)
    uint32_t biSizeImage;     // can be 0 for BI_RGB, but we fill it
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

static inline int rowPaddingBytes(int width) {
    // each pixel 3 bytes, row size must be multiple of 4
    int rowBytes = width * 3;
    return (4 - (rowBytes % 4)) % 4;
}

void MyBmp::resize(int height, int width) {
    if (height < 0) height = 0;
    if (width < 0) width = 0;
    data.assign(height, std::vector<Pixel>(width, Pixel{0, 0, 0}));
}

int MyBmp::width() const {
    return data.empty() ? 0 : (int)data[0].size();
}

int MyBmp::height() const {
    return (int)data.size();
}

void MyBmp::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    int H = height();
    int W = width();
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    data[y][x] = Pixel{ b, g, r }; // BGR
}

MyBmp::Pixel MyBmp::getPixel(int x, int y) const {
    int H = height();
    int W = width();
    if (x < 0 || x >= W || y < 0 || y >= H) return Pixel{0, 0, 0};
    return data[y][x];
}

int MyBmp::save(const std::string& path) const {
    int H = height();
    int W = width();
    if (H <= 0 || W <= 0) return -1;

    int pad = rowPaddingBytes(W);
    uint32_t rowSize = (uint32_t)(W * 3 + pad);
    uint32_t pixelDataSize = rowSize * (uint32_t)H;

    BMPFileHeader fh{};
    BMPInfoHeader ih{};

    fh.bfType = 0x4D42; // 'BM'
    fh.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    fh.bfSize = fh.bfOffBits + pixelDataSize;

    ih.biSize = sizeof(BMPInfoHeader);
    ih.biWidth = W;
    ih.biHeight = H; // bottom-up
    ih.biPlanes = 1;
    ih.biBitCount = 24;
    ih.biCompression = 0;
    ih.biSizeImage = pixelDataSize;
    ih.biXPelsPerMeter = 2835; // ~72 DPI
    ih.biYPelsPerMeter = 2835;

    std::ofstream out(path, std::ios::binary);
    if (!out) return -1;

    out.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    out.write(reinterpret_cast<const char*>(&ih), sizeof(ih));

    // BMP bottom-up: write from last row to first
    const char padBytes[3] = {0, 0, 0};

    for (int y = H - 1; y >= 0; --y) {
        for (int x = 0; x < W; ++x) {
            const Pixel& p = data[y][x];
            out.write(reinterpret_cast<const char*>(&p), 3); // BGR
        }
        out.write(padBytes, pad);
    }

    return out ? 0 : -1;
}

int MyBmp::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return -1;

    BMPFileHeader fh{};
    BMPInfoHeader ih{};

    in.read(reinterpret_cast<char*>(&fh), sizeof(fh));
    in.read(reinterpret_cast<char*>(&ih), sizeof(ih));

    if (!in) return -1;
    if (fh.bfType != 0x4D42) return -1;      // not BM
    if (ih.biBitCount != 24) return -1;      // only 24-bit supported
    if (ih.biCompression != 0) return -1;    // only BI_RGB supported
    if (ih.biWidth <= 0 || ih.biHeight == 0) return -1;

    int W = ih.biWidth;
    int H = std::abs(ih.biHeight);
    bool bottomUp = (ih.biHeight > 0);

    resize(H, W);

    int pad = rowPaddingBytes(W);

    // jump to pixel data
    in.seekg((std::streamoff)fh.bfOffBits, std::ios::beg);
    if (!in) return -1;

    // read rows
    for (int row = 0; row < H; ++row) {
        int y = bottomUp ? (H - 1 - row) : row;

        for (int x = 0; x < W; ++x) {
            Pixel p{};
            in.read(reinterpret_cast<char*>(&p), 3);
            if (!in) return -1;
            data[y][x] = p;
        }
        in.ignore(pad);
        if (!in) return -1;
    }

    return 0;
}

// main.cpp
#include <iostream>
#include <cmath>
#include "MyBmp.h"

// ---------- Bresenham line (точно работает) ----------
void drawLine(MyBmp& img, int x1, int y1, int x2, int y2) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;

    while (true) {
        img.setPixel(x1, y1, 255, 255, 255);

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

// ---------- Преобразования "как на схеме" ----------
// pix <-> coord: size/2 - x
static inline int pix_to_coord(int pix, int size) { return size / 2 - pix; }
static inline int coord_to_pix(int coord, int size) { return size / 2 - coord; }

// coord <-> float: /size*4 и *size/4
static inline float coord_to_float(int coord, int size) {
    return (float)coord / (float)size * 4.0f;
}
static inline int float_to_coord(float v, int size) {
    return (int)std::lround(v * (float)size / 4.0f);
}

// На схеме: coord.x = вертикаль (в пикселях это yPix)
//           coord.y = горизонталь (в пикселях это xPix)
static inline void coord_to_pix_xy(int coordX, int coordY,
                                  int& xPix, int& yPix,
                                  int W, int H)
{
    xPix = coord_to_pix(coordY, W); // горизонталь
    yPix = coord_to_pix(coordX, H); // вертикаль
}

// ---------- Рисуем оси ----------
void drawAxes(MyBmp& img) {
    int H = img.height();
    int W = img.width();
    int cx = W / 2;
    int cy = H / 2;

    // ось X (горизонталь)
    drawLine(img, 0, cy, W - 1, cy);
    // ось Y (вертикаль)
    drawLine(img, cx, 0, cx, H - 1);
}

// ---------- Синусоида "строго по схеме": x = sin(y) ----------
void drawSine_Scheme(MyBmp& img, int stepPix = 50) {
    int H = img.height();
    int W = img.width();
    if (H <= 0 || W <= 0) return;

    bool havePrev = false;
    int prevXPix = 0, prevYPix = 0;

    // идём по yPix с шагом stepPix
    for (int yPix = 0; yPix < H; yPix += stepPix) {

        // (1) pix -> coord: берём coordY из yPix (потому что на схеме горизонталь = y)
        int coordY = pix_to_coord(yPix, H);

        // (3) coordY -> float (триг ось)
        float yFloat = coord_to_float(coordY, H);

        // (4) sin
        float xFloat = std::sinf(yFloat);

        // (5) float -> coordX
        int coordX = float_to_coord(xFloat, H);

        // (6) coord -> pix
        int xPix2, yPix2;
        coord_to_pix_xy(coordX, coordY, xPix2, yPix2, W, H);

        if (havePrev) drawLine(img, prevXPix, prevYPix, xPix2, yPix2);
        else {
            img.setPixel(xPix2, yPix2, 255, 255, 255);
            havePrev = true;
        }

        prevXPix = xPix2;
        prevYPix = yPix2;
    }
}

int main() {
    const int W = 1024;
    const int H = 1024;

    MyBmp img(H, W);

    // фон чёрный уже по умолчанию
    drawAxes(img);
    drawSine_Scheme(img, 50);

    if (img.save("out.bmp") != 0) {
        std::cout << "Save error\n";
        return 1;
    }

    std::cout << "Saved: out.bmp\n";
    return 0;
}
