#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// la fel, pastram UUID-urile
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// aici procesam ce vine de la telefon
// class MyCallbacks: public BLECharacteristicCallbacks {
//     void onWrite(BLECharacteristic *pCharacteristic) {
//       String rxValue = pCharacteristic->getValue(); 

//       if (rxValue.length() > 0) {
//         Serial.print("Giroscop [X, Y, Z]: ");
        
//         // afisam fix valorile primite
//         for (int i = 0; i < rxValue.length(); i++) {
//           Serial.print(rxValue[i]);
//         }
//         Serial.println();
        
//         // aici pe viitor vei pune formula ta de calcul 
//         // ca sa vezi daca soferul e agresiv
//       }
//     }
// };

// aici procesam ce vine de la telefon
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // Preluam valoarea bruta ca std::string
      std::string rxValue = pCharacteristic->getValue(); 

      if (rxValue.length() > 0) {
        // Convertim in String de Arduino pentru a folosi functii mai usoare
        String data = String(rxValue.c_str());
        
        // 1. Căutăm unde sunt virgulele în textul primit
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);

        // Dacă am găsit ambele virgule, înseamnă că formatul este corect
        if (firstComma != -1 && secondComma != -1) {
          
          // 2. Extragem bucățile de text dintre virgule și le facem numere (float)
          float gyroX = data.substring(0, firstComma).toFloat();
          float gyroY = data.substring(firstComma + 1, secondComma).toFloat();
          float gyroZ = data.substring(secondComma + 1).toFloat();

          // 3. Afișăm valorile ordonat în Serial Monitor
          Serial.print("Valori Giroscop -> X: "); Serial.print(gyroX);
          Serial.print(" | Y: "); Serial.print(gyroY);
          Serial.print(" | Z: "); Serial.println(gyroZ);

          // ---------------------------------------------------------
          // 4. LOGICA TA PENTRU TELEMETRIE / CONDUS AGRESIV
          // ---------------------------------------------------------
          // Presupunând că telefonul stă vertical în suport, 
          // axa Y sau Z va înregistra rotația stânga/dreapta.
          // Valorile sunt în radiani pe secundă (rad/s).
          
          float pragViraj = 1.2; // Sensibilitatea. Modifică acest număr după teste.

          // Verificăm axa Z (sau Y, depinde cum ții telefonul în mașină)
          if (gyroZ > pragViraj) {
            Serial.println("ALERTA: Viraj strâns la STÂNGA!");
            // Aici poti aprinde un LED, declansa un buzzer etc.
            // digitalWrite(LED_PIN, HIGH);
          } 
          else if (gyroZ < -pragViraj) {
            Serial.println("ALERTA: Viraj strâns la DREAPTA!");
            // digitalWrite(LED_PIN, HIGH);
          }
          
          Serial.println("-----------------------------------");
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

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
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