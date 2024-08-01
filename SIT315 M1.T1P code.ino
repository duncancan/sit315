
// Global variables
const uint8_t SENSOR_PIN = 2;
const uint8_t LED_PIN = 13;
byte sensorState;

// Setup code that runs once on startup
void setup()
{
  // Set up our pins
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  // Attach an interrupt to our sensor pin, and have it turn on the LED when it triggers.
  attachInterrupt(
  	digitalPinToInterrupt(SENSOR_PIN),
    triggerSensorLight,
    RISING
   );
  
  // Open our serial port to print debugging info
  Serial.begin(9600);
}

// Not much happens in our loop, because everything's handled by interrupts. This just
// prints the current sensor state to serial for debugging purposes.
void loop() {
  sensorState = digitalRead(SENSOR_PIN);
  Serial.println(sensorState);
  delay(500);
}

// Our function to turn on the LED, which is called when the interrupt is triggered.
void triggerSensorLight() {
  // Debugging line to check out interrupt is triggering
  Serial.println("Interrupt triggered");
  // Turn the LED on
  digitalWrite(LED_PIN, HIGH);
  // And start a timer so the LED stays on for a little while.
  startTimer(0.5);
}

// Takes parameter freq in Hz and sets up timer with this frequency
void startTimer(double seconds){
  noInterrupts();
  
  TCCR1A = 0;
  TCCR1B = B00000101; // Set prescaler to 1024, per ATmega8A datasheet
  TCCR1B |= (1 << WGM12); // Set CTC ('clear timer on compare')
  TCNT1 = 0;
  OCR1A = 16000000 / (1024 * (1 / seconds)) - 1;
  TIMSK1 |= (1 << OCIE1A); // Set COMPA interrupt
  
  interrupts();
}

// Once the timer has completed, turn the LED back off.
ISR(TIMER1_COMPA_vect){
  byte motionOngoing = digitalRead(SENSOR_PIN);
  if (!motionOngoing)
  	digitalWrite(LED_PIN, LOW);
}
