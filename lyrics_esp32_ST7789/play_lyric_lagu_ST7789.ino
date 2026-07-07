#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define TFT_DC   2
#define TFT_RST  4
#define TFT_CS  -1

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define MAX_CHARS_PER_LINE 18
#define MAX_BUFFER_LINES   20

int lineHeight = 30;
int startY = 50;
int typingSpeed = 30;

int maxVisibleLines;

String lines[MAX_BUFFER_LINES];
int totalLines = 0;


void setup() {
  Serial.begin(115200);

  tft.init(TFT_WIDTH, TFT_HEIGHT, SPI_MODE3);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  maxVisibleLines = (TFT_HEIGHT - startY) / lineHeight;
}


void redrawAll() {

  tft.fillRect(0, startY, TFT_WIDTH, TFT_HEIGHT - startY, ST77XX_BLACK);

  int startIndex = max(0, totalLines - maxVisibleLines);

  for (int i = 0; i < maxVisibleLines; i++) {

    int idx = startIndex + i;

    if (idx < totalLines) {
      tft.setCursor(10, startY + (i * lineHeight));
      tft.print(lines[idx]);
    }
  }
}



void addLine(String line) {

  if (totalLines < MAX_BUFFER_LINES) {
    lines[totalLines++] = line;
  } else {
    for (int i = 0; i < MAX_BUFFER_LINES - 1; i++) {
      lines[i] = lines[i + 1];
    }
    lines[MAX_BUFFER_LINES - 1] = line;
  }

  redrawAll();   
}



void typeLine(String line) {

  int y = startY + ((maxVisibleLines - 1) * lineHeight);
  tft.setCursor(10, y);

  for (int i = 0; i < line.length(); i++) {
    tft.print(line[i]);
    delay(typingSpeed);
  }

  addLine(line);  
}


void processIncomingLine(String line) {

  while (line.length() > 0) {

    if (line.length() <= MAX_CHARS_PER_LINE) {
      typeLine(line);
      break;
    }

    int splitIndex = line.lastIndexOf(' ', MAX_CHARS_PER_LINE);
    if (splitIndex == -1) splitIndex = MAX_CHARS_PER_LINE;

    String part = line.substring(0, splitIndex);
    typeLine(part);

    line = line.substring(splitIndex);
    line.trim();
  }
}


void loop() {

  if (Serial.available()) {

    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.length() > 0) {
      processIncomingLine(line);
    }
  }
}
