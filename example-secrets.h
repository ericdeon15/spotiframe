// secrets.h
#pragma once

// Toggle network type
#define USE_ENTERPRISE true

// WPA2-Enterprise
static constexpr const char* SSID_ENT    = "ENTNetworkSSID";
#define EAP_IDENTITY "user@random.com"
#define EAP_USERNAME "username"
#define EAP_PASSWORD "password"

// WPA2-PSK
static constexpr const char* SSID_PSK     = "PKNetworkSSID";
static constexpr const char* PSK_PASSWORD = "password";

// Flask/Render host (no scheme, host only)
#define SPOTIFRAME_HOST "your-service.onrender.com"
