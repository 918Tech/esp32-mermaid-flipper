#include <Arduino.h>
#include "ui/UiController.h"

static UiController ui;
static uint32_t nextIdentityMs = 0;

static void emitIdentity() {
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - nextIdentityMs) < 0) return;
  nextIdentityMs = now + 2000;
  Serial.println("MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1");
}

void setup() {
  Serial.begin(115200);
  delay(150);
  ui.begin();
  emitIdentity();
}

void loop() {
  ui.loop();
  emitIdentity();
  delay(5);
}
