#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN    4
#define LED_COUNT 2

#define CRqst 0
#define AAdrs 1
#define StpEN 2
#define SrtEN 3
#define Rst 4
#define SLLT 5
#define ICLR 6

RF24 radio(9, 10); // CE, CSN
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


int ledState = 0;
int prevLedState = 0;
unsigned long start = 0;
unsigned long duration = 10000;

unsigned long waitPairingBlinkPeriod = 1000;
bool ledStatus = false;

byte self[6] = "0Node";
byte child[6] = "0Node";
byte gate[6] = "0Node";
byte voidAddress[6] = "00000";
int address = 0;


int comState = 0;
int prevcomState = 0;

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
Payload P;


void setup() {
  Serial.begin(9600);
  radio.begin();
  strip.begin();
  while(!Serial.available()){};
  strip.show();
  strip.setBrightness(255);
  }
  

void loop(){
  prevcomState = comState;
  prevLedState = ledState;
  comStateMachine();
  ledStateMachine();
  delay(5);
  if(prevLedState != ledState){
    Serial.println("------------------");
    Serial.println(ledState);
    Serial.println("------------------");
  }
  /*if (prevcomState != comState){
    Serial.println("------");
    Serial.println(comState);
    Serial.println("------");
  }
  */
}

void ledStateMachine(){
  switch(ledState){
    case 0:
      if(ledStatus){
        strip.fill(strip.Color(0,0,0));
        strip.show();
        ledStatus = false;
      }
      break;

    case 1:
      start = millis();
      ledStatus = false;
      ledState = 2;
      break;

    case 2:
      if(millis()-start > waitPairingBlinkPeriod){
        start = millis();
        ledStatus = !ledStatus;
        if(ledStatus){
          strip.fill(strip.Color(255,0,0));
        }
        else
        {
          strip.fill(strip.Color(0,0,0));
        }
        strip.show();
      }
      break;
    
    case 3:
      start = millis();
      ledStatus = false;
      ledState = 4;
      break;

    case 4:
      if(millis()-start > waitPairingBlinkPeriod){
        start = millis();
        ledStatus = !ledStatus;
        if(ledStatus){
          strip.fill(strip.Color(0,0,255));
        }
        else
        {
          strip.fill(strip.Color(0,0,0));
        }
        strip.show();
      }
      break;
    
    case 5:
      if(ledStatus){
        strip.fill(strip.Color(0,0,0));
        strip.show();
        ledStatus = false;
      }
      break;

    case 6:
      start = millis();
      ledState = 7;
      ledStatus = true;
      break;

    case 7:
      unsigned long now = millis();
      if(now-start>duration){
        strip.fill(strip.Color(0,0,0));
        strip.show();
        ledState = 5;
      }
      else{
        double value = ((now-start)/((float)duration))*(float)LED_COUNT;
        int numOnLed = floor(value);
        for(int i=0; i<numOnLed;i++){
          strip.setPixelColor(i,255,255,255);
        }
      int color = floor((value-floor(value))*255);
      color = pow(2,color/(float)31)-1;
      strip.setPixelColor(numOnLed,color,color,color);
      strip.show();
        
      }
  }
}

void comStateMachine(){
  switch (comState) {
    case 0:
      radio.openWritingPipe(gate);
      radio.stopListening();
      comState = 1;
      delay(100);
      ledState = 2;
      break;
      
    case 1:
      if(millis()-timer > sendNewCRqstDelay){
        timer = millis();
        P.address = 0;
        P.instruction = CRqst;
        Serial.println("Sending CRqst");
        if(radio.write(&P,sizeof(P))){
          Serial.println("Ack received");
          radio.openReadingPipe(1,gate);
          radio.startListening();
          timer = millis();
          comState = 2;
          Serial.println("Waiting for AAdrs");
        }
        else{
          Serial.println("Ack not received");
        }
      }
      break;

    case 2:
      if (radio.available()){
        Serial.println("Received payload");
        if (readIncoming()){
          Serial.println("Payload is addressed to this node");
          if(res.address == 0 && res.instruction == AAdrs){
            Serial.println("Payload is a AAdrs");
            comState = 3;
            updateAddresses();
          }
        }
      }
      else if (millis()-timer > ackTimeout){
        Serial.println("CRqst timed out");
        comState = 0;
      }
      break;

    case 3:
      radio.openReadingPipe(1,self);
      radio.openReadingPipe(2,gate);
      radio.startListening();
      comState = 4;
      ledState = 4;
      break;
    
    case 4:
      if (radio.available(&sender)){
        Serial.println("Received Payload");
        if (readIncoming()){
          Serial.println("Payload is addressed to this node");
          if (sender == 1 && res.instruction == StpEN){
            Serial.println("Instructed to stop expanding the network by self pipe");
            radio.openReadingPipe(2,voidAddress);
            ledState = 5;
            comState = 6;
          }
          else if (sender == 2 && res.instruction == CRqst /*&& false*/ ){
            Serial.println("Payload is a CRqst from gate pipe");
            radio.openWritingPipe(gate);
            radio.stopListening();
            comState = 5;
          }
          else if (sender == 1 && res.instruction == Rst){
            Serial.println("Instructed to reset by self pipe");
            delay(res.data);
            comState = 0;
          }
        }
      }
      break;

    case 5:
      P.address = 0;
      P.instruction = AAdrs;
      P.data = address+1;
      if (radio.write(&P,sizeof(P))){
        Serial.println("Address assignment successful");
        radio.openWritingPipe(child);
        radio.startListening();
        comState = 6;
        ledState = 5;
      }
      else{
        Serial.println("Address assignment failed");
        comState = 3;
      }
      break;
      
      
    case 6:
      if (radio.available()){
        Serial.println("Received Payload");
        if (readIncoming()){
          Serial.println("Payload is addressed to this node");
          if (res.instruction == SrtEN){
            Serial.println("Instructed to start expanding network by self pipe");
            comState = 3;
          }
          else if (res.instruction == Rst){
            Serial.print("Instructed to reset by self pipe with delay : ");
            Serial.println(res.data);
            delay(res.data);
            comState = 0;
          }
          else if (res.instruction == SLLT){
            Serial.println("Instructed to Start Led Progress Bar");
            duration = res.data;
            ledState = 6;
          }
          else if (res.instruction == ICLR){
            ledState = 5;
            Serial.println("Instructed to Interrupt Current Led Routine");
          }
        }
      }
      break;

    default:
      Serial.println("Default comState");
      break; 

  }
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
  else if(comState == 6){
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
