#include <Keypad.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

/********** Function Declaration *************/

void displayTime();
void setTime();

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

int lastSecond = 0, curSecond;
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

void displayTime() {

    DateTime now = rtc.now();

    curSecond = now.second();

    if (lastSecond != curSecond) {

      sprintf(lcdRow1,"Hoje e: %02d/%02d/%02d", now.day(), now.month(), now.year() - 2000);
      sprintf(lcdRow2,"%02d:%02d:%02d", now.hour(), now.minute(), curSecond);
 
        lcd.clear();
        lcd.print(lcdRow1);
        lcd.setCursor(8,1);
        lcd.print(lcdRow2);

      /*Serial.print(lcdRow1);
      Serial.print(" - ");
      Serial.println(lcdRow2);*/
      lastSecond = curSecond;

    }

}

void setTime() {

    int  index = 0;
    char input[13] = "";

    //Serial.println("Digite o horário");
    lcd.clear();
    lcd.print("Ajuste: __/__/__");
    lcd.setCursor(8,1);
    lcd.print("__:__:__");

    while ((key = keypad.getKey()) != '#') {
        
        if (key != NO_KEY) {

            if (key == '*') {
                if (index > 0)
                  input[--index] = '\0';
            }
          
            else if (index < 12) {

                input[index++] = key;
                input[index] = '\0';
                    
            }

            printTime(input);
            
        }
        
    }

    if (index == 12) {

        DateTime* datetime = parseDate(input);

        if (datetime != NULL) {
            rtc.adjust(*datetime);
            free(datetime);
            Serial.println("Hora reajustada!");
        }
        else
          Serial.println("Hora inválida!");
      
    }
    

}

DateTime* parseDate(char* input) {

    boolean valid = true;
  
    int   day = (input[0] - '0') * 10 + input[1] - '0';
    int month = (input[2] - '0') * 10 + input[3] - '0';
    int  year = (input[4] - '0') * 10 + input[5] - '0' + 2000;

    int hour = (input[ 6] - '0') * 10 + input[ 7] - '0';
    int  min = (input[ 8] - '0') * 10 + input[ 9] - '0';
    int  sec = (input[10] - '0') * 10 + input[11] - '0';
    
    if ((month <  1) || (month > 12) ||
        (  day <  1) || (  day > 31) ||
        ( hour > 23) ||
        (  min > 59) ||
        (  sec > 59)
       )
       valid = false;
    
    else {
    
    if (month == 2) { // If it's february,
      if (year % 4 == 0 && (year % 400 == 0 || year % 100 != 0)) { // the year is leap,
        if (day > 29)
          valid = false;
      }
      else if (day > 28)
        valid = false;
    
    }
    
    else {    // If it's not February
      
      if (month < 8) {
        
        if (((month % 2) == 0) && (day > 30))
          valid = false;
        
      }
      else {
        
        if (((month % 2) != 0) && (day > 30))
          valid = false;
        
      }
      
    }
  }

  if (valid) {
    
      DateTime  local = DateTime(year,month,day,hour,min,sec);
      DateTime* point = (DateTime*) malloc(sizeof(DateTime));

      memcpy(point,&local,sizeof(DateTime));

      return point;
  }

  return NULL;
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
