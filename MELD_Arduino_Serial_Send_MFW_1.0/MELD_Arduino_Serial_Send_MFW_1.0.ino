/*
    --------------------PERIPHERALS CODE--------------------
    Encoder w/PushButton Index:
    - Encoder: LR
    - PushButton: A

    Cherry Buttons Index:
    - Button #1: B
    - Button #2: C
    - Button #3: D
    - Button #4: E
    - Button #5: F
    - Button #6: G   
*/
const int A = 4; //number of encoderButton GPIO

const int B = 5; //number of Cherry Button #1 GPIO

long lastDebounceTime = 0; //the last time the output was toggled
long debounceDelay = 150; //the debounce time, increase if output flickers

static int encoderL = 2; //Encoder Pin 1
static int encoderR = 3; //Encoder Pin 2

int encoderButtonState = LOW; //varibale for button status
int lastEncoderButtonState = LOW; //variable for previous button status
boolean encoderButtonisToggled = false; //variable for toggle of button

int buttonStateB = LOW; //variable for button status
int lastButtonStateB = LOW; //variable for previous button state
boolean isToggledB = false; //variable for toggle of button

volatile byte aFlag = 0; //Variable to signal encoder is at an indent
volatile byte bFlag = 0; //Variable to signal encoder is at an indent
volatile byte encoderPos = 0; //Variable to store Encoder Position
volatile byte oldEncPos = 0; //Variable to store last Encoder Postion
volatile byte reading = 0; //To store direct values from interrupt pins before checking whole detent move
//------------------------------------------------------------------------------------------
void setup() 
{
  pinMode(A, INPUT); //initialise Button as Input
  pinMode(encoderL, INPUT_PULLUP); //set as inputs, using inbuilt HIGH pull to logic voltage
  pinMode(encoderR, INPUT_PULLUP);

  pinMode(B, INPUT); //initialise button as input

  attachInterrupt(0,PinA,RISING); //set interrupts, looking for rising edge signals
  attachInterrupt(1,PinB,RISING);

  Serial.begin(57600); //start serial monitor link
  Serial.println("------Start------");
}
//------------------------------------------------------------------------------------------
 void PinA()
 {
   cli(); //stop interrupts before reading pin values
   reading = PIND & 0xC; //read all pin values and strip all but encoderLR

   if(reading == B00001100 && aFlag)
   {
    encoderPos --; //decrease encoder pos count
    aFlag = 0; //reset 
    bFlag = 0; //reset
   }
   else if(reading == B00000100) bFlag = 1; //expect encoderR to signal detent transition
   sei(); //restart interrupts
 }

void PinB()
{
  cli();
  reading = PIND & 0xC;

  if(reading == B00001100 && bFlag)
  {
    encoderPos ++; //increase encoder pos count
    aFlag = 0; //reset
    bFlag = 0; //reset
  }
  else if(reading == B00001000) aFlag = 1; //expect encoderL to signal detent transition
  sei(); 
}
//------------------------------------------------------------------------------------------
void loop() 
{
  encoderButtonState = digitalRead(A); //read state of button
  buttonStateB = digitalRead(B);

  if((millis() - lastDebounceTime) > debounceDelay) //filter out any noise by setting a time buffer
  {
    
    //only continue if buttonState is high and different to the lastButtonState
    if(encoderButtonState == HIGH && encoderButtonState != lastEncoderButtonState) 
    {
      encoderButtonisToggled = !encoderButtonisToggled; //isToggled state is changed
      lastDebounceTime = millis(); //set the current time
    }
    if(buttonStateB == HIGH && buttonStateB != lastButtonStateB)
    {
      isToggledB = !isToggledB;
      lastDebounceTime = millis();
    }

    if(encoderButtonState != lastEncoderButtonState || buttonStateB != lastButtonStateB || encoderPos != oldEncPos)
    {
    Serial.print("LR");
    Serial.print(encoderPos);
    Serial.print("\t");
    Serial.print("A");
    Serial.print(encoderButtonisToggled);
    Serial.print("\t"); //print tab space
    Serial.print("B");
    Serial.println(isToggledB);
    }

    if(oldEncPos != encoderPos)
    {
      oldEncPos = encoderPos;
    }

  //make the value change once per click, even if held down
  lastEncoderButtonState = encoderButtonState;
  lastButtonStateB = buttonStateB; 
  }
}
