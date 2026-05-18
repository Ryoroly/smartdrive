#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <math.h>

#define SERVICE_UUID              "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define READ_CHARACTERISTIC_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a9"

BLECharacteristic *pCharacteristic;
BLECharacteristic *pReadCharacteristic; 

float filX = 0.0, filY = 0.0, filZ = 0.0;
const float ALPHA = 0.15; 

unsigned long lastCalculationTime = 0;
const unsigned long CALC_INTERVAL = 5000; 

float maxForceInWindow = 0.0;
int samplesInWindow = 0;

float aggressivenessScore = 0.0; 

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

          filX = (ALPHA * rawX) + ((1.0 - ALPHA) * filX);
          filY = (ALPHA * rawY) + ((1.0 - ALPHA) * filY);
          filZ = (ALPHA * rawZ) + ((1.0 - ALPHA) * filZ);

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
  Serial.println("Pornire sistem...");

  BLEDevice::init("ESP32_Telemetrie"); 
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE | 
                                         BLECharacteristic::PROPERTY_WRITE_NR
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());

  pReadCharacteristic = pService->createCharacteristic(
                                         READ_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  pReadCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE pornit. Astept conexiunea...");
}

void loop() {
  if (millis() - lastCalculationTime >= CALC_INTERVAL) {
    lastCalculationTime = millis();

    if (samplesInWindow > 0) {
      if (maxForceInWindow > 4.0) {
         aggressivenessScore += 15.0; 
      } else if (maxForceInWindow > 2.0) {
         aggressivenessScore += 5.0;  
      } else {
         aggressivenessScore -= 8.0;  
      }

      if (aggressivenessScore > 100.0) aggressivenessScore = 100.0;
      if (aggressivenessScore < 0.0) aggressivenessScore = 0.0;

      String driverProfile = "";
      if (aggressivenessScore <= 20)      driverProfile = "Foarte_Bun";
      else if (aggressivenessScore <= 40) driverProfile = "Bun";
      else if (aggressivenessScore <= 60) driverProfile = "Moderat";
      else if (aggressivenessScore <= 80) driverProfile = "Agresiv";
      else                                driverProfile = "Periculos";

      float insuranceModifier = 0.0;
      if (aggressivenessScore <= 20) {
        insuranceModifier = -5.0; 
      } else if (aggressivenessScore <= 40) {
        insuranceModifier = 0.0;  
      } else {
        insuranceModifier = ((aggressivenessScore - 40.0) / 60.0) * 80.0; 
      }

      Serial.println("==================================================");
      Serial.print("Forta Maxima: "); Serial.println(maxForceInWindow);
      Serial.print("Scor Agresivitate: "); Serial.print(aggressivenessScore); Serial.println("%");
      Serial.print("Profil: "); Serial.println(driverProfile);
      Serial.print("Asigurare: "); Serial.print(insuranceModifier); Serial.println("%");
      Serial.println("==================================================\n");

      // Modificarea din poza pentru trimiterea scorului catre telefon
      String scoreString = String((int)aggressivenessScore);
      pReadCharacteristic->setValue(scoreString.c_str());
      pReadCharacteristic->notify(); 

      Serial.println("-> Scor trimis catre telefon: " + scoreString);

      maxForceInWindow = 0.0;
      samplesInWindow = 0;
    }
  }
}
