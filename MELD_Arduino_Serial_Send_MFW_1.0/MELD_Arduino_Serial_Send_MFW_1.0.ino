//----------------------------------------
    /* 
      Encoder Index:
      - Encoder: X
      -PushButton: A
      
      Cherry Index:
      - Button #1: B
      */
//----------------------------------------
    
  static int encoderL = 0; //number of Encoder Left GPIO
  static int encoderR = 1; //number of Encoder Right GPIO
  
  const int A = 4; //number of Encoder Button GPIO
  const int B = 5; //number of Cherry Button #1 GPIO
  
  int buttonStateA = LOW; //variables for button status'
  int buttonStateB = LOW;
  int lastButtonStateA = LOW; //variables for last button status'
  int lastButtonStateB = LOW; 
  boolean isToggledA = false; //variables for toggled button
  boolean isToggledB = false;
  
  long debounceDelay = 150; //the debounce time
  long lastDebounceTime = 0; //the last time output was toggled
  
  volatile byte aFlag = 0; //variables to signal encoder at indent
  volatile byte bFlag = 0; 
  volatile byte encoderPos = 0; //variable to store encoder position
  volatile byte oldEncPos = 0; //variable to store previous encoder position
  volatile byte reading = 0; //variable to store valus from interrupts before checking detent moves
  
//----------------------------------------

void setup(){
  pinMode(encoderL, INPUT_PULLUP); //initialise as inputs, using in-built HIGH pull to logic voltage
  pinMode(encoderR, INPUT_PULLUP);
  pinMode(A, INPUT); //initialise as inputs
  pinMode(B, INPUT);
  
  attachInterrupt(2, PinA, RISING); //set interrupts, looking for rising edge signals
  attachInterrupt(3, PinB, RISING);
  
  Serial.begin(19200); //start serial link, set Baudrate 
}

//----------------------------------------

void PinA(){
 cli(); //stop interrupt before pin read
 reading = PIND & 0xC; //strip all values but encoderLR
 
 if(reading == B00001100 && aFlag){
  encoderPos--; //decrease encoderPos count
  aFlag = 0; //reset flags
  bFlag = 0;
 }
 else if(reading == B00000100) bFlag = 1; //encoderR signal transition
 sei(); //restart interrupts
}

void PinB(){
 cli();
 reading = PIND & 0xC;
 
 if(reading == B00001100 && bFlag){
  encoderPos++; //increase encoderPos count
  aFlag = 0;
  bFlag = 0;
 }
 else if (reading == B00001000) aFlag = 1;
 sei();
}

//----------------------------------------

void loop(){
  buttonStateA = digitalRead(A); //read state of buttons
  buttonStateB = digitalRead(B);
  
  if((millis() - lastDebounceTime) > debounceDelay){ //filter bounce with time delay
     if(buttonStateA == HIGH && buttonStateA != lastButtonStateA){ //continue if buttonState is HIGH & different to the last state
       isToggledA = !isToggledA; //change state of isToggled
       lastDebounceTime = millis(); //set the current time 
     }
     
     if(buttonStateB == HIGH && buttonStateB != lastButtonStateB){
       isToggledB = !isToggledB;
       lastDebounceTime = millis(); 
     }
     
     if(buttonStateA != lastButtonStateA || buttonStateB != lastButtonStateB || encoderPos != oldEncPos){ //continue only the state changes
       Serial.print("["); //print special character to signal start of new input stream
       Serial.print("X"); //print data identifier
       Serial.print(encoderPos); //print data value
       //Serial.print(" "); 
       Serial.print("A");
       Serial.print(isToggledA);
       //Serial.print(" ");
       Serial.print("B");
       Serial.print(isToggledB);
       Serial.println("]"); //print special character to signal end of input stream
     }
     
     if(oldEncPos != encoderPos){
       oldEncPos = encoderPos;
     }
     
     lastButtonStateA = buttonStateA; //assign the last state to the current
     lastButtonStateB = buttonStateB;
  }
}
