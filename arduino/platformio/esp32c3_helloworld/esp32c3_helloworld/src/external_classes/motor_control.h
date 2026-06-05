/*
 * MOTOR CALIBRATION GUIDE:
 *
 * 1. Watch the serial monitor for channel calibration output
 * 2. Move pitch stick full forward/back to see value ranges
 * 3. Check MOTOR DEBUG output to see M1/M2 values
 *
 * DIRECTION FIXES:
 * - If BOTH motors go backwards when stick forward:
 *   -> In calcMotorValues(), flip the comparison: < to > or vice versa
 *      (line ~82: "if ((data.ch[TX_PITCH] - SBUS_VAL_CENTER) > 0)")
 *
 * - If ONE motor goes wrong direction:
 *   -> In initMotors(), toggle that motor's .reversed flag
 *
 * - If left/right steering is backwards:
 *   -> Swap motor1Val and motor2Val assignments
 *      OR swap mixLeft/mixRight in the calculations
 *
 * RANGE FIXES:
 * - Update SBUS_VAL_MIN and SBUS_VAL_MAX in elrs_rx.h based on calibration output
 * - Center should be around 992, typical range is 172-1811
 */

#include <EelMotor.h>

#define motor1a_pin D5
#define motor1b_pin D4
#define motor2a_pin D3
#define motor2b_pin D2

#define motor1_chan 4 // 6 Channels (ESP32-C3) (0-5) are availible
#define motor2_chan 5 // 6 Channels (ESP32-C3) (0-5) are availible

EelMotor motor1(motor1a_pin, motor1b_pin, motor1_chan, resolution, freq);
EelMotor motor2(motor2a_pin, motor2b_pin, motor2_chan, resolution, freq);

int16_t motor1Val;
int16_t motor2Val;

// Motor smoothing variables for remote control mode
float motor1Smoothed = 0;
float motor2Smoothed = 0;
const float motorSmoothingFactor = 0.15; // 0.0 = no change, 1.0 = instant change. Try 0.1-0.3 for smooth ramping

float motorIntensity = 0.8;

int16_t n00d1a, n00d1b, n00d2a, n00d2b;
uint16_t throttle, throttleAdjusted;

void initMotors();
void calcMotorValues();
void driveMotors();

void initMotors()
{
  // Motor reverse flags - swap these if individual motors go wrong way
  motor1.reversed = true; // CHANGED: try false first
  motor2.reversed = true; // CHANGED: try false first

  Serial.println("Motors initialized:");
  Serial.print("  Motor1 reversed: ");
  Serial.println(motor1.reversed);
  Serial.print("  Motor2 reversed: ");
  Serial.println(motor2.reversed);
}

uint8_t motorStrength;

void calcMotorValues()
{
  byte motorPowerRange = 255;
  byte motorOutMode = 0;

  //* position of AUX4 selects diff motor power ranges
  if (data.ch[TX_AUX4] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    motorOutMode = 0; // linear
    motorPowerRange = 255;
  }
  else if (data.ch[TX_AUX4] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    motorOutMode = 0; // linear
    motorPowerRange = 200;
  }
  else
  {
    motorOutMode = 1; // wiggle
    motorPowerRange = 255;
  }
  //*/

  int16_t mix = 1000;

  /* position of AUX4 selects diff motor output modes
  if (data.ch[TX_AUX4] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    motorOutMode = 0;
  }
  else if (data.ch[TX_AUX4] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    motorOutMode = 1;
  }
  else
  {
    motorOutMode = 2;
  }
  //*/

  // calc drive strength and determine fwd/rev direction
  uint16_t pitchOffset = abs(data.ch[TX_PITCH] - SBUS_VAL_CENTER);
  uint16_t rollOffset = abs(data.ch[TX_ROLL] - SBUS_VAL_CENTER_ROLL);
  double stickDistFromCenter = sqrt(pow(pitchOffset, 2) + pow(rollOffset, 2));
  int16_t motorStrength = constrain(map(stickDistFromCenter, 0, 800, 0, motorPowerRange), 0, motorPowerRange);

  // PITCH DIRECTION: Invert this logic if motors go wrong direction
  if (pitchOffset > SBUS_VAL_DEADBAND)
  {
    // CHANGE THIS LINE to flip forward/backward
    // Current: pitch > center = negative (reverse)
    // To flip: change > to <
    if ((data.ch[TX_PITCH] - SBUS_VAL_CENTER) < 0) // Flipped back to <
    {
      motorStrength = -motorStrength;
    }
  }

  if (motorOutMode == 0)
  {

    //*/
    // motor1Val = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
    // motor2Val = motor1Val;

    int16_t throttleVal = motorStrength;
    EVERY_N_MILLIS(250)
    {
      // Serial.print("motorPowerRange: " + String(motorPowerRange));
      // Serial.println(", throttleVal: " + String(motor2Val));
    }

    // float mix = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1000), 750, 250) / 1000.0; // gives a range of .25-.75

    // /*
    // range 0.0-1.0, then an exponent, then map to 250-750
    mix = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN_ROLL, SBUS_VAL_MAX_ROLL, 0, 2000), 0, 2000); // gives a range of 0-2000
    // mix = pow(mix, 1.4);
    // mix = map((mix * 1000.0), 1000, 0, 250, 750) / 1000.0; // reverse the input range because we want to reverse the steering
    // mix = map((mix * 1000.0), 1000, 0, 0, 1000) / 1000.0; // reverse the input range because we want to reverse the steering
    //*/

    // shitty hack
    uint16_t mixLeft = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER_ROLL, SBUS_VAL_MIN_ROLL, 1000, 0), 0, 1000);
    uint16_t mixRight = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER_ROLL, SBUS_VAL_MAX_ROLL, 1000, 0), 0, 1000);

    // motor1Val = (int16_t)((throttleVal * (2000 - mix)) / mulVal);
    // motor2Val = (int16_t)((throttleVal * mix) / mulVal);
    motor1Val = (int16_t)((throttleVal * mixRight) / 1000);
    motor2Val = (int16_t)((throttleVal * mixLeft) / 1000);

    // taper off motor strength as pitch stick gets closer to center
    if (pitchOffset < 75)
    {
      motor1Val *= map(pitchOffset, 0, 75, 350, 1000);
      motor1Val /= 1000;

      motor2Val *= map(pitchOffset, 0, 75, 350, 1000);
      motor2Val /= 1000;
    }
  }

  // wiggle mode: left/right stick deflection yields amplified motor diff strength.
  if (motorOutMode == 1)
  {
    int16_t throttleVal = motorStrength;

    // shitty hack
    int16_t mixLeft = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER_ROLL, SBUS_VAL_MIN_ROLL, 1000, -1000), -1000, 1000);
    int16_t mixRight = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER_ROLL, SBUS_VAL_MAX_ROLL, 1000, -1000), -1000, 1000);

    motor1Val = (int16_t)((throttleVal * mixRight) / 1000);
    motor2Val = (int16_t)((throttleVal * mixLeft) / 1000);

    // taper off motor strength as pitch stick gets closer to center
    if (pitchOffset < 75)
    {
      motor1Val *= map(pitchOffset, 0, 75, 350, 1000);
      motor1Val /= 1000;

      motor2Val *= map(pitchOffset, 0, 75, 350, 1000);
      motor2Val /= 1000;
    }
  }

  /*
  doSineMovement = (data.ch[TX_AUX4] < SBUS_SWITCH_MIN_THRESHOLD) ? true : false;
  doSineMovement = false; // override

  // only applies if doSineMovement is true
  // sinCounterIncrement = map(data.ch[TX_THROTTLE], SBUS_VAL_MIN, SBUS_VAL_MAX, 200, 1000) / 5000.0;
  sinCounterIncrement = 550 / 5000.0; // override for testing
  float sinMulFactor = .9;
  sinMulFactor = constrain(map(data.ch[TX_AUX3], SBUS_VAL_MIN, SBUS_VAL_MAX, 450, 900), 450, 900) / 1000.0;
  float sinMult1 = sin(sinCounter) * sinMulFactor + (1.0 - sinMulFactor);
  float sinMult2 = sin(sinCounter + PI) * sinMulFactor + (1.0 - sinMulFactor);
  sinCounter += sinCounterIncrement;
  // Serial.println("sinCounterIncrement: " + String(sinCounterIncrement));

  if (doSineMovement)
  {
    float ampMul = constrain(map(data.ch[TX_AUX4], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1000), 0, 1000) / 1000.0;
    motor1Val *= (sinMult1 * ampMul) + (1.0 - ampMul);
    motor2Val *= (sinMult2 * ampMul) + (1.0 - ampMul);
  }
  //*/

  motor1Val = constrain(motor1Val, -255, 255);
  motor2Val = constrain(motor2Val, -255, 255);

  motor1Val = (int)(motor1Val * motorIntensity);
  motor2Val = (int)(motor2Val * motorIntensity);

  // Motor debug output
  static unsigned long lastMotorDebug = 0;
  if (millis() - lastMotorDebug > 250 && false)
  {
    Serial.print("MOTOR DEBUG >> ");
    Serial.print("Pitch: ");
    Serial.print(data.ch[TX_PITCH]);
    Serial.print(" | Roll: ");
    Serial.print(data.ch[TX_ROLL]);
    Serial.print(" | Strength: ");
    Serial.print(motorStrength);
    Serial.print(" | M1: ");
    Serial.print(motor1Val);
    Serial.print(" | M2: ");
    Serial.println(motor2Val);
    lastMotorDebug = millis();
  }
}

void driveMotors()
{
  // Apply smoothing to motor values for gradual acceleration/deceleration
  // This prevents jerky on/off behavior
  motor1Smoothed = motor1Smoothed * (1.0 - motorSmoothingFactor) + motor1Val * motorSmoothingFactor;
  motor2Smoothed = motor2Smoothed * (1.0 - motorSmoothingFactor) + motor2Val * motorSmoothingFactor;
  
  // Use smoothed values for motor control
  int16_t motor1Output = (int16_t)motor1Smoothed;
  int16_t motor2Output = (int16_t)motor2Smoothed;
  
  // MOTOR 1
  if (abs(motor1Output) < SBUS_VAL_DEADBAND)
  {
    motor1.motorStop(); // Soft Stop    -no argument
  }
  if (motor1Output < -SBUS_VAL_DEADBAND)
  {
    motor1.motorRev(-motor1Output); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
  if (motor1Output > SBUS_VAL_DEADBAND)
  {
    motor1.motorGo(motor1Output); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }

  // MOTOR 2
  if (abs(motor2Output) < SBUS_VAL_DEADBAND)
  {
    motor2.motorStop(); // Soft Stop    -no argument
  }
  if (motor2Output < -SBUS_VAL_DEADBAND)
  {
    motor2.motorRev(-motor2Output); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
  if (motor2Output > SBUS_VAL_DEADBAND)
  {
    motor2.motorGo(motor2Output); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
}