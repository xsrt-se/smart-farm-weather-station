// Arduino

#include <DHT11.h>
#include <DS18B20.h>

DS18B20 ds18b20(9);
DHT11 dht11(8);

float airTemp, humiidity, contactTemp, soilMoisture;
long currentTime, wateringTime = 0;
bool watering = false;

void setup() {
  // Use for both debugging and serial communication to ESP32
  Serial.begin(115200);
  // MOSFET to control the pump
  pinMode(3, OUTPUT);
}

void loop() {
  // Scan serial for incoming data then parse JSON
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    processData(data);
  }

  // Read sensors
  airTemp = dht11.readTemperature();
  humiidity = dht11.readHumidity();
  contactTemp = ds18b20.getTempC();
  soilMoisture = (1 - (analogRead(A3) / 1024.0)) * 100.0;
 
  // Manually encode JSON
  String JSONString;
  JSONString = "{\"airTemp\":";
  JSONString += airTemp;
  JSONString += ",\"humidity\":";
  JSONString += humiidity;
  JSONString += ",\"contactTemp\":";
  JSONString += contactTemp;
  JSONString += ",\"soilMoisture\":";
  JSONString += soilMoisture;
  JSONString += "}";



  // Print JSON to serial
  Serial.println(JSONString);
}

void processData(String data) {
  // Data is plain text, either "startWatering" or "stopWatering"
  if (data.equals("startWatering")) {
    watering = true;
    digitalWrite(3, HIGH);
    Serial.println('Watering started.');
  } else if (data.equals("stopWatering")) {
    watering = false;
    digitalWrite(3, LOW);
    Serial.println('Watering stopped.');
  } else if (data.substring(0, 9) == "currTime:") {
    // If data starts with currTime: then parse the rest of the string as an int and set currentTime to that value
      currentTime = data.substring(9).toInt();
  } else {
    return;
  }
}

byte castData(float airTemp, float humidity, float contactTemp, float soilMoisture) {
  // Cast floats into bytes
  byte result;
  result = (byte) airTemp * 100;
  result = (result << 8) | (byte) humidity * 100;
  result = (result << 8) | (byte) contactTemp * 100;
  result = (result << 8) | (byte) soilMoisture * 100;
  return result;
}