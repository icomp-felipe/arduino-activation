#include <Keypad.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

/********** Function Declaration *************/

void      displayTime();
void      setTime();
DateTime* parseDate(char* input);

/*************** RTC Variables ***************/

// RTC DS3231 Object
RTC_DS3231 rtc;

/*************** LCD Variables ***************/

// LCD 16x2 Rows
char lcdRow1[20], lcdRow2[20];
LiquidCrystal lcd(A0, A1, 10, 11, 12, 13);

/************ Keyboard Variables *************/

// Key pressed
char key;

// Keyboard mapping (using 4x4 matrix)
char keyMap[4][4] = {
    {'1','2','3', 'A'},
    {'4','5','6', 'B'},
    {'7','8','9', 'C'},
    {'*','0','#', 'D'}
};

// Defining keyboard pin set for RTC module
byte rowPins[4] = { 9, 8, 7, 6 };
byte colPins[4] = { 5, 4, 3, 2 };

// Starting keypad lib object
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, 4, 4);

/************** Menu Variables ***************/

// State machine for menu control
enum menu { menuShowTime, menuSetTime, menuSetActivation };
int curState;

/*********************************************/
/****************** SETUP ********************/
/*********************************************/

int lastSecond = 0;
char lastKey = 'A';

void setup() {

    // Initializing 16x2 display
    lcd.begin(16, 2);

    // Setting default variable values
    curState = menuShowTime;

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

    delay(100);

}

/*********************************************/
/******************* LOOP ********************/
/*********************************************/

void loop() {

    // Retrieving keyboard key pressed
    key = keypad.getKey();

    // State machine control
    if ((key != NO_KEY) && (lastKey != key)) {

        switch (key) {

            case 'A':
                curState = menuShowTime;
                break;

            case 'B':
                curState = menuSetTime;
                break;

        }

    }

    // Menu control (based on current state)
    switch (curState) {

        case menuShowTime:
            displayTime();
            break;

        case menuSetTime:
            setTime();
            curState = menuShowTime;
            break;
        
    }

}


// Displays the current time (every second)
void displayTime() {

    // Retrieving the current DateTime object
    DateTime now = rtc.now();

    int curSecond = now.second();

    // If the time has passed (in terms of seconds), the display is updated
    if (lastSecond != curSecond) {

        sprintf(lcdRow1,"AtivSys|%02d/%02d/%02d", now.day (), now.month (), now.year() - 2000);
        sprintf(lcdRow2," v.1.0 |%02d:%02d:%02d", now.hour(), now.minute(), curSecond);
 
        lcd.clear();
        lcd.print(lcdRow1);
        lcd.setCursor(0,1);
        lcd.print(lcdRow2);

        lastSecond = curSecond;

    }

}

// Allows the user to manually adjust the system's date and time
void setTime() {

    int  index = 0;

    // Stores the typed datetime (numbers only: 6 representing date + 6 representing time + 1 storing '\0')
    char input[13] = "";

    // Displays the menu
    lcd.clear();
    lcd.print("Ajustar|__/__/__");
    lcd.setCursor(0,1);
    lcd.print(" Data  |__:__:__");

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

            // Prints the typed datetime
            printTime(input);
            
        }
        
    }

    // After pressing '*', if the datetime is complete...
    if (index == 12) {

        // ...then it needs to be validated
        DateTime* datetime = parseDate(input);

        lcd.clear();

        // If the date is valid, then it's copied to the RTC module
        if (datetime != NULL) {
            
            rtc.adjust(*datetime);  free(datetime);

            lcd.print(" Data ajustada!");
            
        }
        else
            lcd.print(" Data invalida!");
        
        delay(3000);

    }

}

// Tests if a given date is valid, returns NULL it it's not!
DateTime* parseDate(char* input) {

    // Converting the char input data (from keyboard) to integer values
    int   day = (input[0] - '0') * 10 + input[1] - '0';
    int month = (input[2] - '0') * 10 + input[3] - '0';
    int  year = (input[4] - '0') * 10 + input[5] - '0' + 2000;

    int hour = (input[ 6] - '0') * 10 + input[ 7] - '0';
    int  min = (input[ 8] - '0') * 10 + input[ 9] - '0';
    int  sec = (input[10] - '0') * 10 + input[11] - '0';
    
    /*********************** General input validation ***********************/

    if ((month <  1) || (month > 12) ||     // for month (between 1-12),
        (  day <  1) || (  day > 31) ||     // days (between 1-31),
        ( hour > 23) ||                     // hours (up to 23),
        (  min > 59) ||                     // minutes (up to 59),
        (  sec > 59)                        // and seconds (up to 59)
       )
       return NULL;
    
    /****** Specific input validation (considering days of each month) ******/

    if (month == 2) {                                                   // If it's February,

        if (year % 4 == 0 && (year % 400 == 0 || year % 100 != 0)) {    // the year is leap,
            if (day > 29)                                               // and it has more than 29 days,
                return NULL;                                            // then we have an exception
        }
        else if (day > 28)                                              // otherwise, if the year is not leap and it has more than 28 days,
            return NULL;                                                // we have another exception
    
    }
    
    if (month < 8) {                                // If the month is between January and July (except February)   
        if (((month % 2) == 0) && (day > 30))       // then we do some calculations to determine their valid maximum number of days.
            return NULL;
    }
    else { 
        if (((month % 2) != 0) && (day > 30))       // The same happens here, but for months between August and December.
            return NULL;  
    }

    // If we're here, that means the datetime is valid, so it's time to create and return the object.
    DateTime  local = DateTime(year,month,day,hour,min,sec);
    DateTime* point = (DateTime*) malloc(sizeof(DateTime));

    memcpy(point,&local,sizeof(DateTime));

    return point;

}

void printTime(char* input) {

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

    /*Serial.print(lcdRow1);
    Serial.print(" - ");
    Serial.println(lcdRow2);*/
    lcd.setCursor(8,0);
    lcd.print(lcdRow1);
    lcd.setCursor(8,1);
    lcd.print(lcdRow2);

}
