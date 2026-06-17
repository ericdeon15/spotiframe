#ifndef SPOTIFY_RESPONSE_HPP
#define SPOTIFY_RESPONSE_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

struct SpotifyCurrent {
  String status;
  String title;
  String artist;
  String id;
  String color;
};

void readHttpHeaders(WiFiClient& c);
String readBodyDechunked(WiFiClient& c);
void readBodyBinary(WiFiClient& c, std::vector<uint8_t>& out);
bool parseSpotifyCurrent(const String& jsonPayload, SpotifyCurrent& current);

#endif
