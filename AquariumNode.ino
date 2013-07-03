//fan_control
//To control aquarium temperature

#include <JeeLib.h>
#include <Ports.h>

#ifndef ANALOG_TEMP
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 6
#endif

#define RF12_GRP_ID 1
#define RF12_SRC_ID 2

float temp_pv = 25.0; //Temperature current (process) value
float temp_sp = 25.0; //Temperature set point
float Kp = 10.0;      //Proportinal gain
float Ki = 10.0;      //Integral gain
float Ta = 1;         //Intervall in seconds

MilliTimer update;
MilliTimer rf_upd;
float esum = 0.0;
int rate = 0;
unsigned long spd = 0;

#ifndef ANALOG_TEMP
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
#endif

void setup () {
    Serial.begin(57600);
    Serial.println("\n[aquarium_node 0.1]");

    rf12_initialize(RF12_SRC_ID, RF12_868MHZ, RF12_GRP_ID);
        
    pinMode(3, OUTPUT);
    TCCR2B = TCCR2B & 0b11111000 | 0x01; //We need 25kHz (32kHz also works fine)
    analogWrite(3, 200);   //Initially value for fan pwm
    
#ifdef ANALOG_TEMP
    pinMode(7, INPUT);
#else
    sensors.begin();
#endif
}

float piControl() {
#ifdef ANALOG_TEMP
    temp_pv = analogRead(A3) * 0.03;
#else
    sensors.requestTemperatures();
    temp_pv = sensors.getTempCByIndex(0);
#endif

    float e = temp_sp - temp_pv;
  
    esum = esum + e;
    if(esum > 5)
      esum = 5;
    if(esum < -25)
      esum = -25;
         
    return Kp * e + Ki * Ta * esum;
}

void recvSettings(){
    if(rf12_recvDone() && rf12_crc == 0) {       
       if(rf12_len == 3) {
         temp_sp = ((float)rf12_data[0]) / 5.0; 
         Kp = (float)rf12_data[1];
         Ki = (float)rf12_data[2];
#ifdef DEBUG
         Serial.print("Tsp:");
         Serial.print(temp_sp);
         Serial.print(" Kp:");
         Serial.print(Kp);
         Serial.print(" Ki:");
         Serial.println(Ki);
#endif
       }
       
       if(RF12_WANTS_ACK) {
         rf12_sendStart(RF12_ACK_REPLY,0,0);
       }
    }
}

void loop () {
    recvSettings();
    
    if(update.poll(Ta * 1000)) {      
      rate = -piControl();
      
      //Limit and set rate for fan
      if(rate < 65) 
        rate = 65;
      if(rate > 255)
        rate = 255;

      analogWrite(3, rate);
      
      //Get fan speed
      spd = pulseIn(7,HIGH);
      
#ifdef DEBUG
      Serial.print("Tsp:");
      Serial.print(temp_sp);
      Serial.print(" Tpv:");
      Serial.print(temp_pv);
      Serial.print(" F:");
      Serial.print(rate);
      Serial.print(" FS:");
      Serial.println(spd);
#endif              
    }
    
    if(rf_upd.poll(5000)) {
      uint8_t mess[3] = {(uint8_t)(temp_sp*5), 
                         (uint8_t)(temp_pv*5),
                         (uint8_t)(255 - (spd/100))};
            
      rf12_sendNow(RF12_SRC_ID, mess, sizeof mess);
    }
}
