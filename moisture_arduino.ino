//#include <SoftwareSerial.h>
#include <timer.h>
#include <ArduinoJson.h>
/*
#include <math.h>
#include <avr/wdt.h>*/

//SoftwareSerial mySerial(0, 1); // RX, TX on Arduino
#define SERIAL_BUFFER_SIZE 500


StaticJsonDocument<500> http_response;
JsonArray sensors = http_response.createNestedArray("sensors");
JsonObject sensor_0 = sensors.createNestedObject();
JsonObject sensor_1 = sensors.createNestedObject();
JsonObject sensor_2 = sensors.createNestedObject();
JsonObject sensor_3 = sensors.createNestedObject();
JsonObject sensor_4 = sensors.createNestedObject();
JsonObject sensor_5 = sensors.createNestedObject();

Timer<15> timer;

static const uint8_t analog_pins[] = {A0, A1, A2, A3, A4, A5};

              //A0  A1   A2   A3   A4   A5 
int air[]   = { 767, 705, 775, 777, 776, 703 };
int water[] = { 460, 439, 466, 477, 466, 426 };
int min_moisture[] = { 40, 40, 40, 40, 40, 40 };
int pump_pins[] = { 4, 5, 6, 7};

int latest_moisture_perc[] = {0,0,0,0,0,0};

// this is a workaround for some sensors
int ground_pin = 8;

bool reset_pump(void *pump_index) {
  digitalWrite(pump_pins[(int)pump_index], LOW); 
  return true;   
}

bool water_manager(void *) {

  for (int i = 0; i < 6; i++) { 

    if (latest_moisture_perc[i] < min_moisture[i])  {
        digitalWrite(pump_pins[i], HIGH);
        timer.in(7000, reset_pump, (void *)i);
      }

  }
  
}

bool read_values(void *) {

  //char jsonOutput[500];

  for (int i = 0; i < 6; i++) { 

    int moisture_raw = analogRead(analog_pins[i]);
    int moisture_percentage = map(moisture_raw,air[i],water[i],0,100);
    if (moisture_percentage > 100) { moisture_percentage=100; } else if (moisture_percentage < 0) { moisture_percentage=0; }

    latest_moisture_perc[i] = moisture_percentage;
    
    sensors[i]["mst_raw"] = moisture_raw;
    sensors[i]["mst_perc"] = moisture_percentage;
    sensors[i]["ref_air"] = air[i];
    sensors[i]["ref_water"] = water[i];

    //Serial.print(i);Serial.print(" raw = "); Serial.print(moisture_raw); Serial.print(" percentage = ");Serial.println(moisture_percentage);
    delay(30);
  }
  
  Serial.print('#');
  serializeJson(http_response, Serial);
  Serial.print('$');

  return true;
}

bool sensors_reset(void *) {

  //reset
  digitalWrite(ground_pin, HIGH);
  delay(500);
  digitalWrite(ground_pin, LOW);
  //delay(10000);

  timer.in(1000, read_values);

  return true;
  
}


void read_serial() {

  /*if (mySerial.available()) {
    String msg = mySerial.readString();
    Serial.print("Data received: ");
    Serial.println(msg);
  } */
  
}

void pin_setup() {
   
  pinMode(ground_pin, OUTPUT); 

  for (int i = 0; i < 4; i++) { 
    pinMode(pump_pins[i], OUTPUT); 
    digitalWrite(pump_pins[i], HIGH);
    delay(500);
    digitalWrite(pump_pins[i], LOW);
    delay(500);
    
  }
 

}

void setup() {

  Serial.begin(115200);
  //mySerial.begin(115200);

  pin_setup();
  sensors_reset(NULL); 

  timer.every(30000, sensors_reset);
  timer.every(35000, water_manager);

  //water_manager(,i);
  
  //Serial.println("Setup completed");
  
}

void loop() {

    timer.tick();
  //read_serial();
  
}
