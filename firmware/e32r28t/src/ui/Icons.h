#pragma once
#include <TFT_eSPI.h>
inline void drawTabIcon(TFT_eSPI &t, int idx, int x, int y, uint16_t c) {
  if (idx == 0) { t.drawRoundRect(x + 2, y + 5, 18, 12, 2, c); t.drawLine(x + 5, y + 11, x + 17, y + 11, c); }
  else if (idx == 1) { t.drawCircle(x + 10, y + 10, 7, c); t.drawLine(x + 10, y + 5, x + 10, y + 15, c); }
  else if (idx == 2) { t.drawRect(x + 5, y + 5, 10, 10, c); t.drawLine(x + 5, y + 10, x + 15, y + 10, c); }
  else if (idx == 3) { t.drawLine(x + 4, y + 15, x + 10, y + 4, c); t.drawLine(x + 10, y + 4, x + 16, y + 15, c); }
  else { t.drawCircle(x + 10, y + 10, 7, c); t.drawLine(x + 7, y + 10, x + 13, y + 10, c); }
}
