#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "ACS712.h"
#include <ZMPT101B.h>

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define SENSITIVITY 500.0
#include "Irms_Calc.h"
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ThingSpeak and WiFi credentials
const char* ssid = "Vivo27";
const char* password = "1234567890";
const String apiKey = "5XKOY681U7C1Z6LK"; 
const char* server = "api.thingspeak.com";

// Pin definitions
#define DHT11PIN 25
#define DHTTYPE DHT11
#define RELAY_PIN 15
DHT dht(DHT11PIN, DHTTYPE);
ZMPT101B voltageSensor(35, 50.0);
const int mq2Pin = 32;
const float sensitivity = 0.185;
const int offsetVoltage = 2500; 
const int ldrPin = 34;
int rawValue = 0;
const int threshold = 2000;
float current = 0;
float vol = 0;

void setup() {
  Serial.begin(9600);
  voltageSensor.setSensitivity(SENSITIVITY);
  analogReadResolution(12); 
  pinMode(RELAY_PIN, OUTPUT);   
  
  // Start with relay OFF (HIGH if active-low relay)
  digitalWrite(RELAY_PIN, HIGH);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  display.clearDisplay();

  // Initialize DHT sensor
  dht.begin();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Read sensor values
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int mq2Value = analogRead(mq2Pin);
    float voltage = voltageSensor.getRmsVoltage();
    int ldrValue = analogRead(ldrPin);
    Serial.print("LDR Value: ");
    Serial.println(ldrValue);

    // Compare the LDR value with the threshold
    if (ldrValue < threshold) {
      // If the LDR value is below the threshold (dark), turn on the relay
      digitalWrite(RELAY_PIN,LOW);  // Activate relay (turn ON)
      Serial.println("Relay ON - Dark");
    } 
    else {
      // If the LDR value is above the threshold (light), turn off the relay
      digitalWrite(RELAY_PIN,HIGH);  // Deactivate relay (turn OFF)
      Serial.println("Relay OFF - Bright");
    }


    delay(1000);  // Wait 1 second before the next reading

    // Display data on OLED
    displayDataOnOLED(temperature, humidity, mq2Value, voltage, ldrValue);

    // Send data to ThingSpeak
    sendDataToThingSpeak(temperature, humidity, mq2Value, voltage, ldrValue);

    delay(10000);
  } else {
    Serial.println("WiFi Disconnected! Reconnecting...");
    WiFi.begin(ssid, password);
  }
}

void displayDataOnOLED(float temperature, float humidity, int mq2Value, float voltage, float ldrValue) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Display temperature
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  // Display humidity
  display.setCursor(0, 10);
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");

  // Display gas sensor (MQ2)
  display.setCursor(0, 20);
  display.print("MQ2: ");
  display.print(mq2Value);
  display.println(" ppm");

  // Display voltage
  display.setCursor(0, 30);
  display.print("Voltage: ");
  display.print(voltage);
  display.println(" V");

  // Display current
  display.setCursor(0, 40);
  display.print("LDR: ");
  display.print(ldrValue);
  display.println(" ");

  // Refresh display
  display.display();
}

void sendDataToThingSpeak(float temperature, float humidity, int mq2Value, float voltage, float ldrValue) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    String url = "http://api.thingspeak.com/update?api_key=" + apiKey;
    url += "&field1=" + String(temperature);
    url += "&field2=" + String(humidity);
    url += "&field3=" + String(mq2Value);
    url += "&field4=" + String(voltage);
    url += "&field5=" + String(ldrValue);
    
    Serial.println("Sending data to ThingSpeak...");
    Serial.println(url);
    
    http.begin(url);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
      Serial.println("Data sent to ThingSpeak successfully!");
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected! Unable to send data to ThingSpeak.");
  }
}

