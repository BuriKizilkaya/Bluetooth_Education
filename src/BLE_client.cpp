/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 *Adapted by Roland Zitzke for M5Stick-C Plus
 */

// Include M5Stick-C Plus libraries
#include <M5StickCPlus.h>

#include "BLEDevice.h"
#include "TFTTerminal.h"
//#include "BLEScan.h"

// Display stuff
TFT_eSprite Disbuff = TFT_eSprite(&M5.Lcd);
TFTTerminal terminal = TFTTerminal(&Disbuff);

// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                           uint8_t* pData, size_t length, bool isNotify) {
  terminal.print("Notify callback for characteristic ");
  terminal.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  terminal.print(" of data length ");
  terminal.println(length);
  terminal.print("data: ");
  terminal.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    terminal.println("onDisconnect");
  }
};

bool connectToServer() {
  terminal.print("Forming a connection to ");
  terminal.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  terminal.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of
                               // address, it will be recognized type of peer
                               // device address (public or private)
  terminal.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    terminal.print("Failed to find our service UUID: ");
    terminal.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  terminal.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE
  // server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    terminal.print("Failed to find our characteristic UUID: ");
    terminal.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  terminal.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    terminal.print("The characteristic value was: ");
    terminal.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we
 * are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    terminal.print("BLE Advertised Device found: ");
    terminal.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are
    // looking for.
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }  // Found our server
  }    // onResult
};     // MyAdvertisedDeviceCallbacks

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  M5.begin();
  M5.IMU.Init();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  Disbuff.createSprite(240, 135);
  terminal.setGeometry(0, 16, 240, 110);

  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when
  // we have detected a new device.  Specify that we want active scanning and
  // start the scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}  // End of setup.

// This is the Arduino main loop function.
void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the
  // desired BLE Server with which we wish to connect.  Now we connect to it.
  // Once we are connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      terminal.println("We are now connected to the BLE Server.");
    } else {
      terminal.println(
          "We have failed to connect to the server; there is nothin more we "
          "will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each
  // time we are reached with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis() / 1000);
    terminal.println("Setting new characteristic value to \"" + newValue +
                     "\"");

    // Set the characteristic's value to be the array of bytes that is actually
    // a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    BLEDevice::getScan()->start(
        0);  // this is just example to start scan after disconnect, most likely
             // there is better way to do it in arduino
  }

  delay(1000);  // Delay a second between loops.
}  // End of loop
