#include <ESP8266WiFi.h>
//#include <BlynkSimpleEsp8266.h>



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

enum ERROR_ID // priority
{
  E_NONE,
  E_PIPE,
  E_WATER_HIGH,
  E_WATER_LOW,
  E_POWER,
  E_PUMP,
  E_EMERGENCY,
  E_COMMUNICATION,
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

enum WIFI
{
  WIFI_UNCONNECTED,
  WIFI_CONNECTED,
  WIFI_MAX
};


typedef struct sState
{
  int OperationMode;
  int OperationState;
  int WaterLevel;
  int Connection;
  int ErrorID;
};
sState sState;


typedef struct sControl
{
  int ModeControl;
  int RemainingTime;
  int Capa;
};
sControl sControl;

typedef struct sLock
{
  int status;
  int time;
  int count;
};


sLock sLock[E_MAX] = {0};

int GetStatusLock(int LockID);
int GetTimeLock(int LockID);
void SetLock(int LockID, int time);
void ReleaseLock(int LockID);
void ControlTimeLock(void); // 1s
void Reset(void);
void CheckConnection(void);
void CheckWaterLevel(int avg_times);
void SendMessageError(String msg);
void CheckErrorID(void);
void SendMessageOpState(String msg);
void CheckOperationState(void);
void CtrlWatering(void);
void SendData();