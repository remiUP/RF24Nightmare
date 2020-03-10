#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(9, 10); // CE, CSN         

unsigned long data;
byte self[6] = "0Node";
byte child[6] = "0Node";
int address = 0;

int target = 1;


struct Payload {
  int address;
  unsigned long data;
};

Payload res;

void setup() {
  self[0] += address;
  child[0] += address+1;
  
  Serial.begin(9600);
  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(child); //Setting the address where we will send the data
  radio.openReadingPipe(1,self);
  radio.stopListening();          //This sets the module as transmitter
}
void loop()
{
  if(Serial.available()){
  data = Serial.parseInt();
  Serial.read();
  Payload P = { target, data };
  unsigned long start= millis();
  if(radio.write(&P, sizeof(P))){                 //Sending the message to receiver
  Serial.println("Message sent");
  //radio.startListening();
  //while(!radio.available()){}
  //radio.read(&res,sizeof(res));
  //unsigned long finish = millis();
  //Serial.println(finish-start);
  //radio.stopListening();
  //Serial.println(P.data);
  //Serial.println(P.adress);
  //Serial.println("**************************");
  
  }
  else{
    Serial.println("Transmission failed");
  }
  }
delay(5);
}
