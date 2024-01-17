#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <elapsedMillis.h>


//ESP32 -WROOM-32
  #define TFT_DC         17
  #define TFT_CS         5
  #define TFT_SCLK       18
  #define TFT_RST        21
  #define TFT_MOSI       23

const int tftHeight = 240;
const int tftWidth = 320;

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//const char onBoardLED = 2;
const char implantServoPin = 13;
const char spServoPin = 12;
const char solenoidPin = 14;
const char contactPin = 27;

//photo interrupter pins
const char photoInt1 = 33;
const char photoInt2 = 25;
const char photoInt3 = 26;

//stepper driver pins
const char i1 = 16, i2 = 4;
const char stepPin = 2, dirPin = 15;
const char sleepPin = 22;
//const char MS1 = 26, MS2 = 25;

//joystick pins
const char joySW = 32;
const char joyY = 34;
const char joyX = 35;

//time variables
typedef struct{
  unsigned long elapsedTime;
  unsigned long currentTime;
  unsigned long previousTime;
} nonBlockingStruct;

nonBlockingStruct timeObj1 = {0,0,0};
nonBlockingStruct timeObj2 = {0,0,0};
nonBlockingStruct timeObj3 = {0,0,0};

typedef struct{
  int freq;
  char ledChannel;  //16 channels on the ESP32 in total
  char resolution;
  int targetAng;
} pwmParameters;

pwmParameters implantServoParams = {330,0,8,90};
pwmParameters spServoParams = {330,1,8,90};

typedef struct{
  volatile char buttonPressState;
  char xValue;
  char yValue;
  int tempVal;
} joystickParameters;

joystickParameters joystickParams = {0, 2, 2, 0};

typedef struct{
  int measuredDist;
  char stepperDirection;
  int measuredImplantAng;
  int measuredSpAng;
  char contactDetected;
  char tempVal;
} measurementStruct;

measurementStruct measureObj = {0,0,0,0,0,0};

typedef struct{
  char currentState;
  char executeOnceState;  //checks if the execute once code has been run, 1 if has been run
  char sampleNo;
}stateParams;

stateParams stateObj = {0,0,0};

void ledBlinkTest();
void stateMachine();
void servoCtrl(char servo, int16_t angle);
void stepperMtrCtrl(char direction, char noSteps);
void readJoystick();
void mainMenuDisplay();
void menuTemplate(String title, String description, String option1, String option2);

void pwmSetup(){
  ledcSetup(implantServoParams.ledChannel, implantServoParams.freq, implantServoParams.resolution);  //configuring the LED
  ledcAttachPin(implantServoPin, implantServoParams.ledChannel); //attaches the LED channel to a specific GPIO pin
  //ledcWrite(implantServoParams.ledChannel, 200);  2nd parameter is the duty cycle (0-225 representing 0-100%)

  ledcSetup(spServoParams.ledChannel, spServoParams.freq, spServoParams.resolution);  //configuring the LED
  ledcAttachPin(spServoPin, spServoParams.ledChannel); //attaches the LED channel to a specific GPIO pin
}

void lcdSetup(){
  tft.init(240, 320);  
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);
  tft.setCursor(0,0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Fittable Population"); tft.println("Automation Setup");

  tft.setCursor( 0, (tftHeight)/2);
  tft.setTextSize(2);
  tft.println("Press joystick to start");
}

void distancePage(){  //display the current distance between the SP and implant
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);

  tft.setTextColor(ST77XX_WHITE, ST7735_BLACK);
  tft.setCursor(0,0);
  tft.print("Sample No: "); tft.println(stateObj.sampleNo, DEC);
  tft.println("Implant-SP Distance");
  
  tft.setCursor(0,tftHeight/2);
  tft.print("Distance: "); //tft.print(measuredDist); 
  tft.print(measureObj.measuredDist);
  tft.println(" mm");
}

void rotationPage(){  //display the current angles of the implant and SP circles
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0,0);

  tft.print("Sample No: "); tft.println(stateObj.sampleNo, DEC);
  tft.println("Servo Rotation Angle(s)");

  tft.setCursor(0,tftHeight/2);
  tft.print("Implant Angle: "); tft.print(implantServoParams.targetAng); 
  tft.println(" deg");

  tft.print("SP Angle: ");  tft.print(spServoParams.targetAng); 
  tft.println(" deg");
}

void stepperDriverSetup(char stepMode, float currentLimit){
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(sleepPin, OUTPUT);  digitalWrite(sleepPin, LOW);

  //choose stepping mode (full, half, quarter, etc..)
  //full step mode by default
  /*
  pinMode(MS1,OUTPUT);  pinMode(MS2, OUTPUT);
  switch(stepMode)
  {
    case 0: //full step - default
      digitalWrite(MS1, LOW); digitalWrite(MS2, LOW); 
      break;  
    
    case 1: //Half
      digitalWrite(MS1, HIGH); digitalWrite(MS2, LOW); 
      break;  

    case 2: //Quarter
    digitalWrite(MS1, LOW); digitalWrite(MS2, HIGH); 
    break;  

    case 3: //Eighth
    digitalWrite(MS1, HIGH); digitalWrite(MS2, HIGH); 
    break;  
  }
  */

  //choosing the current limit (Amps)
  if(currentLimit > 0 && currentLimit <= 0.5)
  {
    pinMode(i1, INPUT);   pinMode(i2, INPUT);
  }
  else if(currentLimit > 0.5 && currentLimit <= 1)
  {
    pinMode(i1, OUTPUT);  digitalWrite(i1, LOW);
    pinMode(i2, INPUT);
  }
  else if(currentLimit > 1.5 && currentLimit <= 2)
  {
    pinMode(i1, INPUT);
    pinMode(i2, OUTPUT);  digitalWrite(i2, LOW);
  }
  else if(currentLimit > 2)
  {
    pinMode(i1, OUTPUT);  digitalWrite(i1, LOW);
    pinMode(i2, OUTPUT);  digitalWrite(i2, LOW);
  }

}

void IRAM_ATTR joySW_INT(){
  //debouncing
  timeObj1.currentTime = millis();
  timeObj1.elapsedTime = timeObj1.currentTime - timeObj1.previousTime;
  if(timeObj1.elapsedTime>500)
  {
    timeObj1.previousTime = millis();
    timeObj1.elapsedTime = 0;
    joystickParams.buttonPressState = 1;
    //digitalWrite(sleepPin, !digitalRead(sleepPin));
  }
}

void IRAM_ATTR distanceCounter(){
  timeObj3.currentTime = millis();
  timeObj3.elapsedTime = timeObj3.currentTime - timeObj3.previousTime;
  enum stepperDir{BACKWARD, FORWARD};
  
  
    if(timeObj3.elapsedTime > 150)
    {
      timeObj3.previousTime = millis();
      timeObj3.elapsedTime = 0;
      if(measureObj.stepperDirection == BACKWARD)
      {
        measureObj.measuredDist--;
      }
      else if(measureObj.stepperDirection == FORWARD)
      {
        measureObj.measuredDist++;
      }
    }
  
}

void IRAM_ATTR implantAngCounter(){

}

void IRAM_ATTR spAngCounter(){

}


void IRAM_ATTR contactInt(){
  if(measureObj.contactDetected == 0)
  {
    measureObj.measuredDist = 0;  //reset the distance counter
  }

  measureObj.contactDetected = 1;

  //retract solenoid upon contact (redacted)
  //digitalWrite(solenoidPin, LOW);
}

void joystickSetup(){
  pinMode(joySW, INPUT_PULLUP);
  pinMode(joyX, INPUT);
  pinMode(joyY, INPUT);

  //setting interurpts for the button
  attachInterrupt(joySW, joySW_INT,FALLING);
}

void setup() {
  Serial.begin(9600);
  //pinMode(onBoardLED, OUTPUT);

  //pinMode(implantServoPin, OUTPUT); servoCtrl(implantServoPin, 90);
  //pinMode(spServoPin, OUTPUT); servoCtrl(spServoPin, 90);
  pwmSetup();
  pinMode(solenoidPin, OUTPUT);

  //distance counter
  pinMode(photoInt1, INPUT);
  attachInterrupt(photoInt1, distanceCounter, CHANGE);

  //implant angle counter

  //sp angle counter

  //contact sensor
  pinMode(contactPin, INPUT_PULLUP);
  attachInterrupt(contactPin, contactInt, CHANGE);

  joystickSetup();

  stepperDriverSetup(0, 0.7); //won't turn with a 0.5A limit
  //pwmSetup();

  lcdSetup();
  distancePage();
  rotationPage();

}

void loop() {
  //ledBlinkTest();
  //readJoystick();

  //joystick
  /*
  if(joystickParams.xValue != 2)
  {
    Serial.println(joystickParams.tempVal);
    joystickParams.xValue = 2;
  }
  */

  //photo interrupter
  //Serial.println(measureObj.measuredDist);

  stateMachine();
}

void readJoystick(){
  timeObj2.currentTime = millis();
  timeObj2.elapsedTime = timeObj2.currentTime - timeObj2.previousTime;

  enum yDirections{UP, DOWN, RETURN};
  enum xDirections{BACKWARD, FORWARD};
  enum pressState{PRESS_CLEARED, PRESSED};

  if(joystickParams.buttonPressState == PRESSED)
  {
    Serial.println("Button pressed");
  }

  
  if(timeObj2.elapsedTime >= 250)
  {
    //x direction
    if(analogRead(joyX) <= 10)
    {
      Serial.println("Left");
      joystickParams.xValue = BACKWARD;
      joystickParams.yValue = RETURN;
    }
    else if(analogRead(joyX) >= 4000)
    {
      Serial.println("Right");
      joystickParams.xValue = FORWARD;
    }

    //y direction
    if(analogRead(joyY) <= 10)
    {
      Serial.println("Down");
      joystickParams.yValue = UP; 
    }
    else if(analogRead(joyY) >= 4000)
    {
      Serial.println("Up");
      joystickParams.yValue = DOWN;  
    }

    timeObj2.elapsedTime = 0;
    timeObj2.previousTime = millis();
  }
    
}

void mainMenuDisplay(){
  char menuState = 0;
  char angleIncrement = 5;
  //display options
  /*
    1. Automatic Control
    2. Manual Control
  */
  enum selectionDirection{UP, DOWN, RETURN, NEUTRAL};
  enum stepperDir{BACKWARD, FORWARD};

  const char
    mainMenu = 0,
      automaticControl = 1,
        sp21Menu = 11,
        sp22Menu = 12,
      manualControl = 2,
        rotationControl = 21,
        distanceControl = 22;

  while(true)
  {
    switch(menuState)
    {
      case mainMenu:  
        menuTemplate("Mode Selection", "Press joystick to confirm",
        "1. Automatic Control",
        "2. Manual Control");

        if(joystickParams.yValue == UP)
        {
          menuState = automaticControl;
        }
        else if(joystickParams.yValue == DOWN)
        {
          menuState = manualControl;
        }
        joystickParams.yValue = NEUTRAL;
      break;

      case automaticControl:  
        menuTemplate("Automatic Control Menu","",
        "1. SP21",
        "2. SP22"); 

        if(joystickParams.yValue == UP)
        {
          menuState = sp21Menu;
        }
        else if(joystickParams.yValue == DOWN)
        {
          menuState = sp22Menu;
        }
        else if(joystickParams.yValue == RETURN)
        {
          menuState = mainMenu; Serial.println("Going back to menu");
        }
        joystickParams.yValue = NEUTRAL;
      break;

      case sp21Menu:  
        //the only difference from SP21, is it will control the relay board to use the battery when going standalone
        //will add later, focusing on sp22 only right now
        break;
      break;

      case sp22Menu: 
        Serial.println("SP22 mode selected"); 
        return;
      break;

      case manualControl:  
        menuTemplate("Manual Control Menu","",
        "1. Rotation Control",
        "2. Distance Control");

        if(joystickParams.yValue == UP)
        {
          menuState = rotationControl;
        }
        else if(joystickParams.yValue == DOWN)
        {
          menuState = distanceControl;
        }
        else if(joystickParams.yValue == RETURN)
        {
          menuState = mainMenu; Serial.println("Returning to main menu");
        }
        joystickParams.yValue = NEUTRAL;
      break;

      case rotationControl:  
        rotationPage();
        while(true)
        {
          readJoystick();
          if(joystickParams.yValue == UP)
          {
            joystickParams.yValue = 3;
            implantServoParams.targetAng = implantServoParams.targetAng - angleIncrement; servoCtrl(implantServoPin,  implantServoParams.targetAng);
            spServoParams.targetAng = spServoParams.targetAng - angleIncrement; servoCtrl(spServoPin,  spServoParams.targetAng);
            rotationPage();
          }
          else if(joystickParams.yValue == DOWN)
          {
            joystickParams.yValue = 3;
            implantServoParams.targetAng = implantServoParams.targetAng + angleIncrement; servoCtrl(implantServoPin, implantServoParams.targetAng);
            spServoParams.targetAng = spServoParams.targetAng + angleIncrement; servoCtrl(spServoPin,  spServoParams.targetAng);
            rotationPage();
          }
          else if(joystickParams.yValue == RETURN)
          {
            menuState = manualControl;
            joystickParams.yValue = NEUTRAL; Serial.println("Returning to manual control menu");
            break;
          }
        }
      break;

      case distanceControl:  
        digitalWrite(sleepPin, HIGH); //wake up the stepper driver
        distancePage();
        while(true)
        {
          readJoystick();
          Serial.print("Distance: ");  Serial.println(measureObj.measuredDist);
          if(joystickParams.yValue == UP)
          {
            joystickParams.yValue = NEUTRAL;
            stepperMtrCtrl(FORWARD, 20);
            distancePage();
          }
          else if(joystickParams.yValue == DOWN)
          {
            joystickParams.yValue = NEUTRAL;
            stepperMtrCtrl(BACKWARD, 20);
            distancePage();
          }
          else if(joystickParams.yValue == RETURN)
          {
            digitalWrite(sleepPin, LOW);  //put the stepper driver to sleep to cool down
            //Serial.print("SleepPin check: "); Serial.print(digitalRead(sleepPin));
            measureObj.measuredDist = 0;
            menuState = manualControl;
            joystickParams.yValue = NEUTRAL;
            break;
          }

          if(measureObj.contactDetected == 1)
          {
            measureObj.contactDetected = 0;
            distancePage();
          }
        }
      break;
    }    
  }
  Serial.println("Exited Menu");
}

void menuTemplate(String title, String description,  String option1, String option2){
  char tempVal = 3;
  enum selectionDirection{UP, DOWN, RETURN, NEUTRAL};

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(0,0); 
  tft.setTextSize(2); 
  tft.println(title);
  tft.print(description);

  tft.setCursor(0,50);
  tft.setTextSize(2);
  tft.println(option1);
  tft.print(option2); 

  while(true)
  {
    readJoystick();

    if(joystickParams.yValue != tempVal || joystickParams.xValue == 0)
    {
      if(joystickParams.yValue == UP)  //if user chooses the top option
      {
        tft.setCursor(0,50);
        tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE); tft.println(option1);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); tft.print(option2);
        
        tempVal = joystickParams.yValue;
      }
      else if(joystickParams.yValue == DOWN) //if user chooses the bottom option
      {
        tft.setCursor(0,50);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); tft.println(option1);
        tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE); tft.print(option2);
        tempVal = joystickParams.yValue;
      }
      else if(joystickParams.yValue == RETURN)
      {
        joystickParams.xValue = NEUTRAL;
        break;
      }
      
      if(joystickParams.xValue == 0)
      {
        joystickParams.xValue = NEUTRAL;
        break;
      }
    }

    if(joystickParams.buttonPressState == 1)  //confirm the selection
    {
      joystickParams.buttonPressState = 0;  //reset the buttonPress variable
      break;  //move to the next menu
    }
  }
  
}

void changeProgPodDisplay(char sampleNo){
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(0,0); 
  tft.setTextSize(2); 
  tft.println("Changing pod connection");
  tft.setTextSize(1); 
  tft.println("");

  tft.print("Turning off all relays..."); 
  /*
    code to turn off the relays
  */
  tft.println("->Completed");
  tft.println("");

  tft.print("Turning on the relays for sample ");  tft.print(sampleNo, DEC);
  /*
    code to execute
  */
 tft.println("->Completed");
}

void stateMachine(){
  enum states{
  idle,               //starting state
  changeProgPod,
  programSample,
  changeLoadBoard,
  detectContactPoint,
  activateStandalone,
  captureOscilloscope,
  increaseDistance,
  rotateSamples       //last state
  };

  enum stepperSleep{SLEEP, WAKE};
  
  enum stepperDir{CONVERGE, DIVERGE};

  char angleIncrement = 5;

  switch(stateObj.currentState)
  {
    case idle:
    //display menu options on LCD
      stateObj.sampleNo = 0;
      mainMenuDisplay();
      
      stateObj.currentState++;
      
      //as soon as machine turns on, align the first sample
      //if user presses start move to next state
      //state = state + 1;
    break;

    case changeProgPod: 
      Serial.println("Entered Changing programming pod mode");
      changeProgPodDisplay(1);
      //(corresponding to programming pins)
      //turn off all relays 
      //turn on the relays of the relevant samples 
      stateObj.currentState++;
    break;

    case programSample:
      //turn off all implant relays
      //turn on the implant relays for the relevant samples
      //use the python automation script
        //await for the script to send a message via serial and then move to the next state
      //stateObj.currentState++;
      stateObj.currentState = detectContactPoint;
    break;
    

    //..............//

    case detectContactPoint:
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0,0);
      tft.setTextSize(2); 
      tft.print("Homing... ");
      //Extend the probe using the solenoid (maybe redacted)
      //Reduce linear distance by half stepping the stepper motor
      //Use interrupts to count the number of slits
      //Stop stepping when contact is detected between the two electrode probes, record the number of slits
      //Retract the solenoid
      measureObj.contactDetected = 0;

      if(digitalRead(contactPin) == 0) //contact is already touching
      {
        stateObj.currentState = increaseDistance;  tft.println("->Contact already touching");
        //go back a couple millimeters and then touch again, then go in 3mm (offset)
        break;
      }


      digitalWrite(sleepPin, WAKE);

      Serial.print("Contact detected val: ");  Serial.println(measureObj.contactDetected, DEC);
      //Decrease distance until contact is detected
      while(measureObj.contactDetected != 1)
      {
        stepperMtrCtrl(CONVERGE, 10);
      }
      tft.println("->Contact detected");
      Serial.print("Dist before offset: ");  Serial.println(measureObj.measuredDist);   
      delay(2000);

      tft.print("Applying offset...");
      //Decrease the distance by 3mm (whatever the offset is)
      while(measureObj.measuredDist > -2) //measureDist should be 0 by this point
      {
        stepperMtrCtrl(CONVERGE,10);
        Serial.println(measureObj.measuredDist);
        delay(500);
      }
      tft.println("->Completed");

      digitalWrite(sleepPin, SLEEP);

      measureObj.contactDetected = 0;
      stateObj.currentState = increaseDistance;
    break;

    case increaseDistance:
      //change to the new page on LCD
      distancePage();
      //half step (depends, will find out during experimentation) the stepper motor until 1mm is counted
      //the stepper motor is stepped using a pwm signal

      //use interrupts to count the distance
      /*
      while(measuredDist < targetDist)
      {
        stepperMtrCtrl(0, 3);
        //update the distance measurement on the LCD
      }
      */

      digitalWrite(sleepPin, WAKE);
      
        while(measureObj.measuredDist < 5)
        {
          stepperMtrCtrl(DIVERGE, 10);
          delay(500);
        }
        distancePage();
        delay(2000);
      

      digitalWrite(sleepPin, SLEEP);

      stateObj.currentState++;
    break;

    case rotateSamples: //rotate to the next sample
      //change to the new page on LCD
      int targetAng;  //compatible target angles are 0,-40,-80, 40, 85
      
      switch(stateObj.sampleNo)
      {
        case 0: targetAng = -80; break;
        case 1: targetAng = -40; break;
        case 2: targetAng = 0; break;
        case 3: targetAng = 40; break;
        case 4: targetAng = 85; break;
      }

      implantServoParams.targetAng =targetAng;
      spServoParams.targetAng = targetAng;

      //use interrupts to track the current angle for each
      servoCtrl(implantServoPin, targetAng);
      servoCtrl(spServoPin, targetAng);

      rotationPage();
      
      delay(3000);

      stateObj.sampleNo++;  Serial.print("Sample No:"); Serial.println(stateObj.sampleNo);

      
      if(stateObj.sampleNo > 4)
      {
        stateObj.currentState = idle; //program is done
      }
      else
      {
        stateObj.currentState = changeProgPod;  //change to the programming pod to the next sample
      }
      
    break;
  }
}


void stepperMtrCtrl(char direction, char noSteps){
  //MP6500 driver
  //apply the direction signal;

  digitalWrite(dirPin, direction);
  measureObj.stepperDirection = direction;
  //apply the step signal, the driver looks for rising edges to trigger the step
  for(char i = 0; i < noSteps; i++)
  {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(3);
    digitalWrite(stepPin, LOW);
    delay(3);
  }

  delay(3);
}

void servoCtrl(char servo, int16_t angle){
  //Angle to duty cycle conversion
    //1000us is the -70 position
    //1500us is the neutral position/90
    //2000us is +70 position
    float requiredWid = (((2000.0-1500.0)/(70.0))* (float)angle) + 1500.0; //y = m*x and in uS
    Serial.print("Wid+:");  Serial.println(requiredWid);
    float dutyCycle = (requiredWid/((1.0/330.0)*1E6))*255.0;

    if(servo == implantServoPin)
    {
      ledcWrite(implantServoParams.ledChannel, (int)dutyCycle); 
    }
    else if(servo == spServoPin)
    {
      ledcWrite(spServoParams.ledChannel, (int)dutyCycle); 
    }
    
    
  /*
  for(char i = 0; i < 10; i++) //send several pulses
  {
    digitalWrite(servo, HIGH);
    delayMicroseconds(requiredWid);
    digitalWrite(servo, LOW);
    delayMicroseconds(requiredOffTime);
  }
  */
}

void solenoidCtrl(char probeState){
  //The mosfet should be able to handle at least 1.1A, prehaps a relay is better
  //activate a mosfet or bjt to control the solenoid
  
  if(probeState == HIGH)  //extension
  {
    digitalWrite(solenoidPin, HIGH);
  }
  else //retraction
  {
    digitalWrite(solenoidPin, LOW);
  }
}

/*
void ledBlinkTest(){
  digitalWrite(onBoardLED, HIGH);
  delay(1000);
  digitalWrite(onBoardLED,LOW);
  delay(1000);
}
*/
