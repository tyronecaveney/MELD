//----------------------------------------
    /* 
      Encoder Index:
      - Encoder: X
      - PushButton: A
      
      Cherry Index:
      - Button #1: B
      - Button #2: C
      - Button #3: D
      - Button #4: E
      - Button #5: F
      */
//----------------------------------------
    
  static int encoderL = 0; //number of Encoder Left GPIO
  static int encoderR = 1; //number of Encoder Right GPIO
  
  const int A = 2; //number of Encoder Button GPIO
  const int B = 3; //number of Cherry Button #1 GPIO
  const int C = 5;  //number of Cherry Button #2 GPIO
  const int D = 6;  //number of Cherry Button #3 GPIO
  const int E = 18;  //number of Cherry Button #4 GPIO
  const int F = 20; //number of Cherry Button #5 GPIO
  
  int buttonStateA = LOW; //variables for button status'
  int buttonStateB = LOW;
  int buttonStateC = LOW;
  int buttonStateD = LOW;
  int buttonStateE = LOW;
  int buttonStateF = LOW;

  int lastButtonStateA = LOW; //variables for last button status'
  int lastButtonStateB = LOW;
  int lastButtonStateC = LOW;
  int lastButtonStateD = LOW;
  int lastButtonStateE = LOW;
  int lastButtonStateF = LOW;

  boolean isToggledA = false; //variables for toggled button
  boolean isToggledB = false;
  boolean isToggledC = false;
  boolean isToggledD = false;
  boolean isToggledE = false;
  boolean isToggledF = false;
  
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
  pinMode(C, INPUT);
  pinMode(D, INPUT);
  pinMode(E, INPUT);
  pinMode(F, INPUT);
  
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
  buttonStateC = digitalRead(C);
  buttonStateD = digitalRead(D);
  buttonStateE = digitalRead(E);
  buttonStateF = digitalRead(F);
  
   if((millis() - lastDebounceTime) > debounceDelay){ //filter bounce with time delay
     if(buttonStateA != lastButtonStateA){ //continue if buttonState is HIGH & different to the last state
       Serial.print("A");
       Serial.print(buttonStateA);
       Serial.println("]");
       lastDebounceTime = millis(); //set the current time 
     }
     if(buttonStateB == HIGH && buttonStateB != lastButtonStateB){
       isToggledB = !isToggledB;
       Serial.print("B");
       Serial.print(isToggledB);
       Serial.println("]");
       lastDebounceTime = millis(); 
     } 
     if(buttonStateC != lastButtonStateC){
       Serial.print("C");
       Serial.print(buttonStateC);
       Serial.println("]");
       lastDebounceTime = millis(); 
     } 
     if(buttonStateD == HIGH && buttonStateD != lastButtonStateD){
       isToggledD = !isToggledD;
       Serial.print("D");
       Serial.print(isToggledD);
       Serial.println("]");
       lastDebounceTime = millis(); 
     }  
     if(buttonStateE == HIGH && buttonStateE != lastButtonStateE){
       isToggledE = !isToggledE;
       Serial.print("E");
       Serial.print(isToggledE);
       Serial.println("]");
       lastDebounceTime = millis(); 
     } 
     if(buttonStateF == HIGH && buttonStateF != lastButtonStateF){
       isToggledF = !isToggledF;
       Serial.print("F");
       Serial.print(isToggledF);
       Serial.println("]");
       lastDebounceTime = millis(); 
     }

      if(encoderPos != oldEncPos){
       Serial.print("X");
       Serial.print(encoderPos);
       Serial.println("]"); 
      }
        
       if(oldEncPos != encoderPos){
       oldEncPos = encoderPos;
     }
     
     lastButtonStateA = buttonStateA; //assign the last state to the current
     lastButtonStateB = buttonStateB;
     lastButtonStateC = buttonStateC;
     lastButtonStateD = buttonStateD;
     lastButtonStateE = buttonStateE;
     lastButtonStateF = buttonStateF;
  }
}
