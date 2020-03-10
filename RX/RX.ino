#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>

RF24 radio(9, 10); // CE, CSN
byte self[6] = "0Node";
byte child[6] = "0Node";
int address = 1;



struct Payload {
  int address;
  unsigned long data;
};

Payload res;

void setup() {
  self[0] += address;
  child[0] += address+1;
  
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(child);
  radio.openReadingPipe(1, self);   //Setting the address at which we will receive the data
  radio.startListening();              //This sets the module as receiver
  }
  void loop(){
  if (radio.available())              //Looking for the data.
  {
  radio.read(&res, sizeof(res));    //Reading the data
  if(res.address == address){
    Serial.println(res.data);
    Serial.println("**************************");
  }
  else{
    radio.stopListening();
    if(radio.write(&res,sizeof(res))){
      Serial.println("Message forwarded");
    }
    else{
      Serial.println("Transmission to child failed");
    }
    radio.startListening();
  }
  }
  delay(5);
} 
