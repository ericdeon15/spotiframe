#include "LGFX.h"
#include <LovyanGFX.hpp>

#ifndef DONUT_H
#define DONUT_H

class DonutScreensaver {
  public:
    void begin(LGFX& display, LGFX_Sprite& sprite); // allocate resources and prepare the sprite
    void update(); // compute and render a single frame
    void draw(); // push the sprite to the screen
    void reset(); // reset rotation

  private:
    // Rendering resources
    LGFX* display; // the display screen to draw to
    LGFX_Sprite* sprite; // sprite to hold the donut frames
    float* depthBuffer; // used to determine and keep the nearest pixels
    bool active; //whether the screen saver is active or not

    // Donut geometry parameters
    float R1, R2; // radii of the cross-section and center-to-center
    float thetaStep, phiStep; // step values to change theta and phi by

    // Projection parameters
    float K; // scaling factor
    float D; // viewing depth

    // Rotation state
    float A, B; // rotation angles around X and Z axes
    float dA, dB; // velocities of the rotations

    // Font metrics
    int fontWidth; // width of the chosen font
    int fontHeight; // height of the chosen font

    // ASCII characters (for brightness)
    const char* brightnessChars; // ASCII chars drawn in scaled by "brightness" factor

    // Sprite dimensions
    int spriteWidth; // width of the donut frame
    int spriteHeight; // height of the donut frame
    int asciiWidth; // width of ASCII grid in characters
    int asciiHeight; // height of ASCII grid in characters
    int cx, cy; // center positions of a frame
};


#endif