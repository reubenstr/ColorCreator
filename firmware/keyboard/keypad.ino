
/*

 
 1  2  3  4
 5  6  7  8
 9  10 11 12
 13 14 15 16
 
 
 */
#include <Timer.h>
Timer t;

#define DEVICE_ID 1
#define COMMAND_BUTTON_PRESS 1
#define COMMAND_BUTTON_HELD 2

#define DEBOUNCE_DELAY 200 //button debounce on milliseconds
#define BUTTON_HELD_TIME 2000 //time button needs to be held to trigger a button held command

#define ROW_1_PIN 10
#define ROW_2_PIN 11
#define ROW_3_PIN 12
#define ROW_4_PIN 13
#define ROW_5_PIN A0

#define COLUMN_1_PIN 5
#define COLUMN_2_PIN 7
#define COLUMN_3_PIN 8
#define COLUMN_4_PIN 9



byte getkey;
byte getkey_buffer;

long debounce_counter;

void setup() {

  Serial.begin(57600);
  Serial.println("Color Creator Keypad"); 
  
  //activate timer events
  t.every(1, counter);

  pinMode(COLUMN_1_PIN, INPUT);  
  digitalWrite(COLUMN_1_PIN, HIGH);    //enable internal pullup
  pinMode(COLUMN_2_PIN, INPUT); 
  digitalWrite(COLUMN_2_PIN, HIGH);    //enable internal pullup
  pinMode(COLUMN_3_PIN, INPUT); 
  digitalWrite(COLUMN_3_PIN, HIGH);    //enable internal pullup
  pinMode(COLUMN_4_PIN, INPUT);
  digitalWrite(COLUMN_4_PIN, HIGH);    //enable internal pullup 

  //set output pins to input to make them float
  pinMode(ROW_1_PIN, INPUT); 
  pinMode(ROW_2_PIN, INPUT);
  pinMode(ROW_3_PIN, INPUT);
  pinMode(ROW_4_PIN, INPUT);
  pinMode(ROW_5_PIN, INPUT);

}

void loop(){
  
  t.update(); //required for timer events
  
  

  getkey = 0;

  pinMode(ROW_1_PIN, OUTPUT); //set pin to output  
  digitalWrite(ROW_1_PIN, LOW); //set pin to low

  //check for low columns
  if (digitalRead(COLUMN_1_PIN) == 0) getkey = 1;
  if (digitalRead(COLUMN_2_PIN) == 0) getkey = 2;
  if (digitalRead(COLUMN_3_PIN) == 0) getkey = 3;
  if (digitalRead(COLUMN_4_PIN) == 0) getkey = 4;  

  pinMode(ROW_1_PIN, INPUT); //set pin back to floating state

  pinMode(ROW_2_PIN, OUTPUT); //set pin to output  
  digitalWrite(ROW_2_PIN, LOW); //set pin to low

  //check for low columns
  if (digitalRead(COLUMN_1_PIN) == 0) getkey = 5;
  if (digitalRead(COLUMN_2_PIN) == 0) getkey = 6;
  if (digitalRead(COLUMN_3_PIN) == 0) getkey = 7;
  if (digitalRead(COLUMN_4_PIN) == 0) getkey = 8;  

  pinMode(ROW_2_PIN, INPUT); //set pin back to floating state

  pinMode(ROW_3_PIN, OUTPUT); //set pin to output  
  digitalWrite(ROW_3_PIN, LOW); //set pin to low

  //check for low columns
  if (digitalRead(COLUMN_1_PIN) == 0) getkey = 9;
  if (digitalRead(COLUMN_2_PIN) == 0) getkey = 10;
  if (digitalRead(COLUMN_3_PIN) == 0) getkey = 11;
  if (digitalRead(COLUMN_4_PIN) == 0) getkey = 12;  

  pinMode(ROW_3_PIN, INPUT); //set pin back to floating state

  pinMode(ROW_4_PIN, OUTPUT); //set pin to output  
  digitalWrite(ROW_4_PIN, LOW); //set pin to low

  //check for low columns
  if (digitalRead(COLUMN_1_PIN) == 0) getkey = 13;
  if (digitalRead(COLUMN_2_PIN) == 0) getkey = 14;
  if (digitalRead(COLUMN_3_PIN) == 0) getkey = 15;
  if (digitalRead(COLUMN_4_PIN) == 0) getkey = 16;  

  pinMode(ROW_4_PIN, INPUT); //set pin back to floating state

  pinMode(ROW_5_PIN, OUTPUT); //set pin to output  
  digitalWrite(ROW_5_PIN, LOW); //set pin to low

  //check for low columns
  if (digitalRead(COLUMN_1_PIN) == 0) getkey = 17;
  if (digitalRead(COLUMN_2_PIN) == 0) getkey = 18;
  if (digitalRead(COLUMN_3_PIN) == 0) getkey = 19;
  if (digitalRead(COLUMN_4_PIN) == 0) getkey = 20;  

  pinMode(ROW_5_PIN, INPUT); //set pin back to floating state


  //process getkey
  
  //force max value for debounce_counter
  if (debounce_counter > 10000) debounce_counter = 10000;

  //all keys lifted
  if(getkey == 0) getkey_buffer = 0; 
  
  //indicates the button is released after a hold
  if (getkey == 0 && debounce_counter > 15000) debounce_counter = 0;

  //new data
  if (getkey != 0 && getkey != getkey_buffer && debounce_counter > DEBOUNCE_DELAY) {

    
    debounce_counter = 0;
    getkey_buffer = getkey;

    Serial.print(DEVICE_ID); 
    Serial.print(COMMAND_BUTTON_PRESS); 
    
    if (getkey < 10) {
    Serial.print(0);
    Serial.println(getkey); 
    }
    else
    {
    Serial.println(getkey);   
    }
    
   
  } 
  
  //process if the same button has been held down
  if (getkey != 0 && getkey == getkey_buffer && debounce_counter > BUTTON_HELD_TIME && debounce_counter < 5000) {

    debounce_counter = 20000;
    
    Serial.print(DEVICE_ID); 
    Serial.print(COMMAND_BUTTON_HELD); 
     
    if (getkey < 10) {
    Serial.print(0);
    Serial.println(getkey); 
    }
    else
    {
    Serial.println(getkey);   
    } 
    
   
  } 

} //end main_loop


void counter(void) {
  
  debounce_counter++;
  
}

