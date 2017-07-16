#include <IRremote.h>
#include <EEPROM.h>

#define RECV_PIN 4

IRrecv irrecv(RECV_PIN);
IRsend irsend;
decode_results decode_result;

bool isRecorded = false;
bool LEARN = true;
bool RESET = false;
bool OPERATIONAL = false;

int memPtr;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // BT communication
  Serial1.begin(9600);
  
  irrecv.enableIRIn();
  Serial.println("IR receive active.");
  
  memPtr = EEPROM.read(EEPROM.length() - 1);
  
  Serial.print("Memory location : ");
  Serial.println(memPtr);
}

void loop() {
  while(LEARN){
    // to simulate the code received from the phone
    byte code;
    
    // until receiving an IR signal
    if(irrecv.decode(&decode_result)){
      //LEARN = false;
      byte index = (memPtr)/6;
      save(decode_result.rawlen, decode_result.value, decode_result.decode_type);
      transmitResult(&decode_result);
      Serial.println("Saved successfully..");
      Serial1.print("INDEX ");
      Serial1.println(index);
      irrecv.resume();
    }
  }
  
  if(RESET){
    memPtr = 1;
    EEPROM.write(EEPROM.length() - 1, 0);
  }
  while(OPERATIONAL){
    // to simulate BT input
    byte index = 0;
    for(int i = 0; i < memPtr/6; i++){
      decode_results decode_result;
      getDecoded(i, &decode_result);
      transmitResult(&decode_result);
    }
    delay(1000);
  }
}

void receiveResult(){
  if (irrecv.decode(&decode_result)) {
    // Update state as recorded
    isRecorded = true;
    // Resume IR Read
    transmitResult(&decode_result);

    // Resume IR Read
    irrecv.resume();
  }
}

void save(byte rawLen, int value, byte remote){
  // write the length
  EEPROM.write(memPtr++, rawLen);

  // write the data part
  EEPROM.write(memPtr++, value%256);
  value /= 256;
  EEPROM.write(memPtr++, value%256);
  value /= 256;
  EEPROM.write(memPtr++, value%256);
  value /= 256;
  EEPROM.write(memPtr++, value%256);

  // write remote vendor
  EEPROM.write(memPtr++, remote);

  updateMemory();
}

void getDecoded(byte index, decode_results *dr){
  int loc = index*6;
  byte rawlen = EEPROM.read(loc);
  unsigned long data_0 = EEPROM.read(loc+1);
  unsigned long data_1 = EEPROM.read(loc+2);
  unsigned long data_2 = EEPROM.read(loc+3);
  unsigned long data_3 = EEPROM.read(loc+4);
  byte type = EEPROM.read(loc+5);
  unsigned long data = data_3 << 24 | data_2 << 16 | data_1 << 8 | data_0;
  dr->rawlen = rawlen;
  dr->decode_type = type;
  dr->value = data;
}

void updateMemory(){
  EEPROM.write(EEPROM.length()-1, memPtr);
}

void transmitResult(decode_results* results){
  int count = results->rawlen;
  Serial.print("Raw Len = " );
  Serial.println(count);
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
    //irsend.sendRaw(results->value, count);
  } 
  else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
    irsend.sendNEC(results->value, count);
  } 
  else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
    irsend.sendSony(results->value, count);
  } 
  else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
    irsend.sendRC5(results->value, count);
  } 
  else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
    irsend.sendRC6(results->value, count);
  }
  Serial.print(results->value, HEX);
  Serial.println();
}
#include <IRremote.h>

#define RECV_PIN 4

IRrecv irrecv(RECV_PIN);
IRsend irsend;
decode_results decode_result;

unsigned int *code;
int rawlen;

bool isRecorded = false;
bool LEARN = false;
bool RESET = false;
bool OPERATIONAL = false;

//===================================================

String encodeLearnt(decode_results* dr){
  String line = "~C ";
  if(dr->rawlen < 10){
    line += "0";
    line += String(dr->rawlen);
  }
  else{
    line += String(dr->rawlen);
  }
  for(int i = 0; i < dr->rawlen; i++){
    line += " ";
    String val = String((long)dr->rawbuf[i], DEC);
    int padLength = 8 - val.length();
    for(int j = 0; j < padLength; j++){
      val = "0"+val;
    }
    line += val;
  }
  line += "#";
  return line;
}

//===================================================

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // BT communication
  Serial1.begin(9600);
  
  irrecv.enableIRIn();
  Serial.println("Command receive active.");
}

void loop() {
  
  if(Serial1.available() > 0){
    Serial.println("Serial available..");
    String all = Serial1.readString();
    Serial.print("Read str : ");
    Serial.println(all);
    if(all.charAt(0) != '~' || all.charAt(all.length() - 1) != '#'){
      Serial.println("Invalid code!");
      return;
    }
    int g = 0;
    g++;
    char c = all.charAt(g++);
    
    switch(c){
      case 'L':
        LEARN = true;
        Serial.println("Learning Mode active...");
        break;
      case 'O':
        OPERATIONAL = true;
        Serial.println("Operarional Mode active...");
        break;
       default:
        Serial.println("Invalid command!");
    }
    
    // drop the space
    g++;
    String len = "";
    for(int i = 0; i < 2; i++){
      len+=all.charAt(g++);
    }
    rawlen = len.toInt();
    code = (unsigned int *)malloc(rawlen * sizeof(int));
    
    for(int i = 0; i < rawlen; i++){
      // read and drop space
      g++;
      String valueStr = "";
      for(int j = 0; j < 8; j++){
        valueStr+= all.charAt(g++);
      }
      unsigned int value = (unsigned int)valueStr.toInt();
      code[i] = value;
    }
  }
  
  if(LEARN){
    // Serial.println("In learning mode.");
    byte code;
    while(!irrecv.decode(&decode_result)){
      // pass
      // Serial.print("not recv");
    }
    // received an IR signal
    LEARN = false;
    String line = encodeLearnt(&decode_result);
    Serial1 .println(line);
    Serial.println(line);
    irrecv.resume();
  }
  
  if(OPERATIONAL){
    decode_result.rawbuf = code;
    decode_result.rawlen = rawlen;
    String received = encodeLearnt(&decode_result);
    Serial.println(received);
    irsend.sendRaw(code, rawlen, 38);
    OPERATIONAL = false;
  }
}

