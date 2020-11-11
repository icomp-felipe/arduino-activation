#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

// Custom libraries
#include <DateTimeUtils.h>
#include <EEPROMUtils.h>

/************** Relay Variables **************/
const int R1 = A3;
const int R2 = A2;

int RelayNewState, RelayState;
enum relay { R1Active, R2Active };

/*************** RTC Variables ***************/

// RTC DS3231 Object
const RTC_DS3231 rtc;

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

// State machine for menu control
enum menu { menuShowTime, menuSetTime, menuFlipFlop, menuAlterDate };
int curState;

/*********************************************/
/****************** SETUP ********************/
/*********************************************/

int lastSecond;
DateTime now;

void setup() {

    // Configuring pins
    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);

    // Initializing 16x2 display
    lcd.begin(16, 2);

    // Setting default variable values
    curState = menuShowTime;
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
    RelayState = -1;
    RelayNewState = R1Active;

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
                curState = menuSetTime;
                break;
            
            case 'B':
                curState = menuFlipFlop;
                break;
            
            case 'C':
                curState = menuAlterDate;
                break;

        }

    }

    // Menu control (based on current state)
    switch (curState) {

        case menuShowTime:
            if (timeHasPassed())
                displayTime();
            break;

        case menuSetTime:
            setTime();
            curState = menuShowTime;
            break;
        
        case menuFlipFlop:
            flipFlop();
            curState = menuShowTime;
            break;
        
    }

    // Controls the relays behaviour (synced with time)
    if (timeHasPassed()) {
        relayControl();
        lastSecond = now.second();      // Prevents redundant writes to RAM every cicle
    }
    
}

DateTime a = DateTime(0,0,0,7,30,0);
DateTime b = DateTime(0,0,0,18,0,0);

void relayControl() {

    RelayNewState = (DateTimeUtils::greater(&now,&a) && DateTimeUtils::lower(&now,&b)) ? R1Active : R2Active;

    if (RelayState != RelayNewState) {

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

        RelayState = RelayNewState;

    }

}

// Tells if the time has passed (in terms of seconds)
boolean timeHasPassed() {
    return (lastSecond != now.second());
}

// Displays the current time (every second)
void displayTime() {

    sprintf(lcdRow1,"AtivSys|%02d/%02d/%02d", now.day (), now.month (), now.year() - 2000);
    sprintf(lcdRow2," v.1.0 |%02d:%02d:%02d", now.hour(), now.minute(), now.second());
 
    lcd.clear();
    lcd.print(lcdRow1);
    lcd.setCursor(0,1);
    lcd.print(lcdRow2);

}

char* readKeyboardInput(void (*printer)(char*)) {

    int index = 0;

    // Stores the typed datetime (numbers only: 6 representing date + 6 representing time + 1 storing '\0')
    char* input = (char*) calloc(13, sizeof(char));

    // Wait for user input until the '#' character is typed
    while ((key = keypad.getKey()) != '#') {
        
        // If something has been typed...
        if (key != NO_KEY) {

            // Backspace implementation
            if (key == '*') {
                if (index > 0)
                  input[--index] = '\0';
            }
          
            // Storing typed keys (limited to 12 numeric characters)
            else if ((index < 12) && (key >= '0') && (key <= '9')) {

                input[index++] = key;
                input[index] = '\0';
                    
            }

            // Prints the typed input using the given printer function
            printer(input);
            
        }
        
    }

    if (index == 12)
        return input;
    
    free(input);

    return NULL;
}

// Allows the user to manually adjust the system's date and time
void setTime() {

    // Displays the menu
    lcd.clear();
    lcd.print("Ajustar|__/__/__");
    lcd.setCursor(0,1);
    lcd.print(" Data  |__:__:__");

    char* input = readKeyboardInput(printDate);
    
    // After pressing '*', if the datetime is complete...
    if (input != NULL) {

        // ...then it needs to be validated
        DateTime datetime = DateTimeUtils::extractDateTime(input);

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

void flipFlop() {

    DateTime mem1, mem2;

    readFlipFlopTimes(&mem1, &mem2);

    if (mem1.isValid() && mem2.isValid()) {
        sprintf(lcdRow1, " Liga  |%02d:%02d:%02d", mem1.hour(), mem1.minute(), mem1.second());
        sprintf(lcdRow2, "Desliga|%02d:%02d:%02d", mem2.hour(), mem2.minute(), mem2.second());
    }
    else {
        sprintf(lcdRow1, " Liga  |__:__:__");
        sprintf(lcdRow2, "Desliga|__:__:__");
    }
    
    // Displays the menu
    lcd.clear();
    lcd.print(lcdRow1);
    lcd.setCursor(0,1);
    lcd.print(lcdRow2);

    char* input = readKeyboardInput(printTime);
    
    // After pressing '*', if the all the times were completely typed...
    if (input != NULL) {

        // ...then they need to be validated
        DateTime time1 = DateTimeUtils::extractTime1(input);
        DateTime time2 = DateTimeUtils::extractTime2(input);

        lcd.clear();

        // If the date is valid, then it's copied to the RTC module
        if (time1.isValid() && time2.isValid()) {

            if (DateTimeUtils::same(&time1,&time2)) {
                lcd.print("Horarios Iguais!");
                lcd.setCursor(2,1);
                lcd.print("Ignorando...");
            }
            else {

                if (!DateTimeUtils::same(&mem1, &time1))
                    saveFlipFlopTime(&time1, 0);
                
                if (!DateTimeUtils::same(&mem2, &time2))
                    saveFlipFlopTime(&time2, 6);

                lcd.print("Configuracao OK!");
            }
            
        }
        else {
            lcd.setCursor(3,0);
            lcd.print("Horario(s)");
            lcd.setCursor(2,1);
            lcd.print("Invalido(s)");
        }
        
        delay(3000);

        free(input);

    }

}

void saveFlipFlopTime(DateTime* time, int address) {

    int array[3] = { time->hour(), time->minute(), time->second() };

    EEPROMUtils::writeIntArray(array,3,address);
}

void readFlipFlopTimes(DateTime* time1, DateTime* time2) {

    int* array = EEPROMUtils::readIntArray(6,0);
    
    *time1 = DateTime(2020,1,1,array[0],array[1],array[2]);
    *time2 = DateTime(2020,1,1,array[3],array[4],array[5]);

}

void printDateTime(char* input, boolean isTimeOnly) {

    if (isTimeOnly)
        sprintf(lcdRow1,"__:__:__");
    else
        sprintf(lcdRow1,"__/__/__");

    sprintf(lcdRow2,"__:__:__");

    for (int i=0, shift = 0; input[i] != '\0'; i++) {

        if (i < 6) {

            lcdRow1[i + shift] = input[i];

            if (((i+1) % 2) == 0)
                shift++;
          
        }
            
        else if (i < 12) {

            if (i == 6)
                shift = 0;

            lcdRow2[i - 6 + shift] = input[i];

            if (((i+1) % 2) == 0)
                shift++;
          
        }
        
    }

    lcd.setCursor(8,0);
    lcd.print(lcdRow1);
    lcd.setCursor(8,1);
    lcd.print(lcdRow2);

}

void printDate(char* input) {
    printDateTime(input,false);
}

void printTime(char* input) {
    printDateTime(input,true);
}