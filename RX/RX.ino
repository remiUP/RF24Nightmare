#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CRqst 0
#define AAdrs 1
#define StpEN 2
#define SrtEN 3
#define Rst 4

RF24 radio(9, 10); // CE, CSN
byte self[6] = "0Node";
byte child[6] = "0Node";
byte gate[6] = "0Node";
byte voidAddress[6] = "00000";
int address = 0;


int state = 0;
int prevState = 0;
unsigned long timer = 0;
unsigned long ackTimeout = 10000;
unsigned long sendNewCRqstDelay = 100;

uint8_t sender = 0; 

struct Payload {
  unsigned int address;
  unsigned int instruction;
  unsigned long data;
};

Payload res;


void setup() {
  Serial.begin(9600);
  radio.begin();
  while(!Serial.available()){};
  }
  

void loop(){
  prevState = state;
  switch (state) {
    case 0:
      Serial.println("State 0");
      Serial.println("State 0 - Initializing");
      radio.openWritingPipe(gate);
      radio.stopListening();
      state = 1;
      break;
      
    case 1:
      Serial.println("State 1");
      if(millis()-timer > sendNewCRqstDelay){
        timer = millis();
        Payload P = {0,0,0};
        Serial.println("Sending CRqst");
        if(radio.write(&P,sizeof(P))){
          Serial.println("Ack received");
          radio.openReadingPipe(1,gate);
          radio.startListening();
          timer = millis();
          state = 2;
          Serial.println("Waiting for AAdrs");
        }
        else{
          Serial.println("Ack not received");
        }
      }
      break;

    case 2:
      Serial.println("State 2");
      if (radio.available()){
        Serial.println("Received payload");
        if (readIncoming()){
          Serial.println("Payload is addressed to this node");
          if(res.address == 0 && res.instruction == AAdrs){
            Serial.println("Payload is a AAdrs");
            state = 3;
            updateAddresses();
          }
        }
      }
      else if (millis()-timer > ackTimeout){
        Serial.println("CRqst timed out");
        state = 0;
      }
      break;

    case 3:
      Serial.println("State 3");
      Serial.println("Setting up");
      radio.openReadingPipe(1,self);
      radio.openReadingPipe(2,gate);
      radio.startListening();
      state = 4;
      break;
    
    case 4:
      if (radio.available(&sender)){
        Serial.println("Received Payload");
        if (readIncoming()){
          Serial.println("Payload is addressed to this node");
          if (sender == 1 && res.instruction == StpEN){
            Serial.println("Instructed to stop expanding the network by self pipe");
            radio.openReadingPipe(2,voidAddress);
            state = 6;
          }
          else if (sender == 2 && res.instruction == CRqst){
            Serial.println("Payload is a CRqst from gate pipe");
            radio.openWritingPipe(gate);
            radio.stopListening();
            state = 5;
          }
          else if (sender == 1 && res.instruction == Rst){
            Serial.println("Instructed to reset by self pipe");
            delay(res.data);
            state = 0;
          }
        }
      }
      Serial.println("breaking case 4");
      break;


      
    case 6:
      if (radio.available()){
        Serial.println("Received Payload");
        if (readIncoming()){
          Serial.println("Payload is addressed to this node");
          if (res.instruction == SrtEN){
            Serial.println("Instructed to start expanding network by self pipe");
            state = 3;
          }
          if (res.instruction == Rst){
            Serial.print("Instructed to reset by self pipe with delay : ");
            Serial.println(res.data);
            delay(res.data);
            state = 0;
          }
        }
      }
      Serial.println("breaking case 6");
      break;

    case 5:
      Serial.println("State 5");
      Payload p = {0,AAdrs,address+1};
      
      if (radio.write(&p,sizeof(p))){
        Serial.println("Address assignment successful");
        radio.openWritingPipe(child);
        radio.startListening();
        state = 6;
      }
      else{
        Serial.println("Address assignment failed");
        state = 3;
      }
      Serial.println("breaking case 5");
      break;
    
    default:
      Serial.println("Default state");
      break; 
  }
  if (prevState != state){
    Serial.println("------");
    Serial.println(state);
    Serial.println("------");
  }
  delay(5);
}



void updateAddresses(){
  address = int(res.data);
  self[0] = address;
  child[0] = address+1;
}

bool readIncoming(){
  Serial.println("Reading Incoming");
  radio.read(&res, sizeof(res));    //Reading the data
  if(res.address == address or res.address == 0){
    Serial.println(res.instruction);
    Serial.println(res.data);
    Serial.println("**************************");
    return true;
  }
  else if(state == 6){
    radio.stopListening();
    if(radio.write(&res,sizeof(res))){
      Serial.println("Message forwarded");
    }
    else{
      Serial.println("Transmission to child failed");
    }
    radio.startListening();
    return false;
  }
}
