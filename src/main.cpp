/*
  SimpleMQTTClient.ino
  The purpose of this exemple is to illustrate a simple handling of MQTT and Wifi connection.
  Once it connects successfully to a Wifi network and a MQTT broker, it subscribe to a topic and send a message to it.
  It will also send a message delayed 5 seconds later.
*/

#include "EspMQTTClient.h"
#include <ESP32Servo.h>

#define client_name "10"
#define status_path "/mirror/10/status"
#define position_path "/mirror/10/position"
#define pos_path "/mirror/10/pos"
#define step_delay_path "/mirror/10/step_delay"
#define servo_pin 13

Servo motor;

EspMQTTClient client(
  "moloch",
  "supersecurewifipassword",
  "192.168.1.1",  // MQTT Broker server ip
  "",   // Can be omitted if not needed
  "",   // Can be omitted if not needed
  client_name,     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

unsigned long lastPositionChange;
int currentPosition = 90;
int newPosition = 90;
unsigned int changeDelay = 10;

void send_position() {
  client.publish(pos_path, String(currentPosition)); // You can activate the retain flag by setting the third parameter to true
}

void change_position() {
  unsigned long now = millis();
  if ((now - lastPositionChange > changeDelay) && (currentPosition != newPosition)) {
    lastPositionChange = now;
    if (newPosition > currentPosition) {
      currentPosition = currentPosition + 1;
    } else if (newPosition < currentPosition) {
      currentPosition = currentPosition - 1;
    }
    motor.write(currentPosition);
    send_position();
  }
}

void setup_servo() {
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	// ESP32PWM::allocateTimer(3);
  motor.setPeriodHertz(50);// Standard 50hz servo
  motor.attach(servo_pin, 750, 2000);
}

void setup()
{
  Serial.begin(115200);

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage(status_path, "shutting_down");  // You can activate the retain flag by setting the third parameter to true
  setup_servo();
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // Subscribe to position_path and display received message to Serial
  client.subscribe(position_path, [](const String & payload) {
    newPosition = atoi(payload.c_str());
    Serial.println(newPosition);
  });

  client.subscribe(step_delay_path, [](const String & payload) {
    changeDelay = atoi(payload.c_str());
    Serial.println(changeDelay);
  });


//   client.subscribe(position_path, [](const String & payload) {
//     long position = atoi(payload.c_str());
//     newPosition = position;
//     Serial.println(position);
//   });


  // Publish a message to "mytopic/test"
  client.publish(status_path, "connected"); // You can activate the retain flag by setting the third parameter to true
  send_position();
}

void loop()
{
  client.loop();
  change_position();
}
