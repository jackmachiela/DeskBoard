///
/// Desktop Dashboard Project
/// Part one: FM Radio Module
/// 
/// Author Jack Machiela, https://github.com/jackmachiela
/// Copyright (c) 2022 by Jack Machiela
///
/// Details
/// This is the first Arduino module used in the desktop Dashboard housed in a picture
/// frame. This one controls only the FM radio portion of the project.
/// The main project is listed under https://github.com/jackmachiela/DeskBoard
/// This specific part is listed as  https://github.com/jackmachiela/DeskBoard/DeskBoardFM
///
/// 


// Initialise the Rotary Encoder
  #include <MD_REncoder.h>        // Library "MD_REncoder by MajicDesigns" - Tested at v1.0.1
                                  // lib & docs at https://github.com/MajicDesigns/MD_REncoder

  #include <MD_UISwitch.h>        // Library "MD_UISwitch by MajicDesigns" - Tested at v2.2.2 
                                  // lib & docs at https://github.com/MajicDesigns/MD_UISwitch
                                  // This library for button click (on the rotary)

  #include <TM1637Display.h>      // Library "TM1637 by Avishay Orpaz" - Tested at v1.2.0
                                  // lib & docs at https://github.com/avishorp/TM1637

  #include <SI4703.h>             // Library "Radio by Matthias Hertel" - Tested at v1.2.0
                                  // lib & docs at http://www.mathertel.de/Arduino
  


// Define the Rotary Encoder
MD_REncoder rotEnc = MD_REncoder(4, 3);           // RotEnc's DT, CLK pins -> Arduino's D4, D3 pins
unsigned long long lastRotEncAction = 0;          // Contains millis since last Rotary Encoder was clicked or rotated

// Define the Rotary Encoder button
const uint8_t DIGITAL_SWITCH_PIN = 7;                              // SET SW PIN
const uint8_t DIGITAL_SWITCH_ACTIVE = LOW;                         // digital signal when switch is pressed 'on'
MD_UISwitch_Digital S(DIGITAL_SWITCH_PIN, DIGITAL_SWITCH_ACTIVE);

// Define where the Display is located
const int CLK = 5;             //Set the CLK pin connection to the display; D5
const int DIO = 6;             //Set the DIO pin connection to the display; D6

TM1637Display display(CLK, DIO);               //set up the 4-Digit Display

bool updateFlag = true ;

// Define the Radio
SI4703   radio;    ///< Create an instance of a SI4703 chip radio.

RADIO_FREQ preset[] = {                    // Define some stations available at your locations here. Lowest possible for SI4703 is 8750, highest is 10800
      8900, // 00 - Concert FM
      9060, // 01 - ZM FM
      9140, // 02 (Religious nonsense)
      9220, // 03 - More FM
      9380, // 04 - Magic Talk (Talkback bullshit)
      9460, // 05 - The Sound
      9780, // 06 - The Hits
      9860, // 07 - The Breeze FM
      9940, // 08 - Central FM
     10020, // 09 - NewsTalk ZB (Talkback bullshit)
     10700  // 10 - Dannevirke FM  (Old hits, country, yodelling)
};                                            //[NB - last station gets no comma]

// And some Channel and Frequency stuff
int maxChannel=10;                                             // Total number of available Channels
int curChannel=7;                                              // Start at Station with index=7  (The Breeze FM)
int previousChannel;                                           //

int minFreq=875;                                               // Highest possible frequency
int maxFreq=1080;                                              // Lowest possible frequency
int currentFreq = (preset[curChannel]/10);                     // Starting frequency
int previousFreq = -1;                                         // 

int tuneMode=3;                            // tuneMode 1 = incr=0.1 (97.2, 97.3, 97.4, etc)  [ de/select by clicking down on RotEnc once]
                                           // tuneMode 2 = incr=1   (97.2, 98.2, 99.3, etc)  [ de/select by clicking down on RotEnc once]
                                           // tuneMode 3 = Preset Channel select             [ de/select by double-clicking RotEnc, or deselect by wait for 6 seconds]

int previousRadioVolume;
int currentRadioVolume=0;
const int radioVolumePin=A0;


void setup() {                                                            ///// Setup

  //initialise the serial monitor  
  Serial.begin(115200);
  delay(10);      // let the serial connection calm down

  // Initialise the Radio 
  initRadio();

  // Initialise the Rotary Encoder & Button
  initRotaryEncoder();

  //Initialise the LCD display
  display.setBrightness(7);
  
  updateLCDandRadio();

}


void loop() {                                                              // put your main code here, to run repeatedly:

  // Check if the Rotary Encoder's button is being pushed
  readButton();

  // Check if the Radio Volume is being adjusted
  readVolume();

  // Check if Rotary Encoder has been turned
  readRotary();

  // Update LCD screen
  if (updateFlag == true) updateLCDandRadio();
}

void initRadio(){
  radio.init();
  // radio.setBandFrequency(RADIO_BAND_FM, currentFreq*10);             // preset frequency
  radio.setBandFrequency(RADIO_BAND_FM, preset[curChannel]);             // preset channels
  radio.setMono(false);
  radio.setMute(false);
  radio.setVolume(currentRadioVolume);

}

void initRotaryEncoder() {
  // Initialise the Rotary Encoder & Button
  rotEnc.begin();

  // Initialise the Rotary Button
  S.begin();
  S.setDoublePressTime(500);
  //S.setLongPressTime(3000);
  S.enableDoublePress(true);
  S.enableLongPress(false);
  S.enableRepeat(false);
  S.enableRepeatResult(false);

}


void readButton() {
  MD_UISwitch::keyResult_t k = S.read();

  switch(k)
  {
    case MD_UISwitch::KEY_NULL:      /* Serial.print("KEY_NULL"); */            break;
    case MD_UISwitch::KEY_UP:        Serial.print("\nKEY_UP ");     if (tuneMode==2) tuneMode=1; else if (tuneMode==1) tuneMode=2; break;
    case MD_UISwitch::KEY_DOWN:      Serial.print("\nKEY_DOWN ");               break;
    case MD_UISwitch::KEY_PRESS:     Serial.print("\nKEY_PRESS ");              break;
    case MD_UISwitch::KEY_DPRESS:    Serial.print("\nKEY_DOUBLE "); updateFlag = true; if ((tuneMode==1) or (tuneMode==2)) tuneMode=3; else if (tuneMode==3) tuneMode=1; break;
    case MD_UISwitch::KEY_LONGPRESS: Serial.print("\nKEY_LONG   ");             break;
    case MD_UISwitch::KEY_RPTPRESS:  Serial.print("\nKEY_REPEAT ");             break;
    default:                         Serial.print("\nKEY_UNKNWN ");             break;
  }
  if (k != MD_UISwitch::KEY_NULL)
  {
    if (S.getKey() >= ' ')
    {
      Serial.print((char)S.getKey());
      Serial.print(" ");
    }
    Serial.print("[0x");
    Serial.print(S.getKey(), HEX);
    Serial.print("]");

    Serial.print(tuneMode);

  }


}

void readVolume() {
  currentRadioVolume = map(analogRead(radioVolumePin), 0, 1024, 0, 16);
  if (currentRadioVolume != previousRadioVolume) {   // Only set the volume if it's actually different from previous setting, otherwise do nothing
    radio.setVolume(currentRadioVolume);
  }
  previousRadioVolume = currentRadioVolume;

}


void readRotary() {

    uint8_t rotEncReading = rotEnc.read();

  
  if (rotEncReading)  {
    Serial.print(rotEncReading == DIR_CW ? "\n-1" : "\n+1");

    if (tuneMode == 1) currentFreq=(currentFreq + (rotEncReading == DIR_CW ?  -1 : 1 ));     // Add or subtract 0.1 to the current Freq (tuneMode=1, fine tune)
    if (tuneMode == 2) currentFreq=(currentFreq + (rotEncReading == DIR_CW ? -10 : 10));     // Add or subtract 1 to the current Freq (tuneMode=2, fast tune)
    if (tuneMode == 3) curChannel=(curChannel + (rotEncReading == DIR_CW ? -1 : 1));         // preset channels
    if (tuneMode == 3) currentFreq=((preset[curChannel])/10);

    if (currentFreq > maxFreq) currentFreq = minFreq;                 // If it goes over the maximum available frequency, reset to lowest available frequency
    if (currentFreq < minFreq) currentFreq = maxFreq;                 // If it goes under the lowest available Freq, reset to highest

    if (curChannel > maxChannel) curChannel = 0;                      // If it goes over the maximum available channel, reset to 0
    if (curChannel < 0) curChannel = maxChannel;                      // If it goes under the lowest available (=0), reset to highest

    lastRotEncAction = millis();                                      // Set last action to current time
    
    updateFlag = true;

  }

  if (tuneMode == 3) {

    if ((lastRotEncAction + 6000) <= millis()) {              //if less than 6 seconds have passed since Rotatry encoder was used, revert to display Freq numbers only

      tuneMode = 1; 
      updateFlag = true;

    }

  }

}


void updateLCDandRadio() {

  if (tuneMode == 3) display.showNumberDecEx(curChannel, 0, false);  else   display.showNumberDecEx(currentFreq, 32, false);         // Show Freq as integer*10, with one decimal place , more info see .\libraries\TM1637\TM1637Display.h
  if (tuneMode == 3) radio.setFrequency(preset[curChannel]);         else   radio.setFrequency(currentFreq*10);                      // Change the radio's frequency

  previousFreq = currentFreq;
  updateFlag = false;

}
