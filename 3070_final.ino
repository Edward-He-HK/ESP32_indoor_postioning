#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLE2902.h>
#include <BLEAdvertisedDevice.h>
#include <math.h>
#include <BLEDevice.h>



BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
int scanTime = 1; //In seconds
BLEScan* pBLEScan;
float rssiValue = 0;
float distance = 0;
float txValue = 0;
bool find = false;


//vaierable for reading the voltage
const int V_PIN = 34;
const int BLUE = 25;
const int GREEN = 26;
const int RED = 27;
int Readvalue = 0;
float voltage;
float perc;


#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    void onResult(BLEAdvertisedDevice advertisedDevice) {

        if(advertisedDevice.getAddress().toString() == "6c:9d:03:4f:b2:f1")
        {
         find = true;
          rssiValue=advertisedDevice.getRSSI();
        }
        else if(!find) 
        {
          Serial.println("cannot find the device address");
          Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
         Serial.print(" RSSI: ");
         Serial.println(advertisedDevice.getRSSI());
        }  
      
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(V_PIN, INPUT);

  BLEDevice::init("Peach Parkk");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value


// Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());


  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  
}

void loop() {
// For reading voltage:
  Readvalue = analogRead(V_PIN);
  voltage = float(Readvalue) * 6.95/4095.0;
  perc = (voltage-2.75)*100.0/(4.2-2.75);

//Present battery level on LED
//Turn off all LED
  digitalWrite(BLUE, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(RED, LOW);
  if(perc >= 75)
    digitalWrite(GREEN, HIGH);   //More than 75%, turn on green led
  else if(perc <= 25)
    digitalWrite(RED, HIGH);    //Less than 25%, turn on red led
  else
     digitalWrite(BLUE, HIGH); //Otherwise, turn on green led

  //Calculate the Distance by using RSSI value
  distance=pow(10.0,(-57.0 - rssiValue)/(10.0*3.0));

  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);

   if (deviceConnected) {
    txValue = distance; 

    // Let's convert the value to a char array:
    char txString[8]; // make sure this is big enuffz
    dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

    //Set distance to the value which need to send to the smartphone
    pCharacteristic->setValue(txString);


    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    pCharacteristic->notify(); // Send the value to the app!
   }
    
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
}
