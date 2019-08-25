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
int air[]   = { 653, 668, 664, 665, 585, 585 };
int water[] = { 344, 355, 355, 364, 317, 317 };
int moist_water_trigger[] = { 40, 40, 40, 40, 40, 40 };
int moist_water_stop[] = { 65, 65, 65, 65, 65, 65 };
int watering_time[] = { 2, 2, 7, 7, 7, 7 };
//int pump_pins[] = { 4, 5, 6, 7 };

bool grass_triggered[] = {false, false, false, false, false, false};
int latest_moisture_perc[] = { 0,0,0,0,0,0 };
bool sensors_reading_disabled=false; //while the pump is on the interference falses the values

// this is a workaround for some sensors
//int ground_pin = 8;

// shift register
int clockPin = 4 ; //blue
int dataPin  = 7; //yellow
int latchPin  = 8; //violet

int relay_on = LOW;
int relay_off = HIGH;
int relaysRegister[] = { relay_off, relay_off, relay_off, relay_off, relay_off, relay_off, relay_off, relay_off };



void updateShiftRegister() {

  // I am not using this because it works with byte while I prefer an array here
  /*byte test = 0;
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, test); 
  digitalWrite(latchPin, HIGH);*/

  //digitalWrite(clockPin,LOW);
  digitalWrite(latchPin,LOW);

  for (int i = 0; i < 8; i++) {
    digitalWrite(clockPin,LOW);
    digitalWrite(dataPin,relaysRegister[i]);
    digitalWrite(clockPin,HIGH);
  }

  digitalWrite(latchPin,HIGH);
  
}

bool stop_pump(void *pump_index) {
  //Serial.print("closing=");Serial.println((int)pump_index);
  //digitalWrite(pump_pins[(int)pump_index], relay_off); 
  relaysRegister[(int)pump_index] = relay_off;
  updateShiftRegister();
  sensors_reading_disabled=false;
  return true;   
}

bool start_pump(void *pump_index) {
  sensors_reading_disabled=true;
  relaysRegister[(int)pump_index] = relay_on;
  updateShiftRegister();
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
      start_pump(i);
      timer.in(watering_time[i]*1000, stop_pump , (void *)i);
    }
    else if ( ( latest_moisture_perc[i] <= moist_water_stop[i] ) && grass_triggered[i] ) {  
      start_pump(i);
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

/*bool sensors_reset(void *) {

  //reset
  digitalWrite(ground_pin, HIGH);
  delay(500);
  digitalWrite(ground_pin, LOW);
  //delay(10000);

  timer.in(1000, read_values);

  return true;
  
}*/

void pin_setup() {
   
  //pinMode(ground_pin, OUTPUT);
  pinMode(clockPin, OUTPUT); 
  pinMode(dataPin, OUTPUT); 
  pinMode(latchPin, OUTPUT); 

}

void setup() {

  Serial.begin(115200);
  //mySerial.begin(115200);

  pin_setup();
  //sensors_reset(NULL); 

  for (int i = 0; i < 8; i++ ) { 
    relaysRegister[i] = relay_on;
    updateShiftRegister();
    delay(500);
    relaysRegister[i] = relay_off;
    updateShiftRegister();
    
  }


 //timer.every(30000, sensors_reset);
  timer.every(30000, read_values);
  timer.every(35000, water_manager);

  
}

void loop() {

  timer.tick();

  
}
