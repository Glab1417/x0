#include <cmath>  
#include <algorithm>

static inline void setPixel(MyBmp& img, int x, int y, uint8_t r=255, uint8_t g=255, uint8_t b=255)
{
    int h = (int)img.data.size();
    if (h == 0) return;
    int w = (int)img.data[0].size();

    if (x < 0 || x >= w || y < 0 || y >= h) return;
    img.data[y][x] = { b, g, r };
}

static inline int pixToLogicX(int xPix, int width) {
    return width / 2 - xPix;
}

static inline int pixToLogicY(int yPix, int height) {
    return height / 2 - yPix;
}

static inline int logicToPixX(int xLogic, int width) {
    return width / 2 - xLogic;
}

static inline int logicToPixY(int yLogic, int height) {
    return height / 2 - yLogic;
}

static inline float logicToTrig(float vLogic, float size, float trigRange = 4.0f) {
    return (vLogic / size) * trigRange;
}

static inline float trigToLogic(float vTrig, float size, float trigRange = 4.0f) {
    return (vTrig * size) / trigRange;
}

void drawSineByY(MyBmp& img, int stepPix = 50, float trigRange = 4.0f)
{
    int h = (int)img.data.size();
    if (h == 0) return;
    int w = (int)img.data[0].size();

    bool havePrev = false;
    int prevX = 0, prevY = 0;

    for (int yPix = 0; yPix < h; yPix += stepPix) {

        float yLogic = (float)pixToLogicY(yPix, h);

        float t = logicToTrig(yLogic, (float)h, trigRange);
      
        float s = std::sinf(t);

        float xLogicF = trigToLogic(s, (float)h, trigRange);

        int xPix = logicToPixX((int)std::lround(xLogicF), w);

        if (havePrev) {
            drawLine(img, prevX, prevY, xPix, yPix);
        } else {
            setPixel(img, xPix, yPix);
            havePrev = true;
        }

        prevX = xPix;
        prevY = yPix;
    }
}
