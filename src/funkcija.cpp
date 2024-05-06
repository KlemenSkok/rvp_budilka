

//writes the temperature on display
void updateDisp(){
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