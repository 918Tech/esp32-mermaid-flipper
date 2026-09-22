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
bool apMode = false;
char apSsid[32] = "918-AD5X-CAM";
uint32_t nextIdentityMs = 0;
}

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#050914">
<title>918 AD5X Watch Camera</title>
<style>
:root{color-scheme:dark;--bg:#050914;--panel:#0b1220;--line:#223450;--text:#eef6ff;--muted:#8ea6c7;--cyan:#39dfff;--green:#35e590;--red:#ff667e}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 12% 0%,#0c2940 0,transparent 32%),var(--bg);color:var(--text);font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
main{max-width:980px;margin:auto;padding:14px}.card{background:rgba(11,18,32,.96);border:1px solid var(--line);border-radius:18px;padding:15px;margin:12px 0;box-shadow:0 16px 40px #0006}
.brand{display:flex;align-items:center;gap:12px}.logo{width:52px;height:52px;border:1px solid #315377;border-radius:14px;display:grid;place-items:center;color:var(--cyan);font-weight:900}
h1{font-size:clamp(22px,5vw,34px);margin:0}.muted{color:var(--muted)}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:9px}
@media(max-width:700px){.grid{grid-template-columns:repeat(2,1fr)}}.metric{background:#07101c;border:1px solid #1d3048;border-radius:12px;padding:11px}.metric b{display:block;font-size:18px;margin-top:4px}
.view{position:relative;min-height:260px;background:#02060b;border:1px solid var(--line);border-radius:16px;overflow:hidden;display:grid;place-items:center}
.view img{width:100%;height:auto;display:block}.pill{display:inline-flex;gap:7px;align-items:center;border:1px solid var(--line);border-radius:999px;padding:7px 10px;font-size:12px}
.dot{width:9px;height:9px;border-radius:50%;background:var(--red)}.dot.ok{background:var(--green)}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center;justify-content:space-between}.small{font-size:12px}
button{background:#10243b;color:var(--text);border:1px solid #315377;border-radius:10px;padding:9px 12px;font-weight:700}
</style>
</head>
<body>
<main>
<section class="card">
  <div class="brand"><div class="logo">918</div><div><h1>AD5X Watch Camera</h1><div class="muted">ESP32-S3 N16R8 • 918 Technologies</div></div></div>
</section>
<section class="card">
  <div class="row"><div class="pill"><span id="dot" class="dot"></span><span id="state">connecting</span></div><button onclick="snap()">Refresh frame</button></div>
  <div class="grid" style="margin-top:12px">
    <div class="metric"><span class="muted small">Camera</span><b id="camera">—</b></div>
    <div class="metric"><span class="muted small">Network</span><b id="network">—</b></div>
    <div class="metric"><span class="muted small">RSSI</span><b id="rssi">—</b></div>
    <div class="metric"><span class="muted small">Uptime</span><b id="uptime">—</b></div>
  </div>
</section>
<section class="card">
  <div class="view"><img id="frame" alt="AD5X camera view"></div>
  <div class="row small muted" style="margin-top:10px"><span id="ip">IP: —</span><span id="mem">Flash/PSRAM: —</span></div>
</section>
</main>
<script>
const frame=document.getElementById('frame');
function snap(){frame.src='/snapshot.jpg?t='+Date.now()}
async function poll(){
  try{
    const r=await fetch('/health',{cache:'no-store'});
    const j=await r.json();
    document.getElementById('camera').textContent=j.camera;
    document.getElementById('network').textContent=j.mode+' / '+j.wifi;
    document.getElementById('rssi').textContent=(j.rssi===null?'n/a':j.rssi+' dBm');
    document.getElementById('uptime').textContent=Math.floor(j.uptime_ms/1000)+' s';
    document.getElementById('ip').textContent='IP: '+j.ip;
    document.getElementById('mem').textContent='Flash '+j.flash_mb+' MB / PSRAM '+j.psram_mb+' MB';
    const ok=j.camera==='ready';
    document.getElementById('dot').className='dot'+(ok?' ok':'');
    document.getElementById('state').textContent=ok?'camera online':'camera fault';
  }catch(e){
    document.getElementById('state').textContent='offline';
    document.getElementById('dot').className='dot';
  }
}
setInterval(poll,1500);
setInterval(snap,900);
poll();snap();
</script>
</body>
</html>
)HTML";

static void addCors() {
  Network::server.sendHeader("Access-Control-Allow-Origin", "*");
  Network::server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  Network::server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

static String currentIp() {
  if (Network::apMode) return WiFi.softAPIP().toString();
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return String("0.0.0.0");
}

static void emitIdentity() {
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - Network::nextIdentityMs) < 0) return;
  Network::nextIdentityMs = now + 2000;
  Serial.print("AD5X_CAM_HELLO role=WATCH_CAM hw=ESP32-S3-N16R8 proto=AD5X1 flash=");
  Serial.print(ESP.getFlashChipSize());
  Serial.print(" psram=");
  Serial.print(ESP.getPsramSize());
  Serial.print(" camera=");
  Serial.print(Network::cameraReady ? "ready" : "fault");
  Serial.print(" mode=");
  Serial.print(Network::apMode ? "ap" : "sta");
  Serial.print(" ip=");
  Serial.println(currentIp());
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
  config.jpeg_quality = 12;
  config.fb_count = psramFound() ? 2 : 1;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  const esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("camera init failed: 0x%x\n", static_cast<unsigned>(err));
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_framesize(s, FRAMESIZE_VGA);
  s->set_quality(s, 12);
  return true;
}

static void startAccessPoint() {
  Network::apMode = true;
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.mode(WIFI_AP);
  const uint64_t mac = ESP.getEfuseMac();
  snprintf(Network::apSsid, sizeof(Network::apSsid), "918-AD5X-CAM-%06llX",
           static_cast<unsigned long long>(mac & 0xFFFFFFULL));
  const bool ok = WiFi.softAP(Network::apSsid, "918AD5XCAM");
  Serial.printf("AP %s ssid=%s ip=%s\n",
                ok ? "ready" : "failed",
                Network::apSsid,
                WiFi.softAPIP().toString().c_str());
  Serial.println("AP password=918AD5XCAM");
}

static void connectNetwork() {
  if (strlen(WIFI_SSID) > 0) {
    Network::apMode = false;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("connecting to %s\n", WIFI_SSID);
    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
      delay(250);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("station ip=");
      Serial.println(WiFi.localIP());
      return;
    }
    Serial.println("station connection failed; falling back to AP mode");
  } else {
    Serial.println("no Wi-Fi credentials; starting camera AP");
  }
  startAccessPoint();
}

static void handleRoot() {
  addCors();
  Network::server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

static void handleHealth() {
  addCors();
  const bool staOnline = WiFi.status() == WL_CONNECTED;
  String json;
  json.reserve(320);
  json += "{\"ok\":true";
  json += ",\"camera\":\"";
  json += Network::cameraReady ? "ready" : "fault";
  json += "\"";
  json += ",\"wifi\":\"";
  json += (Network::apMode || staOnline) ? "online" : "offline";
  json += "\"";
  json += ",\"mode\":\"";
  json += Network::apMode ? "ap" : "sta";
  json += "\"";
  json += ",\"ip\":\"";
  json += currentIp();
  json += "\"";
  json += ",\"ssid\":\"";
  json += Network::apMode ? Network::apSsid : WiFi.SSID();
  json += "\"";
  json += ",\"rssi\":";
  if (!Network::apMode && staOnline) json += String(WiFi.RSSI());
  else json += "null";
  json += ",\"uptime_ms\":";
  json += String(millis());
  json += ",\"flash_mb\":";
  json += String(ESP.getFlashChipSize() / (1024 * 1024));
  json += ",\"psram_mb\":";
  json += String(ESP.getPsramSize() / (1024 * 1024));
  json += "}";
  Network::server.send(200, "application/json", json);
}

static void handleSnapshot() {
  addCors();
  if (!Network::cameraReady) {
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

static void handleOptions() {
  addCors();
  Network::server.send(204);
}

static void startServer() {
  Network::server.on("/", HTTP_GET, handleRoot);
  Network::server.on("/health", HTTP_GET, handleHealth);
  Network::server.on("/status.json", HTTP_GET, handleHealth);
  Network::server.on("/snapshot.jpg", HTTP_GET, handleSnapshot);
  Network::server.onNotFound([]() {
    if (Network::server.method() == HTTP_OPTIONS) handleOptions();
    else {
      addCors();
      Network::server.send(404, "application/json", "{\"error\":\"not_found\"}");
    }
  });
  Network::server.begin();
  Serial.print("watch page=http://");
  Serial.print(currentIp());
  Serial.println("/");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("[918 Technologies] AD5X ESP32-S3 watch camera boot");
  connectNetwork();
  Network::cameraReady = initCamera();
  startServer();
  emitIdentity();
}

void loop() {
  Network::server.handleClient();
  emitIdentity();
  delay(2);
}
