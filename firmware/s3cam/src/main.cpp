#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include "wifi_credentials.h"

namespace Network {
WebServer server(80);
camera_fb_t *lastFrame = nullptr;
bool cameraReady = false;
HardwareSerial decisionSerial(1);
char lastDecision[32] = "idle";
char lastProposalId[32] = "none";
char lastDecisionReason[48] = "none";
}

static void releaseFrame() {
  if (Network::lastFrame) {
    esp_camera_fb_return(Network::lastFrame);
    Network::lastFrame = nullptr;
  }
}

static bool initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CameraPins::Y2;
  config.pin_d1 = CameraPins::Y3;
  config.pin_d2 = CameraPins::Y4;
  config.pin_d3 = CameraPins::Y5;
  config.pin_d4 = CameraPins::Y6;
  config.pin_d5 = CameraPins::Y7;
  config.pin_d6 = CameraPins::Y8;
  config.pin_d7 = CameraPins::Y9;
  config.pin_xclk = CameraPins::Xclk;
  config.pin_pclk = CameraPins::Pclk;
  config.pin_vsync = CameraPins::Vsync;
  config.pin_href = CameraPins::Href;
  config.pin_sccb_sda = CameraPins::Siod;
  config.pin_sccb_scl = CameraPins::Sioc;
  config.pin_pwdn = CameraPins::Pwdn;
  config.pin_reset = CameraPins::Reset;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 14;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (esp_camera_init(&config) != ESP_OK) {
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_framesize(s, FRAMESIZE_VGA);
  return true;
}

static void connectWiFi() {
  WiFi.mode(WIFI_STA);
  if (strlen(WIFI_SSID) == 0) {
    Serial.println("wifi provisioning missing");
    return;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("connecting to %s\n", WIFI_SSID);
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("ip=");
    Serial.println(WiFi.localIP());
  }
}

static void initDecisionLink() {
  Network::decisionSerial.begin(115200, SERIAL_8N1, 44, 43);
}

static void processDecisionLine(const char *line) {
  if (strstr(line, "\"t\":\"decision\"") == nullptr) return;
  if (strstr(line, "\"decision\":\"approve\"")) strlcpy(Network::lastDecision, "approved", sizeof(Network::lastDecision));
  else if (strstr(line, "\"decision\":\"deny\"")) strlcpy(Network::lastDecision, "denied", sizeof(Network::lastDecision));
  else if (strstr(line, "\"decision\":\"blocked\"")) strlcpy(Network::lastDecision, "blocked", sizeof(Network::lastDecision));
  else strlcpy(Network::lastDecision, "unknown", sizeof(Network::lastDecision));

  const char *pid = strstr(line, "\"proposal_id\":\"");
  if (pid) {
    pid += strlen("\"proposal_id\":\"");
    char *out = Network::lastProposalId;
    size_t i = 0;
    while (*pid && *pid != '"' && i + 1 < sizeof(Network::lastProposalId)) {
      out[i++] = *pid++;
    }
    out[i] = 0;
  }
  const char *reason = strstr(line, "\"reason\":\"");
  if (reason) {
    reason += strlen("\"reason\":\"");
    size_t i = 0;
    while (*reason && *reason != '"' && i + 1 < sizeof(Network::lastDecisionReason)) {
      Network::lastDecisionReason[i++] = *reason++;
    }
    Network::lastDecisionReason[i] = 0;
  }
}

static void pollDecisionLink() {
  static char line[256];
  static size_t used = 0;
  while (Network::decisionSerial.available()) {
    char c = static_cast<char>(Network::decisionSerial.read());
    if (c == '\n') {
      line[used] = 0;
      processDecisionLine(line);
      used = 0;
    } else if (used + 1 < sizeof(line)) {
      line[used++] = c;
    } else {
      used = 0;
    }
  }
}

static void handleRoot() {
  Network::server.send(200, "text/plain", "S3 camera online");
}

static void handleHealth() {
  String json = "{\"ok\":true,\"wifi\":";
  json += (WiFi.status() == WL_CONNECTED ? "\"online\"" : "\"offline\"");
  json += ",\"camera\":";
  json += (Network::cameraReady ? "\"ready\"" : "\"fault\"");
  json += ",\"decision\":";
  json += "\"";
  json += Network::lastDecision;
  json += "\"";
  json += "}";
  Network::server.send(200, "application/json", json);
}

static void handleSnapshot() {
  if (WiFi.status() != WL_CONNECTED || !Network::cameraReady) {
    Network::server.send(503, "application/json", "{\"error\":\"camera_unavailable\"}");
    return;
  }
  releaseFrame();
  Network::lastFrame = esp_camera_fb_get();
  if (!Network::lastFrame) {
    Network::server.send(503, "application/json", "{\"error\":\"capture_failed\"}");
    return;
  }
  Network::server.setContentLength(Network::lastFrame->len);
  Network::server.sendHeader("Cache-Control", "no-store, max-age=0");
  Network::server.send(200, "image/jpeg", "");
  Network::server.client().write(Network::lastFrame->buf, Network::lastFrame->len);
  releaseFrame();
}

static void startServer() {
  Network::server.on("/", handleRoot);
  Network::server.on("/health", handleHealth);
  Network::server.on("/snapshot.jpg", handleSnapshot);
  Network::server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  connectWiFi();
  initDecisionLink();
  Network::cameraReady = initCamera();
  startServer();
}

void loop() {
  pollDecisionLink();
  Network::server.handleClient();
  delay(2);
}
