/*************************************************************

  ESP32 IoT: TDS (PPM), DHT11 (Suhu & Kelembaban), LDR (Cahaya)
  + Kontrol Relay Lampu & Pompa (Otomatis & Manual)
  Kirim ke Blynk Cloud

  Pin:
  - TDS Sensor OUT: GPIO34
  - DHT11 DATA: GPIO27
  - LDR OUT: GPIO35
  - Relay Lampu: GPIO14
  - Relay Pompa: GPIO12

  Virtual Pin Mapping:
  - V5: TDS (PPM)
  - V6: Suhu (°C)
  - V7: Kelembaban (%)
  - V8: Cahaya (%)
  - V9: Status Lampu
  - V10: Status Pompa
  - V11: Tombol Manual Lampu
  - V12: Tombol Manual Pompa

 *************************************************************/

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "xxxxx"
#define BLYNK_TEMPLATE_NAME         "xxxxx"
#define BLYNK_AUTH_TOKEN            "xxxxx"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// === Pin Definitions ===
#define TDS_PIN        34
#define LDR_PIN        35
#define DHT_PIN        27
#define RELAY_LAMPU    14
#define RELAY_POMPA    12
#define DHT_TYPE       DHT11

// === TDS Calibration ===
#define K_VALUE          1.0
#define ADC_RESOLUTION   4096.0
#define VREF             3.3

// === Thresholds ===
#define CAHAYA_MIN       20
#define CAHAYA_MAX       70
#define PPM_ON           500
#define PPM_OFF          450

// === DHT Setup ===
DHT dht(DHT_PIN, DHT_TYPE);

// WiFi credentials
char ssid[] = "SSID-NAME";
char pass[] = "SSID-PASS";

BlynkTimer timer;

// Status kontrol manual
bool manual_lampu = false;
bool manual_pompa = false;
bool manual_lampu_state = false;
bool manual_pompa_state = false;

// Fungsi untuk kontrol relay
void kontrolRelay() {
  // === 1. Baca Cahaya ===
  int ldr_adc = analogRead(LDR_PIN);
  float light_percent = (1.0 - (float)ldr_adc / 4095.0) * 100.0;
  light_percent = constrain(light_percent, 0.0, 100.0);

  // === 2. Baca TDS ===
  int tds_adc = analogRead(TDS_PIN);
  float tds_voltage = tds_adc * (VREF / ADC_RESOLUTION);
  float tds_ppm = (tds_voltage / VREF) * 1000.0 * K_VALUE;
  if (tds_ppm < 0) tds_ppm = 0;

  // === Mode Otomatis (hanya jika manual tidak aktif) ===
  if (!manual_lampu) {
    if (light_percent < CAHAYA_MIN) {
      digitalWrite(RELAY_LAMPU, HIGH);  // Nyala
      Serial.println("LAMPU NYALA (Otomatis: Cahaya < 20%)");
    } else if (light_percent > CAHAYA_MAX) {
      digitalWrite(RELAY_LAMPU, LOW);   // Mati
      Serial.println("LAMPU MATI (Otomatis: Cahaya > 70%)");
    }
  }

  if (!manual_pompa) {
    if (tds_ppm > PPM_ON) {
      digitalWrite(RELAY_POMPA, HIGH);  // Nyala
      Serial.println("POMPA NYALA (Otomatis: PPM > 500)");
    } else if (tds_ppm < PPM_OFF) {
      digitalWrite(RELAY_POMPA, LOW);   // Mati
      Serial.println("POMPA MATI (Otomatis: PPM < 450)");
    }
  }

  // === Kirim ke Blynk Virtual Pin ===
  Blynk.virtualWrite(V5, tds_ppm);        // PPM
  Blynk.virtualWrite(V6, dht.readTemperature());    // Suhu
  Blynk.virtualWrite(V7, dht.readHumidity());       // Kelembaban
  Blynk.virtualWrite(V8, light_percent);  // Cahaya

  // Kirim status relay
  Blynk.virtualWrite(V9, digitalRead(RELAY_LAMPU));  // Status Lampu
  Blynk.virtualWrite(V10, digitalRead(RELAY_POMPA)); // Status Pompa
}

// === Fungsi untuk kontrol manual lampu ===
BLYNK_WRITE(V11) {
  int value = param.asInt();
  if (value == 1) {
    digitalWrite(RELAY_LAMPU, HIGH);
    manual_lampu = true;
    manual_lampu_state = true;
    Serial.println("LAMPU DINYALAKAN (Manual)");
  } else {
    digitalWrite(RELAY_LAMPU, LOW);
    manual_lampu = true;
    manual_lampu_state = false;
    Serial.println("LAMPU DIMATIKAN (Manual)");
  }
}

// === Fungsi untuk kontrol manual pompa ===
BLYNK_WRITE(V12) {
  int value = param.asInt();
  if (value == 1) {
    digitalWrite(RELAY_POMPA, HIGH);
    manual_pompa = true;
    manual_pompa_state = true;
    Serial.println("POMPA DINYALAKAN (Manual)");
  } else {
    digitalWrite(RELAY_POMPA, LOW);
    manual_pompa = true;
    manual_pompa_state = false;
    Serial.println("POMPA DIMATIKAN (Manual)");
  }
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi pin relay
  pinMode(RELAY_LAMPU, OUTPUT);
  pinMode(RELAY_POMPA, OUTPUT);
  digitalWrite(RELAY_LAMPU, LOW);  // Awal mati
  digitalWrite(RELAY_POMPA, LOW);  // Awal mati

  dht.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Kirim data setiap 5 detik
  timer.setInterval(5000L, kontrolRelay);
}

void loop() {
  Blynk.run();
  timer.run();
}
