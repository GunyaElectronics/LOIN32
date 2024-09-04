#include <WiFi.h>
#include <WiFiClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_sleep.h>

//-- Local constants --------------------------------------------------------
const char* ssid = "YOUR_ROUTER_SSID";
const char* password = "PASSWORD";
const uint32_t baudrate = 921600;
const uint32_t rxTimeoutMs = 100;
const uint16_t connectionTimeoutSec = 30;

//-- Static variables and objects -------------------------------------------
WiFiServer telnetServer(23);  // Port 23 usually used for the Telnet
WiFiClient telnetClient;

//-- Static function prototypes ---------------------------------------------
static void printDirectory(fs::FS &fs, const char *dirname, uint8_t levels);
static void initFileSystem();
static void printMacAddreses();
static void printWiFiNetworkParam();

//-- Initialisation function implementation ---------------------------------
void setup()
{
    Serial.begin(baudrate);
    Serial.setTimeout(rxTimeoutMs);

    sleep(1);
    Serial.println("");

    initFileSystem();

    // Connecting to Wi-Fi
    Serial.print("\nConnect to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    printMacAddreses();

    while (WiFi.status() != WL_CONNECTED) {
        sleep(1);
        Serial.print(".");
    }

    printWiFiNetworkParam();

    // Run Telnet-server
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    Serial.println("Telnet-server run at port 23.");
}

//-- Loop function implementation -------------------------------------------
void loop()
{
    static uint16_t withoutClientSec = 0;

    if (telnetServer.hasClient()) {
        if (!telnetClient || !telnetClient.connected()) {
            if (telnetClient) telnetClient.stop();
            telnetClient = telnetServer.available();
            Serial.print("Telnet has been connected. Сlient IP: ");
            Serial.println(telnetClient.remoteIP());
        } else {
            WiFiClient rejectClient = telnetServer.available();
            rejectClient.stop();
            Serial.println("Additional client rejected.");
        }
    }

    if (telnetClient && telnetClient.connected()) {
        while (telnetClient.available()) {
            char c = telnetClient.read();
            Serial.write(c);
        }
    } else {
        if (withoutClientSec == connectionTimeoutSec) {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);

            Serial.println("Go to the deep sleep.");            
            // 600 second deep sleep
            esp_sleep_enable_timer_wakeup(600 * 1000000);
            esp_deep_sleep_start();
            withoutClientSec = 0;
        }

        withoutClientSec++;
        sleep(1);
    }

    if (Serial.available()) {
        String input = Serial.readString();
        if (telnetClient && telnetClient.connected()) {
            telnetClient.print(input);
        }
        Serial.print(input);
    }
}

//-- Static function implementation -----------------------------------------
static void printDirectory(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Files list in dir: %s\n", dirname);
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        Serial.println("Can`t read directory.");
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("Dir: ");
            Serial.println(file.name());
            if (levels) {
                printDirectory(fs, file.name(), levels - 1);
            }
        } else {
            Serial.print("File: ");
            Serial.print(file.name());
            Serial.print(" | size: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

static void initFileSystem()
{
    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR after init SPIFFS or SPIFFS not found. Formatting...");
        if (SPIFFS.format()) {
            Serial.println("SPIFFS successly formated.");
        } else {
            Serial.println("SPIFFS Formatting ERROR.");
            return;
        }
    } else {
        Serial.println("SPIFFS init success.");
    }

    const uint32_t totalBytes = SPIFFS.totalBytes();
    const uint32_t usedBytes = SPIFFS.usedBytes();
    const uint32_t freeBytes = totalBytes - usedBytes;

    Serial.printf("Total FS size bytes: %u\n", totalBytes);
    Serial.printf("Used FS bytes: %u\n", usedBytes);
    Serial.printf("Free FS bytes: %u\n", freeBytes);

    printDirectory(SPIFFS, "/", 1);
}

static void printMacAddreses()
{
    String macAddressSTA = WiFi.macAddress();
    uint8_t macAddressAP[6];
    esp_read_mac(macAddressAP, ESP_MAC_WIFI_SOFTAP);

    String macAddressAPStr = "";
    for (int i = 0; i < 6; i++) {
        if (macAddressAP[i] < 16) {
            macAddressAPStr += "0";
        }
        macAddressAPStr += String(macAddressAP[i], HEX);
        if (i < 5) {
            macAddressAPStr += ":";
        }
    }
    macAddressAPStr.toUpperCase();

    Serial.print("MAC (STA): ");
    Serial.println(macAddressSTA);

    Serial.print("MAC (AP): ");
    Serial.println(macAddressAPStr);
}

static void printWiFiNetworkParam()
{
    Serial.println("");
    Serial.println("WiFi connected!");

    // Network param
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("Subnet mask: ");
    Serial.println(WiFi.subnetMask());

    Serial.print("DNS server: ");
    Serial.println(WiFi.dnsIP());
}
