/* ESP-NOW Ampelsteuerung 
  Mit diesem Code der darunter Folgt kann man die MAC-Adresse des EPS-NOW Empfängers herausfinden:
  #include "WiFi.h"

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());
}

void loop(){}
  ----------------------------------------
  Dieses Programm steuert eine Ampel (Ampel 1) basierend auf der Entfernung eines Objekts, das von einem Ultraschallsensor (HC-SR04) erkannt wird.
  Wenn ein Objekt näher als 30 cm kommt, sendet Ampel 1 über ESP-NOW einen Befehl an Ampel 2, um auf Rot zu schalten, bevor Ampel 1 selbst auf Grün wechselt.
  
  Verkabelung:
  - LED_ROT    -> Pin 25
  - LED_GELB   -> Pin 26
  - LED_GRUEN  -> Pin 27
  - TRIG_PIN   -> Pin 14 (HC-SR04 Trigger)
  - ECHO_PIN   -> Pin 12 (HC-SR04 Echo)
  
  Hinweis: Die MAC-Adresse von Ampel 2 muss in der Variable 'broadcastAddress' eingetragen werden.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// --- PIN DEFINITIONEN ---
#define LED_RED    25
#define LED_YELLOW 26
#define LED_GREEN  27

// Sensor Pins (HC-SR04)
#define TRIG_PIN   14
#define ECHO_PIN   12

// --- ESP-NOW CONFIG ---
// HIER MUSS DIE MAC-ADRESSE VON AMPEL 2 REIN!
// Beispiel: {0x24, 0x6F, 0x28, 0x1A, 0x50, 0x32}
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

// Struktur der Nachricht
typedef struct struct_message {
  int command; // 1 = Rot werden, 2 = Grün werden
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// --- FUNKTIONEN ---

long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2; // Distanz in cm
}

void sendCommand(int cmd) {
  myData.command = cmd;
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
}

void setLights(int r, int y, int g) {
  digitalWrite(LED_RED, r);
  digitalWrite(LED_YELLOW, y);
  digitalWrite(LED_GREEN, g);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Default Zustand: ROT
  setLights(HIGH, LOW, LOW);

  // WiFi Init
  WiFi.mode(WIFI_STA);

  // ESP-NOW Init
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Peer registrieren
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  long distance = readDistance();
  Serial.print("Distanz: ");
  Serial.println(distance);

  // Wenn jemand näher als 30cm ist und wir aktuell ROT sind
  if (distance > 0 && distance < 30) {
    
    // 1. Ampel 2 auf ROT schalten
    Serial.println("Verkehr erkannt! Schalte Ampel 2 auf ROT...");
    sendCommand(1); // 1 = Befehl für Rot
    
    delay(3000); // Warten bis Ampel 2 sicher rot ist
    
    // 2. Ampel 1 wird GRÜN
    setLights(HIGH, HIGH, LOW); // Rot-Gelb
    delay(1000);
    setLights(LOW, LOW, HIGH);  // Grün
    
    delay(5000); // 5 Sekunden Grünphase
    
    // 3. Ampel 1 wird wieder ROT
    setLights(LOW, HIGH, LOW);  // Gelb
    delay(2000);
    setLights(HIGH, LOW, LOW);  // Rot
    
    delay(1000); // Sicherheitssekunde

    // 4. Ampel 2 darf wieder GRÜN werden
    Serial.println("Gebe Ampel 2 frei...");
    sendCommand(2); // 2 = Befehl für Grün
    
    // Kurze Pause, damit der Sensor nicht sofort wieder auslöst
    delay(5000); 
  }
  
  delay(100);
}