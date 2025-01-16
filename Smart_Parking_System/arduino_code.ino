#include <Servo.h>
#include <WiFiS3.h> // For Wi-Fi and HTTP on Arduino Uno R4 WiFi

// Wi-Fi and Server settings
const char *ssid = "Your WiFi SSID";
const char *password = "Password";
const char *serverIP = "IP-Address"; // Replace with Raspberry Pi's IP
const int serverPort = 5000;

// ThingSpeak settings
const char *thingSpeakServer = "api.thingspeak.com";
const String channelID = "2724338";
const String writeAPIKey = "69TXSYPBOSY21YIO";

// Pins for sensors
const int slot1SensorPin = 4;
const int slot2SensorPin = 5;
const int slot3SensorPin = 6;
const int entrySensorPin = 2;
const int exitSensorPin = 3;

// Servo pin
const int servoPin = 9;

// Initialize servo and Wi-Fi client
Servo barrierServo;
WiFiClient client;

// Timing variables
unsigned long lastTime = 0;
unsigned long timerDelay = 15000;  // 15 seconds
unsigned long barrierCloseTime = 0;
unsigned long lastThingSpeakTime = 0;
unsigned long thingSpeakInterval = 20000; // 20 seconds

// Sensor states
int slot1Availability = 0;
int slot2Availability = 0;
int slot3Availability = 0;
int entryStatus = 0;
int exitStatus = 0;

// Barrier state
int barrierState = 0;

void smoothServoMove(int startAngle, int endAngle, int stepDelay) {
  if (startAngle < endAngle) {
    for (int angle = startAngle; angle <= endAngle; angle++) {
      barrierServo.write(angle);
      delay(stepDelay);
    }
  } else {
    for (int angle = startAngle; angle >= endAngle; angle--) {
      barrierServo.write(angle);
      delay(stepDelay);
    }
  }
}

void sendToThingSpeak() {
  if (client.connect(thingSpeakServer, 80)) {
    String postData = "field1=" + String(slot1Availability) +
                      "&field2=" + String(slot2Availability) +
                      "&field3=" + String(slot3Availability) +
                      "&api_key=" + writeAPIKey;

    client.println("POST /update HTTP/1.1");
    client.println("Host: " + String(thingSpeakServer));
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(postData.length()));
    client.println();
    client.println(postData);

    Serial.println("Data sent to ThingSpeak: " + postData);
    client.stop();
  } else {
    Serial.println("Error: Unable to connect to ThingSpeak");
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi");

  // Setup pins
  pinMode(slot1SensorPin, INPUT);
  pinMode(slot2SensorPin, INPUT);
  pinMode(slot3SensorPin, INPUT);
  pinMode(entrySensorPin, INPUT);
  pinMode(exitSensorPin, INPUT);

  // Attach the servo
  barrierServo.attach(servoPin);
  barrierServo.write(0); // Barrier closed
}

void loop() {
  // Read sensor states
  slot1Availability = (digitalRead(slot1SensorPin) == LOW) ? 1 : 0;
  slot2Availability = (digitalRead(slot2SensorPin) == LOW) ? 1 : 0;
  slot3Availability = (digitalRead(slot3SensorPin) == LOW) ? 1 : 0;
  entryStatus = (digitalRead(entrySensorPin) == LOW) ? 1 : 0;
  exitStatus = (digitalRead(exitSensorPin) == LOW) ? 1 : 0;

  // Send data to Raspberry Pi every 15 seconds
  if (millis() - lastTime > timerDelay) {
    lastTime = millis();

    // Prepare data payload
    String payload = "{\"slot1\":" + String(slot1Availability) +
                     ",\"slot2\":" + String(slot2Availability) +
                     ",\"slot3\":" + String(slot3Availability) + "}";

    // Create HTTP request
    if (client.connect(serverIP, serverPort)) {
      client.println("POST /data HTTP/1.1");
      client.println("Host: " + String(serverIP));
      client.println("Content-Type: application/json");
      client.println("Content-Length: " + String(payload.length()));
      client.println();
      client.println(payload);
      Serial.println("Data sent to Raspberry Pi: " + payload);
    } else {
      Serial.println("Error: Unable to connect to server");
    }
    client.stop();
  }

  // Send data to ThingSpeak every 20 seconds
  if (millis() - lastThingSpeakTime > thingSpeakInterval) {
    lastThingSpeakTime = millis();
    sendToThingSpeak();
  }

  // Handle barrier operation
  if (entryStatus == 1 || exitStatus == 1) {
    if (barrierState == 0) {
      smoothServoMove(0, 90, 10); // Open the barrier
      barrierCloseTime = millis() + 4000;
      barrierState = 2;
    }
  }

  if (barrierState == 2 && millis() > barrierCloseTime) {
    smoothServoMove(90, 0, 10); // Close the barrier
    barrierState = 0;
  }

  delay(500);
}
