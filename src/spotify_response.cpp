#include "spotify_response.hpp"

#include <ArduinoJson.h>

static constexpr uint32_t SOCKET_TIMEOUT_MS = 10000;

void readHttpHeaders(WiFiClient& c) {
  while (c.connected()) {
    String line = c.readStringUntil('\n');
    if (line == "\r" || line == "") break;
  }
}

String readBodyDechunked(WiFiClient& c) {
  String body;

  while (c.connected() || c.available()) {
    body += c.readString();
    delay(10);
  }

  int start = body.indexOf('{');
  int end = body.lastIndexOf('}');

  if (start == -1 || end == -1 || end <= start) {
    Serial.println("No valid JSON object found in response.");
    Serial.println(body);
    return "";
  }

  String jsonPayload = body.substring(start, end + 1);

  Serial.println("Cleaned JSON payload:");
  Serial.println(jsonPayload);
  Serial.println("========================");

  return jsonPayload;
}

void readBodyBinary(WiFiClient& c, std::vector<uint8_t>& out) {
  out.clear();
  c.setTimeout(SOCKET_TIMEOUT_MS);

  while (c.connected() || c.available()) {
    while (c.available()) {
      out.push_back(c.read());
    }
    delay(10);
  }
}

bool parseSpotifyCurrent(const String& jsonPayload, SpotifyCurrent& current) {
  JsonDocument doc;

  if (deserializeJson(doc, jsonPayload)) {
    Serial.println("JSON parse error");
    return false;
  }

  current.status = doc["status"].as<String>();
  current.title = doc["title"].as<String>();
  current.artist = doc["artist"].as<String>();
  current.id = doc["id"].as<String>();
  current.color = doc["color"].as<String>();

  return true;
}
