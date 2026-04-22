#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// =========================================================
// Pins
// =========================================================
int pins[8] = {3,1,16,17,21,22,25,26};


// =========================================================
// UUIDs for Service 1: Receiving Data (RX)
// =========================================================
#define RX_SERVICE_UUID         "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define RX_CHARACTERISTIC_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// =========================================================
// UUIDs for Service 2: Sending Data (TX)
// =========================================================
#define TX_SERVICE_UUID         "8a924409-7756-4279-b1d6-b0722cc2c186"
#define TX_CHARACTERISTIC_UUID  "e894672e-836b-4e1b-b4a1-baf461148cf4"

// =========================================================
// Global variables
// =========================================================
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

bool newDataReceived = false;
int data[8];
uint8_t hexdata[8];
// =========================================================
// Callback class to track device connection state
// =========================================================
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
        Serial.println("Device connected!");
    }

    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected!");
    }
};

// =========================================================
// Callback class to handle incoming BLE data
// =========================================================
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        uint8_t *rxData = pCharacteristic->getData();
        size_t rxLength = pCharacteristic->getLength();

        if (rxLength > 0) {
            newDataReceived = true;

            Serial.print("Received Data: ");
            for (int i = 0; i < rxLength; i++) {
                int val = rxData[i];
                hexdata[i] = rxData[i];
                data[i] = val;

                Serial.print(val);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
};

void setup() {
    
    int n = sizeof(pins) / sizeof(pins[0]);
    for (int i = 0; i < n; i++) {
      pinMode(pins[i], OUTPUT);
    }

    Serial.begin(115200);
    Serial.println("Starting BLE Server...");

    // =========================================================
    // 1. Initialize BLE
    // =========================================================
    BLEDevice::init("ESP32_TwoWay");

    // =========================================================
    // 2. Create BLE server and register connection callbacks
    // =========================================================
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // =========================================================
    // 3. Set up Service 1 (Receiving - RX)
    // =========================================================
    BLEService *pRxService = pServer->createService(RX_SERVICE_UUID);

    BLECharacteristic *pRxCharacteristic = pRxService->createCharacteristic(
        RX_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR
    );

    pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    pRxService->start();

    // =========================================================
    // 4. Set up Service 2 (Sending - TX)
    // =========================================================
    BLEService *pTxService = pServer->createService(TX_SERVICE_UUID);

    pTxCharacteristic = pTxService->createCharacteristic(
        TX_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );

    // Required so the phone can subscribe to notifications
    pTxCharacteristic->addDescriptor(new BLE2902());
    pTxService->start();

    // =========================================================
    // 5. Start advertising
    // =========================================================
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(RX_SERVICE_UUID);  // Advertise at least one main service
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();

    Serial.println("BLE is active. Waiting for connections...");
}

void loop() {
    // If a device is connected, we can send data back
    if (deviceConnected) {
        
        if (newDataReceived) {
            Serial.print("Data in LOOP : ");

            int n = sizeof(data) / sizeof(data[0]);
            for (int i = 0; i < n; i++) {
                Serial.print(pins[i]);
                Serial.print(":");
                Serial.print(data[i]);
                Serial.print(" ");
                analogWrite(pins[i], data[i]);
            }

            
            size_t txLength = sizeof(hexdata);
            pTxCharacteristic->setValue(hexdata, txLength);
            // Send the notification to the phone
            pTxCharacteristic->notify();

            newDataReceived = false;
        }

        delay(50);
    }

    // Handle disconnection smoothly by restarting advertising
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // Give the Bluetooth stack the chance to get things ready
        pServer->startAdvertising();
        Serial.println("Restarted advertising");
        oldDeviceConnected = deviceConnected;
    }

    // Handle connection state changes
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}