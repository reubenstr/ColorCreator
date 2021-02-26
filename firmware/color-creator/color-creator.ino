/*  
 Color Creator
 Reuben Strangelove
 reuben@artofmystate.com
 March 2014
 
 Terminal commands (57600 baud, newline)
 "output on" - turns on data output. displayed, in order, are the follow: mode, submode, preference, brightness, slider_speed, red_slider, green_slider, blue_slider
 "output off" - turns off data output
 "load defaults" - load default vaules into EEPROM, including brightness, and Expanded Keypad color mapping. 
  
 WT588D Command to Sound File Mapping
 
 
 */


#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <Timer.h>
Timer t;

#define DEBUG_OUTPUT true 


//define PINS (PROTOTYPE RELEASE)
#define PIXEL_DATA_PIN 7
#define SLIDER_RED_INPUT A0
#define SLIDER_GREEN_INPUT A1
#define SLIDER_BLUE_INPUT A2
#define SLIDER_SPEED_INPUT A3
#define SWITCH_1 2
#define SWITCH_2 3
#define SWITCH_3 4
#define WT588D_RST 8
#define WT588D_CS 11
#define WT588D_SCL 12 
#define WT588D_SDA 10 //P01 
#define WT588D_BUSY 9 //Pin 15, BUSY
///////////////////////////

#define PIXELS_AMOUNT 48

#define MAX_MODES 3

#define MODE_SOLID 1
#define MODE_SLIDER 2
#define MODE_REVOLVE 3

#define SWITCH_DEBOUNCE_DELAY 25 //debounce time in milliseconds
#define SWITCH_HELD_DELAY 5000

//#define SLIDER_LOWER_CUTOFF_VALUE 5
#define SLIDER_CHANGE_CHECK_RANGE 5

#define WRITE_PROTECT_SOLID_COLORS 13


//WT588D Audio Playback module command mapping
#define PLAY_MODE_SOLID 0
#define PLAY_MODE_SLIDER 1
#define PLAY_MODE_REVOLVE 3

#define PLAY_COLOR_CREATOR 51
#define PLAY_COLOR_VALUE_SET 52
#define PLAY_CUSTOM_COLOR 53
#define PLAY_SET_BRIGHTNESS 55
#define PLAY_NEW_BRIGHTNESS_SET 56
#define PLAY_NO_COLOR 57
#define PLAY_DEFAULT_VALUES_LOADED 58

String serial_rx_data = "";

//setup WS2812B LED strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS_AMOUNT, PIXEL_DATA_PIN, NEO_GRB + NEO_KHZ800);

byte output_flag;


int color_change_counter;
int color_change_delay;
int revolve_position;

byte mode;
byte brightness;
byte volume;

int smooth_timing;
int smooth_counter;

byte forvar;
byte forvar_main;

byte solid_color_select;
byte solid_color_select_buffer;

byte red_slider, green_slider, blue_slider;
byte red_slider_buffer, green_slider_buffer, blue_slider_buffer;
byte slider_speed, slider_speed_buffer;

int switch_1_debounce_counter;
int switch_2_debounce_counter;
int switch_3_debounce_counter;
byte switch_1_flag = 0;
byte switch_2_flag = 0;
byte switch_3_flag = 0;

byte red_slider_output;
byte green_slider_output;
byte blue_slider_output;

byte slider_activity_flag = 0;

uint32_t solid_color_values[21];

uint32_t color;
uint32_t color_buffer;

uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t magenta = strip.Color(255, 0, 255);
uint32_t cyan = strip.Color(0, 255, 255);
uint32_t yellow = strip.Color(255, 255, 0);
uint32_t white = strip.Color(255, 255, 255);

uint32_t amber = strip.Color(255, 175, 0);
uint32_t orange = strip.Color(255, 90, 0);
uint32_t purple = strip.Color(255, 0, 170);
uint32_t pink = strip.Color(255, 0, 90);
uint32_t aquamarine = strip.Color(60, 255, 140);
uint32_t seafoam_green = strip.Color(0, 255, 39);
uint32_t lavender = strip.Color(108, 0, 255);

uint32_t no_color = strip.Color(0, 0, 0);



void setup() {

  //initialize serial:
  Serial.begin(57600); 
  Serial.println("Color Creator v1.01"); 

  // reserve 25 bytes for serial_rx_data:
  serial_rx_data.reserve(25);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  //delare inputs
  pinMode(SLIDER_RED_INPUT, INPUT);      // set pin to input
  pinMode(SLIDER_GREEN_INPUT, INPUT);    // set pin to input
  pinMode(SLIDER_BLUE_INPUT, INPUT);     // set pin to input 
  pinMode(SLIDER_SPEED_INPUT, INPUT);     // set pin to input 

  pinMode(SWITCH_1, INPUT);           // set pin to input
  digitalWrite(SWITCH_1, HIGH);       // turn on pullup resistor
  pinMode(SWITCH_2, INPUT);        // set pin to input
  digitalWrite(SWITCH_2, HIGH);    // turn on pullup resistor
  pinMode(SWITCH_3, INPUT);     // set pin to input
  digitalWrite(SWITCH_3, HIGH); // turn on pullup resistor


  pinMode(WT588D_RST, OUTPUT);  
  pinMode(WT588D_CS, OUTPUT); 
  pinMode(WT588D_SCL, OUTPUT); 
  pinMode(WT588D_SDA, OUTPUT); 

  digitalWrite(WT588D_CS, HIGH);
  digitalWrite(WT588D_RST, HIGH);
  digitalWrite(WT588D_SCL, HIGH);
  
  delay(250);

  WT588D_Send_Command(PLAY_COLOR_CREATOR);

  //store initial buffer values
  red_slider_buffer = map(analogRead(SLIDER_RED_INPUT), 0, 1023, 0, 255); 
  green_slider_buffer = map(analogRead(SLIDER_GREEN_INPUT), 0, 1023, 0, 255); 
  blue_slider_buffer = map(analogRead(SLIDER_BLUE_INPUT), 0, 1023, 0, 255); 

  //activate timer events
  t.every(1, counter); //executes the 'counter' subroutine every 1ms; increments counters
  t.every(1, smooth); //executes the 'smooth' subroutine every 1ms; smooths the transistions between colors
  t.every(50, debug); //executes the 'debug' subroutine every 50ms; outputs system data over serial 

  //set starting values  

  //if the eeprom is fresh, store default values 
  if (EEPROM.read(0) == 255) eeprom_store_default_values();

  mode = EEPROM.read(1); 
  brightness = EEPROM.read(2);
  volume = EEPROM.read(3);

  //set volume
  WT588D_Send_Command(volume);

  //set brightness
  strip.setBrightness(brightness);

  set_speed();

  //retreive saved solid color values from EEPROM
  map_solid_color_values();

  output_flag = DEBUG_OUTPUT;  

  check_for_held_buttons_on_startup();  


} //end setup()


void loop() {

  t.update();  //required for timer events

  //check for input on the switches and color sliders
  check_switches();
  
  //if (mode == MODE_SOLID) void;  
  if (mode == MODE_SLIDER) slider();
  if (mode == MODE_REVOLVE) revolve();

  

} // end loop()


void counter() {  
  for(forvar=0; forvar< strip.numPixels(); forvar++) {   
    color_change_counter++;   
  }   
  switch_1_debounce_counter++;
  switch_2_debounce_counter++;
  switch_3_debounce_counter++;
} //end counter()


void check_switches() {

  //read slider analog values, convert to digital values from 0 to 255
  red_slider = map(analogRead(SLIDER_RED_INPUT), 0, 1023, 0, 255); 
  green_slider = map(analogRead(SLIDER_GREEN_INPUT), 0, 1023, 0, 255); 
  blue_slider = map(analogRead(SLIDER_BLUE_INPUT), 0, 1023, 0, 255); 

  //force maximun combined value to a total of 510 units
  signed int test_value = 510 - red_slider - green_slider- blue_slider;
  if (test_value < 0) {  
    red_slider = red_slider + test_value / 3;
    green_slider = green_slider + test_value / 3;
    blue_slider = blue_slider + test_value / 3;  
  }

  //check for a change on the speed slider
  slider_speed = map(analogRead(SLIDER_SPEED_INPUT), 0, 1023, 0, 255); 
  if (slider_speed + SLIDER_CHANGE_CHECK_RANGE < slider_speed_buffer || slider_speed - SLIDER_CHANGE_CHECK_RANGE > slider_speed_buffer) set_speed();

  //check for new input on the sliders when in MODE_SOLID   
  if (red_slider + SLIDER_CHANGE_CHECK_RANGE < red_slider_buffer || red_slider - SLIDER_CHANGE_CHECK_RANGE > red_slider_buffer ||
    green_slider + SLIDER_CHANGE_CHECK_RANGE < green_slider_buffer || green_slider - SLIDER_CHANGE_CHECK_RANGE > green_slider_buffer ||
    blue_slider + SLIDER_CHANGE_CHECK_RANGE < blue_slider_buffer || blue_slider - SLIDER_CHANGE_CHECK_RANGE > blue_slider_buffer) 
    //&& red_slider > SLIDER_CHANGE_CHECK_RANGE && green_slider > SLIDER_CHANGE_CHECK_RANGE && blue_slider > SLIDER_CHANGE_CHECK_RANGE)
  {
    if (mode!= MODE_SLIDER) {
      mode = MODE_SLIDER; 
      WT588D_Send_Command(PLAY_MODE_SLIDER);      
    }    
  }

  //check if the switches are being pressed
  if (digitalRead(SWITCH_1) == 0 && switch_1_flag == 0) {

    switch_1_debounce_counter = 0;
    switch_1_flag = 1;

    mode = mode + 1; 
    if (mode > MAX_MODES) mode = 1;
  
      set_speed();  

    //force mode to quickly refresh colors
    color_change_counter = 32000;

    EEPROM.write(1, mode);

    if (mode== MODE_SOLID) WT588D_Send_Command(PLAY_MODE_SOLID);
    if (mode== MODE_SLIDER) WT588D_Send_Command(PLAY_MODE_SLIDER);
    if (mode== MODE_REVOLVE) WT588D_Send_Command(PLAY_MODE_REVOLVE);

  } //end SWITCH_MODE


  if (digitalRead(SWITCH_2) == 0 && switch_2_flag == 0) {

    switch_2_debounce_counter = 0;
    switch_2_flag = 1;
    
    if (mode== MODE_SOLID) {
      
       while (solid_color_select == solid_color_select_buffer) {
      
      solid_color_select = random(1, 13);
      
    } //end while
    
    solid_color_select_buffer = solid_color_select;
      
        for(forvar = 0; forvar < strip.numPixels(); forvar++) {          
                color = solid_color_values[solid_color_select];
              }
       
    play_color_sound();

    }
    
  } //end SWITCH_SUBMODE

  if (digitalRead(SWITCH_3) == 0 && switch_3_flag == 0) {

    switch_3_debounce_counter = 0;
    switch_3_flag = 1;

  } //end SWITCH_PREFERENCE

  //release switch flags after the debounce period and the button as been depressed
  if (digitalRead(SWITCH_1) == 1 && switch_1_debounce_counter > SWITCH_DEBOUNCE_DELAY) switch_1_flag = 0;
  if (digitalRead(SWITCH_2) == 1 && switch_2_debounce_counter > SWITCH_DEBOUNCE_DELAY) switch_2_flag = 0;
  if (digitalRead(SWITCH_3) == 1 && switch_3_debounce_counter > SWITCH_DEBOUNCE_DELAY) switch_3_flag = 0;

  //special switch functions  
  if (digitalRead(SWITCH_2) == 0 && switch_2_debounce_counter > SWITCH_HELD_DELAY) {

    WT588D_Send_Command(PLAY_SET_BRIGHTNESS);

    //set brightness when all the color sliders are down and all three control buttons are pressed
    while (red_slider < 10 && green_slider < 10 && blue_slider < 10 && digitalRead(SWITCH_3) == 0)

    {
      for(forvar = 0; forvar < strip.numPixels(); forvar++) {          
        strip.setPixelColor(forvar, 0, 255, 0); 
      }

      brightness = map(analogRead(SLIDER_SPEED_INPUT), 0, 1023, 127, 255);
      strip.setBrightness(brightness);

      strip.show();

    } //end while

    //store brightness
    EEPROM.write(4, brightness);
    WT588D_Send_Command(PLAY_NEW_BRIGHTNESS_SET);


  } //end if

} //end check_switches()


void set_speed(void) {

  //store slider_speed for later reference when checking for user input on the slider
  slider_speed_buffer = slider_speed; 

  if (mode == MODE_SOLID) smooth_timing = 2;
  if (mode == MODE_SLIDER) void();
  
  if (mode == MODE_REVOLVE) { 

    smooth_timing = 0;
      color_change_delay = slider_speed / 2;
      
   

  }
} //end set_speed()


void slider(void) {

  //store buffer values here to prevent a slow ramp up in values in the main loop
  //slider values gathered during check_switches()
  red_slider_buffer = red_slider; 
  green_slider_buffer = green_slider; 
  blue_slider_buffer = blue_slider; 

  //set all LEDs to the same color
  for(forvar = 0; forvar < strip.numPixels(); forvar++) {
    strip.setPixelColor(forvar, strip.Color(red_slider, green_slider, blue_slider));   
  } 

  strip.show(); 
}



void revolve(void) {
  
    if (color_change_counter > color_change_delay) {

      color_change_counter = 0;

      revolve_position++;

        for(forvar = 0; forvar < strip.numPixels(); forvar++) {
          strip.setPixelColor(forvar, Wheel(revolve_position));      
        }  

      
      strip.show();   

    } //end if

} //end revolve()



// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


// Smooth lighting transitions
// The transition from brightness 0 to brightness 255 requires 25.5ms
void smooth(void) {

  //do not attempt to smooth the following modes; these modes automatically update the LED colors as needed.
  if (mode == MODE_SLIDER) return; 
  if (mode == MODE_REVOLVE) return; 

  smooth_counter++;

  if (smooth_counter < smooth_timing / 25) return;

  smooth_counter = 0;

  uint32_t current_led_color, target_led_color;
  uint8_t r, g, b, target_r, target_g, target_b;

  for(forvar = 0; forvar < strip.numPixels(); forvar++) {          

    //get current LED color values
    current_led_color = color_buffer;      
    r = (uint8_t)(current_led_color >> 16),
    g = (uint8_t)(current_led_color >>  8),
    b = (uint8_t)(current_led_color);

    //get target LED values
    target_led_color = color;  
    target_r = (uint8_t)(target_led_color >> 16),
    target_g = (uint8_t)(target_led_color >>  8),
    target_b = (uint8_t)(target_led_color);     

    //increase values by a factor of 3
    //note: we must loop in multiples due to a low clock speed oro there is not enought time to smooth between color changes
    for(byte loop_multiple = 0; loop_multiple < 3; loop_multiple++) {
      //compare values and update values as needed     
      if (r > target_r) r--;  
      if (r < target_r) r++;  
      if (g > target_g) g--;  
      if (g < target_g) g++; 
      if (b > target_b) b--;  
      if (b < target_b) b++; 
    }

    color_buffer = color;

    //set new colors
    strip.setPixelColor(forvar, strip.Color(r, g, b) );

  } //end for

  strip.show();  

} //end smooth


//store saved solid color values from the EEPROM into the solid_color_values
void map_solid_color_values(void) {

  byte i = 97;

  for(byte forvar = 1; forvar < 20; forvar++) {

    i = i + 3;  

    solid_color_values[forvar] = strip.Color(EEPROM.read(i), EEPROM.read(i + 1), EEPROM.read(i + 2)); 

  }//end for

} //end map_solid_color_values()



//note: serialEvent is triggered by the Arduino software automatically when serial data is present    
void serialEvent() {
  while (Serial.available()) {

    //get the new byte:
    char inChar = (char)Serial.read(); 

    //add it to serial_rx_data:
    serial_rx_data += inChar;

    //if the incoming character is a newline, processing the data
    if (inChar == '\n') {

      Serial.print("Data Received: ");
      Serial.print(serial_rx_data);

      //check for terminal commands 
      if (serial_rx_data == "load defaults\n") {

        Serial.println("Default values loaded ");        
        eeprom_store_default_values();

      }

      if (serial_rx_data == "output on\n") output_flag = true;
      if (serial_rx_data == "output off\n") output_flag = false;      

      //check device ID from sending device      
      if (serial_rx_data[0] == 1 + 48) {

        //check command ID issued from sending device
        //set mode solid, display color
        if (serial_rx_data[1] == 1 + 48) {

          //do not update color if submode button is being held
          //the button being held is used to save new color values
          if (digitalRead(SWITCH_2) == 1) {

           
              Serial.println("Displaying Solid Color");   

              solid_color_select = (serial_rx_data[2] - 48) * 10 + serial_rx_data[3] - 48;

              play_color_sound();

              mode = MODE_SOLID;

              set_speed();

              for(forvar = 0; forvar < strip.numPixels(); forvar++) {          
                color = solid_color_values[solid_color_select];
              }

           


          } //end if (digitalRead(SWITCH_SUBMODE) == 1)

        } //end command ID = 1

        //save color on sliders to corresponding held keypad button
        if (serial_rx_data[1] == 2 + 48) {


          //only allow saving of new solid color if the submode button is being held while in slider mode
          //helps prevent unintentional saving
          if (digitalRead(SWITCH_2) == 0 && mode == MODE_SLIDER) {


            byte button_number = (serial_rx_data[2] - 48) * 10 + serial_rx_data[3] - 48;

            //do not allow writing to protected colors
            if (button_number > WRITE_PROTECT_SOLID_COLORS) {

              Serial.println("Saving Solid Color");

              WT588D_Send_Command(PLAY_COLOR_VALUE_SET);

              //save new values
              EEPROM.write(100 + button_number * 3 - 3, red_slider);
              EEPROM.write(100 + button_number * 3 - 2, green_slider);
              EEPROM.write(100 + button_number * 3 - 1, blue_slider); 
              solid_color_values[button_number] = strip.Color(red_slider, green_slider, blue_slider);

              //blink LEDs to indicate the new color is saved          
              for(byte i = 0; i < 5; i++) {
                for(forvar = 0; forvar < strip.numPixels(); forvar++) {          
                  strip.setPixelColor(forvar, strip.Color(0, 0, 0)); 
                }     
                strip.show();            
                delay(50);
                for(forvar = 0; forvar < strip.numPixels(); forvar++) {          
                  strip.setPixelColor(forvar, strip.Color(red_slider, green_slider, blue_slider)); 
                } 
                strip.show();           
                delay(50);

              } //end for
              //end blink

            } //end if (button_number > WRITE_PROTECT_SOLID_COLORS)

          } //end if (digitalRead(SWITCH_SUBMODE) == 0  && mode == MODE_SLIDER)

        } //end command ID = 2

      }// end if (serial_rx_data[0] == 1 + 48)

      serial_rx_data = "";

    } //if (inChar == '\n') {

  } //end while
}



void play_color_sound(){
  
  //scan for color value
              byte play_color = 0;
              if (solid_color_values[solid_color_select] == red) play_color = 21;
              if (solid_color_values[solid_color_select] == green) play_color = 22;
              if (solid_color_values[solid_color_select] == blue) play_color = 23;
              if (solid_color_values[solid_color_select] == yellow) play_color = 24;
              if (solid_color_values[solid_color_select] == magenta) play_color = 25;
              if (solid_color_values[solid_color_select] == cyan) play_color = 26;
              if (solid_color_values[solid_color_select] == amber) play_color = 27;
              if (solid_color_values[solid_color_select] == orange) play_color = 28;
              if (solid_color_values[solid_color_select] == purple) play_color = 29;
              if (solid_color_values[solid_color_select] == pink) play_color = 30;
              if (solid_color_values[solid_color_select] == aquamarine) play_color = 31;
              if (solid_color_values[solid_color_select] == seafoam_green) play_color = 32;
              if (solid_color_values[solid_color_select] == lavender) play_color = 33;

              if (solid_color_values[solid_color_select] == 0)
              {
                WT588D_Send_Command(PLAY_NO_COLOR);
              }
              else 
              {              
                if (play_color == 0) {
                  WT588D_Send_Command(PLAY_CUSTOM_COLOR); 
                }               
                else {
                  WT588D_Send_Command(play_color); 
                }           
              }
              
}//end ..

void eeprom_store_default_values(void) {

  WT588D_Send_Command(PLAY_DEFAULT_VALUES_LOADED);

  EEPROM.write(0, 0); //test address
  EEPROM.write(1, 1); //mode 
  EEPROM.write(2, 255); //brightness
  EEPROM.write(3, 8); //volume

  //store colors in a specific order    
  //1
  EEPROM.write(100, (uint8_t)(red >> 16));
  EEPROM.write(101, (uint8_t)(red >> 8));
  EEPROM.write(102, (uint8_t)(red));  
  //2
  EEPROM.write(103, (uint8_t)(green >> 16));
  EEPROM.write(104, (uint8_t)(green >> 8));
  EEPROM.write(105, (uint8_t)(green));  
  //3
  EEPROM.write(106, (uint8_t)(blue >> 16));
  EEPROM.write(107, (uint8_t)(blue >> 8));
  EEPROM.write(108, (uint8_t)(blue));  
  //4
  EEPROM.write(109, (uint8_t)(yellow >> 16));
  EEPROM.write(110, (uint8_t)(yellow >> 8));
  EEPROM.write(111, (uint8_t)(yellow)); 
  //5
  EEPROM.write(112, (uint8_t)(magenta >> 16));
  EEPROM.write(113, (uint8_t)(magenta >> 8));
  EEPROM.write(114, (uint8_t)(magenta));   
  //6
  EEPROM.write(115, (uint8_t)(cyan >> 16));
  EEPROM.write(116, (uint8_t)(cyan >> 8));
  EEPROM.write(117, (uint8_t)(cyan));  
  //7
  EEPROM.write(118, (uint8_t)(amber >> 16));
  EEPROM.write(119, (uint8_t)(amber >> 8));
  EEPROM.write(120, (uint8_t)(amber)); 
  //8
  EEPROM.write(121, (uint8_t)(orange >> 16));
  EEPROM.write(122, (uint8_t)(orange >> 8));
  EEPROM.write(123, (uint8_t)(orange));   
  //9
  EEPROM.write(124, (uint8_t)(purple >> 16));
  EEPROM.write(125, (uint8_t)(purple >> 8));
  EEPROM.write(126, (uint8_t)(purple));  
  //10
  EEPROM.write(127, (uint8_t)(pink >> 16));
  EEPROM.write(128, (uint8_t)(pink >> 8));
  EEPROM.write(129, (uint8_t)(pink));  
  //11
  EEPROM.write(130, (uint8_t)(aquamarine >> 16));
  EEPROM.write(131, (uint8_t)(aquamarine >> 8));
  EEPROM.write(132, (uint8_t)(aquamarine));  
  //12
  EEPROM.write(133, (uint8_t)(seafoam_green >> 16));
  EEPROM.write(134, (uint8_t)(seafoam_green >> 8));
  EEPROM.write(135, (uint8_t)(seafoam_green));  
  //13
  EEPROM.write(136, (uint8_t)(lavender >> 16));
  EEPROM.write(137, (uint8_t)(lavender >> 8));
  EEPROM.write(138, (uint8_t)(lavender));  
  //14
  EEPROM.write(139, (uint8_t)(no_color >> 16));
  EEPROM.write(140, (uint8_t)(no_color >> 8));
  EEPROM.write(141, (uint8_t)(no_color));  
  //15
  EEPROM.write(142, (uint8_t)(no_color >> 16));
  EEPROM.write(143, (uint8_t)(no_color >> 8));
  EEPROM.write(143, (uint8_t)(no_color));  
  //16
  EEPROM.write(144, (uint8_t)(no_color >> 16));
  EEPROM.write(145, (uint8_t)(no_color >> 8));
  EEPROM.write(146, (uint8_t)(no_color));  
  //17
  EEPROM.write(147, (uint8_t)(no_color >> 16));
  EEPROM.write(148, (uint8_t)(no_color >> 8));
  EEPROM.write(149, (uint8_t)(no_color));  
  //18
  EEPROM.write(150, (uint8_t)(no_color >> 16));
  EEPROM.write(151, (uint8_t)(no_color >> 8));
  EEPROM.write(152, (uint8_t)(no_color));  
  //19
  EEPROM.write(153, (uint8_t)(no_color >> 16));
  EEPROM.write(154, (uint8_t)(no_color >> 8));
  EEPROM.write(155, (uint8_t)(no_color));  
  //20
  EEPROM.write(156, (uint8_t)(no_color >> 16));
  EEPROM.write(157, (uint8_t)(no_color >> 8));
  EEPROM.write(158, (uint8_t)(no_color));  
  
  //retreive saved solid color values from EEPROM and load them into our variables
  map_solid_color_values();  
 
} //end eeprom_store_default_values()



void check_for_held_buttons_on_startup() {

  color_change_counter = 0;

  for(forvar = 0; forvar < strip.numPixels(); forvar++) {          
    strip.setPixelColor(forvar, strip.Color(0, 55, 0)); 
  }     
  strip.show();  

  //while all three buttons are being held 
  while (digitalRead(SWITCH_1) == 0 && digitalRead(SWITCH_2) == 0 && digitalRead(SWITCH_3) == 0) {

    t.update();  //required for timer events

    //if the buttons are held for more than 3 seconds, restore system default values
    if (color_change_counter > 3000) {

      eeprom_store_default_values();

      //blink LEDs to indicate the new color is saved          
      for(byte i = 0; i < 5; i++) {
        for(forvar = 0; forvar < strip.numPixels(); forvar++) {          
          strip.setPixelColor(forvar, strip.Color(0, 0, 0)); 
        }     
        strip.show();            
        delay(50);
        for(forvar = 0; forvar < strip.numPixels(); forvar++) {          
          strip.setPixelColor(forvar, strip.Color(0, 127, 0)); 
        } 
        strip.show();           
        delay(50);

      } //end for
      //end blink

      break;

    }

  } //end while  

} //end check_for_held_buttons_on_startup()


void WT588D_Send_Command(unsigned char addr) {

  unsigned char i;

  digitalWrite(WT588D_CS, LOW); 

  delay(6); //5ms delay per device specifications

  for( i = 0; i < 8; i++)  {    
    digitalWrite(WT588D_SCL, LOW);    
    if(bitRead(addr, i))digitalWrite(WT588D_SDA, HIGH);
    else digitalWrite(WT588D_SDA, LOW);          
    delay(2);  //1ms delay per device specifications   
    digitalWrite(WT588D_SCL, HIGH);    
    delay(2);  //1ms delay per device specifications 
  } //end for

  digitalWrite(WT588D_CS, HIGH);

} //end WT588D_Send_Command


void debug(void) {

  if (output_flag == true) {

    Serial.print(mode);
    Serial.print(" - ");   
    Serial.print(brightness);
    Serial.print(" - - ");
    Serial.print(slider_speed);
    Serial.print(" - - ");
    Serial.print(red_slider);
    Serial.print(" - ");
    Serial.print(green_slider);
    Serial.print(" - ");
    Serial.println(blue_slider);

  }

} //end debug()


