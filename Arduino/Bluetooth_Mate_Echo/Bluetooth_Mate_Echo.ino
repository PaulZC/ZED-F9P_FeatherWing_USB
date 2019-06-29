// Bluetooth Mate Echo

// Simple serial echo for the SparkFun Bluetooth Mate (Gold)

// Designed to run on the Adafruit Feather M0 Adalogger
// https://www.adafruit.com/product/2796
// https://learn.adafruit.com/adafruit-feather-m0-adalogger

void setup()
{
  Serial1.begin(115200);
  Serial.begin(115200);

  pinMode(13, OUTPUT); // Red LED
  pinMode(8, OUTPUT); // Green LED

  // Wait for user to open the serial monitor
  // Comment out if you want the code to run without waiting
  while (!Serial);
}
     
void loop() // run over and over again
{
  if (Serial.available()) {
    digitalWrite(13, HIGH); // Flash red LED
    Serial1.write(Serial.read());
    digitalWrite(13, LOW);
  }
  if (Serial1.available()) {
    digitalWrite(8, HIGH); // Flash green LED
    Serial.write(Serial1.read());
    digitalWrite(8, LOW);
  }  
}
