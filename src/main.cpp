#include <WiFi.h>
#include <WiFiClient.h>

const char* ssid = "YOUR_ROUTER_SSID";
const char* password = "PASSWORD";

WiFiServer telnetServer(23);  // Port 23 usually used for the Telnet
WiFiClient telnetClient;

void setup()
{
    Serial.begin(921600);
    Serial.setTimeout(100); //RX timeout 100 ms
    sleep(1);
    // Connecting to Wi-Fi
    Serial.print("\nConnect to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    
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

    // Run Telnet-server
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    Serial.println("Telnet-server run at port 23.");
}

void loop()
{
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
  }

  if (Serial.available()) {
      String input = Serial.readString();
      if (telnetClient && telnetClient.connected()) {
          telnetClient.print(input);
      }
      Serial.print(input);
  }
}
