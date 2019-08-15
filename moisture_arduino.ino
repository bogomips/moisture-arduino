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

Timer<25> timer;

              //A0  A1   A2   A3   A4   A5 
static const uint8_t analog_pins[] = {A0, A1, A2, A3, A4, A5};
int air[]   = { 663, 670, 775, 777, 776, 703 };
int water[] = { 356, 359, 466, 477, 466, 426 };
int moist_water_trigger[] = { 40, 40, 40, 40, 40, 40 };
int moist_water_stop[] = { 65, 65, 65, 65, 65, 65 };
int watering_time[] = { 2, 2, 7, 7, 7, 7 };
int pump_pins[] = { 4, 5, 6, 7 };

bool grass_triggered[] = {false, false, false, false, false, false};
int latest_moisture_perc[] = { 0,0,0,0,0,0 };
bool sensors_reading_disabled=false; //while the pump is on the interference falses the values

// this is a workaround for some sensors
int ground_pin = 8;

bool stop_pump(void *pump_index) {
  //Serial.print("closing=");Serial.println((int)pump_index);
  digitalWrite(pump_pins[(int)pump_index], LOW); 
  sensors_reading_disabled=false;
  return true;   
}

bool water_manager(void *) {
  
  for (int i = 0; i < 6; i++) { 
    // If I add some serial.print here it basically breaks the code and I have no clue why
    //Serial.print("grass=");Serial.println(i);  
    //Serial.print("latest_moisture_perc=");Serial.println(latest_moisture_perc[i]);
    //Serial.print("moist_water_trigger=");Serial.println(moist_water_trigger[i]);
    //Serial.print("grass_triggered=");Serial.println(grass_triggered[i]);
   
    if ( (latest_moisture_perc[i] < moist_water_trigger[i]) && !grass_triggered[i])  { 
      grass_triggered[i]=true;
      sensors_reading_disabled=true;
      digitalWrite(pump_pins[i], HIGH);
      timer.in(watering_time[i]*1000, stop_pump , (void *)i);
    }
    else if ( ( latest_moisture_perc[i] <= moist_water_stop[i] ) && grass_triggered[i] ) {  
      sensors_reading_disabled=true;
      digitalWrite(pump_pins[i], HIGH);
      timer.in(watering_time[i]*1000, stop_pump, (void *)i);
    }
    else if ( ( latest_moisture_perc[i] >= moist_water_stop[i] ) && grass_triggered[i] ) {  
      grass_triggered[i]=false;
    }

  }

  return true;
}

bool read_values(void *) {

  //char jsonOutput[500];
  if (sensors_reading_disabled)
    return true;
    
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
  
}

void loop() {

  timer.tick();
  
}
