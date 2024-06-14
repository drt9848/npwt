//npwt
#include "SPI.h"

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_ILI9341_Albert.h"
#include <Fonts/FreeSans12pt7b.h> // https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
// #include <Fonts/FreeSans6pt7b.h> // https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
#include <Fonts/FreeSans24pt7b.h> 
#include <EEPROM.h>

// #define LED PC13
#define voltagepin PA0
#define chargingpin PB0
#define pressureInput PB1
#define upbutton PA1//up button
#define menubutton PA2//
#define downbutton PA3
#define enterbutton PA4

#define peristalicpumpA PA11
#define peristalicpumpB PA12

bool setup_state= false;
int buzzerPin = PB5;
int g = 0;
int clear_state=0;
int clear_state_timer=0;

int home_screen_status=0;
int isRectFilled;
#define Motor1 PA11
#define Motor2 PA6
#define EnableA PB6

int running_screeen = 0;
int stop_screeen = 0;

int timout_state=0;

int display_menu=0;
int menu_state =0;
const int sensorZeroPressure = 625;
const int sensorMaxPressure = 0;

float pressureValue = 0; //variable to store the value coming from the pressure transducer


int prevNeedleX = 0;
int prevNeedleY = 0;

float R1 = 100000.0;
float R2 = 10000.0;
float ref_voltage = 5.0;


unsigned long previousMillisforpump = 0;
int motor1Duration = 2000; // 2 seconds
int motor2Duration = 2000; // 2 seconds
boolean motor1On = false;
boolean motor2On = false;

int devicemode = 0;//0=NORMAL,1=PERISTALIC
/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
// Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// For the Adafruit shield, these are the default.
#define display_DC PB11
#define display_CS PB12
#define display_MOSI PA7
#define display_CLK PA5
#define display_RST PB10
#define display_MISO PB14

int batteryLevel = 0; // Example battery level
int setpressureval = 90;
int calibratedPressure = 0;
int peristalicdiffval = 100; // Default brightness level (adjustable between 0 and 255)

int timerLevel = 0;

int selectedOption = 0; // Initially selected option
bool inMenu = false; // Flag to indicate if in menu
bool lastMenuState = false; // Store the previous state of the Menu button
int enteroption = 0;
bool enterstate = true;
int longpressstate = 0;
bool ismotorrunning = false;
bool runmotor  = false;


unsigned long startTime;
unsigned long currentTime;  // Variable to store the last time the timer was updated
int interval = 1;  // Interval in minutes (1 min in this case)
int ischarging = 0;


// Adafruit_ILI9341 display = Adafruit_ILI9341(display_CS, display_DC, display_MOSI, display_CLK, display_RST, display_MISO);


Adafruit_ILI9341_Albert display = Adafruit_ILI9341_Albert(display_CS, display_DC, display_RST);

// Function to write a byte to the display
// Assuming your display has a buffer accessible like this
// extern uint16_t displayBuffer[ILI9341_displayWIDTH * ILI9341_displayHEIGHT];

// Assuming your display has a buffer accessible like this
// void fastfillScreen(uint16_t color) {
//   display.fillScreen(color);
//   display.updateDisplay(); // This might be necessary to refresh the screen
// }
unsigned long testFilledRoundRects() {
  unsigned long start;
  int cx = display.width() / 2 - 1;
  int cy = display.height() / 2 - 1;
  int rectWidth = display.width() - 20; // Adjust the width as needed
  int rectHeight = display.height() - 20; // Adjust the height as needed
  int cornerRadius = 10; // Adjust the corner radius as needed

  display.fillScreen(ILI9341_BLUE);
  start = micros();

  // Draw a single rounded rectangle in the center of the screen
  display.fillRoundRect(cx - rectWidth / 2, cy - rectHeight / 2, rectWidth, rectHeight, cornerRadius, display.color565(0, 255, 0));

  return micros() - start;
}


void setup() {
  Serial.begin(9600);
  setup_state=true;
  // pinMode(LED, OUTPUT);
  pinMode(upbutton, INPUT_PULLDOWN);
  pinMode(menubutton, INPUT_PULLDOWN);
  pinMode(downbutton, INPUT_PULLDOWN);
  pinMode(enterbutton, INPUT_PULLDOWN);
  pinMode(voltagepin, INPUT);
  pinMode(chargingpin, INPUT);
  pinMode(Motor1, OUTPUT);
  pinMode(Motor2, OUTPUT);
  pinMode(EnableA, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  pinMode(peristalicpumpA, OUTPUT);
  pinMode(peristalicpumpB, OUTPUT);


  digitalWrite(buzzerPin, LOW);



  EEPROM.init();
  EEPROM.PageBase0 = 0x801F000;
  EEPROM.PageBase1 = 0x801F800;
  EEPROM.PageSize  = 0x800;

  uint16 Data1;
  uint16 Data2;
  uint16 Data3;

  EEPROM.read(0x10, &Data1);
  EEPROM.read(0x11, &Data2);
  EEPROM.read(0x12, &Data3);
  setpressureval = Data1;
  if (setpressureval > 1000) {
    setpressureval = 90;
  }
  peristalicdiffval = Data2;
  if (peristalicdiffval > 1000) {
    peristalicdiffval = 0;
  }
  timerLevel = Data3;
  if (timerLevel > 1000) {
    timerLevel = 1;
  }


  delay(550); // wait for the OLED to power up
  display.begin(); // Address SPI
   display.setFont(&FreeSans12pt7b);
  //  //display.display(); // Display splash screen


   uint8_t x = display.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = display.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = display.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = display.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = display.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
  
  Serial.println(F("Benchmark                Time (microseconds)"));

  // Serial.print(F("Screen fill              "));
  // Serial.println(testFillScreen());
  // delay(500);
  // delay(500);
    display.setRotation(0);

//  for(uint8_t rotation=0; rotation<4; rotation++) {
//     display.setRotation(rotation);
//     testText();
//     delay(1000);
//   }
  // Serial.println(testFilledRoundRects());

  delay(500);
  //display.fillScreen(ILI9341_BLACK); // Clear display buffer
    display.fillScreen(ILI9341_BLACK);

  // Display loading animation
  displayLoadingAnimation();
  //display.fillScreen(ILI9341_BLACK); // Clear display buffer
    display.fillScreen(ILI9341_BLACK);

  // delay(500);
// display.fillRect(10, 10, 100, 50, ILI9341_WHITE);


  // delay(500);
if(setup_state)
{

  // displayHomeScreen();

  
// delay(1000);
// home_screen_status =0;


  enteroption = -1;
   display.fillScreen(ILI9341_BLACK);

  //   if(clear_state==0){
    
   
  //  display.fillScreen(ILI9341_BLACK);

  //    isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

  //   if (!isRectFilled) {
  //   display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
  //   isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  // }
  //   }

  //      if(clear_state_timer==0){
    
   
  //  display.fillScreen(ILI9341_BLACK);

  //    isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

  //   if (!isRectFilled) {
  //   display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
  //   isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  // }
  //   }


  //    if(running_screeen==0){
    
   
  //  display.fillScreen(ILI9341_BLACK);

  //    isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

  //   if (!isRectFilled) {
  //   display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
  //   isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  // }
  //   }


  //    if(stop_screeen==0){
    
   
  //  display.fillScreen(ILI9341_BLACK);
  //    isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

  //   if (!isRectFilled) {
  //   display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
  //   isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  // }
  //   }

  //    if(menu_state==0){
    
  //   isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled
   
  //  display.fillScreen(ILI9341_BLACK);


  //   if (!isRectFilled) {
  //   display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
  //   isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  // }
  //   }


    
  //    if(timout_state==0){
    
   
  //  display.fillScreen(ILI9341_BLACK);
  //    isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

  //   if (!isRectFilled) {
  //   display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
  //   isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  // }
  //   }

clear_state = 1;
clear_state_timer = 1;
running_screeen = 1;
stop_screeen=1;
timout_state=1;
menu_state = 1;
display_menu =0;

  if (ismotorrunning == true) {
    ismotorrunning = false;
    if (timerLevel > 0) {
      running_screeen =0;
     display.fillScreen(ILI9341_BLACK);
      display.setTextSize(1); //(1); //(1); //(1); //(2);
      display.setTextColor(ILI9341_WHITE);
      display.setCursor(0, 20);
      // display.println("Running..."); // Replace with your title
      // //display.display();
      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      digitalWrite(buzzerPin, LOW);
      digitalWrite(Motor1, LOW);
      digitalWrite(Motor2, HIGH);


      startTime = millis();
      runmotor = true;

    } else {
      runmotor = false;
      digitalWrite(Motor1, LOW);
      digitalWrite(Motor2, LOW);
      digitalWrite(peristalicpumpA, LOW);
      digitalWrite(peristalicpumpB, LOW);

     display.fillScreen(ILI9341_BLACK);
      display.setTextSize(1); //(1); //(1); //(1); //(2);
      display.setTextColor(ILI9341_WHITE);
      display.setCursor(0, 20);
      display.println("Set Timer first"); // Replace with your title
      //display.display();
      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      digitalWrite(buzzerPin, LOW);



    }

  } else {
    if (runmotor == false) {
      digitalWrite(Motor1, LOW);
      digitalWrite(Motor2, LOW);
      digitalWrite(peristalicpumpA, LOW);
      digitalWrite(peristalicpumpB, LOW);

    }

  }


  if (ismotorrunning == false) {
    // Display title at top left corner

  display.drawFastHLine(0, 39, 240, ILI9341_WHITE);
 if (!isRectFilled) {
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
    isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  }
    display.setTextSize(1); //(1); //(1); //(1); //(4);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(5, 30);
    display.println("NPWT"); // Replace with your title
    display.println("  "); // Replace with your title


    display.setTextSize(1); //(1); //(1); //(1); //(3);
    display.setTextColor(ILI9341_WHITE);
        display.setCursor(10, 80); // Position below the title
    display.print("Pressure"); // Replace with your subtitle
    display.print("["+ String(setpressureval)+"]:  "); // Replace with your subtitle


    
  // display.setCursor(0, 120);
 display.setTextSize(1); //(1); //(1); //(1); //(3);
    display.setTextColor(ILI9341_WHITE);
    // display.print("Current: " ); // Replace with your subtitle

      display.fillRect(145, 60, 240, 30, ILI9341_BLACK);

    display.print( String(calibratedPressure)); // Replace with your subtitle

  // display.setCursor(0, 160);

 display.setTextSize(1); //(1); //(1); //(1); //(3);
    display.setTextColor(ILI9341_WHITE);
    // display.print("/" ); // Replace with your subtitle

      // display.fillRect(60, 110, 240, 30, ILI9341_BLACK);

    // display.println( String(setpressureval)); // Replace with your subtitle





   display.setTextSize(1); //(1); //(1); //(1); //(3);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(10, 130); // Position below the title
    // display.print("Timer: " ); // Replace with your subtitle
    display.print("Timer"); // Replace with your subtitle
    display.print("["+ String(timerLevel)+"]:  "); // Replace with your subtitle
    if(currentTime>0){
      display.fillRect(90, 110, 240, 30, ILI9341_BLACK);
      display.print(String((currentTime / (1000 * 60)) % 60) + ":" + String((currentTime / 1000) % 60));
}
else{
      display.print("0:0");
}




    display.setTextSize(1); //(1); //(1); //(1); //(2);
    display.setTextColor(ILI9341_WHITE);

    display.setCursor(10, 180); // Position below the title


    if (devicemode == 0) {
      display.print("Mode: ");
    display.setTextColor(ILI9341_WHITE);
      display.print(" NPWT");

     
    } else {
    display.setTextColor(ILI9341_WHITE);

      display.print("Mode :");
    display.setTextColor(ILI9341_WHITE);

      display.print("INSTILATION");
       
    }

    display.setCursor(0, 2000); // Position below the title



        display.drawFastHLine(0, 285, 240, ILI9341_WHITE);

    display.setCursor(10,312 );  // Adjusted the position of "Menu"
  // display.println(" Powered by");
      // display.print("Status:  Running");


    // display.setCursor(10, 312);  // Adjusted the position of "Menu"
      // display.fillRect(0, 10, 240, 30, ILI9341_PURPLE);

  // display.println(" EPIT Research Labs");





//  display.setTextSize(0); //(1); //(1); //(1); //(3);
//     display.setTextColor(ILI9341_BLUE);
//     display.setCursor(18, 250); // Position below the title
//     display.println("Powered by: " ); // Replace with your subtitle
//     display.setCursor(5, 265); // Position below the title

//     display.print("EPIT RESEARCH LABS (P.LTD) " ); // Replace with your subtitle

    // if (ischarging > 3) {
    //   display.setCursor(140, 20);
    // } else {
    //   display.setCursor(150, 20);
    // }
      display.setCursor(120, 20);

    if (runmotor == true) {
      
      display.setTextSize(1); //(1); //(1); //(1); //(1);
      display.setTextColor(ILI9341_WHITE);
      // display.println("R"); // Replace with your title

         display.setCursor(35,312 );  // Adjusted the position of "Menu"
  // display.println(" Powered by");
      display.print("DEVICE :  ON");

    } else {
     
  display.setTextSize(1); //(1); //(1); //(1); //(1);
      display.setTextColor(ILI9341_WHITE);
      // display.println("R"); // Replace with your title

         display.setCursor(35,312 );  // Adjusted the position of "Menu"
  

      display.print("DEVICE :  OFF");

    }


    // Display battery icon based on the battery level
      display.setCursor(200, 0);

    displayBatteryIcon(batteryLevel); // Function to display battery icon
    Serial.println("homescreen");
  }


}

  // Continue with other display tests or functions...

  analogWrite(EnableA, 255);

  digitalWrite(Motor1, LOW);
  digitalWrite(Motor2, LOW);

  digitalWrite(peristalicpumpA, LOW);
  digitalWrite(peristalicpumpB, LOW);

}



void loop() {


  pressureValue = analogRead(pressureInput); //reads value from input pin and assigns to variable
  calibratedPressure = map(pressureValue, 628, 0, 0, 200);
  if (calibratedPressure <= 0) {
    calibratedPressure = 0;
  }

  int motorSpeed = map(calibratedPressure, 20, setpressureval, 30, 255); // 0-255 for PWM
  analogWrite(EnableA, motorSpeed);


  int voltagepindata = analogRead(voltagepin);
  int chargingpindata = analogRead(chargingpin);

  int voltage  = (voltagepindata * ref_voltage) / 4096.0;
  ischarging  = (chargingpindata * ref_voltage) / 4096.0;

  voltage = voltage * (R1 + R2) / R2;
  ischarging = ischarging * (47000 + 33000) / 33000;

  batteryLevel = map(voltage, 0, 12, 0, 100);
  int ButtonState1 = digitalRead(upbutton);
  int ButtonState2 = digitalRead(menubutton);
  int ButtonState3 = digitalRead(downbutton);
  int ButtonState4 = digitalRead(enterbutton);


  Serial.print(" | ");
  Serial.print(ButtonState2);
  Serial.print(" | ");
  Serial.print(lastMenuState);
  Serial.print(" | ");
  Serial.print(inMenu);
  Serial.print(" | ");
  Serial.print(enterstate);
  Serial.print(" | ");
  Serial.print(batteryLevel);
  Serial.print(" | ");
  Serial.print(ischarging);
  Serial.print(" | ");
  Serial.print(calibratedPressure);
  Serial.println(" | ");

  //  display.setContrast(map((peristalicdiffval), 0, 100, 0, 255));
  motor1Duration = peristalicdiffval * 1000;
  motor2Duration = peristalicdiffval * 1000;

  if (runmotor == true) {
    unsigned long currentTime1 = millis(); // Get the current time

    if (currentTime1 - startTime >= (timerLevel * 60) * 1000 ) {
      // Perform actions after 1 second
      runmotor = false;
      //display.fillScreen(ILI9341_BLACK);
    display.fillScreen(ILI9341_BLACK);
      
      display.setTextSize(1); //(1); //(1); //(1); //(1);
      display.setTextColor(ILI9341_WHITE);
      display.setCursor(0, 20);
      display.println("Time out"); // Replace with your title
      // //display.display();
      timout_state = 0;
      currentTime = 0;
      startTime = 0;

      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      digitalWrite(buzzerPin, LOW);
    } else {
      currentTime = currentTime1 - startTime;
    }

  }



  if (ButtonState2 != lastMenuState) {
    delay(50); // Debouncing delay
    if (ButtonState2 == HIGH) {
      // Toggle the menu state
      inMenu = !inMenu;
      // Display appropriate screen based on menu state
      if (inMenu) {
        displayMenu();
        display_menu=0;
        menu_state = 0;
      } else {
        displayHomeScreen();

      }
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);
    }
  }

  lastMenuState = ButtonState2;


  if (inMenu) {
    enteroption = -1;

        display_menu=1;


    if (ButtonState4 == LOW && enterstate == false) {
      enterstate = true;
    }

    if (ButtonState4 == HIGH && enterstate == true) {
      // ENTER button pressed in menu
      if (selectedOption == 0) {
        // Set Pressure option selected
        displaySetPressureScreen();
        
      } else if (selectedOption == 1) {
        displaytimerScreen();

      } else if (selectedOption == 2) {
        // brightness selected
        displaybrightnessScreen();
      } else if (selectedOption == 3) {
        // brightness selected
        devicemodescreen();

      }
      else if (selectedOption == 4) {
        // brightness selected
        deviceaboutscreen();

      }
      enteroption = selectedOption;
      inMenu = false; // Exit menu after selection
      enterstate = false;
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);
    } else if (ButtonState1 == HIGH) {
      // UP button pressed in menu
      selectedOption = (selectedOption - 1 + 5) % 5; // Decrement option
      displayMenu(); // Update display with new option
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);

    } else if (ButtonState3 == HIGH) {
      // DOWN button pressed in menu

      selectedOption = (selectedOption + 1) % 5; // Increment option
      displayMenu(); // Update display with new option
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);
    }


  }

  if (!inMenu && enteroption == -1) {
    if (ButtonState4 == HIGH && enterstate == true) {

      if (longpressstate > 30) {
        Serial.println("long pressed");


        if (batteryLevel <= 5) {
         display.fillScreen(ILI9341_BLACK);
          display.setTextSize(1); //(1); //(1); //(1); //(2);
          display.setTextColor(ILI9341_WHITE);
          display.setCursor(30, 20);
          display.println("Plug"); // Replace with your title
          display.setCursor(20, 40);
          display.println("Battery"); // Replace with your title
          // //display.display();
          digitalWrite(buzzerPin, HIGH);
          delay(500);
          digitalWrite(buzzerPin, LOW);
          delay(500);
          digitalWrite(buzzerPin, HIGH);
          delay(500);
          digitalWrite(buzzerPin, LOW);
          delay(500);

        } else {
          if (runmotor == true) {
            runmotor = false;
            stop_screeen=0;
           display.fillScreen(ILI9341_BLACK);
            display.setTextSize(1); //(1); //(1); //(1); //(2);
            display.setTextColor(ILI9341_WHITE);
            display.setCursor(0, 20);
            display.println("Stopped"); // Replace with your title
            // //display.display();
            currentTime = 0;
            startTime = 0;
            delay(2000);
          } else {

            if (devicemode == 1) {

              if (peristalicdiffval <= 0) {

               display.fillScreen(ILI9341_BLACK);
               display.setTextSize(1); //(1); //(1); //(1); //(4);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(5, 20);
    display.println("NPWT"); // Replace with your title
    display.println("  "); // Replace with your title
// Replace with your title
                // display.setCursor(0, 30);
                // display.println("is 0");
                // // //display.display();

                digitalWrite(Motor1, LOW);
                digitalWrite(Motor2, LOW);

                digitalWrite(peristalicpumpA, LOW);
                digitalWrite(peristalicpumpB, LOW);
                runmotor = false;

                delay(2000);
                displayHomeScreen();

              } else {
                ismotorrunning = true;
              }
            } else {
              ismotorrunning = true;
            }

          }
        }


        enterstate = false;

      } else {
        longpressstate++;
      }

    }

    displayHomeScreen();


    if (ButtonState4 == LOW && enterstate == false) {
      longpressstate = 0;
      enterstate = true;
    }
  }


  if (enteroption == 0) {

    if (ButtonState4 == LOW && enterstate == false) {

      EEPROM.write(0x10, setpressureval);
      enterstate = true;
    }


    if (ButtonState1 == HIGH) {
      // UP button pressed in menu

        if (setpressureval <300) {
        // setpressureval = setpressureval - 5; // Decrement option
      setpressureval = setpressureval + 5; // Increment option


      } else {
        setpressureval = 300;

      }
      // displaySetPressureScreen();
      displaySetPressureScreen_up_down();
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);
    } else if (ButtonState3 == HIGH) {
      // DOWN button pressed in menu
      if (setpressureval - 10 > 0) {
        setpressureval = setpressureval - 5; // Decrement option
      } else {
        setpressureval = 10;
      }
      // displaySetPressureScreen();
      displaySetPressureScreen_up_down();

      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);

    }


    if (ButtonState4 == HIGH && enterstate == true) {
      displayHomeScreen();
      enterstate = false;
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);
    }


  } else if (enteroption == 2) {

    if (ButtonState4 == LOW && enterstate == false) {
      EEPROM.write(0x11, peristalicdiffval);
      enterstate = true;
    }


    if (ButtonState1 == HIGH) {
      // UP button pressed in menu
      if (peristalicdiffval + 2 <= 100) {
        peristalicdiffval = peristalicdiffval + 2; // Increment option
        displaybrightnessScreen();
      }
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);
    } else if (ButtonState3 == HIGH) {
      // DOWN button pressed in menu
      if (peristalicdiffval - 2 >= 0) {
        peristalicdiffval = peristalicdiffval - 2; // Decrement option
      } else {
        peristalicdiffval = 0;
      }
      displaybrightnessScreen();
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);

    }


    if (ButtonState4 == HIGH && enterstate == true) {
      displayHomeScreen();
      enterstate = false;
      digitalWrite(buzzerPin, HIGH);
      delay(300);
      digitalWrite(buzzerPin, LOW);
    }


  } else if (enteroption == 1) {

    if (ButtonState4 == LOW && enterstate == false) {
      EEPROM.write(0x12, timerLevel);
      enterstate = true;
    }


    if (ButtonState1 == HIGH) {
      // UP button pressed in menu
      if (timerLevel + 1 <= 100) {
        timerLevel = timerLevel + 1; // Increment option
        // displaytimerScreen();
        displaytimerScreen_up_down();
      }
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);
    } else if (ButtonState3 == HIGH) {
      // DOWN button pressed in menu
      if (timerLevel - 1 > 0) {
        timerLevel = timerLevel - 1; // Decrement option
      } else {
        timerLevel = 1;
      }
      // displaytimerScreen();
        displaytimerScreen_up_down();

      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);

    }


    if (ButtonState4 == HIGH && enterstate == true) {
      displayHomeScreen();
      enterstate = false;
      digitalWrite(buzzerPin, HIGH);
      delay(300);
      digitalWrite(buzzerPin, LOW);
    }


  } else if (enteroption == 3) {

    if (ButtonState4 == LOW && enterstate == false) {
      EEPROM.write(0x12, timerLevel);
      enterstate = true;
    }


    if (ButtonState1 == HIGH) {
      // UP button pressed in menu
      if (devicemode == 0) {
        devicemode = 1;

      } else {
        devicemode = 0;
      }
      devicemodescreen();
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);
    } else if (ButtonState3 == HIGH) {
      // DOWN button pressed in menu
      if (devicemode == 0) {
        devicemode = 1;

      } else {
        devicemode = 0;
      }
      devicemodescreen();
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);

    }


    if (ButtonState4 == HIGH && enterstate == true) {
      displayHomeScreen();
      enterstate = false;
      digitalWrite(buzzerPin, HIGH);
      delay(300);
      digitalWrite(buzzerPin, LOW);
    }


  }


  if (runmotor == true && devicemode == 1) {

    if (peristalicdiffval > 0) {
      unsigned long currentMillis = millis();

      if (!motor1On && !motor2On) {
        if (currentMillis - previousMillisforpump >= motor1Duration) {
          previousMillisforpump = currentMillis;
          digitalWrite(Motor1, LOW);
          digitalWrite(Motor2, HIGH);

          digitalWrite(peristalicpumpA, LOW);
          digitalWrite(peristalicpumpB, LOW);
          motor1On = true;
        }
      } else if (motor1On && !motor2On) {
        if (currentMillis - previousMillisforpump >= motor2Duration) {
          previousMillisforpump = currentMillis;
          digitalWrite(Motor1, LOW);
          digitalWrite(Motor2, LOW);

          digitalWrite(peristalicpumpA, LOW);
          digitalWrite(peristalicpumpB, HIGH);
          motor1On = false;
          motor2On = true;
        }
      } else if (!motor1On && motor2On) {
        if (currentMillis - previousMillisforpump >= motor1Duration) {
          previousMillisforpump = currentMillis;
          digitalWrite(Motor1, LOW);
          digitalWrite(Motor2, HIGH);

          digitalWrite(peristalicpumpA, LOW);
          digitalWrite(peristalicpumpB, LOW);
          motor2On = false;
          motor1On = true;
        }
      }

    } else {

     display.fillScreen(ILI9341_BLACK);
      display.setTextSize(1); //(1); //(1); //(1); //(4);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(5, 20);
    display.println("NPWT"); // Replace with your title
    display.println("  "); // Replace with your title
// Replace with your title
      // //display.display();

      digitalWrite(Motor1, LOW);
      digitalWrite(Motor2, LOW);

      digitalWrite(peristalicpumpA, LOW);
      digitalWrite(peristalicpumpB, HIGH);
      runmotor = false;

      delay(2000);
      displayHomeScreen();
    }




  }


  // //display.display();
  delay(10);

}




void displayHomeScreen() {
// delay(1000);
// home_screen_status =0;


  enteroption = -1;

    if(clear_state==0){
    
   
   display.fillScreen(ILI9341_BLACK);

     isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

    if (!isRectFilled) {
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
    isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  }
    }

       if(clear_state_timer==0){
    
   
   display.fillScreen(ILI9341_BLACK);

     isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

    if (!isRectFilled) {
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
    isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  }
    }


     if(running_screeen==0){
    
   


     isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

    if (!isRectFilled) {
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
    isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  }
    }


     if(stop_screeen==0){
    
   
   display.fillScreen(ILI9341_BLACK);
     isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

    if (!isRectFilled) {
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
    isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  }
    }

     if(menu_state==0){
    
    isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled
   
   display.fillScreen(ILI9341_BLACK);


    if (!isRectFilled) {
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
    isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  }
    }


    
     if(timout_state==0){
    
   
   display.fillScreen(ILI9341_BLACK);
     isRectFilled = false;  // Set the flag to true to indicate that the rectangle has been filled

    if (!isRectFilled) {
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
    isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  }
    }

clear_state = 1;
clear_state_timer = 1;
running_screeen = 1;
stop_screeen=1;
timout_state=1;
menu_state = 1;
display_menu =0;

  if (ismotorrunning == true) {
    ismotorrunning = false;
    if (timerLevel > 0) {
      running_screeen =0;
     display.fillScreen(ILI9341_BLACK);
      display.setTextSize(1); //(1); //(1); //(1); //(2);
      display.setTextColor(ILI9341_WHITE);
      display.setCursor(0, 20);
      display.println("Running..."); // Replace with your title
      // //display.display();
      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      digitalWrite(buzzerPin, LOW);
      digitalWrite(Motor1, LOW);
      digitalWrite(Motor2, HIGH);

     display.fillScreen(ILI9341_BLACK);

      startTime = millis();
      runmotor = true;

    } else {
      runmotor = false;
      digitalWrite(Motor1, LOW);
      digitalWrite(Motor2, LOW);
      digitalWrite(peristalicpumpA, LOW);
      digitalWrite(peristalicpumpB, LOW);

    //  display.fillScreen(ILI9341_BLACK);
      display.setTextSize(1); //(1); //(1); //(1); //(2);
      display.setTextColor(ILI9341_WHITE);
      display.setCursor(0, 20);
      display.println("Set Timer first"); // Replace with your title
      //display.display();
      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      digitalWrite(buzzerPin, LOW);



    }

  } else {
    if (runmotor == false) {
      digitalWrite(Motor1, LOW);
      digitalWrite(Motor2, LOW);
      digitalWrite(peristalicpumpA, LOW);
      digitalWrite(peristalicpumpB, LOW);

    }

  }


  if (ismotorrunning == false) {
    // Display title at top left corner
              //  display.fillScreen(ILI9341_BLACK);

  display.drawFastHLine(0, 39, 240, ILI9341_WHITE);
 if (!isRectFilled) {
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
    isRectFilled = true;  // Set the flag to true to indicate that the rectangle has been filled
  }
    display.setTextSize(1); //(1); //(1); //(1); //(4);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(5, 30);
    display.println("NPWT"); // Replace with your title
    display.println("  "); // Replace with your title


    display.setTextSize(1); //(1); //(1); //(1); //(3);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(10, 80); // Position below the title
    display.print("Pressure"); // Replace with your subtitle
    display.print("["+ String(setpressureval)+"]:  "); // Replace with your subtitle


    
  // display.setCursor(0, 120);
 display.setTextSize(1); //(1); //(1); //(1); //(3);
    display.setTextColor(ILI9341_WHITE);
    // display.print("Current: " ); // Replace with your subtitle

      display.fillRect(150, 60, 240, 30, ILI9341_BLACK);

    display.print( String(calibratedPressure)); // Replace with your subtitle

  // display.setCursor(0, 160);

 display.setTextSize(1); //(1); //(1); //(1); //(3);
    display.setTextColor(ILI9341_WHITE);
    // display.print("/" ); // Replace with your subtitle

      // display.fillRect(60, 110, 240, 30, ILI9341_BLACK);

    // display.println( String(setpressureval)); // Replace with your subtitle





   display.setTextSize(1); //(1); //(1); //(1); //(3);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(10, 130); // Position below the title
    // display.print("Timer: " ); // Replace with your subtitle
    display.print("Timer"); // Replace with your subtitle
    display.print("["+ String(timerLevel)+"]:  "); // Replace with your subtitle
    if(currentTime>0){
      display.fillRect(110, 110, 240, 30, ILI9341_BLACK);
      display.print(String((currentTime / (1000 * 60)) % 60) + ":" + String((currentTime / 1000) % 60));
}
else{
      display.print("0:0");
}

    
  // display.setCursor(0, 120);
//  display.setTextSize(1); //(1); //(1); //(1); //(3);
//     display.setTextColor(ILI9341_WHITE);
    // display.print("Current: " ); // Replace with your subtitle

    // display.print( String(calibratedPressure)); // Replace with your subtitle

  // display.setCursor(0, 160);

//  display.setTextSize(1); //(1); //(1); //(1); //(3);
//     display.setTextColor(ILI9341_WHITE);
//     display.print("/" ); // Replace with your subtitle

//       // display.fillRect(60, 110, 240, 30, ILI9341_BLACK);
//  display.println( String(timerLevel) + ":00"); // Replace with your subtitle




    display.setTextSize(1); //(1); //(1); //(1); //(2);
    display.setTextColor(ILI9341_WHITE);

    display.setCursor(10, 180); // Position below the title


    if (devicemode == 0) {
      display.print("Mode: ");
    display.setTextColor(ILI9341_WHITE);
      display.print(" NPWT");

     
    } else {
    display.setTextColor(ILI9341_WHITE);

      display.print("Mode :");
    display.setTextColor(ILI9341_WHITE);

      display.print("INSTILATION");
       
    }

    display.setCursor(0, 2000); // Position below the title



        display.drawFastHLine(0, 285, 240, ILI9341_WHITE);

    display.setCursor(10,312 );  // Adjusted the position of "Menu"
  // display.println(" Powered by");
      // display.print("Status:  Running");


    // display.setCursor(10, 312);  // Adjusted the position of "Menu"
      // display.fillRect(0, 10, 240, 30, ILI9341_PURPLE);

  // display.println(" EPIT Research Labs");





//  display.setTextSize(0); //(1); //(1); //(1); //(3);
//     display.setTextColor(ILI9341_BLUE);
//     display.setCursor(18, 250); // Position below the title
//     display.println("Powered by: " ); // Replace with your subtitle
//     display.setCursor(5, 265); // Position below the title

//     display.print("EPIT RESEARCH LABS (P.LTD) " ); // Replace with your subtitle

    // if (ischarging > 3) {
    //   display.setCursor(140, 20);
    // } else {
    //   display.setCursor(150, 20);
    // }
      display.setCursor(120, 20);

    if (runmotor == true) {
      
      display.setTextSize(1); //(1); //(1); //(1); //(1);
      display.setTextColor(ILI9341_WHITE);
      // display.println("R"); // Replace with your title

         display.setCursor(35,312 );  // Adjusted the position of "Menu"
  // display.println(" Powered by");
      display.print("DEVICE :  ON");

    } else {
     
  display.setTextSize(1); //(1); //(1); //(1); //(1);
      display.setTextColor(ILI9341_WHITE);
      // display.println("R"); // Replace with your title

         display.setCursor(35,312 );  // Adjusted the position of "Menu"
  

      display.print("DEVICE :  OFF");

    }


    // Display battery icon based on the battery level
      display.setCursor(200, 0);

    displayBatteryIcon(batteryLevel); // Function to display battery icon
    Serial.println("homescreen");
  }



}


void displayMenu() {
  
  // Display menu screen with five options


  if(display_menu==0){
  display.fillScreen(ILI9341_BLACK);
  }

display_menu=1;



  int xA = 5;
  int yA = 17;
  int lineSpacing = 5; // Adjust the vertical spacing between lines
  int lineHeight = 19;
  int lineCount = 3;
  int color = ILI9341_WHITE;

  // Draw the three horizontal lines for the menu cursor
  display.drawFastHLine(xA, yA, lineHeight, color);
  display.drawFastHLine(xA, yA + lineSpacing, lineHeight, color);
  display.drawFastHLine(xA, yA + 2 * lineSpacing, lineHeight, color);

  display.drawFastHLine(0, 39, 240, color);
  display.drawFastHLine(0, 265, 240, color);
  


  int width = display.width() - 20;
  int height = 30;
  int xPos, yPos;

  // Display "Menu" at the top left
  display.setTextSize(1);
  display.setTextColor(ILI9341_WHITE);
  display.setCursor(25, 30);  // Adjusted the position of "Menu"
      // display.fillRect(0, 10, 240, 30, ILI9341_PURPLE);

  display.println(" Menu");

  display.setTextColor(ILI9341_WHITE);

    display.setCursor(10,285 );  // Adjusted the position of "Menu"
  display.println(" Powered by");

    display.setCursor(10, 312);  // Adjusted the position of "Menu"
      // display.fillRect(0, 10, 240, 30, ILI9341_PURPLE);

  display.println(" EPIT Research Labs");

  // Display options with background shapes
  for (int i = 0; i < 5; i++) {
    xPos = 10;
    yPos = 80 + i * 35;  // Adjusted Y position and increased gap

    // Only draw a blue rectangle for the selected option
    if (selectedOption == i) {
      display.fillRect(xPos - 5, yPos - 20, width + 10, height, ILI9341_BLUE);  // Adjusted X and width
      display.setTextColor(ILI9341_WHITE);  // Adjusted text color for the selected option
    } else {
      display.setTextColor(ILI9341_WHITE);  // Set text color for unselected options
      display.fillRect(xPos - 5, yPos - 20, width + 10, height, ILI9341_BLACK);  // Adjusted X and width

    }

    // Display option text
    display.setCursor(xPos + 5, yPos);
    display.setTextSize(1);  // Increased text size
    switch (i) {
      case 0:
        display.println("Pressure");
        break;
      case 1:
        display.println("Timer");
        break;
      case 2:
        display.println("Instillation delay");
        break;
      case 3:
        display.println("Mode");
        break;
      case 4:
        display.println("About");
        break;
    }
  }
}






void displaySetPressureScreen() {
  display.fillScreen(ILI9341_BLACK);
  // display.setCursor(10, 5);
       
display_menu =0;



// drawEditSymbol(5,23,10,ILI9341_WHITE);

// drawAnalogPressureMeter(80,200,setpressureval);

drawReversedAnalogPressureMeter(120, 200, setpressureval, prevNeedleX, prevNeedleY);
            // display.fillRect(5, 22, 20, 10, ILI9341_BLACK);  // Adjusted X and width
//  display.fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)

  // display.printNew("     ", String(pressureValue));
  display.drawFastHLine(0, 39, 240, ILI9341_WHITE);

  // Display screen for setting pressure (example)
   display.setTextSize(1); //(1); //(1); //(1); //(2);  // Increased text size
  display.setCursor(35, 23);
  display.setTextColor(ILI9341_WHITE);
  //  display.fillRect(10, 0, 240, 30, ILI9341_BLUE);  // Adjusted X and width
      // display.setTextColor(ILI9341_WHITE);  // 
  display.println("Set Pressure");
  display.println(" ");
  display.println(" ");

   display.setCursor(20, 70);

  display.print("Pressure(mm Hg):");


  display.setCursor(80, 90);

      display.fillRect(10, 70, 240, 30, ILI9341_BLACK);  // Adjusted X and width

     display.setTextSize(1); //(1); //(1); //(1); //(2);  // Increased text size
  display.setTextColor(ILI9341_WHITE);
  display.print(setpressureval);


   display.setCursor(100, 250);

  display.println(setpressureval);

}


void displaySetPressureScreen_up_down() {
  // display.fillScreen(ILI9341_BLACK);
  // display.setCursor(10, 5);
       
clear_state=0;
// drawEditSymbol(5,23,10,ILI9341_WHITE);
// drawNegativePressureIcon(15,15);
// drawPressureMeter(15,15,calibratedPressure);
// drawAnalogPressureMeter(80,200,setpressureval);
drawReversedAnalogPressureMeter(120, 200, setpressureval, prevNeedleX, prevNeedleY);





            // display.fillRect(5, 22, 20, 10, ILI9341_BLACK);  // Adjusted X and width



  // display.printNew("     ", String(pressureValue));

  // Display screen for setting pressure (example)
   display.setTextSize(1); //(1); //(1); //(1); //(2);  // Increased text size
display.setCursor(35, 23);
  // display.setTextColor(ILI9341_WHITE);
  display.setTextColor(ILI9341_WHITE);
  
  //  display.fillRect(10, 0, 240, 30, ILI9341_BLUE);
  //  display.fillRect(10, 5, 240, 30, ILI9341_BLUE);  // Adjusted X and width
      // display.setTextColor(ILI9341_WHITE);  // 
  display.println("Set Pressure");
  display.println(" ");
  display.println(" ");

  // display.setCursor(10, 40);
  display.setCursor(20, 70);

  // display.print("Pressure:");
  display.print("Pressure(mm Hg):");



  // display.setCursor(80, 90);

      // display.fillRect(10, 70, 240, 30, ILI9341_BLACK);  // Adjusted X and width

  //    display.setTextSize(1); //(1); //(1); //(1); //(2);  // Increased text size
  // display.setTextColor(ILI9341_WHITE);
  // display.print(setpressureval);


 display.setCursor(100, 250);
      display.fillRect(80, 232, 80, 20, ILI9341_BLACK);  // Adjusted X and width

  display.println(setpressureval);
  // display.println("  mm Hg");

  // Display options for setting pressure or related content
  //display.display();
}


void displaytimerScreen() {
clear_state_timer=0;
display_menu =0;
// drawEditSymbol(0,23,1);
  display.fillScreen(ILI9341_BLACK);

  display.drawFastHLine(0, 39, 240, ILI9341_WHITE);
drawClockIcon(3,3);


 display.setTextSize(1); //(1); //(1); //(1); //(2);  // Increased text size
  display.setCursor(10, 22);
  display.setTextColor(ILI9341_WHITE);
  //  display.fillRect(10, 0, 240, 30, ILI9341_BLUE);

  //  display.fillRect(10, 5, 240, 30, ILI9341_BLUE);  // Adjusted X and width
      // display.setTextColor(ILI9341_WHITE);  // 
  display.println("     Set Timer:");
  display.println(" ");
  display.println(" ");

  display.setCursor(10, 70);

      // display.fillRect(10, 50, 240, 30, ILI9341_BLACK);  // Adjusted X and width

     display.setTextSize(1); //(1); //(1); //(1); //(2);  // Increased text size
  display.setTextColor(ILI9341_WHITE);
  display.print("Run time(min):   ");
  display.println(timerLevel);
  // Display options for setting pressure or related content
  //display.display();
}


void displaytimerScreen_up_down() {
// clear_state=0;
  // display.fillScreen(ILI9341_BLACK);
drawClockIcon(3,3);

 display.setTextSize(1); //(1); //(1); //(1); //(2);  // Increased text size
  display.setCursor(10, 22);
  display.setTextColor(ILI9341_WHITE);
  //  display.fillRect(10, 0, 240, 30, ILI9341_BLUE);

  //  display.fillRect(10, 5, 240, 30, ILI9341_BLUE);  // Adjusted X and width
      // display.setTextColor(ILI9341_WHITE);  // 
  display.println("     Set Timer:");
  display.println(" ");
  display.println(" ");

  display.setCursor(10, 70);

      display.fillRect(160, 45, 240, 30, ILI9341_BLACK);  // Adjusted X and width

     display.setTextSize(1); //(1); //(1); //(1); //(2);  // Increased text size
  display.setTextColor(ILI9341_WHITE);
  display.print("Run time(min):   ");
  display.println(timerLevel);
  // Display options for setting pressure or related content
  //display.display();
}



void devicemodescreen() {

display_menu =0;

 display.fillScreen(ILI9341_BLACK);
  display.setTextSize(1); //(1); //(1); //(1); //(1);
  display.setTextColor(ILI9341_WHITE);
  display.setCursor(0, 20);
  display.println("Set device mode");
  display.setCursor(0, 60);
  if (devicemode == 0) {
  display.setTextColor(ILI9341_GREEN);


    display.println("NPWT");
  } else {

  display.setTextColor(ILI9341_GREEN);

    display.println("INSTILATION");
  }

  // Display options for setting pressure or related content
  //display.display();

}

void deviceaboutscreen() {

//  display.fillScreen(ILI9341_BLACK);
//   display.setTextSize(1); //(1); //(1); //(1); //(1);
//   display.setTextColor(ILI9341_WHITE);
//   display.setCursor(30, 0);
//   display.println("About");
//   display.setCursor(0, 10);
//   display.println("Device : NPWT");
//   display.setCursor(0, 20);
//   display.println("Model : V1.0.0");
//   display.setCursor(20, 30);
//   display.println("Powered by");
//   display.setCursor(0, 40);
//   display.println("EPIT RESEARCH LABS");
//   display.setCursor(0, 50);
//   display.println("epitresearchlabs.com");

display_menu =0;




display.fillScreen(ILI9341_BLACK);
  unsigned long start = micros();
    display.drawFastHLine(0, 39, 240, ILI9341_WHITE);
  
    display.fillRect(0, 0, 240, 38, ILI9341_BLUE);
  display.setCursor(5, 28);
  display.setTextColor(ILI9341_WHITE);  display.setTextSize(1); //(1); //(1); //(1); //(2);
  display.println("About");

  display.setCursor(0, 80);

  display.setTextColor(ILI9341_RED);    display.setTextSize(1); //(1); //(1); //(1); //(2);
  display.println("      Device : NPWT");
  
  display.setTextColor(ILI9341_GREEN);
  display.setTextSize(1); //(1); //(1); //(1); //(2);
  display.println("      Model : V1.0.0");
  display.println(" ");

  display.setTextSize(1); //(1); //(1); //(1); //(2);

  display.println("Indigenous wound        therapy devices ");
  display.println("powered by");
  display.println("EPIT Research Labs");
   
  display.setTextSize(1); //(1); //(1); //(1); //(2);
  // display.println("Powered by    ");
  // // display.println(" ");
  // // display.setTextSize(1); //(1); //(1); //(1); //(2);
  // display.println("                  EPIT RL");
  //  display.println(" ");

  // display.println(" ");
display.setTextSize(0.2);
  display.setTextColor(ILI9341_RED);

  // Set the font for the entire display
  display.setFont(&FreeSans12pt7b);
  display.println(" ");

  display.println("www.epitresearchlab.com");

  // display.println("with crinkly bindlewurdles,");
  // display.println("Or I will rend thee");
  // display.println("in the gobberwarts");
  // display.println("with my blurglecruncheon,");
  // display.println("see if I don't!");
  //display.display();
}

void displaybrightnessScreen() {
  // Display screen for setting pressure (example)
display_menu =0;

  if (peristalicdiffval > 255) peristalicdiffval = 255;
  if (peristalicdiffval < 0) peristalicdiffval = 0;

 display.fillScreen(ILI9341_BLACK);
  display.setTextSize(1); //(1); //(1); //(1); //(1);
  display.setTextColor(ILI9341_WHITE);
  display.setCursor(0, 20);
  display.println("Set peristalic value");
  display.setCursor(0, 40);
  display.println(peristalicdiffval);
  // Display options for setting pressure or related content
  //display.display();
}

void displayLoadingAnimation() {
  int loadingWidth = 0; // Initialize the width of the loading bar
//  display.fillScreen(ILI9341_BLACK);
  // Loop to simulate the loading progress
  for (int i = 0; i < 150; i++) {
    // Clear display buffer
  //  display.fillScreen(ILI9341_BLACK);

    display.setTextSize(2); //(1); //(1); //(1); //(2);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(50, 50);
    display.println("NPWT"); // Replace with your title
  display.drawFastHLine(0, 65, 240, ILI9341_WHITE);


    display.setTextSize(1); //(1); //(1); //(1); //(1);
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(80, 120); // Position below the title
    display.println("Loading.."); // Replace with your subtitle

    // display.setCursor(50, 120); // Position below the title

    // Draw loading bar with incre asing width
    display.fillRect(50, 150, loadingWidth, 20, ILI9341_WHITE);

    // Update the display with the changes
    //display.display();

    // Increment the loading width for the next frame
    loadingWidth = map(i, 0, 127, 0, 128); // Map i to loading bar width

  }
}

int lowbattery = 0;

void displayBatteryIcon(int level) {

  if (ischarging > 3) {
   
      display.fillCircle(200, 250, 4, ILI9341_GREEN); // Battery terminal

  }

  else{

      display.fillCircle(200, 250, 4, ILI9341_BLACK); // Battery terminal
  }

  if (level <= 5) {
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
    if (lowbattery > 10) {
      lowbattery = 0;
      // Calculate the width of the battery indicator based on the battery level
      int batteryWidth = map(level, 0, 100, 0, 37); // Map battery level to pixel width (0-37)

      // Draw battery outline
      display.drawRect(185, 10, 54, 24, ILI9341_WHITE); // Battery outline
      display.fillRect(239, 17, 6, 12, ILI9341_WHITE); // Battery terminal

      // Draw battery charge level based on the calculated width
      display.fillRect(191, 17, batteryWidth, 12, ILI9341_WHITE); // Battery charge level
    }

    lowbattery++;

  } else {
    // Calculate the width of the battery indicator based on the battery level
    int batteryWidth = map(level, 0, 100, 0, 37); // Map battery level to pixel width (0-37)

    // Draw battery outline
    display.drawRect(185, 10, 54, 24, ILI9341_WHITE); // Battery outline
    display.fillRect(239, 17, 6, 12, ILI9341_WHITE); // Battery terminal

    // Draw battery charge level based on the calculated width
    display.fillRect(191, 17, batteryWidth, 12, ILI9341_WHITE); // Battery charge level
  }
  
  // Move the cursor to the top right corner
  display.setCursor(220, 10);
}






unsigned long testFillScreen() {
  unsigned long start = micros();
  display.fillScreen(ILI9341_BLACK);
  display.fillScreen(ILI9341_RED);
  display.fillScreen(ILI9341_GREEN);
  display.fillScreen(ILI9341_BLUE);
  display.fillScreen(ILI9341_BLACK);
  return micros() - start;
}


unsigned long testText() {
  display.fillScreen(ILI9341_BLUE);
  unsigned long start = micros();
  display.setCursor(0, 0);
  display.setTextColor(ILI9341_WHITE);  display.setTextSize(1); //(1); //(1); //(1); //(1);
  display.println("Hello World!");
  display.setTextColor(ILI9341_YELLOW); display.setTextSize(1); //(1); //(1); //(1); //(2);
  display.println(1234.56);
  display.setTextColor(ILI9341_RED);    display.setTextSize(1); //(1); //(1); //(1); //(3);
  display.println(0xDEADBEEF, HEX);
  display.println();
  display.setTextColor(ILI9341_GREEN);
  display.setTextSize(1); //(1); //(1); //(1); //(5);
  display.println("Groop");
  display.setTextSize(1); //(1); //(1); //(1); //(2);
  display.println("I implore thee,");
  display.setTextSize(1); //(1); //(1); //(1); //(1);
  display.println("my foonting turlingdromes.");
  display.println("And hooptiously drangle me");
  display.println("with crinkly bindlewurdles,");
  display.println("Or I will rend thee");
  display.println("in the gobberwarts");
  display.println("with my blurglecruncheon,");
  display.println("see if I don't!");
  return micros() - start;
}


void drawEditSymbol(int x, int y, int size, uint16_t color) {
  // Draw a pencil icon
  display.drawLine(x, y, x + size, y - size, color);
  display.drawLine(x + size, y - size, x + size / 2, y - 2 * size, color);
  display.drawLine(x + size / 2, y - 2 * size, x + size / 4, y - 2 * size, color);
  display.drawLine(x + size / 4, y - 2 * size, x, y - size, color);

  // Draw pencil body
  display.fillRect(x + size / 4, y - 2 * size, size / 2, size / 4, color);
}


void drawClockIcon(int xt, int yt) {
 
  // Draw the circular watch body
  display.fillCircle(xt + 10, yt + 10, 12, ILI9341_WHITE);

  // Draw clock face
  display.fillCircle(xt + 10, yt + 10, 8, ILI9341_BLACK);

  // Draw hour and minute hands
  display.drawLine(xt + 10, yt + 10, xt + 10, yt + 5, ILI9341_WHITE);  // Hour hand
  display.drawLine(xt + 10, yt + 10, xt + 15, yt + 10, ILI9341_WHITE);  // Minute hand

}

// void drawNegativePressureIcon(int xp, int yp) {
//   // Draw the meter outline
//   display.drawCircle(xp, yp, 15, ILI9341_WHITE); // Outer circle
//   display.drawCircle(xp, yp, 14.5, ILI9341_WHITE); // Inner circle
  
//   // Move the middle horizontal line down
//   // int middleY = yp + 6;
//   // display.drawLine(xp - 14, middleY, xp + 14, middleY, ILI9341_WHITE); // Horizontal line

//   // Draw scale lines
//   for (int i = -30; i <= 30; i += 10) {
//     float angle = radians(map(i, -30, 30, 210, 330)); // Map the angle based on the scale
//     int x1 = xp + 14 * cos(angle);
//     int y1 = yp + 14 * sin(angle);
//     int x2 = xp + 12 * cos(angle);
//     int y2 = yp + 12 * sin(angle);
//     display.drawLine(x1, y1, x2, y2, ILI9341_WHITE);
//   }

//   // Draw the deflection indicator
//   float deflectionAngle = radians(map(0, -30, 30, 210, 330)); // Map the angle for the deflection
//   int x3 = xp + 8 * cos(deflectionAngle);
//   int y3 = yp + 8 * sin(deflectionAngle);
//   int x4 = xp - 8 * cos(deflectionAngle);
//   int y4 = yp - 8 * sin(deflectionAngle);
//   display.drawLine(x3, y3, x4, y4, ILI9341_WHITE);

// }
void drawReversedAnalogPressureMeter(int xp, int yp, float pressureValue, int& prevNeedleX, int& prevNeedleY) {
  // Move the center of the circle 2mm up
  // yp -= 2;

  // Draw the meter outline
  display.drawCircle(xp, yp, 75, ILI9341_WHITE); // Outer circle
  display.drawCircle(xp, yp, 74.25, ILI9341_WHITE); // Inner circle

  // Draw the scale lines for min, max, and in-between values
  for (int i = 0; i <= 180; i += 10) {
    float angle = radians(i);

    // Calculate coordinates for the scale lines
    int x1 = xp + 67.5 * cos(angle);
    int y1 = yp - 67.5 * sin(angle); // Invert the y-coordinate to rotate upside down
    int x2 = xp + (i % 30 == 0 ? 63 : 65) * cos(angle);
    int y2 = yp - (i % 30 == 0 ? 63 : 65) * sin(angle);

    // Draw scale lines
    display.drawLine(x1, y1, x2, y2, ILI9341_WHITE);

    // Draw scale values only for min and max
    if (i == 0 || i == 180) {
      int value = i == 0 ? 0 : 300; // Set min value for 0 and max value for 180
      display.setCursor(x2 - 10, y2+18);
      display.setTextColor(ILI9341_WHITE);
      display.setTextSize(1);
      display.print(value);
    }
  }

  // Limit pressureValue between 0 and 300
  pressureValue = constrain(pressureValue, 0, 300);

  // Calculate the position of the needle based on the pressure value
  float needleAngle = radians(map(pressureValue, 0, 300, 0, 180)); // Map pressure to angle (0-180)
  int needleX = xp + 60 * cos(needleAngle);
  int needleY = yp - 60 * sin(needleAngle); // Invert the y-coordinate to rotate upside down

  // Draw the meter center
  display.fillCircle(xp, yp, 7.5, ILI9341_RED);

  // Fill the area between the previous and current needle position with black
  display.drawLine(xp, yp, prevNeedleX, prevNeedleY, ILI9341_BLACK);

  // Draw the needle
  display.drawLine(xp, yp, needleX, needleY, ILI9341_RED);

  // Update the previous needle position
  prevNeedleX = needleX;
  prevNeedleY = needleY;
}






void drawReversedAnalogPressureMeter_HOME(int xp, int yp, float pressureValue, int& prevNeedleX, int& prevNeedleY) {
  // Move the center of the circle 2mm up
  // yp -= 2;

  // Calculate the scaling factor (60% of the original size)
  float scaleFactor = 0.6;

  // Apply the scaling factor to the relevant dimensions
  int scaledOuterRadius = 75 * scaleFactor;
  int scaledInnerRadius = 74.25 * scaleFactor;
  int scaledScaleLength = 67.5 * scaleFactor;
  int scaledScaleGap = 2.25 * scaleFactor; // Adjusted gap between scale lines
  int scaledNeedleLength = 60 * scaleFactor;
  int scaledCenterRadius = 7.5 * scaleFactor;

  // Draw the meter outline
  display.drawCircle(xp, yp, scaledOuterRadius, ILI9341_WHITE); // Outer circle
  display.drawCircle(xp, yp, scaledInnerRadius, ILI9341_WHITE); // Inner circle

  // Draw the scale lines for min, max, and in-between values
  for (int i = 0; i <= 180; i += 10) {
    float angle = radians(i);

    // Calculate coordinates for the scale lines
    int x1 = xp + scaledScaleLength * cos(angle);
    int y1 = yp - scaledScaleLength * sin(angle); // Invert the y-coordinate to rotate upside down
    int x2 = xp + (i % 30 == 0 ? (scaledScaleLength - scaledScaleGap) : scaledScaleLength) * cos(angle);
    int y2 = yp - (i % 30 == 0 ? (scaledScaleLength - scaledScaleGap) : scaledScaleLength) * sin(angle);

    // Draw scale lines
    display.drawLine(x1, y1, x2, y2, ILI9341_WHITE);

    // Draw scale values only for min and max
    if (i == 0 || i == 180) {
      int value = i == 0 ? 0 : 300; // Set min value for 0 and max value for 180
      display.setCursor(x2 - 10, y2 + 18);
      display.setTextColor(ILI9341_WHITE);
      display.setTextSize(1);
      display.print(value);
    }
  }

  // Limit pressureValue between 0 and 300
  pressureValue = constrain(pressureValue, 0, 300);

  // Calculate the position of the needle based on the pressure value
  float needleAngle = radians(map(pressureValue, 0, 300, 0, 180)); // Map pressure to angle (0-180)
  int needleX = xp + scaledNeedleLength * cos(needleAngle);
  int needleY = yp - scaledNeedleLength * sin(needleAngle); // Invert the y-coordinate to rotate upside down

  // Draw the meter center
  display.fillCircle(xp, yp, scaledCenterRadius, ILI9341_RED);

  // Fill the area between the previous and current needle position with black
  display.drawLine(xp, yp, prevNeedleX, prevNeedleY, ILI9341_BLACK);

  // Draw the needle
  display.drawLine(xp, yp, needleX, needleY, ILI9341_RED);

  // Update the previous needle position
  prevNeedleX = needleX;
  prevNeedleY = needleY;
}





