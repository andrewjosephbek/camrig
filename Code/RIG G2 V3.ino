#include <LiquidCrystal_I2C.h>

#include <Adafruit_MCP23017.h>

//Definitions
#define STEP4 2
#define DIR4 3
#define STEP1 4
#define DIR1 5
#define STEP3 6
#define DIR3 7
#define STEP2 8
#define DIR2 9

#define SDA 4
#define SCL 5

#define M0 7
#define M1 6
#define M2 5

#define RESET 12
#define SLEEP1 2
#define SLEEP2 14
#define SLEEP3 4
#define SLEEP4 0

#define FAULT1 1
#define FAULT2 13
#define FAULT3 3
#define FAULT4 15
#define FOCUS A6
#define SHOT A7

#define inputCLK A2
#define inputDT A1
#define encSwitch A3

#define cursorPulseThreshold 50
#define valuePulseThreshold 50

#define screenElements 5
#define screenElementsSettings 7
#define SPEED 500

//

LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_MCP23017 mcp;

//Motion Variables
float exposure;
float pause;
long shots;

float Motor1Steps;
float Motor2Steps;
float Motor3Steps;
float Motor4Steps;

bool sleep;
float incSpeed = 0;

int Motor1Jog;
int Motor2Jog;
int Motor3Jog;
int Motor4Jog;
int MicroStep;

//

//Display Variables
int currentStateCLK;
int previousStateCLK;

int timeSinceUpdate;
bool screenHasUpdated = false;
int timeWaited;
bool EditView = false;
int ViewCounter;
int ViewSelector;

bool prevButtonState = true;
bool currButtonState = true;
int MenuSelector = 0;
int fractionSelector;

int totalSteps;

byte degrees[] = {
  B11100,
  B10100,
  B11100,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte stairs[] = {
  B00001,
  B00011,
  B00101,
  B01001,
  B10001,
  B10001,
  B11111,
  B00000
};

byte reverseStairs[] = {
  B10000,
  B11000,
  B10100,
  B10010,
  B10001,
  B10001,
  B11111,
  B00000
};
//

byte incSymb[] = {
  B01000,
  B00100,
  B10010,
  B01001,
  B10010,
  B00100,
  B01000,
  B00000
};

byte moon[] = {
  B00000,
  B00110,
  B01100,
  B11000,
  B11000,
  B11001,
  B01111,
  B00110
};
void setup() {
  mcp.begin();
  //pinmodes
  pinMode(STEP4, OUTPUT);
  pinMode(DIR4, OUTPUT);
  pinMode(STEP1, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(STEP3, OUTPUT);
  pinMode(DIR3, OUTPUT);
  pinMode(STEP2, OUTPUT);
  pinMode(DIR2, OUTPUT);
  pinMode(FOCUS, OUTPUT);
  pinMode(SHOT, OUTPUT);

  pinMode(inputCLK, INPUT);
  pinMode(inputDT, INPUT);
  pinMode(12, OUTPUT);

  mcp.pinMode(M0, OUTPUT);
  mcp.pinMode(M1, OUTPUT);
  mcp.pinMode(M2, OUTPUT);

  mcp.pinMode(RESET, OUTPUT);

  mcp.pinMode(SLEEP1, OUTPUT);
  mcp.pinMode(SLEEP2, OUTPUT);
  mcp.pinMode(SLEEP3, OUTPUT);
  mcp.pinMode(SLEEP4, OUTPUT);

  mcp.pinMode(FAULT1, INPUT);
  mcp.pinMode(FAULT2, INPUT);
  mcp.pinMode(FAULT3, INPUT);
  mcp.pinMode(FAULT4, INPUT);

  //Enabling drivers
  mcp.digitalWrite(RESET, HIGH);

  //Micro-step mode  
  mcp.digitalWrite(M0, LOW);
  mcp.digitalWrite(M1, LOW);
  mcp.digitalWrite(M2, LOW);

  mcp.digitalWrite(SLEEP1, LOW);
  mcp.digitalWrite(SLEEP2, LOW);
  mcp.digitalWrite(SLEEP3, LOW);
  mcp.digitalWrite(SLEEP4, LOW);
  //

  previousStateCLK = digitalRead(inputCLK);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degrees);
  lcd.createChar(1, stairs);
  lcd.createChar(2, reverseStairs);
  lcd.createChar(3, incSymb);
  lcd.createChar(4, moon);




  //GUI
  mainMenu();

}

void loop() {

  prevButtonState = currButtonState;
  currButtonState = digitalRead(encSwitch);

  if (prevButtonState == false && currButtonState == true) {
    EditView = !EditView;
  }

  //Main Menu
  if (MenuSelector == 0) {
    //Select Value to Edit
    if (EditView == false) {

      ViewCounter = ViewCounter + rotaryEncoder(1);
      if (ViewCounter < 0) {
        ViewCounter = 10;
      }
      if (ViewCounter > 10) {
        ViewCounter = 0;
      }
      ViewSelector = ViewCounter / 2;

      lcd.cursor_on();
      if (ViewSelector == 0) {
        encoderLCDCursor(0, 0);
      }
      if (ViewSelector == 1) {
        encoderLCDCursor(3, 0);
      }
      if (ViewSelector == 2) {
        encoderLCDCursor(3, 1);
      }
      if (ViewSelector == 3) {
        encoderLCDCursor(3, 2);
      }
      if (ViewSelector == 4) {
        encoderLCDCursor(19, 0);
      }
      if (ViewSelector == 5) {
        encoderLCDCursor(19, 3);
      }
    }

    //Edit values when in Edit Mode
    if (EditView == true) {

      //String message, int x, int y, int wipe
      if (ViewSelector == 0) {
        MenuSelector = 2;
        EditView = false;
        motorMenu();
      }
      if (ViewSelector == 1) {
        shots = rotaryEncoder(5) + shots;
        encoderLCDPrint(String(shots), 3, 0, 6);
      }
      if (ViewSelector == 2) {
        pause = float(rotaryEncoder(1)) / 8 + pause;
        encoderLCDPrint(String(pause), 3, 1, 6);
      }
      if (ViewSelector == 3) {
        exposure = float(rotaryEncoder(1)) / 8 + exposure;
        encoderLCDPrint(String(exposure), 3, 2, 6);
      }
      if (ViewSelector == 4 && prevButtonState == false && currButtonState == true) {
        MenuSelector = 1;
        EditView = false;
        lcd.clear();
        settings();
        ViewCounter = 0;

      }
      if (ViewSelector == 5 && prevButtonState == false && currButtonState == true) {
        TimeLapse();
      }
    }
  }
  //

  //Settings
  if (MenuSelector == 1) {

    if (EditView == false) {

      if (Motor1Jog != 0) {
        stepMotor(((1 << fractionSelector) * (100L * Motor1Jog)) / 3, 2500 - (490 * fractionSelector), STEP1, DIR1, SLEEP1);
        Motor1Jog = 0;
        lcd.setCursor(4, 0);
        lcd.print("0   ");
        lcd.setCursor(4, 0);
      }

      if (Motor2Jog != 0) {
        stepMotor(((1 << fractionSelector) * (100L * Motor2Jog)) / 3, 2500 - (490 * fractionSelector), STEP2, DIR2, SLEEP2);
        Motor2Jog = 0;
        lcd.setCursor(4, 1);
        lcd.print("0   ");
        lcd.setCursor(4, 1);
      }
      if (Motor3Jog != 0) {
        stepMotor((1 << fractionSelector) * ((100L * Motor3Jog) / 3), 2500 - (490 * fractionSelector), STEP3, DIR3, SLEEP3);
        Motor3Jog = 0;
        lcd.setCursor(4, 2);
        lcd.print("0   ");
        lcd.setCursor(4, 2);
      }
      if (Motor4Jog != 0) {
        stepMotor((1 << fractionSelector) * ((100L * Motor4Jog) / 3), 2500 - (490 * fractionSelector), STEP4, DIR4, SLEEP4);
        Motor4Jog = 0;
        lcd.setCursor(4, 3);
        lcd.print("0   ");
        lcd.setCursor(4, 3);
      }

      mcp.digitalWrite(SLEEP1, LOW);
      mcp.digitalWrite(SLEEP2, LOW);
      mcp.digitalWrite(SLEEP3, LOW);
      mcp.digitalWrite(SLEEP4, LOW);

      ViewCounter = ViewCounter + rotaryEncoder(1);

      if (ViewCounter < 0) {
        ViewCounter = 17;
      }
      if (ViewCounter > 17) {
        ViewCounter = 0;
      }
      ViewSelector = ViewCounter / 2;

      lcd.cursor_on();

      if (ViewSelector == 0) {
        encoderLCDCursor(0, 0);
      }

      if (ViewSelector == 1) {
        encoderLCDCursor(4, 0);
      }
      if (ViewSelector == 2) {
        encoderLCDCursor(4, 1);
      }
      if (ViewSelector == 3) {
        encoderLCDCursor(4, 2);
      }
      if (ViewSelector == 4) {
        encoderLCDCursor(4, 3);
      }
      if (ViewSelector == 5) {
        encoderLCDCursor(9, 0);
      }
      if (ViewSelector == 6) {
        encoderLCDCursor(9, 1);
      }
      if (ViewSelector == 7) {
        encoderLCDCursor(9, 2);
      }
      if (ViewSelector == 8) {
        encoderLCDCursor(8, 3);
      }

    }

    //Edit values when in Edit Mode
    if (EditView == true) {
      if (ViewSelector == 1) {
        Motor1Jog = rotaryEncoder(5) + Motor1Jog;
        encoderLCDPrint(String(Motor1Jog), 4, 0, 4);
      }

      if (ViewSelector == 2) {
        Motor2Jog = rotaryEncoder(5) + Motor2Jog;

        encoderLCDPrint(String(Motor2Jog), 4, 1, 4);
      }

      if (ViewSelector == 3) {
        Motor3Jog = rotaryEncoder(5) + Motor3Jog;

        encoderLCDPrint(String(Motor3Jog), 4, 2, 4);
      }

      if (ViewSelector == 4) {
        Motor4Jog = rotaryEncoder(5) + Motor4Jog;
        encoderLCDPrint(String(Motor4Jog), 4, 3, 4);
      }

      if (ViewSelector == 5) {
        MicroStep = rotaryEncoder(1) + MicroStep;
        fractionSelector = (MicroStep) / 2 % 6;

        if (screenHasUpdated == false) {
          if ((1 << fractionSelector) == 1) {
            mcp.digitalWrite(M0, LOW);
            mcp.digitalWrite(M1, LOW);
            mcp.digitalWrite(M2, LOW);
          }
          if ((1 << fractionSelector) == 2) {
            mcp.digitalWrite(M0, HIGH);
            mcp.digitalWrite(M1, LOW);
            mcp.digitalWrite(M2, LOW);
          }
          if ((1 << fractionSelector) == 4) {
            mcp.digitalWrite(M0, LOW);
            mcp.digitalWrite(M1, HIGH);
            mcp.digitalWrite(M2, LOW);
          }
          if ((1 << fractionSelector) == 8) {
            mcp.digitalWrite(M0, HIGH);
            mcp.digitalWrite(M1, HIGH);
            mcp.digitalWrite(M2, LOW);
          }
          if ((1 << fractionSelector) == 16) {
            mcp.digitalWrite(M0, LOW);
            mcp.digitalWrite(M1, LOW);
            mcp.digitalWrite(M2, HIGH);
          }
          if ((1 << fractionSelector) == 32) {
            mcp.digitalWrite(M0, HIGH);
            mcp.digitalWrite(M1, HIGH);
            mcp.digitalWrite(M2, HIGH);
          }
        }
        encoderLCDPrint("1/" + String(1 << fractionSelector), 9, 0, 4);

      }
      if (ViewSelector == 6 && prevButtonState == false && currButtonState == true) {
        sleep = !sleep;
        if (sleep) {
          lcd.setCursor(9, 1);
          lcd.print("true ");
          lcd.setCursor(9, 1);
        } else {
          lcd.setCursor(9, 1);
          lcd.print("false");
          lcd.setCursor(9, 1);
        }
        EditView = false;
      }
      
      if (ViewSelector == 7) {
        incSpeed = float(rotaryEncoder(1))/4 + incSpeed;
        encoderLCDPrint(String(incSpeed), 9, 2, 6);
      }

      if (ViewSelector == 8 && prevButtonState == false && currButtonState == true) {
        mcp.digitalWrite(RESET, LOW);
        delay(250);
        mcp.digitalWrite(RESET, HIGH);
        EditView = false;
      }


      if (ViewSelector == 0 && prevButtonState == false && currButtonState == true) {
        EditView = false;
        MenuSelector = 0;
        lcd.clear();
        mainMenu();
        ViewCounter = 0;
      }
    }
  }
  //

  //Motor Menu
  if (MenuSelector == 2) {

    if (EditView == false) {

      ViewCounter = ViewCounter + rotaryEncoder(1);
      if (ViewCounter < 0) {
        ViewCounter = 8;
      }
      if (ViewCounter > 8) {
        ViewCounter = 0;
      }
      ViewSelector = ViewCounter / 2;

      if (ViewSelector == 0) {
        encoderLCDCursor(3, 0);
      }
      if (ViewSelector == 1) {
        encoderLCDCursor(3, 1);
      }
      if (ViewSelector == 2) {
        encoderLCDCursor(3, 2);
      }
      if (ViewSelector == 3) {
        encoderLCDCursor(3, 3);
      }
      if (ViewSelector == 4) {
        encoderLCDCursor(19, 0);
      }
    }
    if (EditView == true) {
      if (ViewSelector == 0) {
        Motor1Steps = float(rotaryEncoder(1)) / (1 << fractionSelector) + Motor1Steps;
        encoderLCDPrint(String(Motor1Steps), 3, 0, 6);
      }
      if (ViewSelector == 1) {
        Motor2Steps = float(rotaryEncoder(1)) / (1 << fractionSelector) + Motor2Steps;
        encoderLCDPrint(String(Motor2Steps), 3, 1, 6);
      }
      if (ViewSelector == 2) {
        Motor3Steps = float(rotaryEncoder(1)) / (1 << fractionSelector) + Motor3Steps;
        encoderLCDPrint(String(Motor3Steps), 3, 2, 6);
      }
      if (ViewSelector == 3) {
        Motor4Steps = float(rotaryEncoder(1)) / (1 << fractionSelector) + Motor4Steps;
        encoderLCDPrint(String(Motor4Steps), 3, 3, 6);
      }
      if (ViewSelector == 4) {
        mainMenu();
        MenuSelector = 0;
        EditView = false;
        ViewCounter = 0;
      }
    }
  }
  //

}


// timelapse ---------------------
void TimeLapse() {
  lcd.noCursor();
  MenuSelector = 3;
  lcd.clear();
  mcp.digitalWrite(RESET, LOW);
  mcp.digitalWrite(RESET, HIGH);
  int shotsRemaining = shots;

  fractionSelector = (MicroStep) / 2 % 6;

  if (incSpeed == 0) {
    incSpeed = (2500 - (490 * fractionSelector))/1000;
  }

  totalSteps = Motor1Steps + Motor2Steps + Motor3Steps + Motor4Steps;

  //Setting motor direction
  if (Motor1Steps < 0) {
    digitalWrite(DIR1, HIGH);
  }
  if (Motor2Steps < 0) {
    digitalWrite(DIR2, HIGH);
  }
  if (Motor3Steps < 0) {
    digitalWrite(DIR3, HIGH);
  }
  if (Motor4Steps < 0) {
    digitalWrite(DIR4, HIGH);
  }

  delay(1000);

  //making sure 
  mcp.digitalWrite(RESET, HIGH);
  mcp.digitalWrite(SLEEP1, HIGH);
  mcp.digitalWrite(SLEEP2, HIGH);
  mcp.digitalWrite(SLEEP3, HIGH);
  mcp.digitalWrite(SLEEP4, HIGH);


  lcd.setCursor(0, 2);
 lcd.print("Debug Info:");


  lcd.setCursor(0, 3);
  lcd.print(round(Motor1Steps));
  lcd.print(" ");
  lcd.print(round(Motor2Steps));
  lcd.print(" ");
  lcd.print(round(Motor3Steps));
  lcd.print(" ");
  lcd.print(round(Motor4Steps));
  lcd.print(" ");

delay(3000);  

lcd.clear();

  for (shotsRemaining; shotsRemaining > 0; shotsRemaining--) {
    


    //Override
    if (digitalRead(encSwitch) == LOW) {
      lcd.clear();
      delay(1500);
      break;
    }



    //Updating completion value
    float percentFinished = 1 - (float(shotsRemaining) / float(shots));
    lcd.setCursor(4, 1);
    lcd.print("                    ");
    lcd.setCursor(10 - String(String(percentFinished * 100) + "% complete").length() / 2, 1);
    lcd.print(String(percentFinished * 100) + "% complete");
    lcd.setCursor(0, 3);

    //Updating time remaining value
    int timeLeft = ((exposure + pause) * shotsRemaining + (totalSteps / 500) * shotsRemaining) / 60;
    lcd.setCursor(0, 2);
    lcd.print("                    ");
    lcd.setCursor(10 - String("T=" + String(timeLeft) + "min").length() / 2, 2);
    lcd.print(String("T=" + String(timeLeft) + "min"));
    delay((pause * 1000));

    //Debug
  /*  lcd.setCursor(0, 3);
    lcd.print("      ");
    lcd.setCursor(0, 3);
    lcd.print(incSpeed);
    lcd.print("ms");
    */
    //Taking the picture
    digitalWrite(12, HIGH);
    delay((exposure * 1000));
    digitalWrite(12, LOW);

    //Waking the motors
    if (sleep) {
      mcp.digitalWrite(SLEEP1, HIGH);
      mcp.digitalWrite(SLEEP2, HIGH);
      mcp.digitalWrite(SLEEP3, HIGH);
      mcp.digitalWrite(SLEEP4, HIGH);
    }

    delay(500);

    if (Motor1Steps != 0) {
      for (int u = Motor1Steps * (1 << fractionSelector); u > 0; u--) {
        digitalWrite(STEP1, HIGH);
        delay(incSpeed);
        digitalWrite(STEP1, LOW);
      }
    }

    if (Motor2Steps != 0) {
      for (int u = Motor2Steps * (1 << fractionSelector); u > 0; u--) {
        digitalWrite(STEP2, HIGH);
        delay(incSpeed);
        digitalWrite(STEP2, LOW);
      }
    }

    if (Motor3Steps != 0) {
      for (int u = Motor3Steps * (1 << fractionSelector); u > 0; u--) {
        digitalWrite(STEP3, HIGH);
        delay(incSpeed);
        digitalWrite(STEP3, LOW);
      }
    }

    if (Motor4Steps != 0) {
      for (int u = Motor4Steps * (1 << fractionSelector); u > 0; u--) {
        digitalWrite(STEP3, HIGH);
        delay(incSpeed);
        digitalWrite(STEP3, LOW);
      }
    }

    if (sleep) {
      mcp.digitalWrite(SLEEP1, LOW);
      mcp.digitalWrite(SLEEP2, LOW);
      mcp.digitalWrite(SLEEP3, LOW);
      mcp.digitalWrite(SLEEP4, LOW);
    }
  }

  lcd.clear();
  EditView = false;
  mainMenu();
  MenuSelector = 0;
  ViewCounter = 0;
  lcd.setCursor(0, 0);

}

int rotaryEncoder(int increment) {

  int counter = 0;
  counter = 0;

  currentStateCLK = digitalRead(inputCLK);

  if (currentStateCLK != previousStateCLK) {
    timeSinceUpdate = millis();

    if (digitalRead(inputDT) != currentStateCLK) {
      counter = counter - increment;
      screenHasUpdated = false;
    } else {
      counter = counter + increment;
      screenHasUpdated = false;
    }

  }
  previousStateCLK = currentStateCLK;

  timeWaited = millis() - timeSinceUpdate;

  return counter;

}

void encoderLCDPrint(String message, int x, int y, int wipe) {
  if (timeWaited > valuePulseThreshold && screenHasUpdated == false) {

    for (int spaces = 0; spaces < wipe; spaces++) {
      lcd.print(" ");
    }
    lcd.setCursor(x, y);
    lcd.print(message);

    if (MenuSelector == 0) {
      if (ViewSelector == 1 or ViewSelector == 2 or ViewSelector == 3) {
        lcd.setCursor(0, 3);
        lcd.print("          ");

        float duration = ((exposure + pause) * shots + (totalSteps / 500)) / 60;
        lcd.setCursor(1, 3);

        if (duration < 60) {
          lcd.print(duration);
          lcd.print("min");
        } else {
          lcd.print(duration / 60);
          lcd.print("hr");
        }
      }
    }

    if (MenuSelector == 2) {
      if (ViewSelector == 0 or ViewSelector == 1 or ViewSelector == 2) {
        lcd.setCursor(11, y);
        lcd.print("      ");

        lcd.setCursor(11, y);
        lcd.print(shots * (message.toFloat() * 3) / 100);
        lcd.write(0);

      }
    }
    lcd.setCursor(x, y);
    screenHasUpdated = true;
  }
}


void encoderLCDCursor(int x, int y) {
  if (timeWaited > cursorPulseThreshold && screenHasUpdated == false) {
    lcd.setCursor(x, y);
    screenHasUpdated = true;
  }
}

void settings() {
  lcd.setCursor(1, 0);
  lcd.write(2);
  lcd.print("1 ");
  lcd.setCursor(1, 1);
  lcd.write(2);
  lcd.print("2 ");
  lcd.setCursor(1, 2);
  lcd.write(2);
  lcd.print("3 ");
  lcd.setCursor(1, 3);
  lcd.write(2);
  lcd.print("4 ");

  lcd.setCursor(8, 0);
  lcd.write(1);
  lcd.setCursor(8, 3);
  lcd.print("Reset");
  lcd.setCursor(8, 2);
  lcd.write(3);
  lcd.print(incSpeed);
  lcd.setCursor(8, 1);
  lcd.write(4);
  if (sleep) {
    lcd.setCursor(9, 1);
    lcd.print("true ");
    lcd.setCursor(9, 1);
  } else {
    lcd.setCursor(9, 1);
    lcd.print("false");
    lcd.setCursor(9, 1);
  }

  lcd.setCursor(4, 0);
  lcd.print(Motor1Jog);
  lcd.setCursor(4, 1);
  lcd.print(Motor2Jog);
  lcd.setCursor(4, 2);
  lcd.print(Motor3Jog);
  lcd.setCursor(4, 3);
  lcd.print(Motor4Jog);
  lcd.setCursor(9, 0);
  lcd.print("1/" + String(1 << fractionSelector));
  lcd.setCursor(0, 0);
  lcd.print("<");

  lcd.setCursor(0, 0);

}

void mainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("<");

  lcd.setCursor(1, 0);
  lcd.print("S:");
  lcd.setCursor(1, 1);
  lcd.print("D:");
  lcd.setCursor(1, 2);
  lcd.print("E:");

  lcd.setCursor(3, 0);
  lcd.print(shots);
  lcd.setCursor(3, 1);
  lcd.print(pause);
  lcd.setCursor(3, 2);
  lcd.print(exposure);
  lcd.setCursor(1, 3);
  lcd.print(((exposure + pause) * shots + (totalSteps / 500)) / 60);
  lcd.print("min");

  lcd.setCursor(19, 0);
  lcd.print(">");
  lcd.setCursor(19, 3);
  lcd.print("*");
  lcd.cursor();

}

void motorMenu() {
  lcd.clear();

  lcd.setCursor(19, 0);
  lcd.print(">");

  lcd.setCursor(0, 0);
  lcd.write(1);
  lcd.print("1 ");
  lcd.print(Motor1Steps);
  lcd.setCursor(0, 1);
  lcd.write(1);
  lcd.print("2 ");
  lcd.print(Motor2Steps);
  lcd.setCursor(0, 2);
  lcd.write(1);
  lcd.print("3 ");
  lcd.print(Motor3Steps);
  lcd.setCursor(0, 3);
  lcd.write(1);
  lcd.print("4 ");
  lcd.print(Motor4Steps);

  lcd.setCursor(11, 0);
  lcd.print(shots * (Motor1Steps * 3) / 100);
  lcd.write(0);

  lcd.setCursor(11, 1);
  lcd.print(shots * (Motor2Steps * 3) / 100);
  lcd.write(0);

  lcd.setCursor(11, 2);
  lcd.print(shots * (Motor3Steps * 3) / 100);
  lcd.write(0);

  lcd.setCursor(11, 3);
  lcd.print(shots * (Motor4Steps * 3) / 100);
  lcd.write(0);

  lcd.setCursor(3, 0);

  ViewCounter = 0;

  lcd.cursor();

}

void stepMotor(long jogSteps, int speed, int stepPin, int dirPin, int sleep) {

  mcp.digitalWrite(RESET, HIGH);
  mcp.digitalWrite(sleep, HIGH);

  long steps = abs(jogSteps);

  if (jogSteps > 0) {
    digitalWrite(dirPin, HIGH);
  } else {
    digitalWrite(dirPin, LOW);
  }

  for (steps; steps > 0; steps--) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(speed);
    digitalWrite(stepPin, LOW);
  }
  mcp.digitalWrite(sleep, LOW);

}