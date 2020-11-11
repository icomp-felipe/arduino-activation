#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

/********** Function Declaration *************/

void     displayTime();
void     setTime();
DateTime extractDateTime(char* input);

/************** Relay Variables **************/
int R1 = A3;
int R2 = A2;

int RelayNewState, RelayState;
enum relay { R1Active, R2Active };

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
enum menu { menuShowTime, menuSetTime, menuFlipFlop };
int curState;

/*********************************************/
/****************** SETUP ********************/
/*********************************************/

int lastSecond = 0;

void setup() {

    // Configuring pins
    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);

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

    // Defining which relay will be first activated
    RelayState = -1;
    RelayNewState = R1Active;

    delay(100);

}

/*********************************************/
/******************* LOOP ********************/
/*********************************************/

DateTime now;

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

    RelayNewState = (greater(&now,&a) && lower(&now,&b)) ? R1Active : R2Active;

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
        DateTime datetime = extractDateTime(input);

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
        DateTime time1, time2;

        extractTime(input, &time1, &time2);
        lcd.clear();

        // If the date is valid, then it's copied to the RTC module
        if (time1.isValid() && time2.isValid()) {

            if (same(&time1,&time2)) {
                lcd.print("Horarios Iguais!");
                lcd.setCursor(2,1);
                lcd.print("Ignorando...");
            }
            else {

                if (!same(&mem1, &time1))
                    saveFlipFlopTime(&time1, 0);
                
                if (!same(&mem2, &time2))
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

    EEPROMWriteIntArray(array,address);
}

void readFlipFlopTimes(DateTime* time1, DateTime* time2) {

    int* array = EEPROMReadIntArray(0);
    
    *time1 = DateTime(2020,1,1,array[0],array[1],array[2]);
    *time2 = DateTime(2020,1,1,array[3],array[4],array[5]);

}

int* EEPROMReadIntArray(int address) {

    int* array = (int*) malloc(6 * sizeof(int));

    for (int i=0; i<6; i++, address++)
        array[i] = EEPROMReadInt(address++);

    return array;
}

void EEPROMWriteIntArray(int* array, int address) {

    for (int i=0; i<3; i++, address++)
        EEPROMWriteInt(address++, array[i]);

}

void EEPROMWriteInt(int p_address, int p_value) {

    byte lowByte  = ((p_value >> 0) & 0xFF);
    byte highByte = ((p_value >> 8) & 0xFF);

    EEPROM.write(p_address, lowByte);
    EEPROM.write(p_address + 1, highByte);

}

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int EEPROMReadInt(int p_address) {

    byte lowByte  = EEPROM.read(p_address);
    byte highByte = EEPROM.read(p_address + 1);

    return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);

}

// Creates a 'DateTime' object from the given 'input'
DateTime extractDateTime(char* input) {

    // Converting the char input data (from keyboard) to integer values
    int   day = (input[0] - '0') * 10 + input[1] - '0';
    int month = (input[2] - '0') * 10 + input[3] - '0';
    int  year = (input[4] - '0') * 10 + input[5] - '0' + 2000;

    int hour = (input[ 6] - '0') * 10 + input[ 7] - '0';
    int  min = (input[ 8] - '0') * 10 + input[ 9] - '0';
    int  sec = (input[10] - '0') * 10 + input[11] - '0';

    return DateTime(year,month,day,hour,min,sec);
}

void extractTime(char* input, DateTime* time1, DateTime* time2) {

    // Converting the char input data (from keyboard) to integer values
    int hour1 = (input[ 0] - '0') * 10 + input[ 1] - '0';
    int  min1 = (input[ 2] - '0') * 10 + input[ 3] - '0';
    int  sec1 = (input[ 4] - '0') * 10 + input[ 5] - '0';

    int hour2 = (input[ 6] - '0') * 10 + input[ 7] - '0';
    int  min2 = (input[ 8] - '0') * 10 + input[ 9] - '0';
    int  sec2 = (input[10] - '0') * 10 + input[11] - '0';

    *time1 = DateTime(2020,1,1,hour1,min1,sec1);
    *time2 = DateTime(2020,1,1,hour2,min2,sec2);

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

boolean lower(DateTime* a, DateTime* b) {
	return (a->hour() < b->hour() ||
				(a->hour() == b->hour() &&
					(a->minute() < b->minute() ||
						(a->minute() == b->minute() && a->second() < b->second())
					)
				)
			);
}

boolean greater(DateTime* a, DateTime* b) {
	return (a->hour() > b->hour() ||
				(a->hour() == b->hour() &&
					(a->minute() > b->minute() ||
						(a->minute() == b->minute() && a->second() > b->second())
					)
				)
			);
}

boolean same(DateTime* a, DateTime* b) {
	return (a->hour  () == b->hour  () &&
            a->minute() == b->minute() &&
            a->second() == b->second()
			);
}