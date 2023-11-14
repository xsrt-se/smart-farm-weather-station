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
  pinMode(2, INPUT);
}

void loop() {
  // If 2 is HIGH, then start watering
  if (digitalRead(2) == HIGH) {
    digitalWrite(3, HIGH);
    watering = true;
    wateringTime = millis();
  } else if (digitalRead(2) == LOW) {
    digitalWrite(3, LOW);
    watering = false;
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

void processData(char data) {
  Serial.println(data);
  if (data == 'A') {
    // Turn on the pump
    digitalWrite(3, HIGH);
    watering = true;
    wateringTime = millis();
  } else if (data == 'A') {
    // Turn off the pump
    digitalWrite(3, LOW);
    watering = false;
  
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
