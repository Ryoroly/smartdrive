#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- Variabila globala pentru comunicarea BLE ---
BLECharacteristic *pCharacteristic;

// --- Variabile pentru Filtrarea Datelor (Exponential Moving Average) ---
float filX = 0.0, filY = 0.0, filZ = 0.0;
const float ALPHA = 0.15; // Factor de netezire.

// --- Variabile pentru Fereastra de Timp (Procesare la X secunde) ---
unsigned long lastCalculationTime = 0;
const unsigned long CALC_INTERVAL = 5000; // Procesează datele la fiecare 5 secunde

float maxForceInWindow = 0.0;
int samplesInWindow = 0;

// --- Variabile pentru Profilul Șoferului ---
float aggressivenessScore = 0.0; // Procentaj de la 0.0 la 100.0


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
      String data = pChar->getValue(); 

      if (data.length() > 0) {
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);

        if (firstComma != -1 && secondComma != -1) {
          float rawX = data.substring(0, firstComma).toFloat();
          float rawY = data.substring(firstComma + 1, secondComma).toFloat();
          float rawZ = data.substring(secondComma + 1).toFloat();

          // 1. FILTRAREA DATELOR
          filX = (ALPHA * rawX) + ((1.0 - ALPHA) * filX);
          filY = (ALPHA * rawY) + ((1.0 - ALPHA) * filY);
          filZ = (ALPHA * rawZ) + ((1.0 - ALPHA) * filZ);

          // 2. CALCULUL FORȚEI
          float currentForce = sqrt((filX * filX) + (filY * filY));

          if (currentForce > maxForceInWindow) {
            maxForceInWindow = currentForce;
          }
          samplesInWindow++;
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Pornire sistem telemetrie...");

  BLEDevice::init("ESP32_Telemetrie"); 
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Am adaugat PROPERTY_READ si PROPERTY_NOTIFY pentru a putea trimite date inapoi la telefon
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE | 
                                         BLECharacteristic::PROPERTY_WRITE_NR |
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE pornit. Astept date din aplicatie...");
}

void loop() {
  if (millis() - lastCalculationTime >= CALC_INTERVAL) {
    lastCalculationTime = millis();

    if (samplesInWindow > 0) {
      // --- 3. LOGICA DE AGRESIVITATE ---
      if (maxForceInWindow > 4.0) {
         aggressivenessScore += 15.0; 
      } else if (maxForceInWindow > 2.0) {
         aggressivenessScore += 5.0;  
      } else {
         aggressivenessScore -= 8.0;  
      }

      if (aggressivenessScore > 100.0) aggressivenessScore = 100.0;
      if (aggressivenessScore < 0.0) aggressivenessScore = 0.0;

      // --- 4. CLASIFICARE ȘOFER ---
      String driverProfile = "";
      if (aggressivenessScore <= 20)      driverProfile = "Foarte_Bun";
      else if (aggressivenessScore <= 40) driverProfile = "Bun";
      else if (aggressivenessScore <= 60) driverProfile = "Moderat";
      else if (aggressivenessScore <= 80) driverProfile = "Agresiv";
      else                                driverProfile = "Periculos";

      // --- 5. CALCUL ASIGURARE ---
      float insuranceModifier = 0.0;
      if (aggressivenessScore <= 20) {
        insuranceModifier = -5.0; 
      } else if (aggressivenessScore <= 40) {
        insuranceModifier = 0.0;  
      } else {
        insuranceModifier = ((aggressivenessScore - 40.0) / 60.0) * 80.0; 
      }

      // --- 6. AFIȘARE REZULTATE ÎN SERIAL MONITOR ---
      Serial.println("==================================================");
      Serial.print("Forta Maxima: "); Serial.println(maxForceInWindow);
      Serial.print("Scor Agresivitate: "); Serial.print(aggressivenessScore); Serial.println("%");
      Serial.print("Profil: "); Serial.println(driverProfile);
      Serial.print("Asigurare: "); Serial.print(insuranceModifier); Serial.println("%");
      Serial.println("==================================================\n");

      // --- 7. TRIMITERE DATE CATRE TELEFON PRIN BLE ---
      // Formatul va fi de tipul: "Scor,ModificatorAsigurare,ProfilSofer"
      // Exemplu: "25.50,0.00,Bun"
      String payload = String(aggressivenessScore, 2) + "," + 
                       String(insuranceModifier, 2) + "," + 
                       driverProfile;
                       
      // Setam valoarea si notificam telefonul
      pCharacteristic->setValue(payload.c_str());
      pCharacteristic->notify();
      
      Serial.println("-> Date trimise catre telefon: " + payload);

      // Resetăm variabilele
      maxForceInWindow = 0.0;
      samplesInWindow = 0;
    }
  }
}