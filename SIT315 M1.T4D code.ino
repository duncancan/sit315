// Global variables
const byte SENSOR1 = 2;
const byte SENSOR2 = 3;
const byte BUTTON = 7;
const byte LED_PIN = 12;
const uint8_t POT_PIN = A1;
byte sensorState;
int priorPotVal = 0;
int dwellTime; // This sets the time the LED should stay on after motion has ceased
int secondsRemaining = 0; //  This tracks how many seconds are remaining before the LED turns off, after motion has ceased.


void setup() {
  // Set our external LED pin (pin 12; bit 4 of Register B) to output mode.
  DDRB = B00010000; 
  // Set the internal LED pin  (pin 13; bit 5 of Register B) to output mode too.
  DDRB |= B00100000;
  // Set our sensor/button pins to pullup
  PORTD = B10001100; 

  // Set up our interrupts
  PCICR = B00000100; // Enable PCIE2
  PCMSK2 = B10001100; // Enable pin change interrupts on the INT0 and INT1 pins (pins 1 & 2, confusingly) and also another one - the last pin of Register D.

  // Open our serial port to print debugging info
  Serial.begin(9600);

}

// Our loop just periodically calls setDwellTime, which reads the potentiometer 
void loop() {
  setDwellTime();
  delay(100);
}

// Our function to turn on the LED, which is called when the interrupt is triggered.
void triggerSensorLight() {
  digitalWrite(LED_PIN, HIGH);
  startTimer();
}

// Function to read our potentiometer and set the dwell time of the LED to a value of 1-30 seconds based on the pot's position.
void setDwellTime() {
  int potVal = analogRead(POT_PIN);
  if (potVal != priorPotVal) {
    dwellTime = map(potVal, 0, 1023, 1, 30);

    // Print to serial for debugging purposes.
    Serial.print("Dwell time: ");
    Serial.print(dwellTime);
    Serial.println(" seconds.");
    
    priorPotVal = potVal;
  }
}

// A one-second timer. This This timer decrements the dwellTime, which allows us to have longer timers, e.g. up to 30 seconds.
void startTimer(){
  noInterrupts();
  
  TCCR1A = 0;
  TCCR1B = B00000101; // Set prescaler to 1024, per ATmega8A datasheet
  TCCR1B |= (1 << WGM12); // Set CTC ('clear timer on compare')
  TCNT1 = 0;
  OCR1A = 16000000 / (1024) - 1; // One second threshold
  TIMSK1 |= (1 << OCIE1A); // Set COMPA interrupt
  
  interrupts();
}

// When timer interrupt triggers every second, this code runs
ISR(TIMER1_COMPA_vect){
  
  // Motion on either sensor counts as ongoing motion.
  byte motionOngoing = digitalRead(SENSOR1)|| digitalRead(SENSOR2);
  // If motion is still ongoing, don't start counting down the seconds remaining on the LED.
  if (!motionOngoing) {
    // If our timer's hit zero, turn the LED off
    if (secondsRemaining == 0) {
      digitalWrite(LED_PIN, LOW);
      return;
    }
    // Otherwise, decrement the seconds remaining and start another one-second timer.
    secondsRemaining -= 1;
  	startTimer();
  } else {
    // If motion is ongoing, reset the seconds remaining on the LED to the dwell time
    secondsRemaining = dwellTime;
  }
  
}

// Handle our register D interrupts
ISR(PCINT2_vect) {
  // Read our interrupt pins
  byte sensor1State = digitalRead(SENSOR1);
  byte sensor2State = digitalRead(SENSOR2);
  byte buttonToggle = digitalRead(BUTTON);
  
  // Handle motion sensors and on light if there's motion
  if (sensor1State || sensor2State) {
    triggerSensorLight();
  }
  
  // Handle our button press by toggling the internal LED
  if (buttonToggle) {
    PORTB ^= B00100000; // Toggle pin 13 (bit 5 of Register B)
  }
  
  // Print stuff to serial for debugging purposes.
  Serial.print("Interrupt triggered. Sensor 1 state: ");
  Serial.print(sensor1State);
  Serial.print(". Sensor 2 state: ");
  Serial.print(sensor2State);
  Serial.print(". Toggle button state: ");
  Serial.println(buttonToggle);
}