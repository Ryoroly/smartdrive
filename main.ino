#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// UUID pt serviciu si caracteristica
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// clasa pentru a prelua datele care vin de la telefon
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue(); 

      if (rxValue.length() > 0) {
        Serial.print("Date noi primite: ");
        
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Pornire BLE...");

  // setup de baza pt BLE
  BLEDevice::init("Telemetrie_ESP32"); 
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // setam caracteristica doar pt write (sa primeasca date)
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  // setari pt vizibilitate pe bluetooth
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  
  Serial.println("Astept conexiunea. Cauta 'Telemetrie_ESP32'.");
}

void loop() {
  delay(2000); 
}