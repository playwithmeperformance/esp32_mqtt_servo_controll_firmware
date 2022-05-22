/** 
 *  TaskScheduler Test
 *
 *  Initially only tasks 1 and 2 are enabled
 *  Task1 runs every 2 seconds 10 times and then stops
 *  Task2 runs every 3 seconds indefinitely
 *  Task1 enables Task3 at its first run
 *  Task3 run every 5 seconds
 *  Task1 disables Task3 on its last iteration and changed Task2 to run every 1/2 seconds
 *  At the end Task2 is the only task running every 1/2 seconds
 */
 
 
#include <EspMQTTClient.h>
#include <ESP32Servo.h>
#include <TaskScheduler.h>
#include <HCSR04.h>

#define client_name "3"
#define status_path "/mirror/3/status"
#define position_path "/mirror/3/position"
#define distance_path "/mirror/3/distance"
#define pos_path "/mirror/3/pos"
#define step_delay_path "/mirror/3/step_delay"
#define servo_pin 13
#define max_distane_cm 200
#define activate_sensor_task false

#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_TIMECRITICAL


UltraSonicDistanceSensor distanceSensor(12, 14, 200);  // Initialize sensor that uses digital pins 12 trigger and 14 echo pin.


// Callback methods prototypes
void sensorPublish();
void handleMqtt();

void changeMotorPosition();
// void t2Callback();
// void t3Callback();

//Tasks
// Task t4();
Task sensor_task(100, TASK_FOREVER, &sensorPublish);
Task mqtt_task(20, TASK_FOREVER, &handleMqtt);
Task motor_task(75, TASK_FOREVER, &changeMotorPosition);
// Task t3(5000, TASK_FOREVER, &t3Callback);

Scheduler runner;

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

uint8_t currentPosition = 90;
uint8_t newPosition = 90;

void send_position() {
  float pos = currentPosition / 180.0;
  client.publish(pos_path, String(pos)); // You can activate the retain flag by setting the third parameter to true
}

void changeMotorPosition() {
  if (currentPosition != newPosition) {
    if (newPosition > currentPosition) {
      currentPosition = currentPosition + 1;
    } else if (newPosition < currentPosition) {
      currentPosition = currentPosition - 1;
    }
    motor.write(currentPosition);
    send_position();
  } else {
    motor_task.disable();
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



void sensorPublish() {
    float distance = distanceSensor.measureDistanceCm();
    // double distance_ratio;

    // if (distance >= 0) { 
    //   distance_ratio = (1.0 - (distance * 10) / (max_distane_cm * 10));
    // } else {
    //   distance_ratio = 0;
    // }
    
    client.publish(distance_path, String(distance,0)); // You can activate the retain flag by setting the third parameter to true
}

void setup () {
  Serial.begin(115200);
  Serial.println("Scheduler TEST");

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage(status_path, "shutting_down");  // You can activate the retain flag by setting the third parameter to true
  setup_servo();
  
  runner.init();
  Serial.println("Initialized scheduler");
  
  runner.addTask(mqtt_task);

  if (activate_sensor_task) {
    runner.addTask(sensor_task);
  }
  runner.addTask(motor_task);

  Serial.println("added sensor_task");
  
//   runner.addTask(t2);
//   Serial.println("added t2");

//   delay(5000);
  
  mqtt_task.enable();
  Serial.println("Enabled sensor_task");
//   t2.enable();
//   Serial.println("Enabled t2");
}


// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // Subscribe to position_path and display received message to Serial
  client.subscribe(position_path, [](const String & payload) {
    newPosition = abs(round(payload.toFloat() * 180));
    if (newPosition != currentPosition) {
      motor_task.enable();
    }
    Serial.println(newPosition);
  });

  client.subscribe(step_delay_path, [](const String & payload) {
    int changeDelay = atoi(payload.c_str());
    motor_task.setInterval( changeDelay );
    Serial.println(changeDelay);
  });


  sensor_task.enable();
  // motor_task.enable();
//   client.subscribe(position_path, [](const String & payload) {
//     long position = atoi(payload.c_str());
//     newPosition = position;
//     Serial.println(position);
//   });


  // Publish a message to "mytopic/test"
  client.publish(status_path, "connected"); // You can activate the retain flag by setting the third parameter to true
  send_position();
}


void handleMqtt() {
  client.loop();
}

void loop () {
  runner.execute();
}