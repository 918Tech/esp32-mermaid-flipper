#include <Arduino.h>
#include "ui/UiController.h"

static UiController ui;

void setup() {
  Serial.begin(115200);
  ui.begin();
}

void loop() {
  ui.loop();
  delay(5);
}
