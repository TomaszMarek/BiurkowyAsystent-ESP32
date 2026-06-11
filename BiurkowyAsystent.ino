#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

#define TRIG_PIN    25
#define ECHO_PIN    26

#define WYKRYCIE_CM       60 
#define CZAS_USP_MS    15000   
#define POMIARY_US          5    

#define KOL_TLO    0x0841
#define KOL_AKCENT 0x07FF
#define KOL_BIALY  TFT_WHITE
#define KOL_SZARY  0x7BEF
#define KOL_CIEPŁY 0xFD20
#define KOL_ZIMNY  0x041F

TFT_eSPI        tft;
Adafruit_BME280 bme;

enum Stan { UŚPIONY, AKTYWNY, POWITANIE };
Stan aktualnyTryb = AKTYWNY;

unsigned long ostatnioWykryto = 0;
unsigned long ostatniOdczyt   = 0;
unsigned long startPowitania  = 0;
bool wyslacDiscord = false;
bool pierwszyStart = true;

float tempC   = 0;
float wilgPct = 0;

void odczytBME() {
  float nowaTemp = bme.readTemperature();
  float nowaWilg = bme.readHumidity();
  if (!isnan(nowaTemp) && nowaTemp > -10 && nowaTemp < 85) tempC   = nowaTemp;
  if (!isnan(nowaWilg) && nowaWilg > 0  && nowaWilg < 100) wilgPct = nowaWilg;
}

float zmierzOdleglosc() {
  float suma = 0;
  int   poprawne = 0;

  for (int i = 0; i < POMIARY_US; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long czas = pulseIn(ECHO_PIN, HIGH, 30000);
    if (czas > 0) {
      suma += (czas / 2.0) * 0.0343;
      poprawne++;
    }
    delay(10);
  }

  if (poprawne == 0) return 999;
  return suma / poprawne;
}

void wyslijDiscord(float temp, float wilg, bool start){
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(DISCORD_WEBHOOK);
  http.addHeader("Content-Type", "application/json");

  String wiadomosc;

if (start) {
  wiadomosc = "{\"content\":\" **Biurkowy Asystent** uruchomiony!\\n";
} else {
  wiadomosc = "{\"content\":\" **Biurkowy Asystent** się obudził!\\n";
}
  wiadomosc += "Temperatura: **" + String(temp, 1) + " °C**\\n";
  wiadomosc += "Wilgotność: **" + String(wilg, 0) + " %**\"}";

  http.POST(wiadomosc);
  http.end();
}

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  delay(100);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(KOL_TLO);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  if (!bme.begin(0x76, &Wire) && !bme.begin(0x77, &Wire)) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED);
    tft.drawString("BLAD: brak BME280!", 120, 67, 2);
    tft.setTextDatum(TL_DATUM);
    while (true) delay(1000);
  }

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(KOL_SZARY);
  tft.drawString("Laczenie z WiFi...", 120, 67, 2);
  tft.setTextDatum(TL_DATUM);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int próby = 0;
  while (WiFi.status() != WL_CONNECTED && próby < 40) {
    delay(500);
    próby++;
  }

  tft.fillScreen(KOL_TLO);
  tft.setTextDatum(MC_DATUM);
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(KOL_AKCENT);
    tft.drawString("WiFi OK!", 120, 55, 4);
    tft.setTextColor(KOL_SZARY);
    tft.drawString("Polaczono pomyslnie", 120, 95, 2);
  } else {
    tft.setTextColor(TFT_RED);
    tft.drawString("Brak WiFi!", 120, 55, 4);
    tft.setTextColor(KOL_SZARY);
    tft.drawString("Sprawdz haslo i siec", 120, 95, 2);
  }
  tft.setTextDatum(TL_DATUM);
  delay(1000);

  rysujEkranStartowy();
  delay(2000);


  tft.fillScreen(KOL_TLO);
  rysujRamke();
  odczytBME();
  rysujDane();

  ostatniOdczyt   = millis();
  ostatnioWykryto = millis();

  aktualnyTryb = POWITANIE;
  startPowitania = millis();
  rysujPowitanie();

  wyslacDiscord = true;
}

void loop() {
  unsigned long teraz   = millis();
  float         dystans = zmierzOdleglosc();

  if (dystans > 0 && dystans < WYKRYCIE_CM) {
    if (aktualnyTryb == UŚPIONY) {
      aktualnyTryb   = POWITANIE;
      startPowitania = teraz;
      rysujPowitanie();
      wyslacDiscord = true; 
    }
    ostatnioWykryto = teraz;
  } else {
    if (aktualnyTryb != UŚPIONY && (teraz - ostatnioWykryto > CZAS_USP_MS)) {
      aktualnyTryb = UŚPIONY;
      rysujTrybUspienia();
    }
  }

  if (aktualnyTryb == POWITANIE && (teraz - startPowitania > 2000)) {
    aktualnyTryb  = AKTYWNY;
    tft.fillScreen(KOL_TLO);
    rysujRamke();
    ostatniOdczyt = 0;
    odczytBME();
    rysujDane();
     if (wyslacDiscord) {
    wyslijDiscord(tempC, wilgPct, pierwszyStart);
    pierwszyStart = false;
    wyslacDiscord = false;
  }
}

  if (aktualnyTryb == AKTYWNY && (teraz - ostatniOdczyt > 2000)) {
    odczytBME();
    rysujDane();
    rysujMiniWykres(dystans);
    ostatniOdczyt = teraz;
  }

  delay(200);
}

void rysujEkranStartowy() {
  tft.fillScreen(KOL_TLO);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(KOL_AKCENT);
  tft.drawString("Asystent", 120, 40, 4);
  tft.setTextColor(KOL_BIALY);
  tft.drawString("Klimatyczny", 120, 75, 4);
  tft.setTextColor(KOL_SZARY);
  tft.drawString("v2.0  ESP32 TTGO", 120, 110, 2);
  tft.setTextDatum(TL_DATUM);
}

void rysujRamke() {
  tft.drawRect(0, 0, 240, 135, KOL_AKCENT);
  tft.drawRect(1, 1, 238, 133, KOL_TLO);
  tft.setTextColor(KOL_SZARY);
  tft.drawString("Biurkowy Asystent", 75, 4, 1);
}

void rysujDane() {
  tft.fillRect(2, 15, 236, 118, KOL_TLO);

  tft.setTextColor(KOL_SZARY);
  tft.drawString("TEMPERATURA", 10, 20, 1);
  tft.setTextColor(tempC > 25 ? KOL_CIEPŁY : KOL_ZIMNY);
  String strTemp = String(tempC, 1) + " C";
  tft.drawString(strTemp, 10, 32, 4);
  tft.setTextColor(KOL_SZARY);
  tft.drawString("o", 10 + tft.textWidth(strTemp, 4) - 12, 29, 1);

  tft.setTextColor(KOL_SZARY);
  tft.drawString("WILGOTNOSC", 130, 20, 1);
  tft.setTextColor(KOL_AKCENT);
  tft.drawString(String(wilgPct, 0) + " %", 130, 32, 4);

  tft.drawRect(130, 75, 100, 8, KOL_SZARY);
  tft.fillRect(131, 76, (int)(wilgPct / 100.0 * 98), 6, KOL_AKCENT);

  tft.drawFastVLine(122, 18, 100, KOL_SZARY);

  tft.setTextColor(KOL_SZARY);
  unsigned long s = millis() / 1000;
  tft.drawString("up: " + String(s / 60) + "m " + String(s % 60) + "s", 10, 120, 1);
}

void rysujMiniWykres(float dystans) {
  tft.fillRect(130, 85, 108, 40, KOL_TLO);
  tft.setTextColor(KOL_SZARY);
  tft.drawString("ODLEGLOSC", 132, 85, 1);
  tft.setTextColor(dystans < 999 ? KOL_BIALY : KOL_SZARY);
  tft.drawString(dystans < 999 ? String(dystans, 0) + " cm" : "--  cm", 132, 96, 2);
}

void rysujPowitanie() {
  tft.fillScreen(KOL_TLO);
  tft.setTextDatum(MC_DATUM);  
  tft.setTextColor(KOL_AKCENT);
  tft.drawString("Czesc!", 120, 50, 4);
  tft.setTextColor(KOL_BIALY);
  tft.drawString("Witaj przy biurku!", 120, 90, 2);
  tft.setTextDatum(TL_DATUM);  
}

void rysujTrybUspienia() {
  tft.fillScreen(KOL_TLO);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(KOL_SZARY);
  tft.drawString("Tryb uspienia", 120, 55, 2);
  tft.drawString("z  z  z", 120, 85, 4);
  tft.setTextDatum(TL_DATUM);
}