#include <IRremote.h>
#include <EEPROM.h>

#define RECV_PIN 4

IRrecv irrecv(RECV_PIN);
IRsend irsend;
decode_results decode_result;

bool isRecorded = false;
bool LEARN = true;
bool RESET = false;
bool OPERATIONAL = true;

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
      LEARN = false;
      byte index = (memPtr)/6;
      save(decode_result.rawlen, decode_result.value, decode_result.decode_type);
      Serial.println("Saved successfully..");
      Serial1.print("INDEX ");
      Serial1.println(index);
      irrecv.resume();
    }
  }
  
  if(RESET){
    memPtr = 0;
    EEPROM.write(EEPROM.length() - 1, 0);
  }
  while(OPERATIONAL){
    // to simulate BT input
    byte index = 0;
    decode_results decode_result;
    getDecoded(0, &decode_result);
    transmitResult(&decode_result);
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
  int data_0 = EEPROM.read(loc+1);
  int data_1 = EEPROM.read(loc+2);
  int data_2 = EEPROM.read(loc+3);
  int data_3 = EEPROM.read(loc+4);
  byte type = EEPROM.read(loc+5);
  int data = data_3 << 24 | data_2 << 16 | data_1 << 8 | data_0;
  dr->rawlen = rawlen;
  dr->decode_type = type;
  dr->value = data;
}

void updateMemory(){
  EEPROM.write(EEPROM.length()-1, memPtr);
}

void transmitResult(decode_results* results){
  int count = results->rawlen;
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
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 0; i < count; i++) {
    if ((i % 2) == 1) {
      Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    } 
    else {
      Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
    }
    Serial.print(" ");
  }
  Serial.println("");
}

