 * Library: TFT_eSPI, MFRC522 (GithubCommunity/miguelbalboa)
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <MFRC522.h>

TFT_eSPI tft = TFT_eSPI();

// ---- Pin RC522 (terpisah total dari pin layar) ----
#define RFID_SS_PIN   5
#define RFID_RST_PIN  32
#define RFID_SCK_PIN  25
#define RFID_MISO_PIN 26
#define RFID_MOSI_PIN 27

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

void setup() {
  Serial.begin(115200);

  // Backlight (persis seperti sketch yang sudah jalan)
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  // Init layar
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // Init RC522 di jalur VSPI default, pin custom
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  rfid.PCD_Init();
  Serial.println("RC522 siap. Tempelkan kartu...");

  showIdleScreen();
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uidStr += ":";
  }
  uidStr.toUpperCase();

  Serial.print("UID terbaca: ");
  Serial.println(uidStr);

  showCardScreen(uidStr);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1500);
  showIdleScreen();
}

void showIdleScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.println("Tempelkan");
  tft.setCursor(20, 125);
  tft.println("kartu RFID");
}

void showCardScreen(String uid) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 80);
  tft.println("Kartu Terbaca!");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 120);
  tft.println("UID:");
  tft.setTextSize(2);
  tft.setCursor(10, 135);
  tft.println(uid);
}
