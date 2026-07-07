// ============================================================
// SPIDY Quadruped Robot - Final Version
// MCU          : ESP32-dev module
// Servo Driver : PCA9685 (I2C 0x40)
// Display      : SSD1306 128x64 I2C (0x3C)
// I2C Pins     : SDA=GPIO8, SCL=GPIO9
// Libraries    : Adafruit_PWMServoDriver, Adafruit_SSD1306,
//                Adafruit_GFX, WiFi, WebServer, DNSServer, ESPmDNS
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "face-bitmaps.h"
#include "movement-sequences.h"
#include "captive-portal.h"

// ── WiFi Configuration ───────────────────────────────────────
#define AP_SSID  "SPIDY-Controller"
#define AP_PASS  "12345678"

#define NETWORK_SSID "Biuwifi"
#define NETWORK_PASS "AAII2006"
#define ENABLE_NETWORK_MODE true

// ── Display ──────────────────────────────────────────────────
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_I2C_ADDR   0x3C

// ── I2C Pins ─────────────────────────────────────────────────
// ESP32 Klasik : SDA=21, SCL=22  (aktif)
// ESP32-C3     : SDA=8,  SCL=9   (ganti define di bawah jika pindah MCU)
#define I2C_SDA 21
#define I2C_SCL 22

// ── PCA9685 ──────────────────────────────────────────────────
#define PCA9685_ADDR  0x40
#define SERVO_MIN_US  500
#define SERVO_MAX_US  2400
#define PCA_FREQ      50
#define PCA_OSC_FREQ  25000000

// ── DNS ──────────────────────────────────────────────────────
DNSServer dnsServer;
const byte DNS_PORT = 53;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(PCA9685_ADDR);
WebServer server(80);

// ── Global State ─────────────────────────────────────────────
String currentCommand = "";
String currentFaceName = "default";
const unsigned char* const* currentFaceFrames = nullptr;
uint8_t currentFaceFrameCount = 0;
uint8_t currentFaceFrameIndex = 0;
unsigned long lastFaceFrameMs = 0;
int faceFps = 8;
FaceAnimMode currentFaceMode = FACE_ANIM_LOOP;
int8_t faceFrameDirection = 1;
bool faceAnimFinished = false;
int currentFaceFps = 0;
bool idleActive = false;
bool idleBlinkActive = false;
unsigned long nextIdleBlinkMs = 0;
uint8_t idleBlinkRepeatsLeft = 0;

unsigned long lastInputTime = 0;
bool firstInputReceived = false;
bool showingWifiInfo = false;
int wifiScrollPos = 0;
unsigned long lastWifiScrollMs = 0;
String wifiInfoText = "";

bool networkConnected = false;
IPAddress networkIP;
String deviceHostname = "SPIDY-robot";

// ── Servo Mapping & Info (DIPERBAIKI - sesuai movement-sequences.h) ──
// Urutan channel PCA9685 HARUS mengikuti enum ServoName di movement-sequences.h:
//   R1=0, R2=1, L1=2, L2=3, R4=4, R3=5, L3=6, L4=7
//
// Sambungkan kabel servo ke PCA9685 dengan urutan berikut:
// CH 0: R1  = Motor 4 (Hip FR  - kanan depan)   -> SG92R
// CH 1: R2  = Motor 3 (Hip BR  - kanan belakang) -> MG90S
// CH 2: L1  = Motor 2 (Hip FL  - kiri depan)     -> MG90S
// CH 3: L2  = Motor 1 (Hip BL  - kiri belakang)  -> SG90
// CH 4: R4  = Motor 7 (Knee BR - kanan belakang) -> SG90
// CH 5: R3  = Motor 8 (Knee FR - kanan depan)    -> SG90
// CH 6: L3  = Motor 6 (Knee FL - kiri depan)     -> SG90
// CH 7: L4  = Motor 5 (Knee BL - kiri belakang)  -> SG90
// CH: 0=R1  1=R2  2=L1  3=L2  4=R4  5=R3  6=L3  7=L4
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

int frameDelay        = 100;
int walkCycles        = 10;
int motorCurrentDelay = 20;

// ── PWM Helper ───────────────────────────────────────────────
uint16_t degreesToPWM(int degrees) {
  degrees = constrain(degrees, 0, 180);
  long us = map(degrees, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  return (uint16_t)((us * PCA_FREQ * 4096UL) / 1000000UL);
}

// ── setServoAngle ────────────────────────────────────────────
void setServoAngle(uint8_t channel, int angle) {
  if (channel < 8) {
    int adjusted = constrain(angle + servoSubtrim[channel], 0, 180);
    pca.setPWM(channel, 0, degreesToPWM(adjusted));
    delayWithFace(motorCurrentDelay);
  }
}

// ── Face System ──────────────────────────────────────────────
struct FaceEntry {
  const char* name;
  const unsigned char* const* frames;
  uint8_t maxFrames;
};

static const uint8_t MAX_FACE_FRAMES = 6;

#define MAKE_FACE_FRAMES(name) \
  const unsigned char* const face_##name##_frames[] = { \
    epd_bitmap_##name,   epd_bitmap_##name##_1, epd_bitmap_##name##_2, \
    epd_bitmap_##name##_3, epd_bitmap_##name##_4, epd_bitmap_##name##_5 \
  };

#define X(name) MAKE_FACE_FRAMES(name)
FACE_LIST
#undef X
#undef MAKE_FACE_FRAMES

const FaceEntry faceEntries[] = {
#define X(name) { #name, face_##name##_frames, MAX_FACE_FRAMES },
  FACE_LIST
#undef X
  { "default", face_defualt_frames, MAX_FACE_FRAMES }
};

struct FaceFpsEntry { const char* name; uint8_t fps; };
const FaceFpsEntry faceFpsEntries[] = {
  {"walk",1},{"rest",1},{"swim",1},{"dance",1},{"wave",1},{"point",5},
  {"stand",1},{"cute",1},{"pushup",1},{"freaky",1},{"bow",1},{"worm",1},
  {"shake",1},{"shrug",1},{"dead",2},{"crab",1},{"idle",1},{"idle_blink",7},
  {"default",1},{"happy",1},{"talk_happy",1},{"sad",1},{"talk_sad",1},
  {"angry",1},{"talk_angry",1},{"surprised",1},{"talk_surprised",1},
  {"sleepy",1},{"talk_sleepy",1},{"love",1},{"talk_love",1},
  {"excited",1},{"talk_excited",1},{"confused",1},{"talk_confused",1},
  {"thinking",1},{"talk_thinking",1},
};

// ── Prototypes ───────────────────────────────────────────────
void setServoAngle(uint8_t channel, int angle);
void updateFaceBitmap(const unsigned char* bitmap);
void setFace(const String& faceName);
void setFaceMode(FaceAnimMode mode);
void setFaceWithMode(const String& faceName, FaceAnimMode mode);
void updateAnimatedFace();
void delayWithFace(unsigned long ms);
void enterIdle();
void exitIdle();
void updateIdleBlink();
int  getFaceFpsForName(const String& faceName);
bool pressingCheck(String cmd, int ms);
void handleGetSettings();
void handleSetSettings();
void handleGetStatus();
void handleApiCommand();
void updateWifiInfoScroll();
void recordInput();

//---------custom made----------
void runCustomPose();

// ── Web Handlers ─────────────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleCommandWeb() {
  if (server.hasArg("pose")) {
    currentCommand = server.arg("pose");
    recordInput(); exitIdle();
    server.send(200, "text/plain", "OK");
  } else if (server.hasArg("go")) {
    currentCommand = server.arg("go");
    recordInput(); exitIdle();
    server.send(200, "text/plain", "OK");
  } else if (server.hasArg("stop")) {
    currentCommand = "";
    recordInput();
    server.send(200, "text/plain", "OK");
  } else if (server.hasArg("motor") && server.hasArg("value")) {
    int motorNum = server.arg("motor").toInt();
    int servoIdx = servoNameToIndex(server.arg("motor"));
    int angle    = server.arg("value").toInt();
    if (motorNum >= 1 && motorNum <= 8 && angle >= 0 && angle <= 180) {
      setServoAngle(motorNum - 1, angle);
      recordInput();
      server.send(200, "text/plain", "OK");
    } else if (servoIdx != -1 && angle >= 0 && angle <= 180) {
      setServoAngle(servoIdx, angle);
      recordInput();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid motor or angle");
    }
  } else {
    server.send(400, "text/plain", "Bad Args");
  }
}

void handleGetSettings() {
  String json = "{";
  json += "\"frameDelay\":"       + String(frameDelay)       + ",";
  json += "\"walkCycles\":"       + String(walkCycles)       + ",";
  json += "\"motorCurrentDelay\":" + String(motorCurrentDelay) + ",";
  json += "\"faceFps\":"          + String(faceFps);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetSettings() {
  if (server.hasArg("frameDelay"))        frameDelay        = server.arg("frameDelay").toInt();
  if (server.hasArg("walkCycles"))        walkCycles        = server.arg("walkCycles").toInt();
  if (server.hasArg("motorCurrentDelay")) motorCurrentDelay = server.arg("motorCurrentDelay").toInt();
  if (server.hasArg("faceFps"))           faceFps           = (int)max(1L, server.arg("faceFps").toInt());
  server.send(200, "text/plain", "OK");
}

void handleGetStatus() {
  String json = "{";
  json += "\"currentCommand\":\"" + currentCommand + "\",";
  json += "\"currentFace\":\""    + currentFaceName + "\",";
  json += "\"networkConnected\":" + String(networkConnected ? "true" : "false") + ",";
  json += "\"apIP\":\""           + WiFi.softAPIP().toString() + "\"";
  if (networkConnected) json += ",\"networkIP\":\"" + networkIP.toString() + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleApiCommand() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
    return;
  }
  String body = server.arg("plain");
  int faceOnlyStart = body.indexOf("\"face\":\"");
  if (faceOnlyStart == -1) faceOnlyStart = body.indexOf("\"face\": \"");
  bool faceOnly = (faceOnlyStart > 0 &&
                   body.indexOf("\"command\":") == -1 &&
                   body.indexOf("\"command\": ") == -1);
  String command = "", face = "";
  if (faceOnlyStart > 0) {
    faceOnlyStart = body.indexOf("\"", faceOnlyStart + 6) + 1;
    int faceEnd = body.indexOf("\"", faceOnlyStart);
    if (faceEnd > faceOnlyStart) face = body.substring(faceOnlyStart, faceEnd);
  }
  if (!faceOnly) {
    int cmdStart = body.indexOf("\"command\":\"");
    if (cmdStart == -1) cmdStart = body.indexOf("\"command\": \"");
    if (cmdStart == -1) { server.send(400, "application/json", "{\"error\":\"Missing command\"}"); return; }
    cmdStart = body.indexOf("\"", cmdStart + 10) + 1;
    int cmdEnd = body.indexOf("\"", cmdStart);
    if (cmdEnd <= cmdStart) { server.send(400, "application/json", "{\"error\":\"Invalid format\"}"); return; }
    command = body.substring(cmdStart, cmdEnd);
  }
  if (face.length() > 0) setFace(face);
  if (faceOnly) { recordInput(); server.send(200, "application/json", "{\"status\":\"ok\"}"); return; }
  if (command == "stop") { currentCommand = ""; recordInput(); }
  else { currentCommand = command; recordInput(); exitIdle(); }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ═════════════════════════════════════════════════════════════
// SETUP
// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  // I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("SSD1306 gagal!"));
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("SPIDY booting..."));
  display.display();

  // PCA9685
  pca.begin();
  pca.setOscillatorFrequency(PCA_OSC_FREQ);
  pca.setPWMFreq(PCA_FREQ);
  delay(10);

  // Posisi aman semua servo saat boot
  for (uint8_t i = 0; i < 8; i++) {
    pca.setPWM(i, 0, degreesToPWM(90));
  }
  delay(300);

  // ── WiFi: AP aktif DULU, baru coba station ───────────────
  // Step 1: Aktifkan AP terlebih dahulu agar selalu terdeteksi
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS, 6);
  delay(200); // beri waktu AP mulai broadcast
  IPAddress myIP = WiFi.softAPIP();
  Serial.println("AP aktif!");
  Serial.print("SSID : "); Serial.println(AP_SSID);
  Serial.print("IP   : "); Serial.println(myIP);

  display.println(F("AP: SPIDY-Controller"));
  display.println(F("IP: 192.168.4.1"));
  display.display();

  // Step 2: Coba connect ke WiFi rumah (tidak memblokir AP)
  if (ENABLE_NETWORK_MODE && String(NETWORK_SSID).length() > 0) {
    Serial.print("Connecting ke: "); Serial.println(NETWORK_SSID);
    WiFi.begin(NETWORK_SSID, NETWORK_PASS);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500); Serial.print("."); attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      networkConnected = true;
      networkIP = WiFi.localIP();
      Serial.println("\nNetwork IP: " + networkIP.toString());
    } else {
      Serial.println("\nGagal connect WiFi rumah. AP tetap jalan.");
    }
  }

  // Build scrolling info text
  if (networkConnected) {
    wifiInfoText = "AP: " + String(AP_SSID) + " pass:" + String(AP_PASS) +
                   " (" + myIP.toString() + ")  |  WiFi: " + String(NETWORK_SSID) +
                   " (" + networkIP.toString() + ") atau " + deviceHostname + ".local  |  ";
  } else {
    wifiInfoText = "WiFi: " + String(AP_SSID) + "  |  Pass: " + String(AP_PASS) +
                   "  |  IP: " + myIP.toString() + "  |  Buka browser -> 192.168.4.1  |  ";
  }

  lastInputTime     = millis();
  firstInputReceived = false;
  showingWifiInfo   = false;

  // mDNS
  if (MDNS.begin(deviceHostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: " + deviceHostname + ".local");
  }

  // DNS captive portal
  dnsServer.start(DNS_PORT, "*", myIP);

  // Web routes
  server.on("/",           handleRoot);
  server.on("/cmd",        handleCommandWeb);
  server.on("/getSettings",handleGetSettings);
  server.on("/setSettings",handleSetSettings);
  server.on("/api/status", handleGetStatus);
  server.on("/api/command",handleApiCommand);
  server.onNotFound(handleRoot);
  server.begin();

  setFace("rest");

  Serial.println(F("=== SPIDY Robot siap! ==="));
  Serial.println(F("Sambungkan HP ke WiFi: SPIDY-Controller"));
  Serial.println(F("Buka browser: 192.168.4.1"));
  Serial.println(F("CH mapping PCA9685 (DIPERBAIKI - sesuai movement-sequences.h):"));
  Serial.println(F("  CH0=R1(SG92R) CH1=R2(MG90S) CH2=L1(MG90S) CH3=L2(SG90)"));
  Serial.println(F("  CH4=R4(SG90)  CH5=R3(SG90)  CH6=L3(SG90)  CH7=L4(SG90)"));
}

// ═════════════════════════════════════════════════════════════
// LOOP
// ═════════════════════════════════════════════════════════════
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  updateAnimatedFace();
  updateIdleBlink();
  updateWifiInfoScroll();

  if (currentCommand != "") {
    String cmd = currentCommand;
    if      (cmd == "forward")  runWalkPose();
    else if (cmd == "backward") runWalkBackward();
    else if (cmd == "left")     runTurnLeft();
    else if (cmd == "right")    runTurnRight();
    else if (cmd == "rest")     { runRestPose();   if (currentCommand == "rest")  currentCommand = ""; }
    else if (cmd == "stand")    { runStandPose(1); if (currentCommand == "stand") currentCommand = ""; }
    else if (cmd == "wave")     runWavePose();
    else if (cmd == "dance")    runDancePose();
    else if (cmd == "swim")     runSwimPose();
    else if (cmd == "point")    runPointPose();
    else if (cmd == "pushup")   runPushupPose();
    else if (cmd == "bow")      runBowPose();
    else if (cmd == "cute")     runCutePose();
    else if (cmd == "freaky")   runFreakyPose();
    else if (cmd == "worm")     runWormPose();
    else if (cmd == "shake")    runShakePose();
    else if (cmd == "shrug")    runShrugPose();
    else if (cmd == "dead")     runDeadPose();
    else if (cmd == "crab")     runCrabPose();

    //---------custom made-------//
    else if (cmd == "custom") { runCustomPose(); }
  }

  // ── Serial CLI ───────────────────────────────────────────
  if (Serial.available()) {
    static char command_buffer[32];
    static byte buffer_pos = 0;
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (buffer_pos > 0) {
        command_buffer[buffer_pos] = '\0';
        int motorNum, angle;
        recordInput();
        if      (!strcmp(command_buffer,"run walk") || !strcmp(command_buffer,"rn wf")) { currentCommand="forward";  runWalkPose();     currentCommand=""; }
        else if (!strcmp(command_buffer,"rn wb"))   { currentCommand="backward"; runWalkBackward(); currentCommand=""; }
        else if (!strcmp(command_buffer,"rn tl"))   { currentCommand="left";     runTurnLeft();     currentCommand=""; }
        else if (!strcmp(command_buffer,"rn tr"))   { currentCommand="right";    runTurnRight();    currentCommand=""; }
        else if (!strcmp(command_buffer,"run rest")  || !strcmp(command_buffer,"rn rs")) runRestPose();
        else if (!strcmp(command_buffer,"run stand") || !strcmp(command_buffer,"rn st")) runStandPose(1);
        else if (!strcmp(command_buffer,"rn wv"))   { currentCommand="wave";   runWavePose(); }
        else if (!strcmp(command_buffer,"rn dn"))   { currentCommand="dance";  runDancePose(); }
        else if (!strcmp(command_buffer,"rn sw"))   { currentCommand="swim";   runSwimPose(); }
        else if (!strcmp(command_buffer,"rn pt"))   { currentCommand="point";  runPointPose(); }
        else if (!strcmp(command_buffer,"rn pu"))   { currentCommand="pushup"; runPushupPose(); }
        else if (!strcmp(command_buffer,"rn bw"))   { currentCommand="bow";    runBowPose(); }
        else if (!strcmp(command_buffer,"rn ct"))   { currentCommand="cute";   runCutePose(); }
        else if (!strcmp(command_buffer,"rn fk"))   { currentCommand="freaky"; runFreakyPose(); }
        else if (!strcmp(command_buffer,"rn wm"))   { currentCommand="worm";   runWormPose(); }
        else if (!strcmp(command_buffer,"rn sk"))   { currentCommand="shake";  runShakePose(); }
        else if (!strcmp(command_buffer,"rn sg"))   { currentCommand="shrug";  runShrugPose(); }
        else if (!strcmp(command_buffer,"rn dd"))   { currentCommand="dead";   runDeadPose(); }
        else if (!strcmp(command_buffer,"rn cb"))   { currentCommand="crab";   runCrabPose(); }

        //-------------custom made---------------------//
        else if (!strcmp(command_buffer,"rn cu"))   { currentCommand="custom"; runCustomPose(); currentCommand=""; }


        else if (!strcmp(command_buffer,"subtrim") || !strcmp(command_buffer,"st")) {
          Serial.println("Subtrim values:");
          for (int i = 0; i < 8; i++) {
            Serial.print("CH"); Serial.print(i); Serial.print(": ");
            if (servoSubtrim[i] >= 0) Serial.print("+");
            Serial.println(servoSubtrim[i]);
          }
        }
        else if (!strcmp(command_buffer,"subtrim save") || !strcmp(command_buffer,"st save")) {
          Serial.println("Copy paste ke kode:");
          Serial.print("int8_t servoSubtrim[8] = {");
          for (int i = 0; i < 8; i++) { Serial.print(servoSubtrim[i]); if (i<7) Serial.print(","); }
          Serial.println("};");
        }
        else if (!strncmp(command_buffer,"subtrim reset",13)) {
          for (int i=0;i<8;i++) servoSubtrim[i]=0;
          Serial.println("Semua subtrim reset ke 0");
        }
        else if (!strncmp(command_buffer,"subtrim ",8) || !strncmp(command_buffer,"st ",3)) {
          const char* p = (command_buffer[1]=='t') ? command_buffer+3 : command_buffer+8;
          int m,v;
          if (sscanf(p,"%d %d",&m,&v)==2 && m>=0 && m<8 && v>=-90 && v<=90) {
            servoSubtrim[m]=v;
            Serial.print("CH"); Serial.print(m);
            Serial.print(" subtrim="); if(v>=0) Serial.print("+");
            Serial.println(v);
          }
        }
        else if (!strncmp(command_buffer,"all ",4)) {
          if (sscanf(command_buffer+4,"%d",&angle)==1) {
            for (int i=0;i<8;i++) setServoAngle(i,angle);
            Serial.print("Semua servo -> "); Serial.println(angle);
          }
        }
        else if (sscanf(command_buffer,"%d %d",&motorNum,&angle)==2) {
          if (motorNum>=0 && motorNum<8) {
            setServoAngle(motorNum,angle);
            Serial.print("CH"); Serial.print(motorNum);
            Serial.print(" -> "); Serial.println(angle);
          }
        }
        buffer_pos = 0;
      }
    } else if (buffer_pos < sizeof(command_buffer)-1) {
      command_buffer[buffer_pos++] = c;
    }
  }
}

// ═════════════════════════════════════════════════════════════
// DISPLAY & FACE
// ═════════════════════════════════════════════════════════════
void updateFaceBitmap(const unsigned char* bitmap) {
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmap, 128, 64, SSD1306_WHITE);
  display.display();
}

uint8_t countFrames(const unsigned char* const* frames, uint8_t maxFrames) {
  if (!frames || !frames[0]) return 0;
  uint8_t c = 0;
  for (uint8_t i = 0; i < maxFrames; i++) { if (!frames[i]) break; c++; }
  return c;
}

void setFace(const String& faceName) {
  if (faceName == currentFaceName && currentFaceFrames) return;
  currentFaceName       = faceName;
  currentFaceFrameIndex = 0;
  lastFaceFrameMs       = 0;
  faceFrameDirection    = 1;
  faceAnimFinished      = false;
  currentFaceFps        = getFaceFpsForName(faceName);
  currentFaceFrames     = face_defualt_frames;
  currentFaceFrameCount = countFrames(face_defualt_frames, MAX_FACE_FRAMES);
  for (size_t i = 0; i < sizeof(faceEntries)/sizeof(faceEntries[0]); i++) {
    if (faceName.equalsIgnoreCase(faceEntries[i].name)) {
      currentFaceFrames     = faceEntries[i].frames;
      currentFaceFrameCount = countFrames(faceEntries[i].frames, faceEntries[i].maxFrames);
      break;
    }
  }
  if (currentFaceFrameCount == 0) {
    currentFaceFrames     = face_defualt_frames;
    currentFaceFrameCount = countFrames(face_defualt_frames, MAX_FACE_FRAMES);
    currentFaceName       = "default";
    currentFaceFps        = getFaceFpsForName("default");
  }
  if (currentFaceFrameCount > 0 && currentFaceFrames[0])
    updateFaceBitmap(currentFaceFrames[0]);
}

void setFaceMode(FaceAnimMode mode) {
  currentFaceMode    = mode;
  faceFrameDirection = 1;
  faceAnimFinished   = false;
}

void setFaceWithMode(const String& faceName, FaceAnimMode mode) {
  setFaceMode(mode);
  setFace(faceName);
}

int getFaceFpsForName(const String& faceName) {
  for (size_t i = 0; i < sizeof(faceFpsEntries)/sizeof(faceFpsEntries[0]); i++)
    if (faceName.equalsIgnoreCase(faceFpsEntries[i].name)) return faceFpsEntries[i].fps;
  return faceFps;
}

void updateAnimatedFace() {
  if (!currentFaceFrames || currentFaceFrameCount <= 1) return;
  if (currentFaceMode == FACE_ANIM_ONCE && faceAnimFinished) return;
  unsigned long now = millis();
  int fps = max(1, currentFaceFps > 0 ? currentFaceFps : faceFps);
  if (now - lastFaceFrameMs < 1000UL / fps) return;
  lastFaceFrameMs = now;
  if (currentFaceMode == FACE_ANIM_LOOP) {
    currentFaceFrameIndex = (currentFaceFrameIndex + 1) % currentFaceFrameCount;
  } else if (currentFaceMode == FACE_ANIM_ONCE) {
    if (currentFaceFrameIndex + 1 >= currentFaceFrameCount) { faceAnimFinished = true; }
    else currentFaceFrameIndex++;
  } else { // BOOMERANG
    if (faceFrameDirection > 0) {
      if (currentFaceFrameIndex + 1 >= currentFaceFrameCount) { faceFrameDirection = -1; if (currentFaceFrameIndex > 0) currentFaceFrameIndex--; }
      else currentFaceFrameIndex++;
    } else {
      if (currentFaceFrameIndex == 0) { faceFrameDirection = 1; if (currentFaceFrameCount > 1) currentFaceFrameIndex++; }
      else currentFaceFrameIndex--;
    }
  }
  updateFaceBitmap(currentFaceFrames[currentFaceFrameIndex]);
}

void delayWithFace(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    updateAnimatedFace();
    server.handleClient();
    dnsServer.processNextRequest();
    delay(5);
  }
}

void scheduleNextIdleBlink(unsigned long minMs, unsigned long maxMs) {
  nextIdleBlinkMs = millis() + (unsigned long)random(minMs, maxMs);
}

void enterIdle() {
  idleActive = true; idleBlinkActive = false; idleBlinkRepeatsLeft = 0;
  setFaceWithMode("idle", FACE_ANIM_BOOMERANG);
  scheduleNextIdleBlink(3000, 7000);
}

void exitIdle() { idleActive = false; idleBlinkActive = false; }

void updateIdleBlink() {
  if (!idleActive) return;
  if (!idleBlinkActive) {
    if (millis() >= nextIdleBlinkMs) {
      idleBlinkActive = true;
      if (idleBlinkRepeatsLeft == 0 && random(0,100) < 30) idleBlinkRepeatsLeft = 1;
      setFaceWithMode("idle_blink", FACE_ANIM_ONCE);
    }
    return;
  }
  if (currentFaceMode == FACE_ANIM_ONCE && faceAnimFinished) {
    idleBlinkActive = false;
    setFaceWithMode("idle", FACE_ANIM_BOOMERANG);
    if (idleBlinkRepeatsLeft > 0) { idleBlinkRepeatsLeft--; scheduleNextIdleBlink(120, 220); }
    else scheduleNextIdleBlink(3000, 7000);
  }
}

bool pressingCheck(String cmd, int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    dnsServer.processNextRequest();
    updateAnimatedFace();
    if (currentCommand != cmd) { runStandPose(1); return false; }
    yield();
  }
  return true;
}

void recordInput() {
  lastInputTime = millis();
  if (!firstInputReceived) { firstInputReceived = true; showingWifiInfo = false; }
}

void updateWifiInfoScroll() {
  if (firstInputReceived) {
    if (showingWifiInfo) {
      showingWifiInfo = false;
      if (currentFaceFrames && currentFaceFrameCount > 0)
        updateFaceBitmap(currentFaceFrames[currentFaceFrameIndex]);
    }
    return;
  }
  unsigned long now = millis();
  if (!showingWifiInfo && now - lastInputTime >= 30000) {
    showingWifiInfo = true; wifiScrollPos = 0; lastWifiScrollMs = now;
  }
  if (!showingWifiInfo) return;
  if (now - lastWifiScrollMs >= 150) {
    lastWifiScrollMs = now;
    display.clearDisplay();
    if (currentFaceFrames && currentFaceFrameCount > 0)
      display.drawBitmap(0, 0, currentFaceFrames[currentFaceFrameIndex], 128, 64, SSD1306_WHITE);
    display.fillRect(0, 0, 128, 10, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setCursor(-wifiScrollPos, 1);
    display.print(wifiInfoText);
    display.setTextWrap(true);
    display.display();
    wifiScrollPos += 2;
    if (wifiScrollPos >= (int)(wifiInfoText.length() * 6)) wifiScrollPos = 0;
  }
}
