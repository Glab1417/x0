
#pragma once

#include <cstdint>
#include <string>
#include <vector>

class MyBmp {
public:
    struct Pixel {
        uint8_t b, g, r;
    };

    MyBmp() = default;
    MyBmp(int height, int width) { resize(height, width); }

    void resize(int height, int width);

    int width() const;
    int height() const;

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    Pixel getPixel(int x, int y) const;

    int load(const std::string& path);
    int save(const std::string& path) const;

    std::vector<std::vector<Pixel>> data;
};

#include "MyBmp.h"

#include <fstream>
#include <cmath>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType;     
    uint32_t bfSize;      
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;   
};

struct BMPInfoHeader {
    uint32_t biSize;         
    int32_t  biWidth;
    int32_t  biHeight;        
    uint16_t biPlanes;       
    uint16_t biBitCount;      
    uint32_t biCompression;   
    uint32_t biSizeImage;     
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

static inline int rowPaddingBytes(int width) {
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
    data[y][x] = Pixel{ b, g, r }; 
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

    fh.bfType = 0x4D42;
    fh.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    fh.bfSize = fh.bfOffBits + pixelDataSize;

    ih.biSize = sizeof(BMPInfoHeader);
    ih.biWidth = W;
    ih.biHeight = H; 
    ih.biPlanes = 1;
    ih.biBitCount = 24;
    ih.biCompression = 0;
    ih.biSizeImage = pixelDataSize;
    ih.biXPelsPerMeter = 2835; 
    ih.biYPelsPerMeter = 2835;

    std::ofstream out(path, std::ios::binary);
    if (!out) return -1;

    out.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    out.write(reinterpret_cast<const char*>(&ih), sizeof(ih));

    const char padBytes[3] = {0, 0, 0};

    for (int y = H - 1; y >= 0; --y) {
        for (int x = 0; x < W; ++x) {
            const Pixel& p = data[y][x];
            out.write(reinterpret_cast<const char*>(&p), 3); 
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
    if (fh.bfType != 0x4D42) return -1;      
    if (ih.biBitCount != 24) return -1;      
    if (ih.biCompression != 0) return -1;    
    if (ih.biWidth <= 0 || ih.biHeight == 0) return -1;

    int W = ih.biWidth;
    int H = std::abs(ih.biHeight);
    bool bottomUp = (ih.biHeight > 0);

    resize(H, W);

    int pad = rowPaddingBytes(W);

    
    in.seekg((std::streamoff)fh.bfOffBits, std::ios::beg);
    if (!in) return -1;

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

#include <iostream>
#include <cmath>
#include "MyBmp.h"


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


static inline int pix_to_coord(int pix, int size) { return size / 2 - pix; }
static inline int coord_to_pix(int coord, int size) { return size / 2 - coord; }

static inline float coord_to_float(int coord, int size) {
    return (float)coord / (float)size * 4.0f;
}
static inline int float_to_coord(float v, int size) {
    return (int)std::lround(v * (float)size / 4.0f);
}

static inline void coord_to_pix_xy(int coordX, int coordY, int& xPix, int& yPix, int W, int H)
{
    xPix = coord_to_pix(coordY, W); 
    yPix = coord_to_pix(coordX, H); 
}


void drawAxes(MyBmp& img) {
    int H = img.height();
    int W = img.width();
    int cx = W / 2;
    int cy = H / 2;


    drawLine(img, 0, cy, W - 1, cy);
    drawLine(img, cx, 0, cx, H - 1);
}

void drawSine_Scheme(MyBmp& img, int stepPix = 50) {
    int H = img.height();
    int W = img.width();
    if (H <= 0 || W <= 0) return;

    bool havePrev = false;
    int prevXPix = 0, prevYPix = 0;

    for (int yPix = 0; yPix < H; yPix += stepPix) {

        int coordY = pix_to_coord(yPix, H);
        float yFloat = coord_to_float(coordY, H);

        float xFloat = std::sinf(yFloat);

        int coordX = float_to_coord(xFloat, H);
        
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

    drawAxes(img);
    drawSine_Scheme(img, 50);

    if (img.save("out.bmp") != 0) {
        std::cout << "Save error\n";
        return 1;
    }

    std::cout << "Saved: out.bmp\n";
    return 0;
}
