#include <SPI.h>
 
volatile boolean dataReady = false;
char receivedData[100];
volatile int index = 0;
String responseData = "Response from slave"; // Sample response message
 
void setup() {
  //SPDR = responseData[0];
  pinMode(MISO, OUTPUT);
  SPCR |= _BV(SPE); // Enable SPI as slave
  SPI.attachInterrupt(); // Enable SPI interrupt
  Serial.begin(9600); // Start serial communication for debugging
}
 
ISR(SPI_STC_vect) {
  char c = SPDR; // Read character from SPI data register
 
  // Store received character in buffer
  if (index < sizeof(receivedData) - 1) {
    receivedData[index++] = c;
  }
  // for(int i = 0; i < 20; i++){
  //   char c = SPI.transfer(0x00);
  //   if (c == '\0')
  //     break;

  //   recievedData += c;
  // }
 
  // End of message or buffer full
  if (c == '\0' || index >= sizeof(receivedData) - 1) {
    receivedData[index] = '\0'; // Null-terminate the string
    dataReady = true; // Set flag indicating data is ready
    index = 0; // Reset index for next message
  }
 
  // Send response data character by character
  static size_t responseIndex = 0;
  if (responseIndex < responseData.length()) {
    SPDR = responseData[responseIndex++];
  } else {
    SPDR = '\0'; // Send null character when done
    responseIndex = 0; // Reset response index
  }
}
 
void loop() {
  if (dataReady) {
    if (receivedData[0] != '\0') { // Check if data is not empty
      Serial.print("Received from master: ");
      Serial.println(receivedData); // Print received data
    }
    dataReady = false; // Reset flag
  }
}