#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- Variabile pentru Filtrarea Datelor (Exponential Moving Average) ---
float filX = 0.0, filY = 0.0, filZ = 0.0;
const float ALPHA = 0.15; // Factor de netezire. Mai mic = filtru mai puternic (ignoră vibrațiile)

// --- Variabile pentru Fereastra de Timp (Procesare la X secunde) ---
unsigned long lastCalculationTime = 0;
const unsigned long CALC_INTERVAL = 5000; // Procesează datele la fiecare 5 secunde

float maxForceInWindow = 0.0; // Cea mai mare forță înregistrată în ultimele 5 secunde
int samplesInWindow = 0;      // Câte pachete de date am primit în fereastră

// --- Variabile pentru Profilul Șoferului ---
float aggressivenessScore = 0.0; // Procentaj de la 0.0 la 100.0


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String data = pCharacteristic->getValue(); 

      if (data.length() > 0) {
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);

        if (firstComma != -1 && secondComma != -1) {
          // Citim datele brute (Ideal ar fi ca acestea să fie date de la ACCELEROMETRU, nu doar giroscop)
          float rawX = data.substring(0, firstComma).toFloat();
          float rawY = data.substring(firstComma + 1, secondComma).toFloat();
          float rawZ = data.substring(secondComma + 1).toFloat();

          // 1. FILTRAREA DATELOR (reducem fluctuațiile foarte scurte)
          filX = (ALPHA * rawX) + ((1.0 - ALPHA) * filX);
          filY = (ALPHA * rawY) + ((1.0 - ALPHA) * filY);
          filZ = (ALPHA * rawZ) + ((1.0 - ALPHA) * filZ);

          // 2. CALCULUL FORȚEI (Magnitudinea pe plan orizontal)
          // Presupunem că X și Y reprezintă forțele de accelerare/frânare și forța laterală (viraje)
          float currentForce = sqrt((filX * filX) + (filY * filY));

          // Salvăm valoarea maximă a forței din această fereastră de timp
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

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE | 
                                         BLECharacteristic::PROPERTY_WRITE_NR
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE pornit. Aștept date din aplicație...");
}

void loop() {
  // Procesăm și afișăm datele doar o dată la 5 secunde
  if (millis() - lastCalculationTime >= CALC_INTERVAL) {
    lastCalculationTime = millis();

    if (samplesInWindow > 0) {
      // --- 3. LOGICA DE AGRESIVITATE ---
      // NOTĂ: Pragurile (4.0, 2.0) depind de ce unități trimite aplicația ta de telefon. 
      // Va trebui să faci câteva teste practice în mașină pentru a le calibra perfect.
      if (maxForceInWindow > 4.0) {
         aggressivenessScore += 15.0; // Manevră foarte bruscă, creștem penalizarea masiv
      } else if (maxForceInWindow > 2.0) {
         aggressivenessScore += 5.0;  // Manevră moderat de bruscă
      } else {
         aggressivenessScore -= 8.0;  // Condus calm, scorul scade
      }

      // Limităm scorul strict între 0 și 100
      if (aggressivenessScore > 100.0) aggressivenessScore = 100.0;
      if (aggressivenessScore < 0.0) aggressivenessScore = 0.0;

      // --- 4. CLASIFICARE ȘOFER ---
      String driverProfile = "";
      if (aggressivenessScore <= 20)      driverProfile = "Foarte Bun (Calm)";
      else if (aggressivenessScore <= 40) driverProfile = "Bun (Normal)";
      else if (aggressivenessScore <= 60) driverProfile = "Moderat (Atenție necesară)";
      else if (aggressivenessScore <= 80) driverProfile = "Agresiv (Risc crescut)";
      else                                driverProfile = "Foarte Agresiv (Periculos)";

      // --- 5. CALCUL ASIGURARE ---
      float insuranceModifier = 0.0;
      if (aggressivenessScore <= 20) {
        insuranceModifier = -5.0; // Scădere maximă permisă (bonus)
      } else if (aggressivenessScore <= 40) {
        insuranceModifier = 0.0;  // Fără modificare
      } else {
        // Dacă e peste 40, crește progresiv până la maxim 80% (când scorul e 100)
        // Formula: (Scor - Prag_Minim) / Diferenta_Pana_La_Max * Crestere_Maxima
        insuranceModifier = ((aggressivenessScore - 40.0) / 60.0) * 80.0; 
      }

      // --- 6. AFIȘARE REZULTATE ÎN SERIAL MONITOR ---
      Serial.println("==================================================");
      Serial.println("--- RAPORT TELEMETRIE (Ultimele 5 secunde) ---");
      Serial.print("Forța Maximă Detectată: "); Serial.println(maxForceInWindow);
      Serial.print("Scor Agresivitate Curent: "); Serial.print(aggressivenessScore); Serial.println(" %");
      Serial.print("Profil Șofer: "); Serial.println(driverProfile);
      
      Serial.print("Modificare Cost Asigurare: "); 
      if (insuranceModifier > 0) Serial.print("+");
      Serial.print(insuranceModifier); Serial.println(" %");
      Serial.println("==================================================\n");

      // Resetăm variabilele pentru următoarea fereastră de 5 secunde
      maxForceInWindow = 0.0;
      samplesInWindow = 0;
    }
  }
}