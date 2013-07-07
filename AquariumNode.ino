//AquariumNode
//To control aquarium temperature
#include <EEPROM.h>
#include <JeeLib.h>
#include <Ports.h>

#ifndef ANALOG_TEMP
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 6
#endif

#define RF12_GRP_ID 1
#define RF12_SRC_ID 2

float temp_pv = 25.0;   //Temperature current (process) value
float temp_sp = 25.0;   //Temperature set point
float Kp = 1;           //Proportinal gain
float Ki = 1;           //Integral gain
float Ta = 1;           //Intervall in seconds
float min_fan_dc = 0.3; //Minimum fan duty cycle

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

struct {
  uint8_t temp_sp; //Temperature set point
  float temp_pv; //Temperature current (process) value
  uint16_t fan_spd; //Fan speed RPM
} payload;

void setup () {
    Serial.begin(57600);
    Serial.println("\n[AquariumNode 0.4]");

    rf12_initialize(RF12_SRC_ID, RF12_868MHZ, RF12_GRP_ID);
        
    pinMode(3, OUTPUT);
    TCCR2B = TCCR2B & 0b11111000 | 0x01; //We need 25kHz (32kHz also works fine)
    analogWrite(3, 200);   //Initial value for fan pwm
    
    //Get initial controler settings
    int e_temp_sp = EEPROM.read(0x00);
    int e_Kp = EEPROM.read(0x01);
    int e_Ki = EEPROM.read(0x02);
    int e_min_fan_dc = EEPROM.read(0x03);
    int e_cs = EEPROM.read(0x04);

    if((e_temp_sp ^ e_Kp ^ e_Ki ^ e_min_fan_dc) == e_cs) {
      Serial.println("Use settings from EEPROM");
      temp_sp = e_temp_sp / 5.0;
      Kp = e_Kp / 100.0;
      Ki = e_Ki / 100.0;
      min_fan_dc = e_min_fan_dc / 100.0;
    }     
    
    payload.temp_sp = (uint8_t)(temp_sp * 5);

#ifdef ANALOG_TEMP
    pinMode(7, INPUT);
#else
    sensors.begin();
#endif
}

float piControl() {
#ifdef ANALOG_TEMP
    float temp_pv_n = analogRead(A3) * 0.03;
#else
    sensors.requestTemperatures();
    float temp_pv_n = sensors.getTempCByIndex(0);
#endif
    temp_pv = (temp_pv + temp_pv_n) / 2;

    payload.temp_pv = temp_pv;

    float e = temp_sp - temp_pv;
  
    esum = esum + e;
    if(esum > 100)
      esum = 100;
    if(esum < -100)
      esum = -100;
         
    return Kp * e + Ki * Ta * esum;
}

void recvSettings(){
    if(rf12_recvDone() && rf12_crc == 0) {
       if(rf12_len == 4) {
         temp_sp = ((float)rf12_data[0]) / 5.0;
         payload.temp_sp = rf12_data[0];
         
         Kp = (float)rf12_data[1] / 100.0;
         Ki = (float)rf12_data[2] / 100.0;
         min_fan_dc = (float)rf12_data[3] / 100.0;
         
         //Write controler settings to EEPROM         
         EEPROM.write(0x00, rf12_data[0]);         
         EEPROM.write(0x01, rf12_data[1]);         
         EEPROM.write(0x02, rf12_data[2]);         
         EEPROM.write(0x03, rf12_data[3]);         
         EEPROM.write(0x04, rf12_data[0] ^ rf12_data[1] ^ rf12_data[2] ^ rf12_data[3]);         
         
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
      rate = -piControl() * 255;

      //Limit fan rate to minimal duty cycle
      if(rate < 255 * min_fan_dc) 
        rate = 255 * min_fan_dc;
      if(rate > 255)
        rate = 255;

      analogWrite(3, rate);
      
      //Get fan speed (low pulse has better reading)
      unsigned long spd_r = pulseIn(7, LOW);
      if(spd_r > 4000) //Only use possible readings (less than 4000 is more than 3000 RPM)
        spd = (spd + spd_r) / 2;
      
      payload.fan_spd = (uint16_t)(12000000/spd);
      
#ifdef DEBUG
      Serial.print("Tsp:");
      Serial.print(temp_sp);
      Serial.print(" Tpv:");
      Serial.print(temp_pv);
      Serial.print(" F:");
      Serial.print(rate);
      Serial.print(" FS:");
      Serial.println(payload.fan_spd);
#endif              
    }
    
    if(rf_upd.poll(5000)) {   //If you send too often recv fails :-( ?  
      rf12_sendNow(RF12_SRC_ID, &payload, sizeof payload);
    }
}
