/*--------------------------------------------------------------------------*/
/****************************Get started the code****************************/
/*---------------------------------------------------------------------------*/
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "Talkie.h"
#include "Vocab_US_Large.h"
#include "Vocab_Special.h"
Talkie voice;
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
const int totalPhoneNo = 5;
const byte phoneLength = 13;
//total memory consumed by phone numbers in EEPROM
const byte totalMemory = (phoneLength * 5) - 1;
//set starting address of each phone number in EEPROM
int offsetPhone[totalPhoneNo] = {0,phoneLength,phoneLength*2,phoneLength*3,phoneLength*4};
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
#define rxPin 6         //GSM Module TX pin to Arduino 6
#define txPin 7         //GSM Module RX pin to Arduino 7
SoftwareSerial sim800(rxPin,txPin);
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
#define BUTTON_1 5
#define BUTTON_2 4
#define RELAY_1 A0
boolean STATE_RELAY_1 = 0;
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
boolean isReply = false;
unsigned long currentTime;
unsigned long callTime;
unsigned long loopTime;
unsigned long fstcallTime;
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//stores incomming data from sim800l
String dtmf_cmd;
boolean is_call = false;
boolean lucky = false;
boolean my_call = false;
/*******************************************************************************
 * getResponse function
 ******************************************************************************/
boolean getResponse(String expected_answer, unsigned int timeout=1000,boolean reset=false){
  boolean flag = false;
  String response = "";
  unsigned long previous;
  //*************************************************************
  for(previous=millis(); (millis() - previous) < timeout;){
    while(sim800.available()){
      response = sim800.readString();
      //----------------------------------------
      if(response != ""){
        Serial.println(response);
        if(reset == true)
          return true;
      }
      //----------------------------------------
      if(response.indexOf(expected_answer) > -1){
        return true;
      }
    }
  }
  //*************************************************************
  return false;
}
/*******************************************************************************
 * setup function
 ******************************************************************************/
void setup() {
  //-----------------------------------------------------
  Serial.begin(9600);
  Serial.println("Arduino serial initialize");
  //-----------------------------------------------------
  sim800.begin(9600);
  Serial.println("SIM800L software serial initialize");
  //-----------------------------------------------------
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(RELAY_1, OUTPUT); //Relay 1
  digitalWrite(RELAY_1, HIGH);  
  //-------------------------------------------------------------------
  LoadStateEEPROM();
  //-------------------------------------------------------------------
  sim800.println("AT");
  getResponse("OK",100);
  sim800.println("ATE1");
  getResponse("OK",100);  
  sim800.println("AT+CMGF=1");
  getResponse("OK",100);
  sim800.println("AT+CNMI=1,2,0,0,0");
  getResponse("OK",100);
  sim800.println("AT+DDET=1");
  getResponse("OK",100);
  sim800.println("AT+CLIP=1");
  getResponse("OK",100);
  //delete all sms
  sim800.println("AT+CMGD=1,4");
  delay(500);
  sim800.println("AT+CMGDA= \"DEL ALL\"");
  delay(500);
  sim800.println("ATH");
  Serial.println(GetRegisteredPhoneNumbersList());
}
/*******************************************************************************
 * Loop Function
 ******************************************************************************/
void loop() {
  currentTime = millis();
   if((currentTime - loopTime) >= 600000){
    sim800.println("AT");
    getResponse("OK");
    loopTime = currentTime;
  }
      if(currentTime == (fstcallTime + 32000)){
   // mp3_play (007);
   voice.say(sp4_NO);delay(300); voice.say(sp3_NORTH);
    delay(2500);
  }
   if(currentTime == (callTime + 35000)){
    sim800.println("ATH");
    getResponse("OK");
    is_call = false;
  }
//------------------------------------------------------------------------------
while(sim800.available()){
  String response = sim800.readString();
  Serial.println(response);
  //__________________________________________________________________________
  //if there is an incoming SMS
  if(response.indexOf("+CMT:") > -1){
    String callerID = getCallerID(response);
    String cmd = getMsgContent(response);
    //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
    //this if statement will execute only if sim800l received "r" command 
    //and there will be no any phone number stored in the EEPROM
    if(cmd.equals("r")){
      String admin_phone = readFromEEPROM(offsetPhone[0]);
      if(admin_phone.length() != phoneLength){
        RegisterPhoneNumber(1, callerID, callerID);
       // break;
      }
      else {
        String text = "Error: Admin phone number is failed to register";
        Serial.println(text);
        Reply(text, callerID);
       // break;
      }
    }
    //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
    //The action is performed only if the caller ID will matched with
    //any one of the phone numbers stored in the eeprom.
    if(comparePhone(callerID)){
      doAction(cmd, callerID, "");
      //delete all sms
      sim800.println("AT+CMGD=1,4");
      delay(1000);
      sim800.println("AT+CMGDA= \"DEL ALL\"");
      delay(1000);
    }
    else {
      String text = "Error: Please register your phone number first";
      Serial.println(text);
      Reply(text, callerID);
    }
    //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  }
  //__________________________________________________________________________
if(response.indexOf("RING") > -1){
       boolean flag = 0;
    //------------------------------------------
    //+CLIP: "+911234567890",145,"",0,"",0
    if(response.indexOf("+CLIP:")){
     unsigned int index, index1;     
      index = response.indexOf("\"");
      index1 = response.indexOf("\"", index+1);  
      String temp = response.substring(index+1, index1);
     //get phone like this format +911234567890  ,String temp = response.substring(index+4, index1); - get phone like this format 1234567890.
      temp.trim();
      Serial.println("Calling: "+temp);
      if(temp.length() == 13){
        //number formate +91 1234567890
        flag = comparePhone(temp);
      }    
  }
    if(flag == 1){
      delay(300);
      sim800.println("ATA");
      getResponse("OK");
      delay(100);
      //is_call = true;
      lucky = true;
      callTime = currentTime;
     voise();
    if(response.indexOf("NO CARRIER") > -1){
        sim800.println("ATH;");
        getResponse("OK");
        is_call = false;
      }
   }   
    else{
      sim800.println("AT+CHUP;");
      getResponse("OK");
    }
  } 
     if(is_call == true){
      //HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
      if(int index = response.indexOf("+DTMF:") > -1 ){
        index = response.indexOf(":");
        dtmf_cmd = response.substring(index+1, response.length());
        dtmf_cmd.trim();
        Serial.println("dtmf_cmd: "+dtmf_cmd);
        doAction("","",dtmf_cmd);
      }   
      //HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
      if(response.indexOf("NO CARRIER") > -1){
        sim800.println("ATH");
        getResponse("OK");
        is_call = false;
      }
      //HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
    }    
 }
//------------------------------------------------------------------------------
 while(Serial.available())  {
  String response = Serial.readString();
  if(response.indexOf("clear") > -1){
    Serial.println(response);
    DeletePhoneNumberList();
    Serial.println(GetRegisteredPhoneNumbersList());
   // break;
  }
  sim800.println(response);
 }
 listen_push_buttons();
}
//------------------------------------------------------------------------------
//main loop ends
//------------------------------------------------------------------------------
void voise(){
  if(lucky == true){
   delay(500);voice.say(sp2_PRESS);voice.say(sp2_ONE);voice.say(sp4_FOR);voice.say(sp2_MOTOR);delay(100);voice.say(sp4_ON);
      delay(1000);voice.say(sp2_PRESS);voice.say(sp2_TWO);voice.say(sp4_FOR);voice.say(sp2_MOTOR);delay(100);voice.say(sp4_OFF);
      delay(1000);voice.say(sp2_PRESS);voice.say(sp2_THREE);voice.say(sp4_FOR);voice.say(sp2_MOTOR);delay(100);voice.say(sp4_CHECK);
      delay(100);
     }
      callTime = currentTime;
     lucky = false;
     is_call = true;
     return;
}
/*******************************************************************************
 * GetRegisteredPhoneNumbersList function:
 ******************************************************************************/
String GetRegisteredPhoneNumbersList(){
  String text = "Registered Numbers List: \r\n";
  String temp = "";
  for(int i = 0; i < totalPhoneNo; i++){
    temp = readFromEEPROM(offsetPhone[i]);
    if(temp == "")
      { text = text + String(i+1)+". Empty\r\n"; }
    else if(temp.length() != phoneLength)
      { text = text + String(i+1)+". Wrong Format\r\n"; }
    else
      {text = text + String(i+1)+". "+temp+"\r\n";}
  }
  return text;
}  
/*******************************************************************************
 * RegisterPhoneNumber function:
 ******************************************************************************/
void RegisterPhoneNumber(int index, String eeprom_phone, String caller_id){
  if(eeprom_phone.length() == phoneLength){
    writeToEEPROM(offsetPhone[index-1],eeprom_phone);
    String text = "Phone"+String(index)+" is Registered: ";
    //text = text + phoneNumber;
    Serial.println(text);
    Reply(text, caller_id);
  }
  else {
    String text = "Error: Phone number must be "+String(phoneLength)+" digits long";
    Serial.println(text);
    Reply(text, caller_id);
  }
}
/*******************************************************************************
 * UnRegisterPhoneNumber function:
 ******************************************************************************/
void DeletePhoneNumber(int index, String caller_id){
  writeToEEPROM(offsetPhone[index-1],"");
  String text = "Phone"+String(index)+" is deleted";
  Serial.println(text);
  Reply(text, caller_id);
}
/*******************************************************************************
 * DeletePhoneNumberList function:
 ******************************************************************************/
void DeletePhoneNumberList(){
  for (int i = 0; i < totalPhoneNo; i++){
    writeToEEPROM(offsetPhone[i],"");
  }
}  
/*******************************************************************************
 * doAction function:
 * Performs action according to the received sms
 ******************************************************************************/
void doAction(String cmd, String caller_id, String dtmf_cmd){
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  if(cmd == "on"){
    controlRelayGSM(1, RELAY_1,STATE_RELAY_1 = LOW,caller_id);
  }
  else if(cmd == "off"){
    controlRelayGSM(1, RELAY_1,STATE_RELAY_1 = HIGH,caller_id);
  }
  //------------------------------------------------
  else if(dtmf_cmd == "1"){
    delay(1000);voice.say(sp2_MOTOR);voice.say(sp4_ON);delay(300);
    controlRelayGSM(1, RELAY_1,STATE_RELAY_1 = LOW,caller_id);
    callTime = currentTime;
  }
  else if(dtmf_cmd == "2"){
    delay(1000);voice.say(sp2_MOTOR);voice.say(sp4_OFF);delay(300);
    controlRelayGSM(1, RELAY_1,STATE_RELAY_1 = HIGH,caller_id);
    callTime = currentTime;
  }
  else if (dtmf_cmd == "3"){
    sim800.println("AT+DDET=0");
    if(STATE_RELAY_1 == 0){
       delay(1000);voice.say(sp2_MOTOR);voice.say(sp4_IS);voice.say(sp4_ON);delay(500);
    } else{
      if(STATE_RELAY_1 == 1){
        delay(1000);voice.say(sp2_MOTOR);voice.say(sp4_IS);delay(200);voice.say(sp4_OFF);delay(500);
      }
    }
    sim800.println("AT+DDET=1");
    callTime = currentTime;
  }
  else if(dtmf_cmd == "4"){
    lucky = true;
    voise();
    callTime = currentTime;
  }
  else if(dtmf_cmd == "5"){
   // delay(1000);voice.say(sp2_INVALID);voice.say(sp4_INPUT);delay(300);
   delay(1000);voice.say(sp3_ERROR); //voice.say(sp4_NO);voice.say(sp4_NO);
    callTime = currentTime;
  }
  else if(dtmf_cmd == "6"){
    delay(1000);voice.say(sp3_ERROR);
    callTime = currentTime;
  }
  else if(dtmf_cmd == "7"){
    delay(1000);voice.say(sp3_ERROR);
    callTime = currentTime;
  }
  else if(dtmf_cmd == "8"){
    delay(1000);voice.say(sp3_ERROR);
    callTime = currentTime;
  }
  else if(dtmf_cmd == "9"){
    delay(1000);voice.say(sp3_ERROR);
    callTime = currentTime;
  }
  else if(dtmf_cmd == "0"){
    delay(1000);voice.say(sp3_ERROR);
    callTime = currentTime;
  }
  else if(dtmf_cmd == "*"){
    delay(1000);voice.say(sp3_ERROR);
    callTime = currentTime;
  }
  else if(dtmf_cmd == "#"){
    delay(1000);voice.say(sp3_ERROR);
    callTime = currentTime;
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  else if(cmd == "status"){
    sendStatus(1, STATE_RELAY_1,caller_id);
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  else if(cmd.indexOf("r2=") > -1){
    RegisterPhoneNumber(2, getNumber(cmd), caller_id);
  }
  else if(cmd.indexOf("r3=") > -1){
    RegisterPhoneNumber(3, getNumber(cmd), caller_id);
  }
  else if(cmd.indexOf("r4=") > -1){ 
    RegisterPhoneNumber(4, getNumber(cmd), caller_id);
  }
  else if(cmd.indexOf("r5=") > -1){
    RegisterPhoneNumber(5, getNumber(cmd), caller_id);
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  else if(cmd == "list"){
    String listno = (GetRegisteredPhoneNumbersList());
    Serial.println(listno);
    Reply(listno, caller_id);
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  else if(cmd == "del=ureka"){  
    DeletePhoneNumber(1, caller_id);
  }
  else if(cmd == "del=2"){  
    DeletePhoneNumber(2, caller_id);
  }
  else if(cmd == "del=3"){  
    DeletePhoneNumber(3, caller_id);
  }
  else if(cmd == "del=4"){  
    DeletePhoneNumber(4, caller_id);
  }
  else if(cmd == "del=5"){  
    DeletePhoneNumber(5, caller_id);
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  else if(cmd == "del=all"){
    //DeletePhoneNumberList();
    DeletePhoneNumber(2, caller_id);
    DeletePhoneNumber(3, caller_id);
    DeletePhoneNumber(4, caller_id);
     DeletePhoneNumber(5, caller_id);
    String text = "All the phone numbers are deleted.";
    Serial.println(text);
    Reply(text, caller_id);
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
    else if(cmd == "clear=ureka"){
    DeletePhoneNumberList();
    String text = "All the phone numbers are deleted, with admin.";
    Serial.println(text);
    Reply(text, caller_id);
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  else{
    String text = "Error: Unknown command: "+cmd;
    Serial.println(text);
    Reply(text, caller_id);
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
}
//*************************************************************************************
void listen_push_buttons(){
 /* boolean status;
  String text = (status)? "OFF" : "ON";
  String botton = "Motor is " +text + "By Switch";
  Serial.println(botton);
  const String myNumber = readFromEEPROM(offsetPhone[1]);
  const String myNumber2 = readFromEEPROM(offsetPhone[2]);
  const String myNumber3 = readFromEEPROM(offsetPhone[3]);
  const String myNumber4 = readFromEEPROM(offsetPhone[4]);*/
    if(digitalRead(BUTTON_1) == LOW){
      control_relay(1, RELAY_1, STATE_RELAY_1);
  /*    //String botton = (1, STATE_RELAY_1);
      Serial.println(botton);
      Reply(botton, myNumber);
      delay(500);
      Reply(botton, myNumber2);
      delay(500);
      Reply(botton, myNumber3);
      delay(500);
      Reply(botton, myNumber4);*/
    }
    else if(digitalRead(BUTTON_2) == LOW){
      digitalWrite(RELAY_1,STATE_RELAY_1 = HIGH);
      String phsfail = "Motor is OFF for phase problem";
      Serial.println(phsfail);
  //    Reply(phsfail, myNumber);
      my_call = true;
      callTo();
    //  controlRelayGSM(1, RELAY_1,STATE_RELAY_1 = HIGH, PhoneNumber(2,caller_id));
    }
  }
//***************************************************************************************
void control_relay(int relay_no, int relay_pin, boolean &status){
  const String myNumber = readFromEEPROM(offsetPhone[1]);
  const String myNumber2 = readFromEEPROM(offsetPhone[2]);
  const String myNumber3 = readFromEEPROM(offsetPhone[3]);
  const String myNumber4 = readFromEEPROM(offsetPhone[4]);
  delay(200);
  String text = (status)? "ON" : "OFF";
  status = !status;
  digitalWrite(relay_pin, status);
  String botton = ("Motor is "+text + " " "by switch.");
  Serial.println (botton);
  delay(50);
  EEPROM.update(totalMemory+relay_no,status);
      Reply(botton, myNumber);
      delay(500);
      Reply(botton, myNumber2);
      delay(500);
      Reply(botton, myNumber3);
      delay(500);
      Reply(botton, myNumber4);
}
/*******************************************************************************
 * getCallerID function:
 ******************************************************************************/
String getCallerID(String buff){
//+CMT: "+923001234567","","22/05/20,11:59:15+20"
//Hello
  unsigned int index, index2;
  index = buff.indexOf("\"");
  index2 = buff.indexOf("\"", index+1);
  String callerID = buff.substring(index+1, index2);
  callerID.trim();
  //Serial.print("index+1= "); Serial.println(index+1);
  //Serial.print("index2= "); Serial.println(index2);
  //Serial.print("length= "); Serial.println(callerID.length());
  Serial.println("Caller ID: "+callerID);
  return callerID;
}
/*******************************************************************************
 * getMsgContent function:
 ******************************************************************************/
String getMsgContent(String buff){
  //+CMT: "+923001234567","","22/05/20,11:59:15+20"
  //Hello  
  unsigned int index, index2;
  index = buff.lastIndexOf("\"");
  index2 = buff.length();
  String command = buff.substring(index+1, index2);
  command.trim();
  command.toLowerCase();
  Serial.println("Command:"+command);
  return command;
}
/*******************************************************************************
 * getNumber function:
 ******************************************************************************/
String getNumber(String text){
  //r=+923001234567
  String temp = text.substring(3, 16);
  temp.trim();
  return temp;
}
/*******************************************************************************
 * controlRelayGSM function:
 ******************************************************************************/
void controlRelayGSM(int relay_no, int relay_pin, boolean status, String caller_id){
  digitalWrite(relay_pin, status);
  EEPROM.update(totalMemory+relay_no,status);
  String text = (status)? "OFF" : "ON";
  text = "MOTOR IS " +text;
  Serial.println(text);
  Reply(text, caller_id);
}
/*******************************************************************************
 * sendStatus function:
 ******************************************************************************/
void sendStatus(int relay_no, boolean status, String caller_id){
  String text = (status)? "OFF" : "ON";
  text = "MOTOR IS " +text;
  Serial.println(text);
  Reply(text, caller_id);
}
/*******************************************************************************
 * Reply function
 * Send an sms
 ******************************************************************************/
void Reply(String text, String caller_id){
    sim800.print("AT+CMGF=1\r");
    delay(300);
    sim800.print("AT+CMGS=\""+caller_id+"\"\r");
    delay(300);
    sim800.print(text);
    delay(100);
    sim800.write(0x1A); //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
    delay(500);
    Serial.println("SMS Sent Successfully.");
}
/*******************************************************************************
 * writeToEEPROM function:
 * Store registered phone numbers in EEPROM
 ******************************************************************************/
void writeToEEPROM(int addrOffset, const String &strToWrite){
  for (int i = 0; i < phoneLength; i++)
    { EEPROM.write(addrOffset + i, strToWrite[i]); }
}
/*******************************************************************************
 * readFromEEPROM function:
 * Store phone numbers in EEPROM
 ******************************************************************************/
String readFromEEPROM(int addrOffset){
  char data[phoneLength + 1];
  for (int i = 0; i < phoneLength; i++)
    { data[i] = EEPROM.read(addrOffset + i); }
  data[phoneLength] = '\0';
  return String(data);
}
/*******************************************************************************
 * comparePhone function:
 * compare phone numbers stored in EEPROM
 ******************************************************************************/
boolean comparePhone(String number){
  boolean flag = 0;
  String tempPhone = "";
  //--------------------------------------------------
  for (int i = 0; i < totalPhoneNo; i++){
    tempPhone = readFromEEPROM(offsetPhone[i]);
    if(tempPhone.equals(number)){
      flag = 1;
      break;
    }
  }
  //--------------------------------------------------
  return flag;
}
void LoadStateEEPROM(){
  STATE_RELAY_1 = EEPROM.read(totalMemory+1); delay(200);  
  digitalWrite(RELAY_1, STATE_RELAY_1);
}
void callTo(){
const String myNumber = readFromEEPROM(offsetPhone[1]);
if (my_call == true){
  sim800.println("ATD"+myNumber+";");
   getResponse("OK",100);
  String text = "Phone"+myNumber+" to calling";
  Serial.println(text);
   my_call = false;
  delay(20000);
  sim800.println("ATH");
   getResponse("OK",100);
  delay(1000);
  sim800.println("ATDL");
   getResponse("OK",100);
  String ntext = "Phone"+myNumber+" redialing";
  Serial.println(ntext);
  delay(20000);
  sim800.println("ATH");
   getResponse("OK",100);
// my_call = false;
   }
return;
}
//*******************************************end******************************************//
