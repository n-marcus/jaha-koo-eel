int16_t noodVals[] = {0, 0, 0, 0};
int16_t noodAvgVals[] = {0, 0, 0, 0};
uint16_t n00dSegmentIdentifiers[] = {512, 640, 768, 896};

// Tracking structure for each nood channel
struct NoodDebugInfo
{
  int16_t lastRawSBUS;
  uint16_t lastMappedThrottle;
  uint8_t lastIntensity;
  int16_t lastCalculated;
  unsigned long lastUpdateTime;
  uint16_t updateCount;
};
NoodDebugInfo noodDebug[4] = {{0}};

// this is the value amount that we subtract from 127, to allow some deadband between the nood values.
// Effectively, determines the resolution of the noods. 0 is no deadband, 127 is max deadband.
#define NOOD_VALUES_TRANSMISSION_BANDWIDTH 24
#define NOOD_VALUES_BANDWIDTH (127 - NOOD_VALUES_TRANSMISSION_BANDWIDTH)

#define n00d_1a_Pin D8
#define n00d_1b_Pin D1
#define n00d_2a_Pin D10
#define n00d_2b_Pin D0

static const uint8_t nood1a_chan = 0;
static const uint8_t nood1b_chan = 1;
static const uint8_t nood2a_chan = 2;
static const uint8_t nood2b_chan = 3;

void initNoods();
void updateBodyLightValues();
void setBodyLights();
void setn00d(uint8_t chan, uint8_t val);

void initNoods()
{
  pinMode(n00d_1a_Pin, OUTPUT);
  pinMode(n00d_1b_Pin, OUTPUT);
  pinMode(n00d_2a_Pin, OUTPUT);
  pinMode(n00d_2b_Pin, OUTPUT);

  // nood 1a
  ledcSetup(nood1a_chan, freq, resolution);
  ledcAttachPin(n00d_1a_Pin, nood1a_chan);

  // nood 1b
  ledcSetup(nood1b_chan, freq, resolution);
  ledcAttachPin(n00d_1b_Pin, nood1b_chan);

  // nood 2a
  ledcSetup(nood2a_chan, freq, resolution);
  ledcAttachPin(n00d_2a_Pin, nood2a_chan);

  // nood 2b
  ledcSetup(nood2b_chan, freq, resolution);
  ledcAttachPin(n00d_2b_Pin, nood2b_chan);

  setn00d(nood1a_chan, 0);
  setn00d(nood1b_chan, 0);
  setn00d(nood2a_chan, 0);
  setn00d(nood2b_chan, 0);
}

void updateBodyLightValues()
{
  // Track min/max throttle for calibration
  static int16_t throttleMin = 2000;
  static int16_t throttleMax = 0;

  if (data.ch[TX_THROTTLE] < throttleMin)
    throttleMin = data.ch[TX_THROTTLE];
  if (data.ch[TX_THROTTLE] > throttleMax)
    throttleMax = data.ch[TX_THROTTLE];

  // map throttle range to 512-1023 (noods only use upper half)

  // throttle = map(data.ch[TX_THROTTLE], 465, 1907, 512, 1023); keep for later use
  throttle = map(data.ch[TX_THROTTLE], 1005, 1790, 512, 1023);

  throttleAdjusted = 0; // used to store the adjusted throttle value

  // if (throttle & (0x1 << 9)) // 512
  // if (throttle & n00dSegmentIdentifiers[0] == n00dSegmentIdentifiers[0]) // 512
  if (throttle >> 7 == 0x4) // 0b100 aka nood1a
  {
    throttleAdjusted = throttle - 9;
    n00d1a = (throttle & 0x7F) - 9;
    n00d1a = map(constrain(n00d1a, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

    // Track debug info
    noodDebug[0].lastRawSBUS = data.ch[TX_THROTTLE];
    noodDebug[0].lastMappedThrottle = throttle;
    noodDebug[0].lastIntensity = throttle & 0x7F;
    noodDebug[0].lastCalculated = n00d1a;
    noodDebug[0].lastUpdateTime = millis();
    noodDebug[0].updateCount++;
  }
  // if (throttle & (0x1 << 8)) // 256
  // if (throttle & n00dSegmentIdentifiers[1] == n00dSegmentIdentifiers[1]) // 640
  if (throttle >> 7 == 0x5) // 0b101 aka nood1b
  {
    throttleAdjusted = throttle - 10;
    n00d1b = (throttle & 0x7F) - 10;
    n00d1b = map(constrain(n00d1b, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

    // Track debug info
    noodDebug[1].lastRawSBUS = data.ch[TX_THROTTLE];
    noodDebug[1].lastMappedThrottle = throttle;
    noodDebug[1].lastIntensity = throttle & 0x7F;
    noodDebug[1].lastCalculated = n00d1b;
    noodDebug[1].lastUpdateTime = millis();
    noodDebug[1].updateCount++;
  }
  // if (throttle & (0x1 << 7)) // 128
  // if (throttle & n00dSegmentIdentifiers[2] == n00dSegmentIdentifiers[2]) // 768
  if (throttle >> 7 == 0x6) // 0b110 aka nood2a
  {
    throttleAdjusted = throttle - 9;
    n00d2a = (throttle & 0x7F) - 9;
    n00d2a = map(constrain(n00d2a, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

    // Track debug info
    noodDebug[2].lastRawSBUS = data.ch[TX_THROTTLE];
    noodDebug[2].lastMappedThrottle = throttle;
    noodDebug[2].lastIntensity = throttle & 0x7F;
    noodDebug[2].lastCalculated = n00d2a;
    noodDebug[2].lastUpdateTime = millis();
    noodDebug[2].updateCount++;
  }
  // if (throttle & (0x1 << 6)) // 64
  // if (throttle & n00dSegmentIdentifiers[3] == n00dSegmentIdentifiers[3]) // 896
  if (throttle >> 7 == 0x7) // 0b111 aka nood2b
  {
    throttleAdjusted = throttle - 9;
    n00d2b = (throttle & 0x7F) - 9;
    n00d2b = map(constrain(n00d2b, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

    // Track debug info
    noodDebug[3].lastRawSBUS = data.ch[TX_THROTTLE];
    noodDebug[3].lastMappedThrottle = throttle;
    noodDebug[3].lastIntensity = throttle & 0x7F;
    noodDebug[3].lastCalculated = n00d2b;
    noodDebug[3].lastUpdateTime = millis();
    noodDebug[3].updateCount++;
  }

  noodVals[0] = n00d1a;
  noodVals[1] = n00d1b;
  noodVals[2] = n00d2a;
  noodVals[3] = n00d2b;

  // smoothed values
  noodAvgVals[0] = 0.85 * noodAvgVals[0] + 0.15 * noodVals[0];
  noodAvgVals[1] = 0.85 * noodAvgVals[1] + 0.15 * noodVals[1];
  noodAvgVals[2] = 0.85 * noodAvgVals[2] + 0.15 * noodVals[2];
  noodAvgVals[3] = 0.85 * noodAvgVals[3] + 0.15 * noodVals[3];

  // Debug print nood values every 500ms
  static unsigned long lastNoodDebug = 0;
  if (millis() - lastNoodDebug > 500)
  {
    Serial.println("\n========== NOOD ROUND-ROBIN DEBUG ==========");
    Serial.print("SBUS Throttle Range: [");
    Serial.print(throttleMin);
    Serial.print(" - ");
    Serial.print(throttleMax);
    Serial.println("]");

    Serial.println("Current throttle value: " + String(data.ch[TX_THROTTLE]) + " | Mapped: " + String(throttle) + " | Adjusted: " + String(throttleAdjusted));

    // Current segment being received
    uint8_t segment = throttle >> 7;
    Serial.print("Current segment: 0x");
    Serial.print(segment, HEX);
    Serial.print(" (");
    if (segment == 0x4)
      Serial.print("Nood1a");
    else if (segment == 0x5)
      Serial.print("Nood1b");
    else if (segment == 0x6)
      Serial.print("Nood2a");
    else if (segment == 0x7)
      Serial.print("Nood2b");
    else
      Serial.print("OUT OF RANGE");
    Serial.println(")");

    Serial.println("\nChan | RawSBUS | Mapped | Intens | Calc | Smooth | Updates | Age(ms)");
    Serial.println("-----|---------|--------|--------|------|--------|---------|--------");

    for (int i = 0; i < 4; i++)
    {
      Serial.print(i == 0 ? "1a   | " : (i == 1 ? "1b   | " : (i == 2 ? "2a   | " : "2b   | ")));
      Serial.print(noodDebug[i].lastRawSBUS);
      Serial.print("    | ");
      Serial.print(noodDebug[i].lastMappedThrottle);
      Serial.print("     | ");
      Serial.print(noodDebug[i].lastIntensity);
      Serial.print("     | ");
      Serial.print(noodDebug[i].lastCalculated);
      Serial.print("   | ");
      Serial.print(noodAvgVals[i]);
      Serial.print("     | ");
      Serial.print(noodDebug[i].updateCount);
      Serial.print("      | ");
      Serial.println(millis() - noodDebug[i].lastUpdateTime);
    }

    // Calibration hint
    if (throttleMax - throttleMin > 100)
    {
      Serial.print("\nCALIBRATION: Update line 78 to:\n  throttle = map(data.ch[TX_THROTTLE], ");
      Serial.print(throttleMin);
      Serial.print(", ");
      Serial.print(throttleMax);
      Serial.println(", 0, 1023);");
    }

    Serial.println("============================================");
    lastNoodDebug = millis();
  }
}

void setBodyLights()
{
  setn00d(nood1a_chan, noodAvgVals[0]);
  setn00d(nood1b_chan, noodAvgVals[1]);
  setn00d(nood2a_chan, noodAvgVals[2]);
  setn00d(nood2b_chan, noodAvgVals[3]);
}

void setn00d(uint8_t chan, uint8_t val)
{
  ledcWrite(chan, (255 - val));
}
