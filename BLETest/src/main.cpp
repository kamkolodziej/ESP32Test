#include "DHT.h" 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h> 
      
 
#define DHTPIN 5       
#define DHTTYPE DHT11     

#define TRGPIN 26       
#define ECHOPIN 27     

#define MAXRANGE 400        //max range in cm, making this higher makes the results unreliable
#define MINRANGE 3   

#define LEDPIN 2 

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

 
DHT dht(DHTPIN, DHTTYPE); // definicja czujnika

BLECharacteristic *pCharacteristic;

float oldTempValue = 0.0;
float oldDistanceValue = 0.0;
float oldHumidityValue = 0.0;

bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      digitalWrite(LEDPIN, HIGH);
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      digitalWrite(LEDPIN, LOW);
    }
};
 
void setup()
{
  Serial.begin(9600);     // otworzenie portu szeregowego
  pinMode(TRGPIN, OUTPUT); //set the pinmodes
  pinMode(ECHOPIN, INPUT);
  pinMode(LEDPIN, OUTPUT);
  dht.begin();

  BLEDevice::init("BLE"); 

   // Create the BLE Server
   BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);   

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    ); 

                      // Start the service
  pCharacteristic->addDescriptor(new BLE2902());
  
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();

}

float readTemperature(){
  float t = dht.readTemperature();
  if (isnan(t))
  {
    Serial.println("Temperature sensor read error.");
  }

  return t;
  }

 float readHumidity(){
  float h = dht.readHumidity();
  if (isnan(h))
  {
    Serial.println("Humidity sensor read error.");
  }

  return h;
  }

float readDuration() {
  digitalWrite(TRGPIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRGPIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRGPIN, LOW);
    float duration = (pulseIn(ECHOPIN, HIGH));
    return duration;
}

float correctedDistance( float temp){
  float duration = readDuration();
  float compensatedDistance = (duration / 2.0) / 29.14;
  Serial.println("Original distance: ");
  Serial.println(compensatedDistance);
  if(!isnan(temp)){
  float speedOfSound = 331.3 + (0.606 * temp);
  compensatedDistance = (duration / 20000.0) * speedOfSound;
  }
  
if (compensatedDistance >= MAXRANGE || compensatedDistance <= MINRANGE) {   
    return -1.0;
  }
  return compensatedDistance;
  }
 
void loop()
{



  if (deviceConnected) {
    Serial.println("Connected: ");

float temp = readTemperature();
float humi = readHumidity();
float dist = correctedDistance(temp);
dist = roundf(dist * 100) / 100;

if( humi!= oldHumidityValue || dist != oldDistanceValue || temp != oldTempValue){
  oldHumidityValue = humi;
  oldDistanceValue = dist;
  oldTempValue = temp;
  
  String buf;
  buf += String(humi, 1);
  buf += "|";
  buf += String(temp, 1);
  buf += "|";
  buf += String(dist, 1);

  Serial.println("Temp|Hum|Dist: ");
  Serial.println(buf);

   char txString[18]; // make sure this is big enuffz
   strcpy(txString, buf.c_str());
   pCharacteristic->setValue(txString);
   pCharacteristic->notify();

}

Serial.println("Temperature: ");
Serial.println(temp);
Serial.println("Humidity with correction: ");
Serial.println(humi);
Serial.println("Distance with correction: ");
Serial.println(dist);

  }
delay(2000); 

}

