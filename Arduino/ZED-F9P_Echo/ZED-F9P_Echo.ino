// ZED-F9P Echo

// Simple serial echo for the u-blox ZED-F9P to help with firmware updates

// ** Changes the ZED-F9P Baud rate from 38400 to 9600 **

// Designed to run on the Adafruit Feather M0 Adalogger
// https://www.adafruit.com/product/2796
// https://learn.adafruit.com/adafruit-feather-m0-adalogger

// Code is based on:
// https://github.com/adafruit/Adafruit_GPS/tree/master/examples/GPS_HardwareSerial_EchoTest

// Set UART1 to 9600 Baud
// UBX-CFG-VALSET message with a key ID of 0x40520001 (CFG-UART1-BAUDRATE) and a value of 0x00002580 (9600 decimal)
static const uint8_t setUART1BAUD[] = { 0xb5, 0x62,  0x06, 0x8a,  0x0c, 0x00,  0x00, 0x01, 0x00, 0x00,  0x01, 0x00, 0x52, 0x40,  0x80, 0x25, 0x00, 0x00 };
static const int len_setUART1BAUD = 18;

// Send message in u-blox UBX format
// Calculates and appends the two checksum bytes
// Doesn't add the 0xb5 and 0x62 sync chars (these need to be included at the start of the message)
void sendUBX(const uint8_t *message, const int len) {
  int csum1 = 0; // Checksum bytes
  int csum2 = 0;
  for (int i=0; i<len; i++) { // For each byte in the message
    Serial1.write(message[i]); // Write the byte
    if (i >= 2) { // Don't include the sync chars in the checksum
      csum1 = csum1 + message[i]; // Update the checksum bytes
      csum2 = csum2 + csum1;
    }
  }
  csum1 = csum1 & 0xff; // Limit checksums to 8-bits
  csum2 = csum2 & 0xff;
  Serial1.write((uint8_t)csum1); // Send the checksum bytes
  Serial1.write((uint8_t)csum2);
}

void setup()
{
  pinMode(13, OUTPUT); // Red LED
  digitalWrite(13, LOW);
  pinMode(8, OUTPUT); // Green LED
  digitalWrite(8, LOW);

  // Give the ZED-F9P time to power up
  delay(2000);

  Serial1.begin(38400); // Start off using 38400 Baud to talk to the ZED-F9P

  // Change the ZED-F9P UART Baud rate
  sendUBX(setUART1BAUD, len_setUART1BAUD); // Set ZED-F9P UART1 baud rate to 9600

  // Allow time for Baud rate change
  delay(1000);

  // Restart serial communications at 9600 Baud
  Serial1.begin(9600);
}
     
void loop() // run over and over again
{
  if (Serial.available()) {
    digitalWrite(13, HIGH); // Flash red LED
    char c = Serial.read();
    Serial1.write(c);
    digitalWrite(13, LOW);
  }
  if (Serial1.available()) {
    digitalWrite(8, HIGH); // Flash green LED
    char c = Serial1.read();
    Serial.write(c);
    digitalWrite(8, LOW);
  }  
}
