// BOARD B: ELEGOO Uno R3 (SPI slave)
// Receives the 8-byte packet from board A, checks it, and prints X/Y/Z on this board's
// own Serial Monitor (9600 baud).
//
// Wiring (A -> B):
//   Mega 52 (SCK)  -> Uno 13
//   Mega 51 (MOSI) -> Uno 11
//   Mega 50 (MISO) -> Uno 12
//   Mega 53 (SS)   -> Uno 10
//   Mega GND       -> Uno GND

#include <SPI.h>

const uint8_t PACKET_SIZE = 8;
const uint8_t START_BYTE  = 0xAA;
// Packet: [0xAA][x_high][x_low][y_high][y_low][z_high][z_low][checksum]

volatile uint8_t rx[PACKET_SIZE];   // bytes received so far
volatile uint8_t rxCount = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(MISO, OUTPUT);            // required for slave mode (pin 12)
  SPCR |= _BV(SPE);                 // turn SPI on; the master bit stays 0, so this board is the slave
  SPI.attachInterrupt();            // run the function below every time a byte arrives
  Serial.println("Board B ready");
}

// Interrupt Service Routine (ISR): runs automatically for each received byte
ISR(SPI_STC_vect)
{
  uint8_t b = SPDR;
  if (rxCount < PACKET_SIZE)
  {
    rx[rxCount++] = b;
  }
}

int16_t toInt16(uint8_t high, uint8_t low)
{
  return (int16_t)(((uint16_t)high << 8) | low);
}

void loop()
{
  // SS (pin 10) goes HIGH when A has finished sending
  if (digitalRead(SS) == HIGH && rxCount > 0)
  {
    uint8_t packet[PACKET_SIZE];
    uint8_t received = rxCount;
    for (uint8_t i = 0; i < PACKET_SIZE; i++) packet[i] = rx[i];
    rxCount = 0;                    // get ready for the next packet

    uint8_t checksum = 0;
    for (uint8_t i = 1; i <= 6; i++) checksum ^= packet[i];

    if (received == PACKET_SIZE && packet[0] == START_BYTE && checksum == packet[7])
    {
      float x = toInt16(packet[1], packet[2]) / 100.0;
      float y = toInt16(packet[3], packet[4]) / 100.0;
      float z = toInt16(packet[5], packet[6]) / 100.0;

      Serial.print("X: "); Serial.print(x); Serial.print("  ");
      Serial.print("Y: "); Serial.print(y); Serial.print("  ");
      Serial.print("Z: "); Serial.print(z); Serial.println("  m/s^2");
    }
    else
    {
      Serial.println("Bad packet (wrong length or checksum)");
    }
  }
}
