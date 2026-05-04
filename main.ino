#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// la fel, pastram UUID-urile
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      
      String data = pCharacteristic->getValue(); 

      // 1. AFISAM DATELE BRUTE EXACT CUM VIN (pentru testare)
      Serial.print("Date primite de la Flutter: ");
      Serial.println(data);

      if (data.length() > 0) {
        
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);

        if (firstComma != -1 && secondComma != -1) {
          
          float gyroX = data.substring(0, firstComma).toFloat();
          float gyroY = data.substring(firstComma + 1, secondComma).toFloat();
          float gyroZ = data.substring(secondComma + 1).toFloat();

          Serial.print("Valori Decodate -> X: "); Serial.print(gyroX);
          Serial.print(" | Y: "); Serial.print(gyroY);
          Serial.print(" | Z: "); Serial.println(gyroZ);

          float pragViraj = 1.2; 

          if (gyroZ > pragViraj) {
            Serial.println("ALERTA: Viraj strâns la STÂNGA!");
          } 
          else if (gyroZ < -pragViraj) {
            Serial.println("ALERTA: Viraj strâns la DREAPTA!");
          }
          
          Serial.println("-----------------------------------");
        } else {
          Serial.println("Eroare: Format gresit (lipsesc virgulele)!");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Pornire sistem telemetrie...");

  BLEDevice::init("ESP32_Giroscop"); 
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // AICI AM REPARAT CONFLICTUL: Am permis atat WRITE cat si WRITE_NR
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
  
  Serial.println("BLE pornit. Astept date din aplicatie...");
}

void loop() {
  // procesorul e liber, toata treaba se face in onWrite cand vin date
  delay(2000); 
}