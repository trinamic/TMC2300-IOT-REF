#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include "include/Functions.h"
#include "include/TMC2300.h"
#include "include/CRC.h"

/******************************************************************************/
// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "TMC-WLAN";
char pass[] = "trinamicmotioncontrol";

// Auth Token for the Blynk App.
char auth[] = "mMiEvpo2mzj7UXmH1ALbPZVn-jHREMmh";
/******************************************************************************/

#define VIRTUAL_PIN_CURRENT    1
#define VIRTUAL_PIN_VELOCITY   2
#define VIRTUAL_PIN_DIRECTION  3
#define VIRTUAL_PIN_ENABLE     4

BlynkTimer timer;

int targetVelocity = 0;
bool direction = true;
bool enable = false;

/******************************************************************************/
// These functions are called whenever the corresponding virtual pin is updated
// in the Blynk app

BLYNK_WRITE(VIRTUAL_PIN_CURRENT)
{
  Serial.print("New MaxCurrent set: ");
  Serial.println(param.asInt());
  
  uint32_t value = 1 << TMC2300_IHOLDDELAY_SHIFT
                 | ((param.asInt() << TMC2300_IRUN_SHIFT) & TMC2300_IRUN_MASK)
                 | 8 << TMC2300_IHOLD_SHIFT;
  tmc2300_writeInt(TMC2300_IHOLD_IRUN, value);
}

BLYNK_WRITE(VIRTUAL_PIN_VELOCITY)
{
  Serial.print("New TargetVelocity set: ");
  Serial.println(param.asInt());

  targetVelocity = param.asInt();
  
  tmc2300_writeInt(TMC2300_VACTUAL, direction? targetVelocity : -targetVelocity);
}

BLYNK_WRITE(VIRTUAL_PIN_DIRECTION)
{
  Serial.println("Changed Direction");
  
  direction = param.asInt() != 0;

  tmc2300_writeInt(TMC2300_VACTUAL, direction? targetVelocity : -targetVelocity);
}


BLYNK_WRITE(VIRTUAL_PIN_ENABLE)
{
  enable = param.asInt() != 0;
  
  if (enable)
  {
    Serial.println("Enable Motor: True");
  }
  else
  {
    Serial.println("Enable Motor: False");
  }
  
  digitalWrite(32, enable? HIGH:LOW);
}

/******************************************************************************/

// Called once per second by the Blynk timer
void periodicJob()
{
  if (enable)
  {
    // Toggle the status LED while the motor is active
    digitalWrite(18, HIGH);
    delay(250);
    digitalWrite(18, LOW);
    delay(250);
    digitalWrite(18, HIGH);
    delay(250);
    digitalWrite(18, LOW);
  }

  // Re-write the CHOPCONF register periodically
  tmc2300_writeInt(TMC2300_CHOPCONF, 0x14008001);
}

void setup()
{
  // Debug console
  Serial.begin(115200);

  // TMC2300 IC UART connection
  Serial1.begin(115200);

  // Status LED
  pinMode(18, OUTPUT);

  // Enable Pin
  pinMode(32, OUTPUT);

  // Initialize CRC calculation for TMC2300 UART datagrams
  tmc_fillCRC8Table(0x07, true, 0);

  // Connect to the WiFi access point
  Serial.println("Connecting to the WiFi access point");
  Blynk.connectWiFi(ssid, pass);

  // Connect to the Blynk server
  Serial.println("Authentificating with the Blynk server");
  Blynk.config(auth);

  Serial.println("Initialization complete");

  // Start the timer for the periodic function
  timer.setInterval(1000L, periodicJob);
}

void loop()
{
  Blynk.run();
  timer.run();
}
