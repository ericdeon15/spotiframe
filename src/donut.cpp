#include "donut.hpp"
#include <LovyanGFX.hpp>
#include "LGFX.hpp"

#include <HardwareSerial.h>

extern HardwareSerial Serial0;

static constexpr float PI_ = 3.14159265f;

void DonutScreensaver::begin(LGFX& display, LGFX_Sprite& sprite) {
  Serial0.println("BEGIN");

  // Store the display and sprite pointers
  this->display = &display;
  this->sprite  = &sprite;
  
  // Configure font metrics;
  this->display->setFont(&fonts::Font0);
  this->fontWidth = this->display->textWidth("."); // mono-space font, so any character works
  this->fontHeight = this->display->fontHeight();

  // Choose and assign donut parameters
  this->R1 = 1; // arbitrary due to scaling during projection
  this->R2 = 2; // arbitrary due to scaling during projection
  this->thetaStep = 0.08;
  this->phiStep = 0.03;

  // Choose projection constants
  this->D = 6;
  this->K = 200;
  
  // Sprite members
  this->spriteWidth = 300;
  this->spriteHeight = 300;
  this->cx = spriteWidth * 0.5f;
  this->cy = spriteHeight * 0.45f;


  // Compute ASCII grid size
  this->asciiWidth = this->spriteWidth  / this->fontWidth;
  this->asciiHeight = this->spriteHeight / this->fontHeight;
  
  Serial0.print("asciiWidth: ");
  Serial0.println(asciiWidth);

  Serial0.print("asciiHeight: ");
  Serial0.println(asciiHeight);


  // Allocate the depth buffer once so the sprite can be released between polls.
  int bufferSize = this->asciiWidth * this->asciiHeight;
  if (this->depthBuffer == nullptr) {
    this->depthBuffer = new float[bufferSize];
  }

  for (int i = 0; i < bufferSize; i++) {
    depthBuffer[i] = -1e9;
  }

  if (!this->initialized) {
    // Initialize rotation parameters
    this->A = 0;
    this->B = 0;
    this->dA = 0.1;
    this->dB = 0.1;

    // Initialize the brightness characters
    this->brightnessChars = ".,-~:;=!*#$@";
    this->initialized = true;
  }
}

void DonutScreensaver::update() {
  // "Clear" the sprite
  this->sprite->fillScreen(TFT_BLACK);

  // Reset the depth buffer
  int bufferSize = this->asciiWidth * this->asciiHeight;
  for (int i = 0; i < bufferSize; i++) {
    depthBuffer[i] = -1e9;
  }

  // Increment A and B
  this->A += this->dA;
  this->B += this->dB;

  // Precompute sin and cos vals
  float sinA = sinf(this->A);
  float cosA = cosf(this->A);
  
  float sinB = sinf(this->B);
  float cosB = cosf(this->B);

  // Normalized light vector
  float lx = 0;
  float ly = 0.7071f;
  float lz = -0.7071f;

  // Compute, rotate, and project each donut point
  for (float theta = 0; theta < 2 * PI_; theta += this->thetaStep) {
    // Compute sinθ, cosθ
    float sinT = sinf(theta);
    float cosT = cosf(theta);
    for (float phi = 0; phi < 2 * PI_; phi += this->phiStep) {
        // Compute sinφ, cosφ
        float sinP = sinf(phi);
        float cosP = cosf(phi);

        // Compute unrotated 3D coords and normal
        float circleX = this->R2 + this->R1 * cosT; // distance from center of donut to current point in the cross-section
        float x = circleX * cosP;
        float y = circleX * sinP;
        float z = this->R1 * sinT;

        float nx = cosT * cosP;
        float ny = cosT * sinP;
        float nz = sinT;

        // Rotate point and normal

        // X-axis rotation
        float xA = x;
        float yA = y * cosA - z * sinA;
        float zA = y * sinA + z * cosA;
        // Z-axis rotation
        float xB = xA * cosB - yA * sinB;
        float yB = xA * sinB + yA * cosB;
        float zB = zA;

        // X-axis rotation
        float nxA = nx;
        float nyA = ny * cosf(this->A) - nz * sinf(this->A);
        float nzA = ny * sinf(this->A) + nz * cosf(this->A);
        // Z-axis rotation
        float nxB = nxA * cosf(this->B) - nyA * sinf(this->B);
        float nyB = nxA * sinf(this->B) + nyA * cosf(this->B);
        float nzB = nzA;

        // Lighting

        // dot(normal, light)
        float brightness = nxB * lx + nyB * ly + nzB * lz;
        if (brightness < 0) brightness = 0;

        // Map brightness to ASCII
        int idx = brightness * (strlen(this->brightnessChars) - 1);

        if (idx < 0) idx = 0;
        if (idx >= strlen(this->brightnessChars)) {
          idx = strlen(this->brightnessChars) - 1;
        }

        char c = this->brightnessChars[idx];

        // Project

        float invZ = 1.0f / (zB + this->D);

        float xp = this->cx + this->K * xB * invZ;
        float yp = this->cy - this->K * yB * invZ;

        // ASCII cell
        int px = (int)xp;
        int py = (int)yp;

        int ax = px / this->fontWidth;
        int ay = py / this->fontHeight;

        if (ax < 0 || ax >= asciiWidth) continue;
        if (ay < 0 || ay >= asciiHeight) continue;

        // Draw point if it is nearer that currently drawn point
        int depthIndex = ay * asciiWidth + ax;

        if (invZ > depthBuffer[depthIndex]) {
            depthBuffer[depthIndex] = invZ;

            int drawX = ax * this->fontWidth;
            int drawY = ay * this->fontHeight;

            char buf[2] = { c, '\0' };
            sprite->drawString(buf, drawX, drawY);

        }
    }
  }
}

void DonutScreensaver::draw() {
    this->display->pushImage(250, 175, spriteWidth, spriteHeight, (lgfx::rgb565_t*)sprite->getBuffer());
}
