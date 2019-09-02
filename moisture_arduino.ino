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
static const uint8_t analog_pins[] = { A0, A1, A2, A3, A4, A5 };
int active_grasses = 6; //sizeof(analog_pins) / sizeof(uint8_t);
//int air[]   = { 653, 668, 664, 665, 585, 585 };
//int water[] = { 344, 355, 355, 364, 317, 317 }; here i've used the last as the first
int air[]   = { 590, 668, 664, 665, 585, 585 };
int water[] = { 317, 355, 355, 364, 317, 317 };
int moist_water_trigger[] = { 40, 40, 40, 40, 40, 40, 40, 40 };
int moist_water_stop[] = { 65, 65, 65, 65, 65, 65, 65, 65 };
int watering_time[] = { 6, 6, 5, 5, 5, 5, 5, 5 };
int waiting_sensors_time[] = { 3, 3, 3, 3, 3, 3, 3, 3 }; //skipping cycle
int max_cycles[] = { 20, 20, 20, 20, 20, 20, 20, 20 }; //skipping cycle
int cycles_break[] = { 20000, 20000, 20000, 20000, 20000, 20000, 20000, 20000 }; //skipping cycle

int grass_triggered[] = { 0, 0, 0, 0, 0, 0, 0, 0};
int sensors_waiting[] = { 0, 0, 0, 0, 0, 0, 0, 0};
//int grass_trigger_limit = 1; //test value, fix it
int latest_moisture_perc[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
bool sensors_reading_disabled=false; //while the pump is on the interference falses the values

double water_manager_timer = 35000;
int read_sensors_timer  = 30000;

//this is a workaround for some sensors
//int ground_pin = 8;

// shift register
int clockPin = 4 ; //blue
int dataPin  = 7; //yellow
int latchPin = 8; //violet

//The relay modules works with an inverted logic
int relay_on = LOW;
int relay_off = HIGH;
int relaysRegister[] = { relay_off, relay_off, relay_off, relay_off, relay_off, relay_off, relay_off, relay_off };

void updateShiftRegister() {

  // I am not using this because it works with byte while I prefer an array here
  /*byte test = 0;
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, test); 
  digitalWrite(latchPin, HIGH);*/

  digitalWrite(latchPin,LOW);

  for (int i = 0; i < 8; i++) {
    digitalWrite(clockPin,LOW);
    digitalWrite(dataPin,relaysRegister[i]);
    digitalWrite(clockPin,HIGH);
  }

  digitalWrite(latchPin,HIGH);
  
}

bool stop_pump(void *pump_index) {
 
  relaysRegister[(int)pump_index] = relay_off;
  updateShiftRegister();
  sensors_reading_disabled=false;
  return true;   
}

bool start_pump(void *pump_index) {

  sensors_reading_disabled=true;
  relaysRegister[(int)pump_index] = relay_on;
  updateShiftRegister();

  return true;

}

bool reset_grass_triggered(void *pump_index) {

  grass_triggered[(int)pump_index]=0;
  return true;
}

bool water_manager(void *) {
  
  for (int i = 0; i < active_grasses; i++) { 
    // If I add some serial.print here it basically breaks the code and I have no clue why
   
    if ( (latest_moisture_perc[i] < moist_water_trigger[i]) && (grass_triggered[i] == 0) ) { 

      // Water one grass at the time
      //if (sensors_reading_disabled)
      //  break;

      grass_triggered[i]=1;
      start_pump(i);
      timer.in(watering_time[i]*1000, stop_pump , (void *)i);

    }
    else if ( ( latest_moisture_perc[i] <= moist_water_stop[i] ) && grass_triggered[i] > 0 ) {  

      grass_triggered[i]++;

      //protection
      if (grass_triggered >= max_cycles[i]) {

        //grass_triggered[i]=0;
        //break;
        timer.in(cycles_break[i]*1000, reset_grass_triggered , (void *)i);
      }

      /***
       * Some sensors are very slow, I want to give the enough time by skipping waiting_sensors_time[i] loops.
       * Total time is waiting_sensors_time[i]* water_manager_time
      */

      else if ((sensors_waiting[i] >= waiting_sensors_time[i]) && (grass_triggered < max_cycles[i])) {

        sensors_waiting[i]=0;
        start_pump(i);
        timer.in(watering_time[i]*1000, stop_pump, (void *)i);

      }
      else {
        sensors_waiting[i]++;
      }

    }
    else if ( ( latest_moisture_perc[i] >= moist_water_stop[i] ) && grass_triggered[i] > 0 ) {  
      grass_triggered[i]=0;
    }

  }

  return true;
}

bool read_sensors(void *) {

  //char jsonOutput[500];
  if (sensors_reading_disabled)
    return true;
    
  for (int i = 0; i < active_grasses; i++) { 

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

  timer.in(1000, read_sensors);

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
    delay(300);
    relaysRegister[i] = relay_off;
    updateShiftRegister();
    
  }

 //timer.every(30000, sensors_reset);
  timer.every(read_sensors_timer, read_sensors);
  timer.every(water_manager_timer, water_manager);

}

void loop() {

  timer.tick();
}
