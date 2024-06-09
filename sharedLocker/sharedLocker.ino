#if defined(ARDUINO_PORTENTA_C33)
#include <WiFiC3.h>
#elif defined(ARDUINO_UNOWIFIR4)
#include <WiFiS3.h>
#endif

#include <PubSubClient.h>
#include "WiFiSSLClient.h"

#include <DFRobot_RGBLCD1602.h>

#include <ArduinoJson.h>

// LCD
DFRobot_RGBLCD1602 lcd(/*RGBAddr*/0x2D ,/*lcdCols*/16,/*lcdRows*/2);  //16 characters and 2 lines of show

// 보관함 정보
const char *thisBuildingName = "정보공학관";
const int thisBuildingNumber = 23;
const int thisFloorNumber = 1;
const int lockerNumbers[] = { 102 };

// 솔레노이드 핀  HIGH: 열림, LOW: 닫힘
int solpin = 7;

// WiFi credentials
const char *ssid = "network";
const char *pass = "00001234";

// MQTT Broker settings
const int mqtt_port = 8883;  // MQTT port (TLS)
const char *mqtt_broker = "r14333b1.ala.eu-central-1.emqxsl.com";  // EMQX broker endpoint
const char *mqtt_topic_req = "sharedLocker-request";     // MQTT topic
const char *mqtt_topic_res = "sharedLocker-response";
const char *mqtt_username = "capstone";  // MQTT username for authentication
const char *mqtt_password = "capstone";  // MQTT password for authentication

// WiFi and MQTT client initialization
WiFiSSLClient client;
PubSubClient mqtt_client(client);

int wifiStatus = WL_IDLE_STATUS;

static const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void connectToWiFi(){
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    wifiStatus = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to WiFi");
  printWifiStatus();
}

void connectToMQTT() {
    client.setCACert(ca_cert);
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-" + random(300);
        Serial.println("Connecting to MQTT Broker.....");
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.println(mqtt_client.state());
            delay(5000);
        }
    }
}

void mqttCallback(char *mqtt_topic, byte *payload, unsigned int length) {
    Serial.print("Message received on mqtt_topic: ");
    Serial.println(mqtt_topic);
    Serial.print("Message: ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println("\n-----------------------");

  JsonDocument doc;
  deserializeJson(doc, payload);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  int success = doc["success"];
  String message = doc["message"];
  
  if (doc["success"] == true) {
    // String buildingName = doc["value"]["buildingName"];
    const int buildingNumber = doc["value"]["buildingNumber"];
    const int floorNumber = doc["value"]["floorNumber"];
    const int lockerNumber = doc["value"]["lockerNumber"];

    if (!isMatchingLocker(buildingNumber, floorNumber, lockerNumber)) {
      // 서버의 응답 정보가 현재 보관함과 일치하지 않는 경우
      Serial.println("invalid locker");
      lcd.clear();
      lcd.print("This is not your");
      lcd.setCursor(0, 1);
      lcd.print("Locker");
      delay(5000);
      lcd.clear();
      lcd.print("Shared Locker");
      return;
    }
    lcd.clear();
    lcd.print("Locker ");
    lcd.print(String(lockerNumber));
    lcd.setCursor(0, 1);
    lcd.print("Opened");

    digitalWrite(solpin, HIGH);
    delay(10000);
    digitalWrite(solpin, LOW);

    lcd.clear();
    lcd.print("Shared Locker");
  } else {
    Serial.println("failed response");
    lcd.clear();
    lcd.print("Invalid QR Code");
    delay(5000);
    lcd.clear();
    lcd.print("Shared Locker");
  }
}
// 보관함 정보가 일치하는지 확인
bool isMatchingLocker(const int buildingNumber, const int floorNumber, const int lockerNumber) {
    if (thisBuildingNumber != buildingNumber) {
        return false;
    }
    if (thisFloorNumber != floorNumber) {
        return false;
    }

    // lockerNumbers 배열에서 lockerNumber가 존재하는지 확인
    int size = sizeof(lockerNumbers) / sizeof(lockerNumbers[0]);
    for (int i = 0; i < size; i++) {
        if (lockerNumbers[i] == lockerNumber) {
            return true;
        }
    }
    return false;
}
void setup(){
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial);

  connectToWiFi();

  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTT();
  
  lcd.init();
  lcd.clear();
  // lcd.print(thisLockerNumber);
  lcd.print("Shared Locker");

  pinMode(solpin, OUTPUT);
  digitalWrite(solpin, LOW);
}

void loop(){
  // MQTT Broker 연결
  if (!mqtt_client.connected()) {
      connectToMQTT();
  }
  mqtt_client.loop();

  while (Serial1.available()) {
    String barcode_data = Serial1.readStringUntil('\n');

    int startIndex = 0;
    int endIndex = 0;
    int index = 0;

    StaticJsonDocument<256> jsonDoc;

    const char* keys[] = {"key", "buildingNumber", "floor", "lockerNumber"};

    while ((endIndex = barcode_data.indexOf(' ', startIndex)) != -1 && index < 4) {
    String word = barcode_data.substring(startIndex, endIndex);
    jsonDoc[keys[index]] = word;
    startIndex = endIndex + 1;
    index++;
  }

    // 마지막 단어 처리
    if (index < 4) {
      String lastWord = barcode_data.substring(startIndex, barcode_data.indexOf('\r', startIndex));
      jsonDoc[keys[index]] = lastWord;
    }

    char jsonString[256];
    serializeJson(jsonDoc, jsonString, sizeof(jsonString));
    Serial.println(jsonString);

    mqtt_client.publish(mqtt_topic_req, jsonString);
    mqtt_client.subscribe(mqtt_topic_res);
  }
}