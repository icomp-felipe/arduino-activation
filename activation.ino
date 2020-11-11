#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

// Custom libraries
#include <DateTimeUtils.h>
#include <EEPROMUtils.h>

/************ EEPROM Addresses ***************/
const int   ACTIVATION_TIME_ADDR = 0;
const int DEACTIVATION_TIME_ADDR = 6;

const int   ACTIVATION_DAY_ADDR = 12;
const int DEACTIVATION_DAY_ADDR = 14;

/*************** LCD Variables ***************/

// LCD 16x2 Rows
char lcdRow1[20], lcdRow2[20];
const LiquidCrystal lcd(A0, A1, 10, 11, 12, 13);

/************ Keyboard Variables *************/

// Key pressed
char key;

// Keyboard mapping (using 4x4 matrix)
const char keyMap[4][4] = {
    {'1','2','3', 'A'},
    {'4','5','6', 'B'},
    {'7','8','9', 'C'},
    {'*','0','#', 'D'}
};

// Defining keyboard pin set for RTC module
const byte rowPins[4] = { 9, 8, 7, 6 };
const byte colPins[4] = { 5, 4, 3, 2 };

// Starting keypad lib object
const Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, 4, 4);

/************** Menu Variables ***************/

// Menu state machine variables and constants
enum menu { menuShowDateTime, menuSetDateTime, menuTimeActivation, menuDateActivation, menuDateTimeActivation };
int  menuCurrentState;

/************** Relay Variables **************/

// Relay output pins
const int R1 = A3;
const int R2 = A2;

// Relay state machine variables and constants
enum relay { R1Active, R2Active };
int  RelayNewState, RelayCurrentState;

/*************** RTC Variables ***************/

// RTC DS3231 Object
const RTC_DS3231 rtc;

/********** Time Control Variables ***********/ 

int lastSecond;
DateTime now;
DateTime activationTime, deactivationTime;
int      activationDay , deactivationDay ;

/*********************************************/
/****************** SETUP ********************/
/*********************************************/

void setup() {

    // Configuring pins
    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);

    // Initializing 16x2 display
    lcd.begin(16, 2);

    // Setting default variable values
    menuCurrentState = menuShowDateTime;
    lastSecond = 0;

    // Testing if the RTC module is present
    if (!rtc.begin()) {

        while (true) {

            lcd.clear();
            lcd.print("Modulo relogio");
            lcd.setCursor(0,1);
            lcd.print("nao encontrado");

            delay(3000);

            lcd.clear();
            lcd.print("Verifique o mo-");
            lcd.setCursor(0,1);
            lcd.print("dulo e reinicie");

            delay(3000);

        }

    }

    // Defining which relay will be first activated
    RelayCurrentState = -1;
    RelayNewState = R1Active;

    // Retrieving data from EEPROM
    activationTime   = DateTimeUtils::readTime(  ACTIVATION_TIME_ADDR);
    deactivationTime = DateTimeUtils::readTime(DEACTIVATION_TIME_ADDR);

    activationDay   = DateTimeUtils::readDay(  ACTIVATION_DAY_ADDR);
    deactivationDay = DateTimeUtils::readDay(DEACTIVATION_DAY_ADDR);

    delay(100);

}

/*********************************************/
/******************* LOOP ********************/
/*********************************************/

void loop() {

    // Retrieving the current DateTime object
    now = rtc.now();

    // Retrieving keyboard key pressed
    key = keypad.getKey();

    // State machine control (based on keyboard input)
    if (key != NO_KEY) {

        switch (key) {

            case 'A':
                menuCurrentState = menuSetDateTime;
                break;
            
            case 'B':
                menuCurrentState = menuTimeActivation;
                break;
            
            case 'C':
                menuCurrentState = menuDateActivation;
                break;

        }

    }

    // Menu control (based on current state)
    switch (menuCurrentState) {

        case menuShowDateTime:
            if (DateTimeUtils::timeHasPassed(lastSecond, now.second()))
                displayTime();
            break;

        case menuSetDateTime:
            setDateTime();
            menuCurrentState = menuShowDateTime;
            break;
        
        case menuTimeActivation:
            setTimeActivation();
            menuCurrentState = menuShowDateTime;
            break;
        
        case menuDateActivation:
            setDateActivation();
            menuCurrentState = menuShowDateTime;
            break;
        
    }

    // Sync operations need to be inside this 'if' clause. This means they'll only be run as second passes instead of every 'loop' call
    if (DateTimeUtils::timeHasPassed(lastSecond, now.second())) {

        relayStateControl();

        lastSecond = now.second();

    }
    
}

/*********************************************/
/************ Relay State Machine ************/
/*********************************************/

void relayStateControl() {

    RelayNewState = (DateTimeUtils::timeBetween(&now,&activationTime,&deactivationTime)) ? R1Active : R2Active;

    if (RelayCurrentState != RelayNewState) {

        if (RelayNewState == R1Active) {

            digitalWrite(R1, LOW);
            delay(1000);
            digitalWrite(R2, HIGH);

        }
        else if (RelayNewState == R2Active) {

            digitalWrite(R2, LOW);
            delay(1000);
            digitalWrite(R1, HIGH);

        }

        RelayCurrentState = RelayNewState;

    }

}

/*********************************************/
/************* Menu Functions ****************/
/*********************************************/

// Displays the current time (every second)
void displayTime() {

    sprintf(lcdRow1,"AtivSys|%02d/%02d/%02d", now.day (), now.month (), now.year() - 2000);
    sprintf(lcdRow2," v.1.0 |%02d:%02d:%02d", now.hour(), now.minute(), now.second());
 
    lcd.clear();
    lcd.print(lcdRow1);
    lcd.setCursor(0,1);
    lcd.print(lcdRow2);

}

// Allows the user to manually adjust the system's date and time
void setDateTime() {

    // Displays the menu
    lcd.clear();
    lcd.print("Ajustar|__/__/__");
    lcd.setCursor(0,1);
    lcd.print(" Data  |__:__:__");

    // Stores the typed datetime (numbers only: 6 representing date + 6 representing time + 1 storing '\0')
    char buffer[13] = "";
    
    // Retrieve keyboard numeric data (until '*' is pressed)
    boolean bufferIsComplete = readKeyboardInput(buffer,12,printDate);
    
    // If the datetime data is complete (all 12 numeric characters have been typed)...
    if (bufferIsComplete) {

        // ...then it needs converted to a 'DateTime' class instance
        DateTime datetime = DateTimeUtils::extractDateTime(buffer);

        lcd.clear();

        // If the date is valid, then it's copied to the RTC module
        if (datetime.isValid()) {
            
            rtc.adjust(datetime);

            lcd.print(" Data ajustada!");
            
        }
        else
            lcd.print(" Data invalida!");
        
        delay(3000);

    }

}

// Enables the user to set the activation/deactivation times
void setTimeActivation() {

    // If both previously saved times are valid, then they're printed to the LCD strings
    if (activationTime.isValid() && deactivationTime.isValid()) {
        sprintf(lcdRow1, " Liga  |%02d:%02d:%02d", activationTime  .hour(), activationTime  .minute(), activationTime  .second());
        sprintf(lcdRow2, "Desliga|%02d:%02d:%02d", deactivationTime.hour(), deactivationTime.minute(), deactivationTime.second());
    }

    // This will be only run when EEPROM has no DateTime data for the time activation function
    else {
        sprintf(lcdRow1, " Liga  |__:__:__");
        sprintf(lcdRow2, "Desliga|__:__:__");
    }
    
    // Displays the menu
    lcd.clear();
    lcd.print(lcdRow1);
    lcd.setCursor(0,1);
    lcd.print(lcdRow2);

    // Stores the typed times (numbers only: 6 representing the activation time + 6 representing the deactivation time + 1 storing '\0')
    char buffer[13] = "";
    
    // Retrieve keyboard numeric data (until '#' is pressed)
    boolean bufferIsComplete = readKeyboardInput(buffer,12,printTime);
    
    // If the buffer data is complete (all 12 numeric characters have been typed)...
    if (bufferIsComplete) {

        // ...then they need to be converted to 'DateTime' class instances
        DateTime time1 = DateTimeUtils::extractTime(buffer,0);
        DateTime time2 = DateTimeUtils::extractTime(buffer,6);

        lcd.clear();

        // If the times are valid...
        if (time1.isValid() && time2.isValid()) {

            // ...and they're the same, then they'll be ignored
            if (DateTimeUtils::sameTime(&time1,&time2)) {
                lcd.print("Horarios Iguais!");
                lcd.setCursor(2,1);
                lcd.print("Ignorando...");
            }

            // Otherwise, they'll be stored in Arduino's EEPROM only if they have been actually changed
            else {

                if (!DateTimeUtils::sameTime(&activationTime, &time1)) {
                    DateTimeUtils::saveTime(&time1, ACTIVATION_TIME_ADDR);
                    activationTime = time1;
                }
                
                if (!DateTimeUtils::sameTime(&deactivationTime, &time2)) {
                    DateTimeUtils::saveTime(&time2, DEACTIVATION_TIME_ADDR);
                    deactivationTime = time2;
                }

                lcd.print("Configuracao OK!");
            }
            
        }

        // If the time(s) are not valid, an error message is shown
        else {
            lcd.setCursor(3,0);
            lcd.print("Horario(s)");
            lcd.setCursor(2,1);
            lcd.print("Invalido(s)");
        }
        
        delay(3000);

    }

}

// Enables the user to set the activation/deactivation days
void setDateActivation() {

    // If both previously saved days are valid, then they're printed to the LCD strings
    if (DateTimeUtils::dayIsValid(activationDay) && DateTimeUtils::dayIsValid(deactivationDay)) {
        sprintf(lcdRow1, "   Liga dia: %02d",   activationDay);
        sprintf(lcdRow2, "Desliga dia: %02d", deactivationDay);
    }

    // This will be only run when EEPROM has no data for the day activation function
    else {
        sprintf(lcdRow1, "   Liga dia: __");
        sprintf(lcdRow2, "Desliga dia: __");
    }

    // Displays the menu
    lcd.clear();
    lcd.print(lcdRow1);
    lcd.setCursor(0,1);
    lcd.print(lcdRow2);

    // Stores the typed times (numbers only: 2 representing the activation day + 2 representing the deactivation day + 1 storing '\0')
    char buffer[5] = "";

    // Retrieve keyboard numeric data (until '#' is pressed)
    boolean bufferIsComplete = readKeyboardInput(buffer,4,printDay);
    
    // If the buffer data is complete (all 4 numeric characters have been typed)...
    if (bufferIsComplete) {

        // ...then, they need to be converted to 'int' values
        int day1 = DateTimeUtils::extractDay(buffer,0);
        int day2 = DateTimeUtils::extractDay(buffer,2);

        lcd.clear();

        // If the days are valid...
        if (DateTimeUtils::dayIsValid(day1) && DateTimeUtils::dayIsValid(day2)) {

            // ...they'll be stored in Arduino's EEPROM only if they have been actually changed
            if (activationDay != day1) {
                DateTimeUtils::saveDay(day1, ACTIVATION_DAY_ADDR);
                activationDay = day1;
            }
                
            if (deactivationDay != day2) {
                DateTimeUtils::saveDay(day2, DEACTIVATION_DAY_ADDR);
                deactivationDay = day2;
            }

            lcd.print("Configuracao OK!"); 

        }

        // If the day(s) are not valid, an error message is shown
        else {
            lcd.setCursor(5,0);
            lcd.print("Dia(s)");
            lcd.setCursor(3,1);
            lcd.print("Invalido(s)");
        }

        delay(3000);

    }

}

/*********************************************/
/*************** Utilities *******************/
/*********************************************/

// Prints the days as they're typed
void printDay(char* input) {

    // Defines the mask for the LCD rows
    sprintf(lcdRow1,"__");
    sprintf(lcdRow2,"__");

    // Prints the actual data
    for (int i=0; input[i] != '\0'; i++) {

        // Calculating the first LCD row string
        if (i < 2)
            lcdRow1[i] = input[i];
        
        // Calculating the second LCD row string
        else if (i < 4)
            lcdRow2[i - 2] = input[i];
        
    }

    // After calculating the strings, it's time to show them!
    lcd.setCursor(13,0);
    lcd.print(lcdRow1);
    lcd.setCursor(13,1);
    lcd.print(lcdRow2);

}

// Prints the dates or times as they're typed
void printDateTime(char* input, boolean isTimeOnly) {

    // Defines the mask for the first LCD row
    if (isTimeOnly)
        sprintf(lcdRow1,"__:__:__");
    else
        sprintf(lcdRow1,"__/__/__");

    // Defines the mask for the second LCD row
    sprintf(lcdRow2,"__:__:__");

    // Prints the actual data, considering the 'shift' variable to prevent overwriting the separators (: or /)
    for (int i=0, shift = 0; input[i] != '\0'; i++) {

        // Calculating the first LCD row string
        if (i < 6) {

            lcdRow1[i + shift] = input[i];

            if (((i+1) % 2) == 0)
                shift++;
          
        }
        
        // Calculating the second LCD row string
        else if (i < 12) {

            if (i == 6)
                shift = 0;

            lcdRow2[i - 6 + shift] = input[i];

            if (((i+1) % 2) == 0)
                shift++;
          
        }
        
    }

    // After calculating the strings, it's time to show them!
    lcd.setCursor(8,0);
    lcd.print(lcdRow1);
    lcd.setCursor(8,1);
    lcd.print(lcdRow2);

}

// Prints a date in the first LCD row and a time in the second LCD row
void printDate(char* date) {
    printDateTime(date,false);
}

// Prints two dates in the LCD rows
void printTime(char* times) {
    printDateTime(times,true);
}

/* Reads 'length' numeric characters to the given 'buffer', displaying the typed data using the given 'printer' function.
 * To break execution, the '#' character must be typed.
 * Backspace is available at the '*' character.
 * Returns 'true' only if all 'length' characters have been typed. */
boolean readKeyboardInput(char* buffer, int length, void (*printer)(char*)) {

    int index = 0;

    // Wait for user input until the '#' character is typed
    while ((key = keypad.getKey()) != '#') {
        
        // If something has been typed...
        if (key != NO_KEY) {

            // Backspace implementation
            if (key == '*') {
                if (index > 0)
                  buffer[--index] = '\0';
            }
          
            // Storing typed keys (limited to 'length' numeric characters)
            else if ((index < length) && (key >= '0') && (key <= '9')) {

                buffer[index++] = key;
                buffer[index] = '\0';
                    
            }

            // Prints the typed input using the given printer function
            printer(buffer);
            
        }
        
    }

    return (index == length);
}