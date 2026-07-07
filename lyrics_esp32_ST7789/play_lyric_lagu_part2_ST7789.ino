#include <TFT_eSPI.h>

extern const GFXfont FreeSans9pt7b;
extern const GFXfont FreeSans12pt7b;
extern const GFXfont FreeSansOblique12pt7b;

#define TFT_BL 22

TFT_eSPI tft = TFT_eSPI();

// ── Konstanta 
#define SCREEN_W     240
#define SCREEN_H     280
#define CENTER_Y     140
#define LINE_HEIGHT   36
#define MAX_LINES     12
#define SCROLL_STEP    2
#define FRAME_MS      16

// ── Buffer lirik
String lines[MAX_LINES];
int    head       = 0;
int    totalLines = 0;
String lastLine   = "";

// ── State animasi 
float offsetY     = 0;
bool  isScrolling = false;
bool  needRedraw  = false;

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextWrap(false);
  drawScene(0);
}

void loop() {
  while (Serial.available()) {
    String newLine = Serial.readStringUntil('\n');
    newLine.trim();
    if (newLine.length() > 0 && newLine != lastLine) {
      lastLine = newLine;
      lines[head] = newLine;
      head = (head + 1) % MAX_LINES;
      if (totalLines < MAX_LINES) totalLines++;
      isScrolling = true;
      offsetY     = 0;
      needRedraw  = true;
    }
  }

  if (isScrolling) {
    drawScene(offsetY);
    offsetY += SCROLL_STEP;
    if (offsetY >= LINE_HEIGHT) {
      offsetY     = 0;
      isScrolling = false;
      drawScene(0);
    }
    delay(FRAME_MS);
  }
  else if (needRedraw) {
    drawScene(0);
    needRedraw = false;
  }
}

const GFXfont* pickFont(float dist) {
  if (dist < 12) return &FreeSansOblique12pt7b;  
  if (dist < 55) return &FreeSans12pt7b;          
  return         &FreeSans9pt7b;                  
}

uint16_t pickColor(float dist) {
  if (dist < 12) return TFT_WHITE;
  if (dist < 55) return tft.color565(160, 160, 160);
  return         tft.color565(70, 70, 70);
}

void drawScene(float offset) {
  tft.fillScreen(TFT_BLACK);
  if (totalLines == 0) return;

  for (int i = 0; i < totalLines; i++) {
    int relPos = i - (totalLines - 1);

    float y;
    if (isScrolling) {
      y = CENTER_Y + (relPos + 1) * LINE_HEIGHT - offset;
    } else {
      y = CENTER_Y + relPos * LINE_HEIGHT;
    }

    if (y < -30 || y > SCREEN_H + 30) continue;

    int     bufIdx = (head - totalLines + i + MAX_LINES) % MAX_LINES;
    float   dist   = abs(y - CENTER_Y);
    const GFXfont *font  = pickFont(dist);
    uint16_t       color = pickColor(dist);

    tft.setFreeFont(font);
    int tw = tft.textWidth(lines[bufIdx]);

    if (tw > SCREEN_W - 10) {
      tft.setFreeFont(&FreeSans9pt7b);
      tw = tft.textWidth(lines[bufIdx]);

      if (tw > SCREEN_W - 10) {
        String s = lines[bufIdx];
        while (tft.textWidth(s + "...") > SCREEN_W - 10 && s.length() > 0) {
          s.remove(s.length() - 1);
        }
        tft.setTextColor(color, TFT_BLACK);
        tft.drawString(s + "...", SCREEN_W / 2, (int)y);
        continue;
      }
    }

    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(lines[bufIdx], SCREEN_W / 2, (int)y);
  }
}
