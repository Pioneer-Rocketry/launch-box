#include <Arduino.h>
#include <ArduinoJson.h>

const int ARMED_PIN             = A4;   // Voltage across the battery. Step down by 75%
const int IGN_CONTINUITY_PIN    = A7;   // Continuity across the igniter 

const int TRIGGER_PIN           = 2;    // Trigger pin (D2) for lighting the igniter
const int BUZZER_PIN            = 7;    // Buzzer pin (D7)
const int MANUAL_TRIG_GND      = 10;   // Override switch
const int MANUAL_TRIG_POS      = 11;
const int MANUAL_OVERRIDE_TOGGLE = 12;

const int TRIGGER_LENGTH        = 1000; // 1000 ms = 1 s

const float IS_ARMED_SET_POINT  = 3.4;  // Voltage at which the system is armed
const float HAS_CONTINUITY_POINT = 3.4; // Voltage at which the igniter has continuity
const float MANUAL_TRIG_POINT = 3.4;

const String pc_TOPIC           = "TOPIC";
const String pc_REQ_TRI         = "REQ_TRI";
const String pc_REQ_CON         = "REQ_CON";
const String pc_RES_CON         = "RES_CON";
const String pc_RES_TRI         = "RES_TRI";

const bool pc_debug                = false;


void startBuzzer()
{
  digitalWrite(BUZZER_PIN, HIGH);
}

void endBuzzer()
{
  digitalWrite(BUZZER_PIN, LOW);
}

void trigger()
{
  digitalWrite(TRIGGER_PIN, HIGH);
  delay(TRIGGER_LENGTH);
  digitalWrite(TRIGGER_PIN, LOW);
}

bool isArmedFunc()
{
  bool isArmed = false;

  int rawVoltage = analogRead(ARMED_PIN);
  
  float currentVoltage = rawVoltage * (5.0 / 1023.0); // clamps the voltage between 0V - 5V

  if (pc_debug)
  {
    // Serial.println("rawVoltage: " + String(rawVoltage));
    // Serial.println("currentVoltage: " + String(currentVoltage));
  }

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

bool hasContinuityFunc()
{
  bool hasContinuity = false;
  int rawValue = analogRead(IGN_CONTINUITY_PIN);
  float currentVoltage = rawValue * (5.0 / 1023.0); // clamps the voltage between 0V - 5V

  if (currentVoltage >= HAS_CONTINUITY_POINT)
  {
    hasContinuity = true;
  }
  else
  {
    hasContinuity = false;
  }

  return hasContinuity;
}

bool shouldManualTrigger()
{

  bool manualTrig = false;

  bool GND = digitalRead(MANUAL_TRIG_GND);
  bool POS = digitalRead(MANUAL_TRIG_POS);

  if (!GND && POS)
  {
    manualTrig = true;
    return manualTrig;
  }

  return manualTrig;

}

void setup() {
  // put your setup code here, to run once:
  pinMode(ARMED_PIN, INPUT);
  pinMode(IGN_CONTINUITY_PIN, INPUT);

  pinMode(MANUAL_TRIG_GND, INPUT_PULLUP);
  pinMode(MANUAL_TRIG_POS, INPUT);
  digitalWrite(MANUAL_TRIG_POS, LOW);
  
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(19200);

}

void loop() {
  
  bool isArmed = false;
  bool hasContinuity = false;
  bool manualTrigger = false;
  String msg = "";
  JsonDocument doc;

  while (1)
  {
    isArmed = isArmedFunc();
    hasContinuity = hasContinuityFunc();
    manualTrigger = shouldManualTrigger();

    if (hasContinuity)
    {
      startBuzzer();
    }
    else
    {
      endBuzzer();
    }

    if (Serial.available())
    {
      if (pc_debug)
      {
        Serial.println("Serial.available()");
      }
      msg = Serial.readStringUntil('\n');
      msg.trim();
      if(msg.length() > 0)
      {
        const char* msgCStr = msg.c_str();
        if (pc_debug)
        {
          Serial.println("Msg length: " + String(msg.length()));
          Serial.println("msg: " + msg);
        }
        DeserializationError err1 = deserializeJson(doc, msgCStr);
        String err1Str = err1.f_str();
        if (err1 && pc_debug)
        {
          Serial.println("{\"TOPIC\":\"ERROR\",\"msg\":\"1. Error occured desearializing incoming msg:'"+err1Str+"'\",\"msgRecv\":\""+msg+"\"}");
        }
        else
        {
          String topic = doc[pc_TOPIC];

          if (topic == pc_REQ_TRI)
          {
            if (isArmed && hasContinuity)
            {
              trigger();
              doc.clear();
              String res = "{\"TOPIC\":\""+pc_RES_TRI+"\",\"msg\":\"success\"}";
              DeserializationError err2 = deserializeJson(doc, res.c_str());
              String err2Str = err2.f_str();
              if (err2 && pc_debug)
              {
                Serial.println("{\"TOPIC\":\"ERROR\",\"msg\":\"2. Error occured desearializing incoming msg:'"+err2Str+"'\"");
              }
              else
              {
                String resSerialize = "";
                serializeJson(doc, resSerialize); 
                Serial.println(resSerialize);
              }
            }
          }
          else if (topic == pc_REQ_CON)
          {
            doc.clear();
            String isArmedStr = String((int)isArmed);
            String hasContinuityStr = String((int)hasContinuity);

            String res = "{\"TOPIC\":\""+pc_RES_CON+"\",\"isArmed\":"+isArmedStr+",\"hasContinuity\":"+hasContinuity+"}";
            DeserializationError err2 = deserializeJson(doc, res.c_str());

            String err2Str = err2.f_str();
            if (err2 && pc_debug)
            {
              Serial.println("{\"TOPIC\":\"ERROR\",\"msg\":\"3. Error occured desearializing incoming msg:'"+err2Str+"'\"");
            }
            else
            {
              String resSerialize = "";
              serializeJson(doc, resSerialize);
              Serial.println(resSerialize);
            }
          }
        }
      }

    }
    else if (digitalRead(MANUAL_OVERRIDE_TOGGLE))
    {
      if (manualTrigger) // comes from launch control unit
      {
        if (isArmed && hasContinuity) // comes from launch pad box
        {
          trigger();
        }
      }
    }
  }
  
}