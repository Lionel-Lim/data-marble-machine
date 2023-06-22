#define MOTOR_PIN D2 // Specify the motor pin
#define CHANNEL 0 // Specify the LEDC channel to be used
#define FREQUENCY 5000 // Specify the PWM frequency
#define RESOLUTION 8 // Specify the resolution in bits (range of the duty cycle)

void setup() {
  Serial.begin(115200);
  ledcSetup(CHANNEL, FREQUENCY, RESOLUTION); // Set up the channel, frequency and resolution
  ledcAttachPin(MOTOR_PIN, CHANNEL); // Attach the pin to the channel
}

void loop() {
  Serial.println("Loop Started");
  // Rotate the motor at full speed
  ledcWrite(CHANNEL, 255); // 100% duty cycle
  delay(2000);

  // Stop the motor
  ledcWrite(CHANNEL, 0); // 0% duty cycle
  delay(2000);

  // Rotate the motor at half speed
  ledcWrite(CHANNEL, 127); // 50% duty cycle
  delay(2000);
}
