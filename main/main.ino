#define BLYNK_TEMPLATE_ID "TMPLh6RSrMPB"
#define BLYNK_DEVICE_NAME "Smart Watering"
#define BLYNK_AUTH_TOKEN "TGFKyDNnYmW9KyjD1nR_okdLGH2vkxic"

#define BLYNK_GREEN     "#23C48E"
#define BLYNK_BLUE      "#04C0F8"
#define BLYNK_YELLOW    "#ED9D00"
#define BLYNK_RED       "#D3435C"
#define BLYNK_DARK_BLUE "#5F7CD8"

#define PIN_OUT_CAPA_PUMP D1
#define PIN_IN_SENSOR A0

#define BLYNK_PRINT Serial
#define V_Connect   V0
#define V_ADC       V1
#define V_Timer     V2
#define V_OperationState V3
#define V_OperationMode      V4
#define V_ModeControl V5
#define V_WaterLevel V6
#define V_ErrorID V7
#define V_RemainingTime V8
#define V_Capa  V9
#define V_Emergency V10


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>


BlynkTimer timer;
WidgetLED  WLED_IsConnect(V_Connect);
unsigned long previousMillis = 0;
unsigned long interval = 30000;

enum OPERATION_STATE
{
  OP_POWERDOWN,
  OP_STANDBY,
  OP_RUNNING,
  OP_STOP,
  OP_MAX
};

enum WATER_LEVEL
{
  WL_LOW,
  WL_NORMAL,
  WL_HIGH,
  WL_MAX
};

enum ERROR_ID
{
  E_NONE,
  E_WATER_LOW,
  E_WATER_HIGH,
  E_PIPE,
  E_POWER,
  E_COMMUNICATION,
  E_PUMP,
  E_MAX
};

enum OPERATION_MODE
{
  AUTO,
  MANUAL
};

enum MODE_CONTROL
{
  MC_STOP = 0,
  MC_FAST = 5,
  MC_NORMAL = 10,
  MC_EXTRA = 30,
  MC_DRAIN = 60
};


typedef struct sState
{
  int OperationMode;
  int OperationState;
  int WaterLevel;
  int ErrorID;
  int WLED_IsConnect;
  
};
sState sState;

typedef struct sControl
{
  int ModeControl;
  int RemainingTime;
  int Capa;
};
sControl sControl;


void Reset(void);
void CheckConnection(void);
int CheckWaterLevel(int avg_times);
int CheckErrorID(void);
void CheckOperationState(void);
void CtrlWatering(void);
void SendData();




/************************ User Code *********************************/

void Reset(void)
{
  sState.OperationMode = MANUAL;
  sState.OperationState = OP_POWERDOWN;
  sState.WaterLevel = WL_NORMAL;
  sState.ErrorID = E_NONE;
  sState.WLED_IsConnect = 0;

  sControl.Capa = 0;
  sControl.ModeControl = MC_STOP;
  sControl.RemainingTime = MC_STOP;

  analogWrite(PIN_OUT_CAPA_PUMP, 0);
  Blynk.virtualWrite(V_Capa, 0);
  Blynk.virtualWrite(V_ErrorID, E_NONE);
  Blynk.virtualWrite(V_ModeControl, MC_STOP);
  Blynk.virtualWrite(V_RemainingTime, MC_STOP);

}

void CheckConnection(void)
{
  if (sState.WLED_IsConnect)
  {    
    WLED_IsConnect.off();
    sState.WLED_IsConnect = 0;
    WLED_IsConnect.setColor(BLYNK_GREEN);
  }
  else
  {
    if (sState.OperationState == OP_STANDBY) 
    {
      WLED_IsConnect.setColor(BLYNK_BLUE);
    }
    else if (sState.OperationState == OP_RUNNING)
    {
      WLED_IsConnect.setColor(BLYNK_GREEN);
    }
    else
    {
      WLED_IsConnect.setColor(BLYNK_YELLOW);
    }
    WLED_IsConnect.on();
    sState.WLED_IsConnect = 1;
  }
}

int CheckWaterLevel(int avg_times)
{
  float tmp = 0.0;
  int i = avg_times;
  while(i){
    i--;
    tmp = tmp + (analogRead(PIN_IN_SENSOR)/1024) * 100;
  }

  Blynk.virtualWrite(V_ADC, tmp/avg_times);

  if (tmp/avg_times <= 1)
  {
    return WL_LOW;
  }
  else if (tmp/avg_times > 100)
  {
    return WL_HIGH;
  }
  else
  {
    return WL_NORMAL;
  }
  // return WL_NORMAL;

}

int CheckErrorID(void) // priority
{
  // Power
  // PumpDown
  if (!WiFi.status())
  {
    return E_COMMUNICATION;
  }
  // Water Level
  else if (sState.WaterLevel == WL_LOW)
  {
    return E_WATER_LOW;
  }
  else if (sState.WaterLevel == WL_HIGH)
  {
    return E_WATER_HIGH;
  }
  else
  {
    return E_NONE;
  }

  unsigned long currentMillis = millis();

  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    // Restart your board
    ESP.restart();
  }
  previousMillis = currentMillis;

}

void CheckOperationState(void)
{
  switch (sState.OperationState)
  {
    case OP_POWERDOWN:
      Reset();
      sState.OperationState = OP_STOP;
      break;

    case OP_STOP:
      if (sState.ErrorID == E_NONE)
      {
        sState.OperationState = OP_STANDBY;
      }
      break;

    case OP_STANDBY:
      if ((sControl.ModeControl != MC_STOP))
      {
        sState.OperationState = OP_RUNNING;

      }
      if (sState.ErrorID != E_NONE)
      {
        sState.OperationState = OP_STOP;
      }
      break;

    case OP_RUNNING:
      if (sControl.RemainingTime == MC_STOP)
      {
        sState.OperationState = OP_STANDBY;
      }
      if (sState.ErrorID != E_NONE)
      {
        sState.OperationState = OP_STOP;
      }
      break;

    default:
      break;
    }

}

void CtrlWatering(void)
{
  if (sState.OperationMode == MANUAL)
  {
    switch (sState.OperationState)
    {
      case OP_STOP:
        analogWrite(PIN_OUT_CAPA_PUMP, 0);
        break;

      case OP_STANDBY:
        sControl.RemainingTime = MC_STOP;
        analogWrite(PIN_OUT_CAPA_PUMP, 0);
        Blynk.virtualWrite(V_RemainingTime, sControl.RemainingTime);
        break;

      case OP_RUNNING:
        Blynk.virtualWrite(V_RemainingTime, sControl.RemainingTime);
        if (sControl.RemainingTime > MC_STOP)
        {
          analogWrite(PIN_OUT_CAPA_PUMP, sControl.Capa);
          sControl.RemainingTime--;
        }
        else
        {
          analogWrite(PIN_OUT_CAPA_PUMP, 0);
          sControl.ModeControl = MC_STOP;
          Blynk.virtualWrite(V_ModeControl, MC_STOP);
        }
        break;

      default:
        break;
      }
  }
  else
  {
    ;
  }

}



void SendData()
{
  // Check the connection between ESP and APP
  CheckConnection();

  // Check the water level  
  sState.WaterLevel = CheckWaterLevel(5);
  // Blynk.virtualWrite(V_WaterLevel, sState.WaterLevel);

  // Check Error occurs
  sState.ErrorID = CheckErrorID();
  switch (sState.ErrorID) {
    case E_NONE:
    Blynk.virtualWrite(V_ErrorID, "Hệ thống hoạt động tốt 😊"); break;
    case E_WATER_LOW:
    Blynk.virtualWrite(V_ErrorID, "Mức nước quá thấp 💧"); break;
    case E_WATER_HIGH:
    Blynk.virtualWrite(V_ErrorID, "Mức nước quá cao 💧"); break;
    case E_PIPE:
    Blynk.virtualWrite(V_ErrorID, "Ống nước bị nghẽn 💧"); break;
    case E_POWER:
    Blynk.virtualWrite(V_ErrorID, "Nguồn không ổn định 🔌"); break;
    case E_COMMUNICATION:
    Blynk.virtualWrite(V_ErrorID, "Kết nối không ổn định ✈️"); break;
    case E_PUMP:
    Blynk.virtualWrite(V_ErrorID, "Bơm nước không hoạt động ⛽"); break;
    default: break;
  }

  // Check Operation state
  CheckOperationState();
  switch (sState.OperationState) {
    case OP_POWERDOWN:
    Blynk.virtualWrite(V_OperationState, "POWERDOWN"); break;
    case OP_STANDBY:
    Blynk.virtualWrite(V_OperationState, "STANDBY"); break;
    case OP_RUNNING:
    Blynk.virtualWrite(V_OperationState, "RUNNING"); break;
    case OP_STOP:
    Blynk.virtualWrite(V_OperationState, "STOP"); break;
    default: break;
  }

  // Control Water Plant
  CtrlWatering();


}



void setup()
{
  // Init system
  Serial.begin(115200);
  char auth[] = BLYNK_AUTH_TOKEN;
  //char ssid[] = "HTH 2";
  //char pass[] = "22222222";
  char ssid[] = "HgtA";
  char pass[] = "11111112";
  Blynk.begin(auth, ssid, pass);
  
  // PWM config
  analogWriteRange(1023);
  analogWriteFreq(16000);

  // GPIO config
  pinMode(PIN_IN_SENSOR, INPUT);
  pinMode(PIN_OUT_CAPA_PUMP, OUTPUT);

  // Init parameters
  Reset();

  // Call back function
  timer.setInterval(1000L, SendData);
}





BLYNK_WRITE(V_OperationMode)
{
  Reset();
  sState.OperationMode = param.asInt();
}

BLYNK_WRITE(V_ModeControl)
{
  sControl.ModeControl = param.asInt();

  switch(sControl.ModeControl)
  {
    case MC_STOP:
      sControl.RemainingTime = MC_STOP;
      break;

    case MC_FAST:
      sControl.RemainingTime = MC_FAST;
      break;

    case MC_NORMAL:
      sControl.RemainingTime = MC_NORMAL;
      break;
      
    case MC_EXTRA:
      sControl.RemainingTime = MC_EXTRA;
      break;
      
    case MC_DRAIN:
      sControl.RemainingTime = MC_DRAIN;
      break;
      
    default:
    break;
  }
}

BLYNK_WRITE(V_Capa)
{
  sControl.Capa = 1023 * param.asInt();
  Blynk.virtualWrite(V_OperationState,  sControl.Capa);
}

BLYNK_WRITE(V_Emergency)
{
  Reset();
}



void loop()
{
  Blynk.run();
  timer.run();
}
