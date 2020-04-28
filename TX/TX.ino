#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SimpleCLI.h>

#define CRqst 0
#define AAdrs 1
#define StpEN 2
#define SrtEN 3
#define Rst 4

RF24 radio(9, 10); // CE, CSN
byte self[6] = "9Node";
byte child[6] = "9Node";
byte gate[6] = "9Node";
byte voidAddress[6] = "00000";
int address = 1;

int target = 2;
unsigned long timer;
unsigned long pairingTimeout = 10000;

struct Payload {
  unsigned int address;
  unsigned int instruction;
  unsigned long data;
};

Payload res;

SimpleCLI cli;
Command cmdPair;
Command cmdSend;



void setup() {
  self[0] = address;
  child[0] = address + 1;
  
  Serial.begin(9600);
  radio.begin();                  //Starting the Wireless communication
  cmdPair = cli.addCommand("pair");
  cmdSend = cli.addCommand("send");
  cmdSend.addArgument("t");
  cmdSend.addArgument("i",120);
  cmdSend.addArgument("d",0);
}

void loop()
{
  if(Serial.available()){
    String input = Serial.readStringUntil('\n');
    Serial.println(">" + input);
    cli.parse(input);
    }
  if (cli.available()){
    Command cmd = cli.getCommand();
    if (cmd == cmdPair){
      pairNode();
    }
    if (cmd==cmdSend){
      Argument argT = cmd.getArgument("t");
      Argument argI = cmd.getArgument("i");
      Argument argD = cmd.getArgument("d");
      String argTval = argT.getValue();
      unsigned int t = argTval.toInt();
      String argIval = argI.getValue();
      unsigned int i = argIval.toInt();
      String argDval = argD.getValue();
      unsigned long d = argDval.toInt();
      Payload P = {t,i,d};
      radio.stopListening();
      radio.openWritingPipe(child);
      if (radio.write(&P,sizeof(P))){
        Serial.println("Transmition successful");
      }
      else{
        Serial.println("Transmition failed");
      }
    }
  }
delay(5);
}

void pairNode(){
  Serial.println("Trying to pair new node");
  radio.openReadingPipe(1,gate);
  radio.startListening();
  timer = millis();
  while(!radio.available()) {
    if (millis()-timer>pairingTimeout){
      Serial.println("Pairing Timed-out");
      return;
    }
    };
  readIncoming();
  if (res.instruction == CRqst){
    Serial.println("Received CRqst");
    radio.openWritingPipe(gate);
    radio.stopListening();
    Payload P = {0,AAdrs,address+1};
    if (radio.write(&P,sizeof(P))){
      Serial.println("Pairing successful");
    }
    else{
      Serial.println("Node did not ack AAdrs");
    }
  }
  else{
    Serial.println("Received payload is not a CRqst");
  }
}


void readIncoming(){
  radio.read(&res, sizeof(res));    //Reading the data
  Serial.println(res.instruction);
  Serial.println(res.data);
  Serial.println("**************************");
}
