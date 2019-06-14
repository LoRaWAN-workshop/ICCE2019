#include "UTenTrackIoT.h"

//Set data rate
u1_t DR = DR_SF10;

//Set Tx power
s1_t PW = 14;

UTenTrackIoT A;

int LDR= 0;
int TEMP = 0;
int HUMID = 0;

int DATA = 0;

void setup() {
  
  Serial.begin(9600);
  
}

void loop() {
  
  LDR = A.LDR(A0);
  //TEMP = A.DHT11Temp(6);
  //HUMID = A.DHT11Humid(6);

  delay(2500);

  DATA = LDR;
  
  A.LoRaSend(DATA, DR, PW);

  delay(2000);

}
