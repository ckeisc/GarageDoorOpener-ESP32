#include <EEPROM.h>
#include <HomeSpan.h>
#include <WiFi.h>

#include "Command.h"
#include "GarageDoorAccessory.h"
#include "WiFiHelpers.h"

#define RELAY_PIN 23

String ssid = "YOUR_SSID";
String pass = "YOUR_PASSWORD";

std::shared_ptr<GarageDoorAccessory> garage;
bool homeSpanStarted = false;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_STA);
  homeSpan.setPairingCode("11112222");

  // Hand WiFi credentials to HomeSpan BEFORE begin() and let HomeSpan own
  // the WiFi connection from here on. Do NOT call WiFi.begin() in the
  // sketch: HomeSpan calls it itself on its first poll(), and a second
  // WiFi.begin() while already connected forces a disconnect/reconnect
  // cycle (Arduino-ESP32 STAClass::connect() disconnects first when it
  // sees an active connection). That fight is what caused the endless
  // "WiFi Connection Lost!" loop.
  homeSpan.setWifiCredentials(ssid.c_str(), pass.c_str());

  Serial.println("* ESP32 booted up.");

  delay(2000);
  connect();
}

void loop() {
  // If we have content available, let's parse it
  if (Serial.available()) {
    // Get our raw command string and remove any extra newline characters
    String rawCommand = Serial.readStringUntil('\n');
    rawCommand.trim();

    // Interpret the raw string into a `Command` enum and handle it
    Command command = getCommand(rawCommand);
    handleCommand(command);
  }

  // Poll unconditionally. HomeSpan manages WiFi itself, including
  // reconnecting if the link drops. Gating poll() on WiFi.status()
  // starves HomeSpan's own connection state machine.
  homeSpan.poll();
}

void handleCommand(Command command) {
  switch (command.type) {
    case GET_WIFI_CONN:
      {
        getWiFiConnectionStatus();
        break;
      }
    case GET_IP_ADDR:
      {
        getIPAddress();
        break;
      }
    case GET_MAC_ADDR:
      {
        getMACAddress();
        break;
      }
    case GET_HOSTNAME:
      {
        getHostName();
        break;
      }
    case CONNECT:
      {
        connect();
        break;
      }
    case DISCONNECT:
      {
        disconnect();
        break;
      }
    case RESET:
      {
        reset();
        break;
      }
    case OPEN_GARAGE: {
      garage->open();
      break;
    }
    case CLOSE_GARAGE: {
      garage->close();
      break;
    }
    default:
      {
        Serial.println("* Unknown command");
        break;
      }
  }
}

void getWiFiConnectionStatus() {
  Serial.println(wifiStatusDescription(WiFi.status()));
}

void getIPAddress() {
  Serial.println(WiFi.localIP().toString());
}

void getMACAddress() {
  Serial.println(WiFi.macAddress());
}

void getHostName() {
  Serial.println(WiFi.getHostname());
}

void connect() {
  // homeSpan.begin() may only run once; a second CONNECT command must
  // not re-initialize it.
  if (homeSpanStarted) {
    Serial.println("* HomeSpan already started.");
    return;
  }

  homeSpan.begin(Category::GarageDoorOpeners, "Garage Door");

  // Create a new accessory
  new SpanAccessory();

  // Add Accessory Information Service with required characteristics
  new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name("Garage Door");
    new Characteristic::Manufacturer("William Lumley");
    new Characteristic::SerialNumber("123456789");
    new Characteristic::Model("ESP32 Garage Door");
    new Characteristic::FirmwareRevision("1.0");

  garage = std::make_shared<GarageDoorAccessory>();
  garage->onOpen = []() {
    toggleRelayPin();
  };
  garage->onClose = []() {
    toggleRelayPin();
  };

  homeSpanStarted = true;

  // Note: at this point HomeSpan has not connected yet - it connects on
  // its first poll(). Expect WL_DISCONNECTED/WL_IDLE_STATUS here; the
  // "Trying to connect..." / "WiFi Connected!" lines appear right after.
  Serial.println("* WiFi Connection: " + String(wifiStatusDescription(WiFi.status())));
}

void disconnect() {
  WiFi.disconnect();
  Serial.println("* Disconnected from WiFi. HomeSpan will reconnect on its own.");
}

void reset() {
  // Tell the HomeSpan to unpair
  homeSpan.processSerialCommand("U");
}

// Active-LOW relay module: LOW energizes the relay (button "pressed"),
// HIGH releases it. 800 ms press, then release.
void toggleRelayPin() {
  digitalWrite(RELAY_PIN, HIGH);
  delay(800);
  digitalWrite(RELAY_PIN, LOW);
}

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}
