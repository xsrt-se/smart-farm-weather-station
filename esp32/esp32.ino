// ESP32

#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define SPI_SS 5

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <SPI.h>
#include <WebSocketsClient.h>

// NTP Time server
const char *ntpServer = "pool.ntp.org";

// Define a payload struct
struct SensorData {
  float airTemp;
  float humidity;
  float contactTemp;
  float soilMoisture;
};

// Create an instance of the server
AsyncWebServer server(80);

Preferences preferences;

// Create an instance of WiFiUDP for NTP
WiFiUDP ntpUDP;

// Create an instance of NTPClient
NTPClient timeClient(ntpUDP, ntpServer, 0);

// Create a WebSocket client
WebSocketsClient ws;
String payloadString;

void setup()
{

  Serial.begin(115200);
  Serial.println("Smart Farm ESP32 Client.");
  Serial2.begin(115200); // Initialize UART for ESP32 <-> Arduino communication

  preferences.begin("wifi-creds", false); // Open the preferences storage

  // Check if Wi-Fi credentials are already saved
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");

  if (savedSSID != "" && savedPassword != "")
  {
    connectToWiFi(savedSSID, savedPassword);
    setupNTP();
    connectToWebSocket();
  }
  else
  {
    setupWiFiAP(); // Start the configuration portal
  }
  Serial.println("Setup done.");
}

void loop()
{
  // Arduino is connected to ESP32 via UART, so we can send data from Arduino to WebSocket server via ESP32
  if (Serial2.available())
  {
    String data = Serial2.readStringUntil('\n');
    ws.sendTXT(data);
    Serial.println("Received data:");
    Serial.println(data);
    // Send back current time to Arduino
    String currentTime = "currTime:" + String(timeClient.getEpochTime());
    Serial2.println(currentTime);
  }
  ws.loop();
}

void connectToWiFi(const String &ssid, const String &password)
{
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void setupNTP()
{
  timeClient.begin();
  timeClient.update();
}

void setupWiFiAP()
{
  Serial.println("called");
  // Start an access point with a captive portal for configuration
  WiFi.softAP("ESP32-Setup");
  Serial.println("Open your browser to configure Wi-Fi...");

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String html = "<html><body>";
    html += "<h1>Wi-Fi Configuration</h1>";
    html += "<form method='post' action='/save'>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "Password: <input type='password' name='password'><br>";
    html += "<input type='submit' value='Save'>";
    html += "</form></body></html>";
    request->send(200, "text/html", html); });

  // Route to handle the POST request to save Wi-Fi credentials
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    String ssid = request->arg("ssid");
    String password = request->arg("password");

    // Save Wi-Fi credentials to preferences
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // Connect to the Wi-Fi network
    connectToWiFi(ssid, password);
    setupNTP();  // Initialize NTP after connecting to Wi-Fi

    request->send(200, "text/plain", "Wi-Fi credentials saved. You can now close this page."); });

  // Start the server
  server.begin();
}

void connectToWebSocket()
{
  Serial.println("Connecting to WebSocket...");

  // Connect to the WebSocket server
  ws.begin("api.websocketserver.xyz", 80, "/");
  ws.onEvent(webSocketEvent);
  ws.setReconnectInterval(1000);
  if (ws.isConnected())
  {
    Serial.println("Connected to WebSocket");
  }

}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[WSc] Disconnected!\n");
    break;
  case WStype_CONNECTED:
    Serial.printf("[WSc] Connected to url: %s\n", payload);
    break;
  case WStype_TEXT:
    payloadString = String((char *)payload);
    if (payloadString == "$SERVERPING")
    {
      pongServer();
      break;
    }
    else if (payloadString == "$IDENTIFICATION_REQUEST") {
      ws.sendTXT("$IDENTIFY ESP32");
      break;
    }
    Serial.printf("[WSc] Received text: %s\n", payloadString);
    char strBuf;

    relayBackData((char *)payload);
    break;
  case WStype_BIN:
    Serial.printf("[WSc] Received binary data of length %u\n", length);
    // Convert them to string
    String resultString = "";
    for (int i = 0; i < length; i++)
    {
      resultString += (char)payload[i];
    }
    Serial.println(resultString);
    Serial2.println(resultString);
    break;
  }
}

void pongServer()
{
  // Server will send periodic $SERVERPING, we have to reply with $CLIENTPONG or else our connection will get terminated.
  ws.sendTXT("$CLIENTPONG");
}

void relayBackData(char *data)
{
  // Relay back data from WebSocket along with current time from NTP server to Arduino
  String dataString = String(data);
  String JSONString;
  JSONString = "{\"data\":";
  JSONString += dataString;
  JSONString += ",\"time\":";
  JSONString += timeClient.getEpochTime();
  JSONString += "}";

  Serial2.println(JSONString);
}

SensorData decodeData(byte data) {
  // Decode byte into floats
  SensorData result;
  result.soilMoisture = (float) (data & 0xFF) / 100.0;
  data >>= 8;
  result.contactTemp = (float) (data & 0xFF) / 100.0;
  data >>= 8;
  result.humidity = (float) (data & 0xFF) / 100.0;
  data >>= 8;
  result.airTemp = (float) (data & 0xFF) / 100.0;
  return result;
}