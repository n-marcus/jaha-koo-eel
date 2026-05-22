#include "sbus.h"
#include <HardwareSerial.h>

// Use UART1 to avoid conflict with USB Serial (UART0)
HardwareSerial MySerial1(1);

/* SBUS object, reading SBUS */
// D7 = GPIO20 (RX), D6 = GPIO21 (TX) on Seeed XIAO ESP32-C3
bfs::SbusRx sbus_rx(&MySerial1, D7, D6, true, false);

/* SBUS data */
bfs::SbusData data;

#define SBUS_PACKET_PRINT_INTERVAL 20 // ms (originally 100ms)
u_long sbusPacketPrintPrevTime = 0;

u_long sbusPrevPacketTime;
bool sbusLost = false;

// Debug counters
u_long lastDebugTime = 0;
#define DEBUG_INTERVAL 500 // Print debug info every 500ms
int totalBytesReceived = 0;

// Calibration tracking
int16_t channelMin[16];
int16_t channelMax[16];
bool calibrationInitialized = false;

#define SBUS_VAL_MIN 176  // 191
#define SBUS_VAL_MAX 1808 // 1793
#define SBUS_VAL_CENTER 992
#define SBUS_VAL_DEADBAND 6
#define SBUS_LOST_TIMEOUT 100
#define SBUS_SWITCH_MIN 192
#define SBUS_SWITCH_MAX 1792
#define SBUS_SWITCH_MIN_THRESHOLD 1400
#define SBUS_SWITCH_MAX_THRESHOLD 550

#define TX_ROLL 0
#define TX_PITCH 1
#define TX_THROTTLE 2
#define TX_YAW 3
#define TX_AUX1 4
#define TX_AUX2 5
#define TX_AUX3 6
#define TX_AUX4 7

void initELRSRX();
void resetSbusData();
void parseSBUS(bool serialPrint);

void initELRSRX()
{
  /* Begin the SBUS communication */

  Serial.println("\n=== Initializing SBUS ===");
  Serial.print("RX Pin D7 = GPIO");
  Serial.println(D7);
  Serial.print("TX Pin D6 = GPIO");
  Serial.println(D6);
  Serial.println("Using UART1 (not UART0 which is USB Serial)");

  // TODO -> define the RX as input_pullup, so that we might prevent / circumvent the bootloader mode error?
  pinMode(D7, INPUT_PULLUP); // pull up the RX pin

  Serial.print("D7 initial state (should be 1): ");
  Serial.println(digitalRead(D7));

  Serial.println("\n** HARDWARE TEST **");
  Serial.println("Briefly touch D7 to GND to test if pin reading works...");
  delay(3000);
  Serial.print("D7 state after 3 sec: ");
  Serial.println(digitalRead(D7));
  Serial.println("** END HARDWARE TEST **\n");

  sbus_rx.Begin();
  // sbus_tx.Begin();

  Serial.println("SBUS Begin() called on UART1 - waiting for data...");
  Serial.println("Expected: 100000 baud, 8E2, inverted signal");
  Serial.println("=========================\n");

  // by default, let's have the program assume sbus is lost
  sbusPrevPacketTime = -SBUS_LOST_TIMEOUT;
}

void resetSbusData()
{
  for (int8_t i = 0; i < data.NUM_CH; i++)
  {
    data.ch[i] = SBUS_VAL_MIN;
    if (i < 4)
    {
      data.ch[i] = (SBUS_VAL_MIN + SBUS_VAL_MAX) / 2;
      if (i == TX_YAW)
        data.ch[i] = SBUS_VAL_MIN;
    }
  }
}

void parseSBUS(bool serialPrint)
{
  // Low-level debug: Check if ANY bytes are available
  static int debugCounter = 0;
  debugCounter++;

  int bytesAvailable = MySerial1.available();
  if (bytesAvailable > 0)
  {
    totalBytesReceived += bytesAvailable;
    // Serial.print("RAW DATA! Bytes available: ");
    // Serial.print(bytesAvailable);
    // Serial.print(" | Total: ");
    // Serial.println(totalBytesReceived);

    // Read and dump first few bytes in hex
    // Serial.print("Hex: ");
    // for (int i = 0; i < min(bytesAvailable, 25); i++)
    // {
    //   if (MySerial1.available())
    //   {
    //     byte b = MySerial1.read();
    //     if (b < 0x10)
    //       Serial.print("0");
    //     Serial.print(b, HEX);
    //     Serial.print(" ");
    //   }
    // }
    // Serial.println();
  }

  // Periodic calibration display
  if (millis() - lastDebugTime > DEBUG_INTERVAL && calibrationInitialized)
  {
    // Serial.println("\n=== CHANNEL CALIBRATION ===");

    // // Main control channels
    // Serial.print("ROLL     (Ch0): [");
    // Serial.print(data.ch[TX_ROLL]);
    // Serial.print("] (");
    // Serial.print(channelMin[TX_ROLL]);
    // Serial.print("-");
    // Serial.print(channelMax[TX_ROLL]);
    // Serial.println(")");

    // Serial.print("PITCH    (Ch1): [");
    // Serial.print(data.ch[TX_PITCH]);
    // Serial.print("] (");
    // Serial.print(channelMin[TX_PITCH]);
    // Serial.print("-");
    // Serial.print(channelMax[TX_PITCH]);
    // Serial.println(")");

    // Serial.print("THROTTLE (Ch2): [");
    // Serial.print(data.ch[TX_THROTTLE]);
    // Serial.print("] (");
    // Serial.print(channelMin[TX_THROTTLE]);
    // Serial.print("-");
    // Serial.print(channelMax[TX_THROTTLE]);
    // Serial.println(")");

    // Serial.print("YAW      (Ch3): [");
    // Serial.print(data.ch[TX_YAW]);
    // Serial.print("] (");
    // Serial.print(channelMin[TX_YAW]);
    // Serial.print("-");
    // Serial.print(channelMax[TX_YAW]);
    // Serial.println(")");

    // // Aux channels
    // for (int i = 4; i < 8; i++)
    // {
    //   Serial.print("AUX");
    //   Serial.print(i - 3);
    //   Serial.print("     (Ch");
    //   Serial.print(i);
    //   Serial.print("): [");
    //   Serial.print(data.ch[i]);
    //   Serial.print("] (");
    //   Serial.print(channelMin[i]);
    //   Serial.print("-");
    //   Serial.print(channelMax[i]);
    //   Serial.println(")");
    // }

    // Serial.print("\nFlags: Lost=");
    // Serial.print(data.lost_frame);
    // Serial.print(" Failsafe=");
    // Serial.println(data.failsafe);
    // Serial.println("===========================");

    lastDebugTime = millis();
  }

  if (sbus_rx.Read())
  {
    sbusPrevPacketTime = millis();
    if (connectionState == CONNECTION_LOST || connectionState == DISCONNECTED)
    {
      Serial.println("Regained SBUS connection");
      connectionState = CONNECTED;
    }

    /* Grab the received data */
    data = sbus_rx.data();

    // Initialize calibration arrays on first packet
    if (!calibrationInitialized)
    {
      for (int i = 0; i < 16; i++)
      {
        channelMin[i] = 2000;
        channelMax[i] = 0;
      }
      calibrationInitialized = true;
      Serial.println("\n*** CALIBRATION MODE ***");
      Serial.println("Move all sticks to their extremes to calibrate range");
      Serial.println("Format: CH# [Current] (Min-Max)\n");
    }

    // Track min/max for calibration
    for (int8_t i = 0; i < data.NUM_CH; i++)
    {
      if (data.ch[i] < channelMin[i])
        channelMin[i] = data.ch[i];
      if (data.ch[i] > channelMax[i])
        channelMax[i] = data.ch[i];
    }
  }
  // No else block - reduces spam when no packets

  // if SBUS lost, reset the channels
  if (millis() - sbusPrevPacketTime > SBUS_LOST_TIMEOUT)
  {
    if (connectionState != DISCONNECTED)
    {
      Serial.print("Lost SBUS connection >> setting throttle/pitch/roll to ");
      Serial.print((SBUS_VAL_MIN + SBUS_VAL_MAX) / 2);
      Serial.println(" and yaw to 0 ");

      connectionState = DISCONNECTED;
    }
  }

  // if (connectionState == CONNECTION_LOST)
  // {
  //   resetSbusData();
  // }
}
