/*
  INA226 Power Logger – ESP32-C3
  ------------------------------
  Mittaa jännitteen, virran ja tehon INA226-sensorilla ja tulostaa ne
  sarjaporttiin CSV-muodossa sekunnin välein. Oletuksena 10 minuutin loki
  (600 näytettä), jonka voi avata suoraan suomalaiseen Exceliin
  (puolipiste-erotin, desimaalipilkku).

  Kirjasto:  Rob Tillaart, "INA226"  (Arduino Library Manager, versio >= 0.6.0)
  Alusta:    ESP32-C3 (DevKitM-1, SuperMini, XIAO ESP32C3 ...)

  Lisenssi: MIT – katso LICENSE
*/

#include <Wire.h>
#include "INA226.h"

// ======================= ASETUKSET – MUOKKAA NÄITÄ ========================

// I2C-pinnit. ESP32-C3 DevKitM-1 ja useimmat SuperMini-levyt: SDA=8, SCL=9.
// Seeed XIAO ESP32C3: SDA=6, SCL=7.
const uint8_t I2C_SDA = 8;
const uint8_t I2C_SCL = 9;

// INA226:n I2C-osoite. 0x40 kun A0 ja A1 on kytketty GND:hen (moduulin oletus).
const uint8_t INA_ADDRESS = 0x40;

// Shunttivastus ohmeina. Lue moduulin vastuksen merkintä:
//   "R100" = 0.100 ohm   -> max 0.8 A
//   "R050" = 0.050 ohm   -> max 1.6 A
//   "R010" = 0.010 ohm   -> max 8 A
//   "R002" = 0.002 ohm   -> max 40 A
const float SHUNT_OHM = 0.100;

// Suurin mitattava virta ampeereina. INA226:n shunttijännitteen alue on
// +-81.92 mV, joten MAX_CURRENT_A * SHUNT_OHM on oltava alle 0.080 V.
// Arvo 0.75 jättää pienen marginaalin liukulukupyöristykselle.
const float MAX_CURRENT_A = 0.75;

// Näytteiden määrä ja väli. 600 x 1000 ms = 10 minuuttia.
const int      MAX_SAMPLES = 600;
const uint32_t INTERVAL_MS = 1000;

// Laitteistotason keskiarvoistus. Vaihtoehdot:
//   INA226_1_SAMPLE, INA226_4_SAMPLES, INA226_16_SAMPLES, INA226_64_SAMPLES,
//   INA226_128_SAMPLES, INA226_256_SAMPLES, INA226_512_SAMPLES, INA226_1024_SAMPLES
// 256 näytettä oletuskonversioajoilla (1.1 ms + 1.1 ms) kestää n. 0.56 s,
// joten se ehtii hyvin sekunnin mittausväliin.
const uint8_t AVERAGING = INA226_256_SAMPLES;

// CSV-muoto. Suomalainen Excel: erotin ';' ja desimaalipilkku ','.
// Englanninkielinen Excel / Python: erotin ',' ja desimaalipiste '.'.
const char CSV_SEPARATOR   = ';';
const char DECIMAL_POINT   = ',';

// ======================= ASETUKSET PÄÄTTYVÄT ==============================

INA226 ina(INA_ADDRESS);

int      sampleCount  = 0;
uint32_t startMs      = 0;
uint32_t nextSampleAt = 0;
bool     finished     = false;

// Pysäyttää ohjelman virheilmoitukseen. delay() loopissa pitää ESP32-C3:n
// idle-taskin hengissä, jolloin task watchdog ei ala huudella sarjaporttiin.
void halt(const char* msg) {
  Serial.println(msg);
  while (true) {
    delay(1000);
  }
}

// Tulostaa liukuluvun halutulla desimaalimerkillä.
void printNumber(float value, int decimals) {
  String s = String(value, decimals);
  if (DECIMAL_POINT != '.') {
    s.replace('.', DECIMAL_POINT);
  }
  Serial.print(s);
}

void setup() {
  Serial.begin(115200);
  delay(1000);                       // annetaan sarjaportin avautua

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!ina.begin()) {
    halt("VIRHE: INA226 ei vastaa. Tarkista SDA/SCL, VCC, osoite ja yhteinen GND.");
  }

  // Kalibrointi. Ilman tätä getCurrent/getPower palauttavat nollaa.
  // Paluuarvo tarkistetaan: 0 = OK, muu = virhe (ks. README).
  int err = ina.setMaxCurrentShunt(MAX_CURRENT_A, SHUNT_OHM);
  if (err != INA226_ERR_NONE) {
    Serial.print("VIRHE: kalibrointi epäonnistui, koodi 0x");
    Serial.println(err, HEX);
    halt("Tarkista MAX_CURRENT_A ja SHUNT_OHM (tulon oltava < 0.080 V).");
  }

  // Keskiarvoistus. Parametri on rekisteriarvo 0-7, EI näytteiden lukumäärä.
  if (!ina.setAverage(AVERAGING)) {
    halt("VIRHE: setAverage() epäonnistui.");
  }

  // Odotetaan että ensimmäinen keskiarvoistettu mittaus on valmis,
  // jotta ensimmäinen rivi ei ole puolivalmis.
  delay(700);

  // CSV-otsikkorivi
  Serial.print("Aika(s)");   Serial.print(CSV_SEPARATOR);
  Serial.print("Jannite(V)"); Serial.print(CSV_SEPARATOR);
  Serial.print("Virta(A)");  Serial.print(CSV_SEPARATOR);
  Serial.println("Teho(W)");

  startMs      = millis();
  nextSampleAt = startMs;
}

void loop() {
  if (sampleCount >= MAX_SAMPLES) {
    if (!finished) {
      Serial.println("Valmis");
      finished = true;
    }
    delay(1000);
    return;
  }

  // Odotetaan seuraavaa näytehetkeä. Erotus castataan etumerkilliseksi,
  // jolloin vertailu toimii myös millis()-ylivuodon yli (49 vrk).
  if ((int32_t)(millis() - nextSampleAt) < 0) {
    return;
  }
  nextSampleAt += INTERVAL_MS;       // kiinteä aikataulu: ei liu'u mittausajan verran

  float t = (millis() - startMs) / 1000.0f;
  float V = ina.getBusVoltage();             // V
  float I = ina.getCurrent_mA() / 1000.0f;   // mA -> A
  float P = ina.getPower_mW()   / 1000.0f;   // mW -> W

  printNumber(t, 1); Serial.print(CSV_SEPARATOR);
  printNumber(V, 3); Serial.print(CSV_SEPARATOR);
  printNumber(I, 3); Serial.print(CSV_SEPARATOR);
  printNumber(P, 3); Serial.println();

  sampleCount++;
}
