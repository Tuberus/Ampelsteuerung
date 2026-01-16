#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// --- PIN DEFINITIONEN ---
#define LED_RED    25
#define LED_YELLOW 26
#define LED_GREEN  27

// Struktur muss gleich sein wie beim Sender
typedef struct struct_message {
    int command;
} struct_message;

struct_message myData;

// --- FUNKTIONEN ---

void setLights(int r, int y, int g) {
  digitalWrite(LED_RED, r);
  digitalWrite(LED_YELLOW, y);
  digitalWrite(LED_GREEN, g);
}

// Callback wenn Daten empfangen werden
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  
  if (myData.command == 1) {
    // BEFEHL: WERDE ROT
    // Von Grün auf Gelb
    setLights(LOW, HIGH, LOW);
    delay(2000); // Da dies in einem Callback ist, ist delay() nicht ideal, aber ok für einfachen Test
    // Auf Rot
    setLights(HIGH, LOW, LOW);
    
  } else if (myData.command == 2) {
    // BEFEHL: WERDE GRÜN
    // Von Rot auf Rot-Gelb
    setLights(HIGH, HIGH, LOW);
    delay(1000);
    // Auf Grün
    setLights(LOW, LOW, HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Startzustand: GRÜN (Da Ampel 1 standardmäßig Rot ist)
  setLights(LOW, LOW, HIGH);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Callback registrieren
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void loop() {
  // Ampel 2 macht im Loop nichts, sie reagiert nur auf Empfang
  delay(100);
}