#pragma once
#include <TFT_eSPI.h>
inline void drawFlowGraph(TFT_eSPI &t, int x, int y, int w, int h, uint16_t cyan, uint16_t violet, uint16_t aqua) {
  t.drawRoundRect(x, y, w, h, 8, 0x2988);
  t.drawLine(x + 10, y + h - 20, x + w / 2, y + 18, cyan);
  t.drawLine(x + w / 2, y + 18, x + w - 10, y + h - 20, violet);
  t.drawLine(x + 10, y + 18, x + w - 10, y + 18, aqua);
}
