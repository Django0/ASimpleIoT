/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point *********************************/
// Last updated: 2018-01-22
#define WLAN_SSID       ""
#define WLAN_PASS       ""
/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "192.168.43.182"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    ""
#define AIO_KEY         ""

/************ Global State (you don't need to change this!) ******************/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
const int ledBlue = D1;
const int ledRed = D2;
const int readLedGreenInterruptPin = D4;
uint32_t pressedCount = 0;
uint32_t localPressedCount = pressedCount;
uint32_t x = 0;

// named constant for the pin the temperature sensor is connected to
const int temperatureSensorPin = A0;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'heartbeat' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish heartbeat = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/heartbeat");
Adafruit_MQTT_Publish physicalButton = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/physicalButton");
Adafruit_MQTT_Publish temperatureReadOut = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbuttonBlue = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoffblue");
Adafruit_MQTT_Subscribe onoffbuttonRed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoffred");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();
//##############################################################################################
void buttonInterrupted() {
  pressedCount++;
}
//##############################################################################################
void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbuttonBlue);
  mqtt.subscribe(&onoffbuttonRed);


  pinMode(ledBlue, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(readLedGreenInterruptPin, INPUT);

  digitalWrite(ledBlue, HIGH);
  delay(1000);
  digitalWrite(ledBlue, LOW);
  delay(1000);
  attachInterrupt(digitalPinToInterrupt(readLedGreenInterruptPin), buttonInterrupted, HIGH);  // CHANGE is too fast!
}

//##############################################################################################
void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbuttonBlue) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbuttonBlue.lastread);
      if (0 == strcmp((const char*)onoffbuttonBlue.lastread, "1")) {
        digitalWrite(ledBlue, HIGH);
      } else {
        digitalWrite(ledBlue, LOW);
      }
    }

    if (subscription == &onoffbuttonRed) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbuttonRed.lastread);
      int dutycycle = atoi((const char *)onoffbuttonRed.lastread);
      Serial.println(dutycycle);
      analogWrite(ledRed, dutycycle);
    }
  }

  // Now we can publish heartbeat!
  Serial.print(F("\nSending heartbeat val "));
  Serial.print(x);
  Serial.print("...");
  if (! heartbeat.publish(x++)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // Now we can publish physical button press!
  if (pressedCount != localPressedCount) {
    localPressedCount = pressedCount;
    Serial.print(F("\nSending physical button state = "));
    Serial.print(localPressedCount);
    Serial.print("...");
    if (! physicalButton.publish(pressedCount)) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
  }

  int sensorVal = analogRead(temperatureSensorPin);

  // send the 10-bit sensor value out the serial port
  Serial.print("sensor Value: ");
  Serial.print(sensorVal);
  // convert the ADC reading to voltage
  float voltage = (sensorVal / 1024.0) * 3.1;
  // Send the voltage level out the Serial port
  Serial.print(", Volts: ");
  Serial.print(voltage);
  // convert the voltage to temperature in degrees C
  // the sensor changes 10 mV per degree
  // the datasheet says there's a 500 mV offset
  // ((volatge - 500mV) times 100)
  Serial.print(", degrees C: ");
  float temperature = (voltage - .5) * 100;
  Serial.println(temperature);

  // Now we can publish stuff!
  Serial.print(F("\nSending temperature: "));
  Serial.print(temperature);
  Serial.print("...");
  if (! temperatureReadOut.publish(temperature)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
    if(! mqtt.ping()) {
    mqtt.disconnect();
    }
  */
}
//##############################################################################################
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}
