
#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>
#include <string.h>

TFT_eSPI tft = TFT_eSPI();

#define BG      0x0000
#define SPGREEN 0x06C4
#define WHITE   0xFFFF
#define GRAY    0x4228
#define DGRAY   0x1082
#define LGRAY   0x8C71
#define DARK    0x0841

// =============================================
// LIRIK
// =============================================
const char* lyrics[] = {
 "I know a place",
  "It's somewhere I go",
  "when I need to",
  "remember your face",
  " ",
  "We get married",
  "in our heads",
  "Something to do",
  "while we try to",
  "recall how we met",
  " ",
  "Do you think",
  "I have forgotten?",
  "Do you think",
  "I have forgotten?",
  "Do you think",
  "I have forgotten",
  "about you?",
};

int lyricsCount = 18;


// =============================================
// DELAY TIAP LIRIK
// =============================================
const int lyricDur[] = {

  150, // I know a place
  120, // It's somewhere I go
  60, // when I need to
  150, // remember your face
  40,  // kosong

  120, // We get married
  100, // in our heads
  60, // Something to do
  60, // while we try to
  120, // recall how we met
  40,  // kosong

  80, // Do you think
  60, // I have forgotten?
  80, // Do you think
  60, // I have forgotten?
  80, // Do you think
  60, // I have forgotten
  180  // about you?

};


// =============================================
// VARIABEL
// =============================================
int lyricIndex = 0;
int lyricTimer = 0;

float progress = 0;
bool isPlaying = true;

int eqH[7], eqT[7];
int eqTimer = 0;

int vinylAngle = 0;
int frameCount = 0;

bool loadingDone = false;
int loadTimer = 0;
int loadStep = 0;



// =============================================
// DECLARATION
// =============================================
void runLoadingScreen();
void drawMainLayout();
void drawHeader();
void drawAlbum();
void drawSongInfo();
void drawLyricsBox();
void drawControls();
void updateLyrics();
void updateVinyl();
void updateEQ();
void updateProgress();

// =============================================
// SETUP
// =============================================
void setup() {

  Serial.begin(115200);

  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(BG);

  randomSeed(analogRead(0));

  for (int i = 0; i < 7; i++) {
    eqH[i] = random(4, 18);
    eqT[i] = random(4, 18);
  }
}

// =============================================
// LOOP
// =============================================
void loop() {

  if (!loadingDone) {
    runLoadingScreen();
    return;
  }

  frameCount++;

  updateVinyl();
  updateEQ();
  updateLyrics();
  updateProgress();

  delay(30);
}

// =============================================
// LOADING SCREEN
// =============================================
void runLoadingScreen() {

  loadTimer++;

  if (loadStep == 0 && loadTimer == 1) {

    tft.fillScreen(BG);

    tft.fillCircle(120, 88, 36, SPGREEN);

    tft.fillRoundRect(98, 78, 44, 5, 2, BG);
    tft.fillRoundRect(101, 88, 38, 5, 2, BG);
    tft.fillRoundRect(105, 98, 30, 5, 2, BG);

    loadStep = 1;
  }

  if (loadStep == 1 && loadTimer == 60) {

    tft.setTextSize(3);
    tft.setTextColor(WHITE);

    tft.setCursor(52, 135);
    tft.print("SPOTIFY");

    loadStep = 2;
  }

  if (loadStep == 2 && loadTimer == 110) {

    tft.setTextSize(1);
    tft.setTextColor(SPGREEN);

    tft.setCursor(42, 160);
    tft.print("Music For Everyone");

    loadStep = 3;
  }

  if (loadStep == 3 && loadTimer == 150) {

    tft.drawRoundRect(60, 178, 120, 6, 3, GRAY);

    loadStep = 4;
  }

  if (loadStep == 4 &&
      loadTimer > 150 &&
      loadTimer <= 300) {

    int filled =
      (int)(120.0 * (loadTimer - 150) / 700.0);

    tft.fillRoundRect(
      61,
      179,
      filled,
      4,
      2,
      SPGREEN
    );
  }

  if (loadStep >= 4 &&
      loadTimer > 180 &&
      loadTimer <= 850) {

    tft.fillRect(104, 194, 30, 8, BG);

    tft.setTextColor(SPGREEN);
    tft.setTextSize(1);

    int d = loadTimer % 45;

    if (d < 15) {
      tft.setCursor(104, 194);
      tft.print(".      ");
    }
    else if (d < 30) {
      tft.setCursor(104, 194);
      tft.print(". .    ");
    }
    else {
      tft.setCursor(104, 194);
      tft.print(". . .  ");
    }
  }

  if (loadTimer >= 1600) {

    tft.fillScreen(BG);

    loadingDone = true;

    drawMainLayout();
  }
  delay(15);
}

// =============================================
// LAYOUT
// =============================================
void drawMainLayout() {

  for (int y = 0; y < 240; y++) {

    uint8_t b = 8 + (y / 12);

    uint16_t color =
      ((0 >> 3) << 11) |
      ((0 >> 2) << 5) |
      (b >> 3);

    tft.drawFastHLine(0, y, 240, color);
  }

  drawHeader();
  drawAlbum();
  drawSongInfo();
  drawLyricsBox();
  drawControls();
}

// =============================================
// HEADER
// =============================================
void drawHeader() {

  tft.fillRect(0, 0, 240, 32, DARK);

  tft.fillCircle(18, 16, 11, SPGREEN);

  tft.fillRoundRect(11, 12, 14, 2, 1, BG);
  tft.fillRoundRect(12, 16, 12, 2, 1, BG);
  tft.fillRoundRect(13, 20, 10, 2, 1, BG);

  tft.setTextColor(WHITE);
  tft.setTextSize(1);

  tft.setCursor(34, 10);
  tft.print("SPOTIFY");

  tft.setTextColor(SPGREEN);

  tft.setCursor(34, 20);
  tft.print("Now Playing");

  tft.setTextColor(GRAY);

  tft.setCursor(192, 14);
  tft.print("13:16");
}

// =============================================
// ALBUM
// =============================================
void drawAlbum() {

  int ax = 10;
  int ay = 40;

  for (int i = 0; i < 76; i++) {

    uint8_t r = 16 + i / 8;
    uint8_t g = 3 + i / 12;
    uint8_t b = 32 + i / 4;

    uint16_t color =
      ((r >> 3) << 11) |
      ((g >> 2) << 5) |
      (b >> 3);

    tft.drawFastHLine(ax, ay + i, 76, color);
  }

  tft.drawRoundRect(ax, ay, 76, 76, 5, GRAY);

  tft.drawCircle(48, 78, 24, LGRAY);
  tft.drawCircle(48, 78, 14, LGRAY);

  tft.fillCircle(48, 78, 4, WHITE);
}

// =============================================
// SONG INFO
// =============================================
void drawSongInfo() {

  tft.setTextSize(2);
  tft.setTextColor(WHITE);

  tft.setCursor(95, 48);
  tft.print("About You");

  tft.setTextSize(1);
  tft.setTextColor(SPGREEN);

  tft.setCursor(95, 74);
  tft.print("The 1975");

  tft.setTextColor(GRAY);

  tft.setCursor(95, 88);
  tft.print("Being Funny...");
}

// =============================================
// LYRICS BOX
// =============================================
void drawLyricsBox() {

  tft.drawFastHLine(0, 130, 240, DGRAY);

  tft.setTextColor(SPGREEN);
  tft.setTextSize(1);

  tft.setCursor(10, 136);
  tft.print("L Y R I C S");

  tft.drawFastHLine(0, 180, 240, DGRAY);
}

// =============================================
// UPDATE LYRICS
// =============================================
void updateLyrics() {

  lyricTimer++;

  if (lyricTimer >= lyricDur[lyricIndex]) {

    lyricTimer = 0;

    lyricIndex++;

    if (lyricIndex >= lyricsCount) {
      lyricIndex = 0;
    }
  }

  tft.fillRect(0, 145, 240, 30, BG);

  const char* curr = lyrics[lyricIndex];

  tft.setTextColor(WHITE);
  tft.setTextSize(2);

  int width = strlen(curr) * 12;

  int x = (240 - width) / 2;

  if (x < 4) x = 4;

  tft.setCursor(x, 154);
  tft.print(curr);

  tft.drawFastHLine(80, 176, 80, DGRAY);

  int fill = map(
    lyricTimer,
    0,
    lyricDur[lyricIndex],
    0,
    80
  );

  tft.drawFastHLine(80, 176, fill, SPGREEN);
}

// =============================================
// VINYL
// =============================================
void updateVinyl() {

  if (frameCount % 3 != 0) return;

  vinylAngle += 4;

  if (vinylAngle >= 360) {
    vinylAngle = 0;
  }

  int cx = 48;
  int cy = 78;

  float rad = vinylAngle * 3.14159 / 180.0;

  int dx = cos(rad) * 10;
  int dy = sin(rad) * 10;

  tft.fillCircle(cx, cy, 4, WHITE);

  tft.fillCircle(cx + dx / 2, cy + dy / 2, 2, SPGREEN);
}

// =============================================
// EQ
// =============================================
void updateEQ() {

  eqTimer++;

  int eqX = 95;
  int eqY = 118;

  int bw = 12;
  int gap = 4;

  if (eqTimer % 6 == 0) {

    for (int i = 0; i < 7; i++) {
      eqT[i] = random(4, 18);
    }
  }

  for (int i = 0; i < 7; i++) {

    int bx = eqX + i * (bw + gap);

    tft.fillRect(bx, eqY - 20, bw, 20, BG);

    if (eqH[i] < eqT[i]) eqH[i]++;
    if (eqH[i] > eqT[i]) eqH[i]--;

    tft.fillRoundRect(
      bx,
      eqY - eqH[i],
      bw,
      eqH[i],
      2,
      SPGREEN
    );
  }
}

// =============================================
// PROGRESS
// =============================================
void updateProgress() {

  if (isPlaying) {

    progress += 0.0012;

    if (progress >= 1.0) {
      progress = 0;
    }
  }

  int x = 12;
  int y = 188;
  int w = 216;

  int fill = progress * w;

  tft.fillRect(x, y, w, 10, BG);

  tft.fillRoundRect(x, y, w, 3, 1, DGRAY);

  tft.fillRoundRect(x, y, fill, 3, 1, SPGREEN);

  tft.fillCircle(x + fill, y + 1, 4, WHITE);

  tft.setTextColor(LGRAY);
  tft.setTextSize(1);

  tft.setCursor(12, 196);
  tft.print("1:24");

  tft.setCursor(204, 196);
  tft.print("5:26");
}

// =============================================
// CONTROLS
// =============================================
void drawControls() {

  int cy = 220;

  tft.fillTriangle(
    55, cy,
    68, cy - 8,
    68, cy + 8,
    LGRAY
  );

  tft.fillCircle(120, cy, 15, WHITE);

  tft.fillTriangle(
    115, cy - 7,
    115, cy + 7,
    128, cy,
    BG
  );

  tft.fillTriangle(
    185, cy,
    172, cy - 8,
    172, cy + 8,
    LGRAY
  );
}
