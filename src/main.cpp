
/**
 * @file main.cpp
 * @brief program za budilko pr rvp
 * 
 * uporabla funkcijo updateDisp() za mojo ploscico
 * ! ČE KOPIRAS KODO ZAKOMENTIRI MOJO FUNKCIJO IN UPORAB UNO OD KRISLJA
 * 
 * Ta koda je na githubu: https://github.com/KlemenSkok/rvp_budilka.git
 *
 * 
 * ? PINI:
 * 2 - gumb za budilko
 * 3 - gumb 1 (leva stran displaya)
 * 4 - gumb 2 (desna stran displaya)
 * 5 - gumb 3 (+)
 * 6 - RTC CLK
 * 7 - RTC DAT
 * 8 - RTC RST
 * 9 - gumb 4 (-)
 * 10 - DHT11 data
 * 11 - clockPin
 * 12 - latchPin
 * 13 - dataPin
 * A0 - led za budilko
 * 
 * 
 * 
*/


#include <Arduino.h>
#include <virtuabotixRTC.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <EEPROM.h>


//! isti shit, zakomentirana tabela je originalna
//const int digitPins[4] = {B11101111,B11010000,B10111111,B01111111};
const int digitPins[4] = {B01111111, B10111111, B11010000, B11101111};

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

void dump_eeprom();

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
    pinMode(gb, INPUT_PULLUP);

    pinMode(budilka_led, OUTPUT); // led indikator budilke

    // Set the current date, and time in the following format:
    // seconds, minutes, hours, day of the week, day of the month, month, year
    //RTC.setDS1302Time(00, 41, 11, 1, 26, 2, 2024);
    dht.begin();

    Serial.begin(9600);

    //dump_eeprom();

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

        if(digitalRead(g1) == LOW) {
            edit_ure();
        }
        if(digitalRead(g2) == LOW) {
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

        if(digitalRead(g1) == LOW) { // edit dan
            edit_dan();
        }
        else if(digitalRead(g2) == LOW) { // edit mesec
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

        if(digitalRead(g1) == LOW) { // edit leto
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

    int temp_int = (int)(dht.readTemperature() * 10);
	// prikaz temperature
	start_t = end_t = millis();
	while(end_t - start_t <= 4000) {
        if(end_t % 1000 == 0)
            temp_int = int(dht.readTemperature() * 10); // preberi temperaturo vsako sekundo

		digitBuffer[0] = temp_int / 100;
		digitBuffer[1] = (temp_int / 10) % 10;
		digitBuffer[2] = temp_int % 10;
		digitBuffer[3] = 10; // znak za stopinje
		end_t = millis();
		updateDisp(true);

		check_gb();
	}

    int vlaga_int = (int)dht.readHumidity();
	// prikaz vlage
	start_t = end_t = millis();
	while(end_t - start_t <= 4000) {
		if(end_t % 1000 == 0)
            vlaga_int = int(dht.readHumidity());

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
    if(gb_curr == LOW) {
        if(gb_prev == HIGH) {
            gb_last_pressed = millis();
        }
        else if(gb_prev == LOW && millis() - gb_last_pressed >= 3000) { // ce drzis tipko za vec kot 3s, jo nastavis
            Serial.println("nastavljam budilko");
            nastavi_budilko();
        }
    }
    else {
        if(gb_prev == LOW && millis() - gb_last_pressed >= 30) { // toggle budilka
            vklopi_budilko = !vklopi_budilko;
            digitalWrite(budilka_led, vklopi_budilko);
            Serial.println("togglam budilko");
        }
    }
    gb_prev = gb_curr;
}

void nastavi_budilko() {

    // pocakamo, da uporabnik spusti gumb, da program ne gre takoj ven
    while(digitalRead(gb) == LOW);
 
    // preberi z zacetka eeproma
    struct CasBudilke cajt;
    EEPROM.get(0, cajt);

    Serial.print("Berem iz eeproma: ");
    Serial.print(cajt.ura);
    Serial.print(":");
    Serial.println(cajt.minuta);

    if(cajt.ura == 255 && cajt.minuta == 255) {
        cajt.ura = RTC->hours;
        cajt.minuta = RTC->minutes;
    }

    auto show_changes = [&]() {
        digitBuffer[0] = cajt.ura / 10;
        digitBuffer[1] = cajt.ura % 10;
        digitBuffer[2] = cajt.minuta / 10;
        digitBuffer[3] = cajt.minuta % 10;
        updateDisp(true);
    };


    while(digitalRead(gb) == HIGH) {
        while(digitalRead(g1) == LOW) { // edit ure
            if(digitalRead(g3) == LOW && g3_prev == HIGH && millis() - g3_last_pressed >= 30) {
                cajt.ura++;
                if(cajt.ura >= 24)
                    cajt.ura = 0;

                g3_prev = LOW;
                g3_last_pressed = millis();
            }
            if(digitalRead(g4) == LOW && g4_prev == HIGH && millis() - g4_last_pressed >= 30) {
                cajt.ura--;
                if(cajt.ura < 0)
                    cajt.ura = 23;

                g4_prev = LOW;
                g4_last_pressed = millis();
            }
            g1_prev = LOW;
            g3_prev = digitalRead(g3);
            g4_prev = digitalRead(g4);

            show_changes();
        }
        g1_prev = LOW;

        while(digitalRead(g2) == LOW) { // edit minute
            if(digitalRead(g3) == LOW && g3_prev == HIGH && millis() - g3_last_pressed >= 30) {
                cajt.minuta++;
                if(cajt.minuta > 59)
                    cajt.minuta = 0;

                g3_prev = LOW;
                g3_last_pressed = millis();
            }
            if(digitalRead(g4) == LOW && g4_prev == HIGH && millis() - g4_last_pressed >= 30) {
                cajt.minuta--;
                if(cajt.minuta < 0)
                    cajt.minuta = 59;

                g4_prev = LOW;
                g4_last_pressed = millis();
            }
            g2_prev = LOW;
            g3_prev = digitalRead(g3);
            g4_prev = digitalRead(g4);

            show_changes();
        }
        g2_prev = LOW;

        show_changes();

    }

    gb_prev = LOW;
    gb_last_pressed = millis();
    // zapisi novi cas v eeprom
    EEPROM.put(0, cajt);

    Serial.print("Zapisujem v eeprom: ");
    Serial.print(cajt.ura);
    Serial.print(":");
    Serial.println(cajt.minuta);

    delay(50);

}


void edit_ure() {

    int ure = RTC->hours;
    while(digitalRead(g1) == LOW) { // edit ure
        if(digitalRead(g3) == LOW && g3_prev == HIGH && millis() - g3_last_pressed >= 30) {
            ure++;
            if(ure >= 24)
                ure = 0;
            g3_prev = LOW;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == LOW && g4_prev == HIGH && millis() - g4_last_pressed >= 30) {
            ure--;
            if(ure < 0)
                ure = 23;
            g4_prev = LOW;
            g4_last_pressed = millis();
        }
        g1_prev = LOW;
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
    g1_prev = HIGH;

}

void edit_minute() {

    int minute = RTC->minutes;
    while(digitalRead(g2) == LOW) { // edit minute
        if(digitalRead(g3) == LOW && g3_prev == HIGH && millis() - g3_last_pressed >= 30) {
            minute++;
            if(minute >= 60)
                minute = 0;
            g3_prev = LOW;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == LOW && g4_prev == HIGH && millis() - g4_last_pressed >= 30) {
            minute--;
            if(minute < 0)
                minute = 59;
            g4_prev = LOW;
            g4_last_pressed = millis();
        }
        g2_prev = LOW;
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
    g2_prev = HIGH;

}

void edit_dan() {

    int dan = RTC->dayofmonth;
    while(digitalRead(g1) == LOW) { // edit dan
        if(digitalRead(g3) == LOW && g3_prev == HIGH && millis() - g3_last_pressed >= 30) {
            dan++;
            if(dan > 31)
                dan = 1;
            g3_prev = LOW;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == LOW && g4_prev == HIGH && millis() - g4_last_pressed >= 30) {
            dan--;
            if(dan < 1)
                dan = 31;
            g4_prev = LOW;
            g4_last_pressed = millis();
        }
        g1_prev = LOW;
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
    g1_prev = HIGH;

}

void edit_mesec() {

    int mesec = RTC->month;
    while(digitalRead(g2) == LOW) { // edit mesec
        if(digitalRead(g3) == LOW && g3_prev == HIGH && millis() - g3_last_pressed >= 30) {
            mesec++;
            if(mesec > 12)
                mesec = 1;
            g3_prev = LOW;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == LOW && g4_prev == HIGH && millis() - g4_last_pressed >= 30) {
            mesec--;
            if(mesec < 1)
                mesec = 12;
            g4_prev = LOW;
            g4_last_pressed = millis();
        }
        g2_prev = LOW;
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
    g2_prev = HIGH;

}

void dump_eeprom() {
    byte data;
    for(int i = 0; i < 1024; i++) {
        data = EEPROM.read(i);
        Serial.print(data, HEX);
        if(i % 16 == 15)
            Serial.println();
        else if(i % 8 == 7)
            Serial.print("  ");
        else
            Serial.print(" ");
    }
}


void edit_leto() {

    int leto = RTC->year;
    while(digitalRead(g1) == LOW) { // edit leto
        if(digitalRead(g3) == LOW && g3_prev == HIGH && millis() - g3_last_pressed >= 30) {
            leto++;
            g3_prev = LOW;
            g3_last_pressed = millis();
        }
        if(digitalRead(g4) == LOW && g4_prev == HIGH && millis() - g4_last_pressed >= 30) {
            leto--;
            if(leto < 0)
                leto = 0;
            g4_prev = LOW;
            g4_last_pressed = millis();
        }
        g1_prev = LOW;
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
    g1_prev = HIGH;

}

//! MOJA
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
    if(digitScan==1 && dp)
        shiftOut(dataPin, clockPin, MSBFIRST, digit[digitBuffer[digitScan]] | B00000001);
    else
        shiftOut(dataPin, clockPin, MSBFIRST, digit[digitBuffer[digitScan]]);
 
    shiftOut(dataPin, clockPin, MSBFIRST, digitPins[digitScan]);
    digitalWrite(latchPin, HIGH);
    digitScan++;
    if(digitScan>3) digitScan=0; 
}

//! TVOJA
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
