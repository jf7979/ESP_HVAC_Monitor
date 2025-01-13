// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <ESP8266WiFi.h>
#include "SSD1306Wire.h"
//#include <DNSServer.h>
#include <ESP8266WebServer.h>
//#include <WiFiManager.h>
SSD1306Wire display(0x3C, 4, 5);

#include <Wire.h>
#include <SPI.h>
//#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

const char* ssid = "wifi";                // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "pass";  // The password of the Wi-Fi network


// Data wire is plugged into port 2 on the ESP8266
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress outputThermometer = { 0x28, 0xAB, 0xE6, 0x45, 0x92, 0x0D, 0x02, 0xCF };

IPAddress ip;

Adafruit_BME280 bme;   // I2C
Adafruit_BME280 bme2;  // I2C

float outputTemp;
float outputTempOld;
float intakeTemp;
float intakeHumidity;
float intakePressure;
float innerTemp;
float innerHumidity;
float innerPressure;
float avgIntakeTemp;
float avgIntakeHumidity;
float pressureDiff;
float calibration = 0.0;
String sensor1 = "Internal: Good to go";
String sensor2 = "External: Good to go";


void setup(void) {
  //Serial.begin(115200);

  pinMode(16, OUTPUT);  //D0
  digitalWrite(16, LOW);
  pinMode(15, OUTPUT);  //D8 The UV light relay
  digitalWrite(15, LOW);
  pinMode(14, INPUT);  //D5

  display.init();
  display.setFont(ArialMT_Plain_10);

  display.clear();
  display.drawString(0, 0, "Starting...");
  display.display();

  if (!bme.begin(0x76, &Wire)) {
    sensor1 = "Internal: Not Found";
  }

  if (!bme2.begin(0x77, &Wire)) {
    sensor2 = "External: Not Found";
  }

  sensors.begin();
  sensors.setResolution(outputThermometer, TEMPERATURE_PRECISION);

  delay(1000);

  WiFi.hostname("HVAC_MONITOR");
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  delay(3000);

  display.clear();
  display.drawString(0, 0, sensor1);
  display.drawString(0, 11, sensor2);
  display.drawString(0, 21, "IP:" + WiFi.localIP().toString());
  display.display();

  delay(3000);
}





void loop() {
  sensors.requestTemperatures();
  //float Sensor1TempC = sensors.getTempC(outputThermometer);
  outputTemp = DallasTemperature::toFahrenheit(sensors.getTempC(outputThermometer));
  if (outputTemp < 1) {
    outputTemp = outputTempOld;  //Sometimes the probe doesn't register right
  }

  outputTempOld = outputTemp;

  intakeTemp = DallasTemperature::toFahrenheit(bme.readTemperature());
  intakeHumidity = bme.readHumidity();
  intakePressure = bme.readPressure();

  innerTemp = DallasTemperature::toFahrenheit(bme2.readTemperature());
  innerHumidity = bme2.readHumidity();
  innerPressure = bme2.readPressure();

  avgIntakeTemp = (intakeTemp + innerTemp) / 2;
  avgIntakeHumidity = (intakeHumidity + innerHumidity) / 2;
  pressureDiff = abs(intakePressure - innerPressure);

  display.clear();

  display.drawString(0, 0, "Output Temp    : " + String(outputTemp));
  display.drawString(0, 11, "Intake Temp    : " + String(avgIntakeTemp));
  display.drawString(0, 21, "Intake Humidity: " + String(avgIntakeHumidity));
  display.drawString(0, 31, "Pressure Diff  : " + String(pressureDiff));
  display.drawString(0, 51, "POSTING...");
  display.display();


  if (true) {
    const uint16_t port = 601;
    const char* host = "172.16.20.17";
    WiFiClient client;

    String PostData = "hvac";
    PostData += " ";
    //PostData += "outputTemp=";
    PostData += outputTemp;
    PostData += " ";

    //PostData += "intakeTemp=";
    PostData += intakeTemp;
    PostData += " ";

    //PostData += "intakeHumidity=";
    PostData += intakeHumidity;
    PostData += " ";

    //PostData += "intakePressure=";
    PostData += intakePressure;
    PostData += " ";

    //PostData += "innerTemp=";
    PostData += innerTemp;
    PostData += " ";

    //PostData += "innerHumidity=";
    PostData += innerHumidity;
    PostData += " ";

    //PostData += "innerPressure=";
    PostData += innerPressure;
    //PostData += " ";

    //PostData += " }";
    if (client.connect(host, port)) {
      client.println(PostData);
      client.stop();
    }
  }

  for (int i = 10; i > 0; i--) {
    sensors.requestTemperatures();
    //float Sensor1TempC = sensors.getTempC(outputThermometer);
    outputTemp = DallasTemperature::toFahrenheit(sensors.getTempC(outputThermometer));

    intakeTemp = DallasTemperature::toFahrenheit(bme.readTemperature());
    intakeHumidity = bme.readHumidity();
    intakePressure = bme.readPressure() - 25;  // Pressure adjustment
    innerTemp = DallasTemperature::toFahrenheit(bme2.readTemperature());
    innerHumidity = bme2.readHumidity();
    innerPressure = bme2.readPressure();

    avgIntakeTemp = (intakeTemp + innerTemp) / 2;
    avgIntakeHumidity = (intakeHumidity + innerHumidity) / 2;
    pressureDiff = abs(intakePressure - innerPressure);

    if (digitalRead(14) == HIGH) {
      calibration = pressureDiff;
    }

    pressureDiff = abs(pressureDiff - calibration);

    if (pressureDiff < 10) {
      pressureDiff = 0;
    }


    display.clear();
    //display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Intake Temp    : " + String(avgIntakeTemp));
    display.drawString(0, 11, "Output Temp    : " + String(outputTemp));
    display.drawString(0, 21, "Temp Diff:    : " + String(abs(avgIntakeTemp - outputTemp)));
    display.drawString(0, 31, "Intake Humidity: " + String(avgIntakeHumidity));
    display.drawString(0, 41, "Pressure Diff  : " + String(pressureDiff));
    //display.drawString(0, 41, "Pressure Diff  : " + String(innerPressure));
    //display.drawString(0, 11, "IP: " + ipStr);
    display.drawString(0, 51, "Next Post: " + String(i));
    display.display();




    if (pressureDiff > 45) {
      digitalWrite(16, HIGH);  // sets the digital pin 13 on
    } else {
      digitalWrite(16, LOW);  // sets the digital pin 13 off
    }

    delay(1000);
  }

  //delay(60000);
}
