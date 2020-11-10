#include <Wire.h>
#include <RTClib.h>
#include <Keypad.h>

RTC_DS3231 rtc;

char lcdRow1[20], lcdRow2[20];
enum menu { showTime, timeConfig, activationConfig };

// Keyboard mapping (using 4x4 matrix)
char keyMap[4][4] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

// Defining keyboard pin set
byte rowPins[4] = { 11, 10, 9, 8 };
byte colPins[4] = { 7, 6, 5, 4 };

char key;

// Starting keypad lib object
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, 4, 4);


int curState;

void setup() {

  Serial.begin(9600);

  curState = showTime;
  
  if(!rtc.begin()) {
    
    Serial.println("Módulo RTC não encontrado!");
    while(1);
    
  }
  
  /*if (rtc.lostPower()) {

  }*/
  
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  delay(100);
  
}

int lastSecond = 0, curSecond;
char lastKey = 'A';

void loop() {

    key = keypad.getKey();

    if ((key != NO_KEY) && (lastKey != key)) {
        switch (key) {
            case 'A':
                curState = showTime;
                break;
            case 'B':
                curState = timeConfig;
                break;
        }
    }

    switch (curState) {
        case showTime:
          displayTime();
          break;
        case timeConfig:
          setTime();
          curState = showTime;
          break;
    }

    //displayTime();

    
    
    /*if (key != NO_KEY) {
      
      Serial.print("Tecla pressionada: ");
      Serial.println(key);

      if (key == 'A')
        setTime();
      
    }    */

}

void displayTime() {

    DateTime now = rtc.now();

    curSecond = now.second();

    if (lastSecond != curSecond) {

      sprintf(lcdRow1,"Hoje é: %02d/%02d/%02d", now.day(), now.month(), now.year());
      sprintf(lcdRow2,"%02d:%02d:%02d", now.hour(), now.minute(), curSecond);
 
      Serial.print(lcdRow1);
      Serial.print(" - ");
      Serial.println(lcdRow2);
      lastSecond = curSecond;

    }

}

void setTime() {

    int  index = 0;
    char input[13] = "";

    Serial.println("Digite o horário");

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

    Serial.print(lcdRow1);
    Serial.print(" - ");
    Serial.println(lcdRow2);
  
}
