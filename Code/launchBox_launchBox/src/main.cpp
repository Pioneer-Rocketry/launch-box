#include <Arduino.h>
#include <ArduinoJson.h>

const int pc_ARMED_PIN             = A1;
const int pc_BUZZER_PIN             = 7;  // Digital pin D7
const int pc_TRIGGER_PIN            = 3;  // Digital D3
const int pc_MSG_RECV_PIN           = 4;  // Digital D4, LED
const int pc_BTN_LED_PIN            = 5; // Digital D5
const int pc_RAIL_ARMED_PIN         =       2; // 

const String pc_TOPIC           = "TOPIC";
const String pc_REQ_TRI         = "REQ_TRI";
const String pc_REQ_CON         = "REQ_CON";
const String pc_RES_CON         = "RES_CON";
const String pc_RES_TRI         = "RES_TRI";
const String pc_IS_ARMED        = "isArmed";
const String pc_HAS_CONTINUITY  = "hasContinuity";
const String pc_SUCCESS         = "success";
const String pc_ERROR           = "ERROR";

const float IS_ARMED_SET_POINT  = 3.4;  // Voltage at which the system is armed

const bool pc_debug             = false;

bool isHandHeldArmedFunc()
{
  bool isArmed = false;

  int rawVoltage = analogRead(pc_ARMED_PIN);
  
  float currentVoltage = rawVoltage * (5.0 / 1023.0); // clamps the voltage between 0V - 5V

  if (currentVoltage >= IS_ARMED_SET_POINT)
  {
    isArmed = true;
  }
  else
  {
    isArmed = false;
  }

  return isArmed;
}

bool isTriggerPressFunc()
{
  bool isArmed = false;

  int rawVoltage = analogRead(pc_ARMED_PIN);
  
  float currentVoltage = rawVoltage * (5.0 / 1023.0); // clamps the voltage between 0V - 5V

  if (currentVoltage >= IS_ARMED_SET_POINT)
  {
    isArmed = true;
  }
  else
  {
    isArmed = false;
  }

  return isArmed;
}

void turnOnBuzzer()
{
  digitalWrite(pc_BUZZER_PIN, HIGH);
}

void turnOffBuzzer()
{
  digitalWrite(pc_BUZZER_PIN, LOW);
}

void turnOnBTNLED()
{
  digitalWrite(pc_BTN_LED_PIN, HIGH);
}

void turnOffBTNLED()
{
  digitalWrite(pc_BTN_LED_PIN, LOW);
}

void turnOnRailArmed()
{
  digitalWrite(pc_RAIL_ARMED_PIN, HIGH);
}

void turnOffRailArmed()
{
  digitalWrite(pc_RAIL_ARMED_PIN, LOW);
}

void msgRecv()
{
  digitalWrite(pc_MSG_RECV_PIN, HIGH);
  delay(250);
  digitalWrite(pc_MSG_RECV_PIN, LOW);
}

void setup() {
  
  pinMode(pc_BUZZER_PIN, OUTPUT);
  pinMode(pc_MSG_RECV_PIN, OUTPUT);
  pinMode(pc_BTN_LED_PIN, OUTPUT);
  pinMode(pc_RAIL_ARMED_PIN, OUTPUT);

  pinMode(pc_TRIGGER_PIN, INPUT);
  pinMode(pc_ARMED_PIN, INPUT);

  Serial.begin(19200);

}

void loop() {

  bool isHandHeldArmed = false;
  bool isRailArmed    = false;
  bool hasContinuity = false;
  bool isTriggerPress = false;
  JsonDocument doc;
  String msg = "";
  String reqContinuitiesSerialize = "";
  String reqTriggerSerialize = "";


  // ----------------------------- REQ_CON ----------------------------- 
  String reqContinuities = "{\"TOPIC\":\""+pc_REQ_CON+"\"}";
  DeserializationError err1 = deserializeJson(doc, reqContinuities.c_str());
  String err1Str = err1.f_str();
  if (err1 && pc_debug)
  {
    Serial.println("{\"TOPIC\":\"ERROR\",\"msg\":\"2. Error occured desearializing incoming msg:'"+err1Str+"'\"");
  }
  else
  {
    reqContinuitiesSerialize = "";
    serializeJson(doc, reqContinuitiesSerialize); 
  }
  doc.clear();
  // ----------------------------- End ----------------------------- 



  // ----------------------------- REQ_TRI ----------------------------- 
  String reqTrigger = "{\"TOPIC\":\""+pc_REQ_TRI+"\"}";
  DeserializationError err2 = deserializeJson(doc, reqTrigger.c_str());
  String err2Str = err2.f_str();
  if (err2 && pc_debug)
  {
    Serial.println("{\"TOPIC\":\"ERROR\",\"msg\":\"2. Error occured desearializing incoming msg:'"+err2Str+"'\"");
  }
  else
  {
    reqTriggerSerialize = "";
    serializeJson(doc, reqTriggerSerialize); 
  }
  doc.clear();
  // ----------------------------- End ----------------------------- 

  while (1)
  {
    isHandHeldArmed = isHandHeldArmedFunc();
    isTriggerPress = digitalRead(pc_TRIGGER_PIN);

    doc.clear();
    Serial.println(reqContinuitiesSerialize); // Sending REQ_CON

    if (Serial.available() > 0)
    {
      msg = Serial.readStringUntil('\n');
      msg.trim(); // Takes out any spaces
      
      // turning into a json document
      const char* msgCStr = msg.c_str();
      if (pc_debug)
      {
        // Serial.println(msg);
      }
      doc.clear();
      DeserializationError err1 = deserializeJson(doc, msgCStr);
      String err1Str = err1.f_str();
      if (err1 && pc_debug)
      {
        Serial.println("{\"TOPIC\":\"ERROR\",\"msg\":\"1. Error occured desearializing incoming msg:'"+err1Str+"'\",\"msgRecv\":\""+msg+"\"}");
      }
      else
      {
        String topic = doc["TOPIC"];

        if (topic == pc_RES_CON)
        {
          msgRecv();
          isRailArmed = doc[pc_IS_ARMED];
          hasContinuity = doc[pc_HAS_CONTINUITY];

          if (hasContinuity)
          {
            turnOnRailArmed();
          }
          else
          {
            turnOffRailArmed();
          }

          if (pc_debug)
          {
            Serial.println("IsArmed: " + String(isRailArmed));
            Serial.println("isHandHeldArmed: " + String(isHandHeldArmed));
            Serial.println("asContinuity: " + String(hasContinuity));
            Serial.println("isTriggerPress: " + String(isTriggerPress));
          }
          
          doc.clear();
        }
        else if (topic == pc_RES_TRI)
        {
          msgRecv();
          String isSuccess = doc[pc_SUCCESS];
          doc.clear();
        }
        else if (topic == pc_ERROR)
        {
          msgRecv();
          /// TODO: handle errors
          doc.clear();
        }

        if (hasContinuity && isHandHeldArmed)
        {
          turnOnBuzzer();
          turnOnBTNLED();
        }
        else
        {
          turnOffBuzzer();
          turnOffBTNLED();
        }

        if (isTriggerPress && hasContinuity && isHandHeldArmed && isRailArmed)
        {
          Serial.println(reqTriggerSerialize);
        }

      }
    }
    else
    {
      delay(100);
    }
    msg = "";
    
  }


}