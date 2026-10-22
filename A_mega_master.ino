// BOARD A: ELEGOO Mega 2560 (SPI master)
// Reads the LSM303 accelerometer exactly as before, prints it on the Serial Monitor,
// and now also packages X/Y/Z into an 8-byte packet and sends it to board B over SPI.
//
// Wiring (A -> B):
//   Mega 52 (SCK)  -> Uno 13
//   Mega 51 (MOSI) -> Uno 11
//   Mega 50 (MISO) -> Uno 12
//   Mega 53 (SS)   -> Uno 10
//   Mega GND       -> Uno GND

#include <Wire.h>
#include <SPI.h>                    // NEW
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

// NEW: SPI settings and packet layout
const uint8_t SS_PIN      = 53;     // Mega's hardware SS pin
const uint8_t PACKET_SIZE = 8;
const uint8_t START_BYTE  = 0xAA;   // marks the beginning of a packet
// Packet: [0xAA][x_high][x_low][y_high][y_low][z_high][z_low][checksum]

void displaySensorDetails(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

// NEW: package the three readings and send them to board B
void sendPacket(float ax, float ay, float az)
{
  // Multiply by 100 to keep two decimal places, then store as 16-bit integers
  int16_t x = (int16_t)(ax * 100);
  int16_t y = (int16_t)(ay * 100);
  int16_t z = (int16_t)(az * 100);

  uint8_t packet[PACKET_SIZE];
  packet[0] = START_BYTE;
  packet[1] = highByte(x);
  packet[2] = lowByte(x);
  packet[3] = highByte(y);
  packet[4] = lowByte(y);
  packet[5] = highByte(z);
  packet[6] = lowByte(z);

  // Checksum: a check value B recalculates to detect corrupted bytes
  uint8_t checksum = 0;
  for (uint8_t i = 1; i <= 6; i++) checksum ^= packet[i];
  packet[7] = checksum;

  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
  digitalWrite(SS_PIN, LOW);        // LOW = "board B, listen"
  delayMicroseconds(50);
  for (uint8_t i = 0; i < PACKET_SIZE; i++)
  {
    SPI.transfer(packet[i]);
    delayMicroseconds(50);          // gives B time to store each byte
  }
  digitalWrite(SS_PIN, HIGH);       // HIGH = "done"
  SPI.endTransaction();
}

void setup(void)
{
#ifndef ESP8266
  while (!Serial);     // will pause Zero, Leonardo, etc until serial console opens
#endif
  Serial.begin(9600);
  Serial.println("Accelerometer Test"); Serial.println("");

  // NEW: start SPI as master
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  SPI.begin();

  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }

  /* Display some basic information on this sensor */
  displaySensorDetails();
}

void loop(void)
{
  /* Get a new sensor event */
  sensors_event_t event;
  accel.getEvent(&event);

  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");

  /* NEW: send the same values to board B */
  sendPacket(event.acceleration.x, event.acceleration.y, event.acceleration.z);

  /* Delay before the next sample */
  delay(500);
}
