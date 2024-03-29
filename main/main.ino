#define BLYNK_TEMPLATE_ID "TMPLh6RSrMPB"
#define BLYNK_DEVICE_NAME "Smart Watering"
#define BLYNK_AUTH_TOKEN "TGFKyDNnYmW9KyjD1nR_okdLGH2vkxic"

#define BLYNK_WIFI_SSID "HTH 2"
#define BLYNK_WIFI_PASS "22222222"
// #define BLYNK_WIFI_SSID "HgtA"
// #define BLYNK_WIFI_PASS "11111112"
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = BLYNK_WIFI_SSID;
char pass[] = BLYNK_WIFI_PASS;

#define BLYNK_FIRMWARE_VERSION    "0.6.1"

#define APP_DEBUG
#define USE_NODE_MCU_BOARD

#include "main.h"
#include "Edgent_ESP8266/BlynkEdgent.h"

BlynkTimer timer;
WidgetLED  WLED_IsConnect(V_Connect);




/************************ User Code - Time Lock *********************************/

int GetStatusLock(int LockID)
{
  return sLock[LockID].status;
}

int GetTimeLock(int LockID)
{
  return sLock[LockID].time;
}

void SetLock(int LockID, int time)
{
  sLock[LockID].status = 1;
  sLock[LockID].time = time;
  sLock[LockID].count++;
}

void ReleaseLock(int LockID)
{
  if ( sLock[LockID].status)
  {
    sLock[LockID].status = 0;
    sLock[LockID].time = 0;
  }
}

void ControlTimeLock(void) // 1s
{
  for (int i = 0; i < E_MAX; i++)
  {
    if(sLock[i].time > 0)
    {
      sLock[i].time--;

      if (sLock[i].time == 0)
      {
        ReleaseLock(i);
      }
    }
  }
}




/************************ User Code - Operation *********************************/

void Reset(void)
{
  sState.Connection = WIFI_CONNECTED;
  sState.ErrorID = E_NONE;
  sState.OperationMode = MANUAL;
  sState.OperationState = OP_POWERDOWN;
  sState.WaterLevel = WL_NORMAL;

  sControl.Capa = 0;
  sControl.ModeControl = MC_STOP;
  sControl.RemainingTime = MC_STOP;
  sControl.HighPerformance = NONE;

  analogWrite(PIN_OUT_CAPA_PUMP, 0);
  Blynk.virtualWrite(V_Capa, 0);
  Blynk.virtualWrite(V_ErrorID, E_NONE);
  Blynk.virtualWrite(V_ModeControl, MC_STOP);
  Blynk.virtualWrite(V_RemainingTime, MC_STOP);
  Blynk.virtualWrite(V_HighPerformance, NONE);
}

void CheckConnection(void)
{
  static bool BlinkLed = true;
  static int  CountErr = 0;

  if (Blynk.connected())
  {
    sState.Connection = WIFI_CONNECTED;
    ReleaseLock[E_COMMUNICATION];
    CountErr = 0;
  }
  else // Disconnect to Blynk server
  {
    if (++CountErr < 10)
    {
      sState.Connection = WIFI_UNCONNECTED;
      SetLock(E_COMMUNICATION, 10);
    }
    else
    {
      analogWrite(PIN_OUT_CAPA_PUMP, 1023*0);

      Blynk.begin(auth, ssid, pass);
      CountErr = 0;
    }
  }

  if (BlinkLed)
  {    
    WLED_IsConnect.off();
    BlinkLed = false;
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
    BlinkLed = true;
  }
}

void CheckWaterLevel(int avg_times)
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
    sState.WaterLevel = WL_LOW;
    SetLock(E_WATER_LOW, 1);
  }
  else if (tmp/avg_times > 100)
  {
    sState.WaterLevel = WL_HIGH;
    SetLock(E_WATER_HIGH, 1);
  }
  else
  {
    sState.WaterLevel = WL_NORMAL;
  }
  // return WL_NORMAL;
}

void SendMessageError(String msg)
{
  Blynk.virtualWrite(V_ErrorID, msg);
}

void CheckErrorID(void) // priority
{
  String msg = "";

  for (int i = 0; i < E_MAX; i++)
  {
    if (sLock[i].status)
    {
      sState.ErrorID = sLock[i].status;

      switch (i)
      {
        // case E_NONE:
        // msg = "Hệ thống hoạt động tốt 😊"; break;
        case E_WATER_LOW:
        msg += "Mức nước quá thấp 💧 __  "; break;
        case E_WATER_HIGH:
        msg += "Mức nước quá cao 💧 __  "; break;
        case E_PIPE:
        msg += "Ống nước bị nghẽn 💧 __  "; break;
        case E_POWER:
        msg +=  "Nguồn không ổn định 🔌 __  "; break;
        case E_COMMUNICATION:
        msg +=  "Kết nối không ổn định ✈️ __  "; break;
        case E_PUMP:
        msg +=  "Bơm nước không hoạt động ⛽ __  "; break;
        case E_EMERGENCY:
        msg +=  "Hệ thống buộc dừng khẩn cấp 🤾‍♂️ __  "; break;
        
        default: break;
      }
    }
  }

  if (msg == "")
  {
    sState.ErrorID = E_NONE;
    msg = "Hệ thống hoạt động tốt 😊 __  ";
  }

  SendMessageError(msg);

  // Power
  // PumpDown

}

void SendMessageOpState(String msg)
{
  Blynk.virtualWrite(V_OperationState, msg);
}


void CheckOperationState(void)
{
  String msg;

  switch (sState.OperationState)
  {
    case OP_POWERDOWN:
      msg = "POWERDOWN";
      Reset();
      sState.OperationState = OP_STOP;
      break;

    case OP_STOP:
      msg = "STOP";
      if (sState.ErrorID == E_NONE)
      {
        sState.OperationState = OP_STANDBY;
      }
      break;

    case OP_STANDBY:
      msg = "STANDBY";
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
      msg = "RUNNING";
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

  SendMessageOpState(msg);

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




/************************ User Code - Schedule *********************************/

void SendData()
{
  ControlTimeLock();

  // Check the connection between ESP and APP
  CheckConnection();

  // Check the water level  
  CheckWaterLevel(5);
  // Blynk.virtualWrite(V_WaterLevel, sState.WaterLevel);

  // Check Error occurs
  CheckErrorID();

  // Check Operation state
  CheckOperationState();

  // Control Water Plant
  CtrlWatering();

}


void setup()
{
  // Init system
  Serial.begin(115200);

  Blynk.begin(auth, ssid, pass);
  BlynkEdgent.begin();
  
  // PWM config
  analogWriteRange(1023);
  analogWriteFreq(16000);

  // GPIO config
  pinMode(PIN_IN_SENSOR, INPUT);
  pinMode(PIN_OUT_CAPA_PUMP, OUTPUT);

  // Init parameters
  Reset();
  SendMessageError("Init System");

  // Call back function
  timer.setInterval(1000L, SendData);
}



/************************ Blynk App *********************************/

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
  double tmp = param.asDouble();
  
  if (tmp && !sControl.HighPerformance)
  {
    // y = ax + b
    tmp =  (1.0/900)*tmp + (593.0/990) ;

  }
  else
  {
    tmp =  (1.0/900)*tmp + (593.0/990) + 0.29;
  }

  sControl.Capa = 1023 * tmp;
}

BLYNK_WRITE(V_HighPerformance)
{
  sControl.HighPerformance = param.asInt();
}

BLYNK_WRITE(V_Emergency)
{
  if (param.asInt())
  {
    SetLock(E_EMERGENCY, 3600);
//    SendMessageError("Hệ thống buộc dừng khẩn cấp 🤾‍♂️");
    Reset();
  }
  else
  {
    ReleaseLock(E_EMERGENCY);
  }
}


void loop()
{
  Blynk.run();
  BlynkEdgent.run();
  timer.run();
}
