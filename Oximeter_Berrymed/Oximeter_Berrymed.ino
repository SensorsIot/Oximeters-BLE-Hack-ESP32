/*
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"
//#include "BLEScan.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// The remote service we wish to connect to.
static BLEUUID serviceUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455");
// The characteristic of the remote service we are interested in.
static BLEUUID    charOximeterUUID("49535343-1e4d-4bd9-ba61-23c647249616");
static BLEUUID    charDeviceDataUUID("00005344-0000-1000-8000-00805f9b34fb");
// The address of the target device (needed for connection when the device does not properly advertise services)
static BLEAddress berryMed("00:a0:50:db:83:94");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristicOximeter;
static BLERemoteCharacteristic* pRemoteCharacteristicDeviceData;
static BLEAdvertisedDevice* myDevice;
static unsigned int connectionTimeMs = 0;

int bpm;
int spo2;
float pi;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
//    Serial.print("Notify callback for characteristic ");
//    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
//    Serial.print(" of data length ");
//    Serial.println(length);
//    Serial.print("data (HEX): ");
//    for (int i = 0; i < length; i++) {
//      Serial.print(pData[i],HEX);
//      Serial.print(" ");
//    }
//    Serial.println();

    // readable values 
    char output[45];
    for (int i = 0; i < length / 5; i++) {
      uint8_t value1 = pData[i*5 + 1]; // this seems to be the absoption value (pulse oximeter plethysmographic trace, PPG)
      uint8_t value2 = pData[i*5 + 2]; // this seems to be the PPG value, devided by 7 and rounded to an integer
      uint8_t bpm = pData[i*5 + 3];
      uint8_t spo2 = pData[i*5 + 4];
      sprintf(output, "PPG: %3u; PPG/7: %3u; BPM: %3u; SPO2: %2u", value1, value2, bpm, spo2);
      Serial.println(output);
    }
  bpm = pData[3];
  spo2 = pData[4];
  pi = pData[0];

//    // output for use in CSV-based analysis (Excel etc.)
//    for (int i = 0; i < length / 5; i++) {
//      uint8_t value1 = pData[i*5 + 1];
//      uint8_t value2 = pData[i*5 + 2];
//      uint8_t bpm = pData[i*5 + 3];
//      uint8_t spo2 = pData[i*5 + 4];
//      Serial.print(value1,DEC);
//      Serial.print(",");
//      Serial.print(value2,DEC);
//      Serial.print(",");
//      Serial.print(bpm,DEC);
//      Serial.print(",");
//      Serial.print(spo2,DEC);
//      Serial.println("");
//    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristicDeviceData = pRemoteService->getCharacteristic(charDeviceDataUUID);
    if (pRemoteCharacteristicDeviceData == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charDeviceDataUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.print(" - Found our characteristic ");
    Serial.println(charDeviceDataUUID.toString().c_str());

    // Read the value of the characteristic.
    if(pRemoteCharacteristicDeviceData->canRead()) {
      Serial.println(" - Our characteristic can be read.");
      std::string value = pRemoteCharacteristicDeviceData->readValue();
      byte buf[64]= {0};
      memcpy(buf,value.c_str(),value.length());
      Serial.print("The characteristic value was: (0x) ");
      for (int i = 0; i < value.length(); i++) {
        Serial.print(buf[i],HEX);
        Serial.print(" ");
      }
      Serial.println();
      Serial.println("Past value                  : (0x) 94 83 DB 50 A0 00");
    }
    else {
      Serial.println(" - Our characteristic cannot be read.");
    }

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristicOximeter = pRemoteService->getCharacteristic(charOximeterUUID);
    if (pRemoteCharacteristicOximeter == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charOximeterUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.print(" - Found our characteristic ");
    Serial.println(charOximeterUUID.toString().c_str());

    // Read the value of the characteristic.
    if(pRemoteCharacteristicOximeter->canRead()) {
      Serial.println(" - Our characteristic can be read.");
      std::string value = pRemoteCharacteristicDeviceData->readValue();
      byte buf[64]= {0};
      memcpy(buf,value.c_str(),value.length());
      Serial.print("The characteristic value was: ");
      for (int i = 0; i < value.length(); i++) {
        Serial.print(buf[i],HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    else {
      Serial.println(" - Our characteristic cannot be read.");
    }

    if(pRemoteCharacteristicOximeter->canNotify()) {
      Serial.println(" - Our characteristic can notify us, registering notification callback.");
      pRemoteCharacteristicOximeter->registerForNotify(notifyCallback, true);
    }
    else {
      Serial.println(" - Our characteristic cannot notify us.");
    }

    if (pRemoteCharacteristicOximeter->canIndicate() == true) {
      Serial.println(" - Our characteristic can indicate.");
    } else {
      Serial.println(" - Our characteristic cannot indicate.");
    }

    // needed to start the notifications:
    pRemoteCharacteristicOximeter->readValue();
    const uint8_t notificationOn[] = {0x1, 0x0};
    pRemoteCharacteristicOximeter->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("\nBLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    Serial.print("Address: ");
    Serial.println(advertisedDevice.getAddress().toString().c_str());
    if (advertisedDevice.haveServiceUUID()) {
      Serial.println("Device has Service UUID");
      if (advertisedDevice.isAdvertisingService(serviceUUID)) {Serial.println("Device is advertising our Service UUID");}
      else {Serial.println("Device is not advertising our Service UUID");}
    }
    else {Serial.println("Device does not have Service UUID");}
    
    // We have found a device, let us now see if it contains the service we are looking for.
    if ((advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) || (advertisedDevice.getAddress().equals(berryMed))) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);

  connectionTimeMs = millis();
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
  display.display();
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    if (pRemoteCharacteristicOximeter->canWrite()) {
      // Set the characteristic's value to be the array of bytes that is actually a string.
      String newValue = "Time since boot: " + String(millis()/1000);
      Serial.println("Setting new characteristic value to \"" + newValue + "\"");
      pRemoteCharacteristicOximeter->writeValue(newValue.c_str(), newValue.length());
    }
  }
  else {
    if (doScan) {
      BLEDevice::getScan()->start(0);  // this is just an example to start scan after disconnect, most likely there is better way to do it in arduino
    }
    else { // enable connects if no device was found on first boot
      if (millis() > connectionTimeMs + 6000) {
        Serial.println("Enabling scanning.");
        doScan = true;
      }
    }
  }
  

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("BPM");
  display.setCursor(60, 10);
  display.println(bpm);
  display.setCursor(0, 30);
  display.println("SPO2:");
  display.setCursor(60, 30);
  display.println(spo2);
  display.setCursor(0, 50);
  display.println("PI:");
  display.setCursor(60, 50);
  display.println(pi / 10.0);
  display.display();

  delay(1000); // Delay a second between loops.
} // End of loop
