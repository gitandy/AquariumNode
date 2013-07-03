//fan_control
//To control aquarium temperature

#include <JeeLib.h>
#include <Ports.h>

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

void setup () {
    Serial.begin(57600);
    Serial.println("\n[AquariumNode 0.0]");

    rf12_initialize(RF12_SRC_ID, RF12_868MHZ, RF12_GRP_ID);
        
    pinMode(3, OUTPUT);
    TCCR2B = TCCR2B & 0b11111000 | 0x01; //We need 25kHz (32kHz also works fine)
    analogWrite(3, 200);   //Initially value for fan pwm

    //analogReadResolution(12);
    
    pinMode(7, INPUT);
}

float piControl() {
    temp_pv = analogRead(A3) * 0.03;
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
      if(rate < 75) 
        rate = 75;
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
                         (uint8_t)(spd/100)};
      
      rf12_recvDone();
      while(!rf12_canSend());       
      rf12_sendStart(RF12_SRC_ID, mess, sizeof mess);
    }
}
