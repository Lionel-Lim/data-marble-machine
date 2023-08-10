#define BUTTON1_PIN 25
#define BUTTON2_PIN 26

void setup() {
  pinMode(BUTTON1_PIN, INPUT_PULLUP); // Initialize Button 1 as input with pull-up resistor
  pinMode(BUTTON2_PIN, INPUT_PULLUP); // Initialize Button 2 as input with pull-up resistor
  Serial.begin(9600); // Start the serial communication
}

void loop() {
  int button1State = digitalRead(BUTTON1_PIN); // Read the state of Button 1
  int button2State = digitalRead(BUTTON2_PIN); // Read the state of Button 2
  
  if (button1State == LOW) {
    Serial.println("Button 1 is pressed");
  }

  if (button2State == LOW) {
    Serial.println("Button 2 is pressed");
  }

  delay(10); // Short delay to debounce the button
}
