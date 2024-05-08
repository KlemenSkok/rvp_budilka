
/**
 * 
 * program za budilko pr rvp
 * 
 * uporabla funkcijo updateDisp() za mojo ploscico
 * ! ČE KOPIRAS KODO ZAKOMENTIRI MOJO FUNKCIJO IN UPORAB UNO OD KRISLJA
 * 
 * Ta koda je na githubu: https://github.com/KlemenSkok/rvp_budilka.git
 *
*/


#include <Arduino.h>
#include <virtuabotixRTC.h>
#include <DHT.h>
#include <EEPROM.h>

const int digitPins[4] = {B11101111,B11010000,B10111111,B01111111};

const int clockPin = 11;    //74HC595 Pin 11
const int latchPin = 12;    //74HC595 Pin 12
const int dataPin = 13;     //74HC595 Pin 14

const byte digit[12] =      //seven segment digits in bits
{
    B01111110, //0
    B00001100, //1
    B10110110, //2
    B10011110, //3
    B11001100, //4
    B11011010, //5
    B11111010, //6
    B00001110, //7
    B11111110, //8
    B11011110, //9
	B11000110, // znak za stopinje
	B00000000  // prazno
};
int digitBuffer[4] = {0};
int digitScan=0;

virtuabotixRTC *RTC = new virtuabotixRTC(6, 7, 8);

const int gb = 2; // od budilke
const int g1 = 3;
const int g2 = 4;
const int g3 = 5;
const int g4 = 9;
const int budilka_led = A0;

int dht_pin = 10;
DHT dht(dht_pin, DHT11);

void updateDisp(bool);

void setup(){
    for(int i = 0; i < 4; i++) {
        pinMode(digitPins[i],OUTPUT);
    }
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);  

    pinMode(g1, INPUT_PULLUP);
    pinMode(g2, INPUT_PULLUP);
    pinMode(g3, INPUT_PULLUP);
    pinMode(g4, INPUT_PULLUP);
    pinMode(gb, INPUT_PULLUP); // DAS NA INTERRUPT (pin 2)

    pinMode(budilka_led, OUTPUT); // led indikator budilke

    // Set the current date, and time in the following format:
    // seconds, minutes, hours, day of the week, day of the month, month, year
    //RTC.setDS1302Time(00, 41, 11, 1, 26, 2, 2024);
    dht.begin();

}

//previous button states
int g1_prev = 0;
int g2_prev = 0;
int g3_prev = 0;
int g4_prev = 0;
int gb_prev = 0;

int g3_last_pressed = 0,
    g4_last_pressed = 0,
    gb_last_pressed = 0;

bool vklopi_budilko = false;

void edit_ure();
void edit_minute();
void edit_dan();
void edit_mesec();
void edit_leto();


// check budilka-gumb
void check_gb();
void nastavi_budilko();


struct CasBudilke {
    uint8_t ura, minuta;
};

long start_t, end_t;


void loop() {

    start_t = end_t = millis();

    while(end_t - start_t <= 4000){ // čas
        RTC->updateTime();

        if(digitalRead(g1) == HIGH) {
            edit_ure();
        }
        if(digitalRead(g2) == HIGH) {
            edit_minute();
        }

        digitBuffer[0] = RTC->hours / 10;
        digitBuffer[1] = RTC->hours % 10;
        digitBuffer[2] = RTC->minutes / 10;
        digitBuffer[3] = RTC->minutes % 10;
        end_t = millis();
        updateDisp(true);

        check_gb();
    }

    start_t = end_t = millis();
    while(end_t - start_t <= 4000){ // datum
        RTC->updateTime();

        if(digitalRead(g1) == HIGH) { // edit dan
            edit_dan();
        }
        else if(g2 == HIGH) { // edit mesec
            edit_mesec();
        }

        digitBuffer[0] = RTC->dayofmonth / 10;
        digitBuffer[1] = RTC->dayofmonth % 10;
        digitBuffer[2] = RTC->month / 10;
        digitBuffer[3] = RTC->month % 10;
        end_t = millis();
        updateDisp(true);
   
        check_gb();
    }

    start_t = end_t = millis();
    while(end_t - start_t <= 4000) { // leto
        RTC->updateTime();

        if(digitalRead(g1) == HIGH) { // edit leto
            edit_leto();
        }

        digitBuffer[0] = RTC->year / 1000;
        digitBuffer[1] = (RTC->year / 100) % 10;
        digitBuffer[2] = (RTC->year / 10) % 10;
        digitBuffer[3] = RTC->year % 10;
        end_t = millis();
        updateDisp(false);
   
        check_gb();
    }

	// prikaz temperature
	start_t = end_t = millis();
	while(end_t - start_t <= 4000) {
		int temp_int = (int)(dht.readTemperature() * 10);

		digitBuffer[0] = temp_int / 100;
		digitBuffer[1] = (temp_int / 10) % 10;
		digitBuffer[2] = temp_int % 10;
		digitBuffer[3] = 10; // znak za stopinje
		end_t = millis();
		updateDisp(true);

		check_gb();
	}

	// prikaz vlage
	start_t = end_t = millis();
	while(end_t - start_t <= 4000) {
		int vlaga_int = (int)dht.readHumidity();

		digitBuffer[0] = 11; // prazno
		digitBuffer[1] = (vlaga_int / 100 > 0) ? vlaga_int / 100 : 11;
		digitBuffer[2] = (vlaga_int / 10) % 10;
		digitBuffer[3] = vlaga_int % 10;
        end_t = millis();
        updateDisp(false);

        check_gb();
	}


}


void check_gb() {

    bool gb_curr = digitalRead(gb);
    if(gb_curr == HIGH) {
        if(gb_prev == LOW) {
            gb_last_pressed = millis();
        }
        else if(gb_prev == HIGH && millis() - gb_last_pressed >= 1500) { // ce drzis tipko za vec kot 1,5s, jo nastavis
            nastavi_budilko();
        }
    }
    else {
        if(gb_prev == HIGH && millis() - gb_last_pressed >= 30) { // toggle budilka
            vklopi_budilko = !vklopi_budilko;
            digitalWrite(budilka_led, vklopi_budilko);
        }
    }
    gb_prev = gb_curr;
}

void nastavi_budilko() {

    // pocakamo, da uporabnik spusti gumb, da program ne gre takoj ven
    while(digitalRead(gb) == HIGH);
 
    // preberi z zacetka eeproma
    struct CasBudilke cajt;
    EEPROM.get(0, cajt);


    while(digitalRead(gb) == LOW) {
        while(digitalRead(g1) == HIGH) { // edit ure
            if(digitalRead(g3) == HIGH && g3_prev == LOW && millis() - g3_last_pressed >= 30) {
                cajt.ura++;
                g3_prev = HIGH;
                g3_last_pressed = millis();
            }
            if(digitalRead(g4) == HIGH && g4_prev == LOW && millis() - g4_last_pressed >= 30) {
                cajt.ura--;
                g4_prev = HIGH;
                g4_last_pressed = millis();
            }
            g1_prev = HIGH;
            g3_prev = digitalRead(g3);
            g4_prev = digitalRead(g4);

            digitBuffer[0] = cajt.ura / 10;
            digitBuffer[1] = cajt.ura % 10;
            digitBuffer[2] = cajt.minuta / 10;
            digitBuffer[3] = cajt.minuta % 10;
            updateDisp(true);
        }
        g1_prev = HIGH;

        while(digitalRead(g2) == HIGH) { // edit minute
            if(digitalRead(g3) == HIGH && g3_prev == LOW && millis() - g3_last_pressed >= 30) {
                cajt.minuta++;
                g3_prev = HIGH;
                g3_last_pressed = millis();
            }
            if(digitalRead(g4) == HIGH && g4_prev == LOW && millis() - g4_last_pressed >= 30) {
                cajt.minuta--;
                g4_prev = HIGH;
                g4_last_pressed = millis();
            }
            g2_prev = HIGH;
            g3_prev = digitalRead(g3);
            g4_prev = digitalRead(g4);

            digitBuffer[0] = cajt.ura / 10;
            digitBuffer[1] = cajt.ura % 10;
            digitBuffer[2] = cajt.minuta / 10;
            digitBuffer[3] = cajt.minuta % 10;
            updateDisp(true);
        }
        g2_prev = HIGH;

    }

    gb_prev = HIGH;
    // zapisi novi cas v eeprom
    EEPROM.put(0, cajt);

}


void edit_ure() {

    int ure = RTC->hours;
    while(digitalRead(g1) == HIGH) { // edit ure
        if(digitalRead(g3) == HIGH && g3_prev == LOW && millis() - g3_last_pressed >= 30) {
            ure++;
            g3_prev = HIGH;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == HIGH && g4_prev == LOW && millis() - g4_last_pressed >= 30) {
            ure--;
            g4_prev = HIGH;
            g4_last_pressed = millis();
        }
        g1_prev = HIGH;
        g3_prev = digitalRead(g3);
        g4_prev = digitalRead(g4);

        digitBuffer[0] = ure / 10;
        digitBuffer[1] = ure % 10;
        digitBuffer[2] = RTC->minutes / 10;
        digitBuffer[3] = RTC->minutes % 10;
        updateDisp(true);
    }
    // save changes
    RTC->setDS1302Time(RTC->seconds, RTC->minutes, ure, RTC->dayofweek, RTC->dayofmonth, RTC->month, RTC->year);
    start_t = end_t = millis();
    g1_prev = LOW;

}

void edit_minute() {

    int minute = RTC->minutes;
    while(digitalRead(g2) == HIGH) { // edit minute
        if(digitalRead(g3) == HIGH && g3_prev == LOW && millis() - g3_last_pressed >= 30) {
            minute++;
            g3_prev = HIGH;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == HIGH && g4_prev == LOW && millis() - g4_last_pressed >= 30) {
            minute--;
            g4_prev = HIGH;
            g4_last_pressed = millis();
        }
        g2_prev = HIGH;
        g3_prev = digitalRead(g3);
        g4_prev = digitalRead(g4);

        digitBuffer[0] = RTC->hours / 10;
        digitBuffer[1] = RTC->hours % 10;
        digitBuffer[2] = minute / 10;
        digitBuffer[3] = minute % 10;
        updateDisp(true);
    }
    // save changes
    RTC->setDS1302Time(RTC->seconds, minute, RTC->hours, RTC->dayofweek, RTC->dayofmonth, RTC->month, RTC->year);
    start_t = end_t = millis();
    g2_prev = LOW;

}

void edit_dan() {

    int dan = RTC->dayofmonth;
    while(digitalRead(g1) == HIGH) { // edit dan
        if(digitalRead(g3) == HIGH && g3_prev == LOW && millis() - g3_last_pressed >= 30) {
            dan++;
            g3_prev = HIGH;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == HIGH && g4_prev == LOW && millis() - g4_last_pressed >= 30) {
            dan--;
            g4_prev = HIGH;
            g4_last_pressed = millis();
        }
        g1_prev = HIGH;
        g3_prev = digitalRead(g3);
        g4_prev = digitalRead(g4);

        digitBuffer[0] = dan / 10;
        digitBuffer[1] = dan % 10;
        digitBuffer[2] = RTC->month / 10;
        digitBuffer[3] = RTC->month % 10;
        updateDisp(true);
    }
    // save changes
    RTC->setDS1302Time(RTC->seconds, RTC->minutes, RTC->hours, RTC->dayofweek, dan, RTC->month, RTC->year);
    start_t = end_t = millis();
    g1_prev = LOW;

}

void edit_mesec() {

    int mesec = RTC->month;
    while(digitalRead(g2) == HIGH) { // edit mesec
        if(digitalRead(g3) == HIGH && g3_prev == LOW && millis() - g3_last_pressed >= 30) {
            mesec++;
            g3_prev = HIGH;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == HIGH && g4_prev == LOW && millis() - g4_last_pressed >= 30) {
            mesec--;
            g4_prev = HIGH;
            g4_last_pressed = millis();
        }
        g2_prev = HIGH;
        g3_prev = digitalRead(g3);
        g4_prev = digitalRead(g4);

        digitBuffer[0] = RTC->dayofmonth / 10;
        digitBuffer[1] = RTC->dayofmonth % 10;
        digitBuffer[2] = mesec / 10;
        digitBuffer[3] = mesec % 10;
        updateDisp(true);
    }
    // save changes
    RTC->setDS1302Time(RTC->seconds, RTC->minutes, RTC->hours, RTC->dayofweek, RTC->dayofmonth, mesec, RTC->year);
    start_t = end_t = millis();
    g2_prev = LOW;

}

void edit_leto() {

    int leto = RTC->year;
    while(digitalRead(g1) == HIGH) { // edit leto
        if(digitalRead(g3) == HIGH && g3_prev == LOW && millis() - g3_last_pressed >= 30) {
            leto++;
            g3_prev = HIGH;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == HIGH && g4_prev == LOW && millis() - g4_last_pressed >= 30) {
            leto--;
            g4_prev = HIGH;
            g4_last_pressed = millis();
        }
        g1_prev = HIGH;
        g3_prev = digitalRead(g3);
        g4_prev = digitalRead(g4);

        digitBuffer[0] = leto / 1000;
        digitBuffer[1] = (leto / 100) % 10;
        digitBuffer[2] = (leto / 10) % 10;  
        digitBuffer[3] = leto % 10;
        updateDisp(false);
    }
    // save changes
    RTC->setDS1302Time(RTC->seconds, RTC->minutes, RTC->hours, RTC->dayofweek, RTC->dayofmonth, RTC->month, leto);
    start_t = end_t = millis();
    g1_prev = LOW;

}

// DEFAULT FUNKCIJA
/* //writes the temperature on display
void updateDisp(bool dp){ // ----------------------
    for(byte j=0; j<4; j++)  
        digitalWrite(digitPins[j], HIGH);
 
    digitalWrite(latchPin, LOW);  
    shiftOut(dataPin, clockPin, MSBFIRST, B11111111);
    shiftOut(dataPin, clockPin, MSBFIRST, B11111111);
    digitalWrite(latchPin, HIGH);
 
    delayMicroseconds(100);

    digitalWrite(latchPin, LOW);    
    shiftOut(dataPin, clockPin, MSBFIRST, digitPins[digitScan]);
    if(digitScan==1 && dp){ // ----------------------
        shiftOut(dataPin, clockPin, MSBFIRST, (digit[digitBuffer[digitScan]]|B00000001));
    }
    else
        shiftOut(dataPin, clockPin, MSBFIRST, digit[digitBuffer[digitScan]]);
    digitalWrite(latchPin, HIGH);
    digitScan++;
    if(digitScan>3) digitScan=0;
} */

// skuhu funkcijo da dela za moj cip. ce kopiras uporab uno zgori
//writes the temperature on display
void updateDisp(bool dp){
  for(byte j=0; j<4; j++)  
    digitalWrite(digitPins[j], HIGH);
 
  digitalWrite(latchPin, LOW);  
  shiftOut(dataPin, clockPin, MSBFIRST, B11111111);
  shiftOut(dataPin, clockPin, MSBFIRST, B11111111);
  digitalWrite(latchPin, HIGH);
 
  delayMicroseconds(100);

  digitalWrite(latchPin, LOW);    
    shiftOut(dataPin, clockPin, MSBFIRST, digit[digitBuffer[digitScan]]);
    if(digitScan==1){
      //shiftOut(dataPin, clockPin, MSBFIRST, (digit[digitBuffer[digitScan]]|B00000001));
          shiftOut(dataPin, clockPin, MSBFIRST, digitPins[digitScan] | B00000001);
    }
    else
    shiftOut(dataPin, clockPin, MSBFIRST, digitPins[digitScan]);
  digitalWrite(latchPin, HIGH);
  digitScan++;
  if(digitScan>3) digitScan=0; 
}