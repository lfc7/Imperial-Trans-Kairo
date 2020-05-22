/* Trans Kairo
   teensy 3.6 + sgtl_500 (audio chip)

    play "WAGONAVG.WAV" on sdcard ( // filenames are always uppercase 8.3 format )
*/

//#define USE_TEENSY3_OPTIMIZED_CODE 1

//#define USE_SDIO          1


#define DUCK_VOL  0.10   // 0.0 to 1.0
#define DUCK_ATTACK_TIME 200 // in ms
#define DUCK_RELEASE_TIME 1500 // in ms

#define WAV_VOL     0.65
#define JINGLE_VOL  0.50
#define MIC_VOL     0.80
#define LINEOUT_LEVEL 29 //lineOutLevel(both) 13 max - 31 default 29;

#define BLINKTIME 800 
#define BOUNCE_MS 50
#define FILENAMEFORMAT  "WAGON%d.WAV" // 8.1 upper name ; where %d is 1-4 5-8 9-12
//#define AUDIOFILENAME  "WAGONAVG.WAV"

#define REFRESHLCD  500
#define REFRESH_INFO 3000




#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce2.h>
//LCD controls & special char
#include "LCDcontrol.h"


// GUItool: begin automatically generated code
AudioInputI2S            input;          //xy=55,224
AudioSynthWaveformDc     cmdVca;            //xy=55,426.75
AudioPlaySdWav           playSdWav;      //xy=65,306
AudioAnalyzePeak         mic_peak;       //xy=275,163
AudioEffectMultiply      vcaJingle;      //xy=283.75,237.75
AudioEffectMultiply      vcaWavL;      //xy=287.75,304.75
AudioEffectMultiply      vcaWavR;      //xy=289.75,336.75
AudioEffectFade          fadeJingles;    //xy=464,237
AudioEffectFade          fadeMic;        //xy=465,207
AudioEffectFade          fadeWavR;       //xy=465.75,337
AudioEffectFade          fadeWavL;       //xy=466.75,305
AudioMixer4              mixerR;         //xy=737.75,318
AudioMixer4              mixerL;         //xy=739.75,225
AudioOutputI2S           output;         //xy=938.75,273
AudioConnection          patchCord1(input, 0, fadeMic, 0);
AudioConnection          patchCord2(input, 0, mic_peak, 0);
AudioConnection          patchCord3(input, 1, vcaJingle, 0);
AudioConnection          patchCord4(cmdVca, 0, vcaJingle, 1);
AudioConnection          patchCord5(cmdVca, 0, vcaWavL, 1);
AudioConnection          patchCord6(cmdVca, 0, vcaWavR, 1);
AudioConnection          patchCord7(playSdWav, 0, vcaWavL, 0);
AudioConnection          patchCord8(playSdWav, 1, vcaWavR, 0);
AudioConnection          patchCord9(vcaJingle, fadeJingles);
AudioConnection          patchCord10(vcaWavL, fadeWavL);
AudioConnection          patchCord11(vcaWavR, fadeWavR);
AudioConnection          patchCord12(fadeJingles, 0, mixerL, 1);
AudioConnection          patchCord13(fadeJingles, 0, mixerR, 1);
AudioConnection          patchCord14(fadeMic, 0, mixerL, 0);
AudioConnection          patchCord15(fadeMic, 0, mixerR, 0);
AudioConnection          patchCord16(fadeWavR, 0, mixerR, 2);
AudioConnection          patchCord17(fadeWavL, 0, mixerL, 2);
AudioConnection          patchCord18(mixerR, 0, output, 1);
AudioConnection          patchCord19(mixerL, 0, output, 0);
AudioControlSGTL5000     ctrl_sgtl;      //xy=57.75,97
// GUItool: end automatically generated code

// pin assignations ***************
//bus on all teensy
#define UNMUTEJINGLES_PIN 25  //not in use yet // jingles are muted by jingle module himself
#define TALKOVER_PIN      24
#define STARTPAUSE_PIN    26
#define STOP_PIN          27
//specific
#define UNMUTEMIC_PIN     28
#define UNMUTEPFL_PIN     29

//outputs
#define LEDPLAY_PIN       30
#define LEDMIC_PIN        31

#define BATT_PIN          33


// Use these with the Teensy 3.5 & 3.6 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)

//debounce
#define NUM_BUTTONS 6
const uint8_t BUTTONS_PINS[NUM_BUTTONS] = {UNMUTEMIC_PIN, UNMUTEJINGLES_PIN, UNMUTEPFL_PIN, TALKOVER_PIN, STARTPAUSE_PIN, STOP_PIN};
Bounce * buttons = new Bounce[NUM_BUTTONS];

void  unMuteMic();
void  unMuteJingles();
void  unMutePFL();
void  duckWav();
void  playAudio();
void  stopAudio();
void  doNothing();

typedef void (*buttonFellActionList[])();
buttonFellActionList buttonFellAction = { unMuteMic, doNothing, unMutePFL, duckWav, playAudio, stopAudio };

void  muteMic();
void  muteJingles();
void  mutePFL();
void  restoreWav();
void  pauseAudio();
void  setReadyToPlay();


typedef void (*buttonRoseActionList[])();
buttonRoseActionList buttonRoseAction = { muteMic, doNothing, mutePFL, restoreWav, doNothing, setReadyToPlay };
/*
  typedef void (*buttonReadActionList[])();
  buttonReadActionList buttonReadAction = { ctrlMic, doNothing, ctrlPFL, doNothing, doNothing, doNothing };
*/
#define NUM_LEDS  2
#define LEDPLAY   0
#define LEDMIC    1
const uint8_t LEDS_PINS[NUM_LEDS] = {LEDPLAY_PIN, LEDMIC_PIN};
uint8_t       ledStatus[NUM_LEDS] = {0, 0}; // 0 off, 1 on, 2 blink
elapsedMillis ledTimer[NUM_LEDS];

boolean flag_unMuteMic = false;
boolean flag_unMutePFL = false;
boolean flag_readyToPlay = false;
boolean flag_duck = false;
boolean flag_pause = false;
//boolean	flag_JinglesMuted = true;

elapsedMillis refresh_lcd;
elapsedMillis refresh_Info ;

bool  flagAnnoncesAvail = false;
bool  flagPrioAnnonces = false;
char AnnoncesLCD[21];
int AnnoncesLCDindex;

bool  flagExtAvail = false;
bool  flagPrioExt = false;
char ExtLCD[21];
int ExtLCDindex;

//LPF
float SmoothData;
float SmoothMicLevel;
float LPF_Beta = 0.1; // 0<ß<1 (0.025)

File myFile;

//wav filename
const size_t bufferLen = 13; 
char filename[bufferLen];

int selectedWav=1;

/***********************************************************************************************************************************/

void setup() {

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(16);

  ctrl_sgtl.enable();

  delay(10);

  ctrl_sgtl.muteHeadphone();
  ctrl_sgtl.muteLineout();

  ctrl_sgtl.volume(0.7, 0.7);
  ctrl_sgtl.inputSelect(AUDIO_INPUT_LINEIN);
  ctrl_sgtl.lineInLevel(7, 7); //lineInLevel(left, right)  0 - 15 default 5
  ctrl_sgtl.lineOutLevel(LINEOUT_LEVEL); //lineOutLevel(both) 13-31 default 29;


  ctrl_sgtl.audioPostProcessorEnable();
  ctrl_sgtl.autoVolumeControl(0, 0, 0, -12.0, 48.0, 12.0);//autoVolumeControl(maxGain, response, hardLimit, threshold, attack, decay);
  ctrl_sgtl.autoVolumeEnable();
  ctrl_sgtl.eqSelect(3);
  ctrl_sgtl.eqBands(0.8, 0.7, -0.1, -0.5, -0.3); //115Hz, 330Hz, 990Hz, 3kHz, and 9.9kHz. Each band has a range of adjustment from 1.00 (+12dB) to -1.00 (-11.75dB).
  //ctrl_sgtl.adcHighPassFilterDisable();

  mixerL.gain(0, MIC_VOL); //mic
  mixerL.gain(1, JINGLE_VOL); //annonces
  mixerL.gain(2, WAV_VOL); //wav left
  mixerL.gain(3, 0.00); //spare right

  mixerR.gain(0, MIC_VOL); //mic
  mixerR.gain(1, JINGLE_VOL); //annonces
  mixerR.gain(2, WAV_VOL); //wav right
  mixerR.gain(3, 0.00); //wav right

  cmdVca.amplitude(1.0, DUCK_RELEASE_TIME);

  // Setup the first button with an internal pull-up :
  pinMode(UNMUTEMIC_PIN, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  buttons[0].attach(UNMUTEMIC_PIN);
  buttons[0].interval(BOUNCE_MS); // interval in ms

  pinMode(UNMUTEJINGLES_PIN, INPUT_PULLUP);
  buttons[1].attach(UNMUTEJINGLES_PIN);
  buttons[1].interval(BOUNCE_MS); // interval in ms

  pinMode(UNMUTEPFL_PIN, INPUT_PULLUP);
  buttons[2].attach(UNMUTEPFL_PIN);
  buttons[2].interval(BOUNCE_MS); // interval in ms

  pinMode(TALKOVER_PIN, INPUT);
  buttons[3].attach(TALKOVER_PIN);
  buttons[3].interval(BOUNCE_MS); // interval in ms

  pinMode(STARTPAUSE_PIN, INPUT_PULLUP);
  buttons[4].attach(STARTPAUSE_PIN);
  buttons[4].interval(BOUNCE_MS); // interval in ms

  pinMode(STOP_PIN, INPUT_PULLUP);
  buttons[5].attach(STOP_PIN);
  buttons[5].interval(BOUNCE_MS); // interval in ms

  pinMode(LEDPLAY_PIN, OUTPUT);
  pinMode(LEDMIC_PIN, OUTPUT);

  //start serial1 (LCD)
  Serial1.begin(9600); //9600
  //start serial3 (port com )
  Serial3.begin(9600); //9600

  //LCD boot screen
  displayInit();
  delay(100);
  displayClear();
  displayHome();
  Serial1.print("Initialisation");
  //delay(1000);

  //SPI.setMOSI(SDCARD_MOSI_PIN);
  //SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN)))
  {
    // stop here, but print a message repetitively
    while (1) {
      displayClear();
      displayHome();
      Serial1.print("Unable to access SD");
      delay(2000);
      displayClear();
      displayHome();
      Serial1.print("error");
      delay(2000);
    }
  } else {
    sprintf(filename, FILENAMEFORMAT, 1);
    myFile = SD.open(filename);
    if ( ! myFile )
    {
      while (1) {
        displayClear();
        displayHome();
        Serial1.print("WAGON1.WAV");
        Serial1.print(" not found");
        delay(2000);
        displayClear();
        displayHome();
        Serial1.print("verifier SDCard");
        delay(1000);
      }
    } else {
      // close the file:
      myFile.close();
    }
  }

  displayBoot();
  // delay(1500);

  initialCond();

  ctrl_sgtl.unmuteHeadphone();
  ctrl_sgtl.unmuteLineout();

}

void playAudio()
{
  if ( ! flag_readyToPlay )return;

  if ( playSdWav.isPlaying())
  {
    if ( ! flag_pause)
    {
      playSdWav.pause();
      flag_pause = true;
      LedPlayBLINK();
      return;
    } else {
      playSdWav.resume();
      flag_pause = false;
      LedPlayON();
      return;
    }
  }

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playSdWav.play(filename); // filenames are always uppercase 8.3 format

  // A brief delay for the library read WAV info
  delay(5);

  // Simply wait for the file to finish playing.
  LedPlayON();

  //	while (playSdWav.isPlaying())
  //	{
  //		updateAll();
  //	}

  //	LedPlayOFF();
}

void stopAudio()
{
  if( ! buttons[4].read() )
  {
    sprintf(filename, FILENAMEFORMAT, 1);
    displayClear();
    displayHome();
    Serial1.print(filename);
    delay(1000);
  }else{

    playSdWav.resume();
    playSdWav.stop();
    flag_readyToPlay = false;
	//fadeJingles.fadeIn(250);	//promenade
  }
  
}

void setReadyToPlay()
{
  
  if( ! buttons[4].read() )
  {
    sprintf(filename, FILENAMEFORMAT, 2);
    displayClear();
    displayHome();
    Serial1.print(filename);
    delay(1000);
    //Serial1.print(" not found");
  }else{
	//if(flag_JinglesMuted)fadeJingles.fadeOut(250);//mute promenade
    flag_readyToPlay = true;
  }
}

void pauseAudio()
{
  return;
}

void muteMic()
{
  fadeMic.fadeOut(250); 
  flag_unMuteMic = false;
  restoreWav();
  if (flag_unMutePFL)
  {
    LedMicON();
  } else {
    LedMicOFF();
  }
}

void unMuteMic()
{
  if ( flag_unMutePFL )
  {
    fadeMic.fadeIn(250);
    LedMicBLINK();
  }
  flag_unMuteMic = true;
}

void muteJingles()
{
  fadeJingles.fadeOut(250);
  
}

void unMuteJingles()
{
  fadeJingles.fadeIn(250);
}

void duckWav()
{
  if (flag_unMuteMic)
  {
    //mixerL.gain(2,DUCK_VOL);
    //mixerR.gain(3,DUCK_VOL);
    //mixerL.gain(1,DUCK_VOL);  //annonces
    //mixerR.gain(0,DUCK_VOL);  //annonces
    cmdVca.amplitude(DUCK_VOL, DUCK_ATTACK_TIME);
    flag_duck = true;
  }
}

void restoreWav()
{
  //mixerL.gain(2,0.50);
  //mixerR.gain(3,0.50);
  //mixerL.gain(1,JINGLE_VOL);  //annonces
  //mixerR.gain(1,JINGLE_VOL);  //annonces
  cmdVca.amplitude(1.0, DUCK_RELEASE_TIME);
  flag_duck = false;
}

void mutePFL()
{
  ctrl_sgtl.muteHeadphone();
  flag_unMutePFL = false;
  if (flag_unMuteMic)muteMic();
  LedMicOFF();
}

void unMutePFL()
{
  ctrl_sgtl.unmuteHeadphone();
  flag_unMutePFL = true;
  LedMicON();
  if(flag_unMuteMic)unMuteMic();
}

void muteLineOut()
{
  ctrl_sgtl.muteLineout();
}

void UnMuteLineOut()
{
  ctrl_sgtl.unmuteLineout();
}

void doNothing()
{
  return;
}

void initialCond()
{
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    // Update the Bounce instance :
    buttons[i].update();
    if (buttons[i].read())
    {
      buttonRoseAction[i]();

    } else {
      buttonFellAction[i]();
    }
  }
}

void updateBounce()
{
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    // Update the Bounce instance :
    buttons[i].update();
    if (buttons[i].fell())buttonFellAction[i]();
    if (buttons[i].rose())buttonRoseAction[i]();
  }
}

void updateLeds()
{
  if ( ! playSdWav.isPlaying())LedPlayOFF();
  for (int i = 0 ; i < NUM_LEDS; i++)
  {
    switch (ledStatus[i])
    {
      case 0:
        digitalWrite(LEDS_PINS[i], false);
        break;
      case 1:
        digitalWrite(LEDS_PINS[i], true);
        break;
      case 2:
        if ( ledTimer[i] > BLINKTIME )
        {
          digitalWrite(LEDS_PINS[i], true);
          ledTimer[i] = 0;
        }
        if ( ledTimer[i] > ( BLINKTIME / 2 ) )
        {
          digitalWrite(LEDS_PINS[i], false);
        }
        break;
      default:
        // if nothing else matches, do the default
        // default is optional
        break;
    }
  }

}

void LedPlayON()
{
  ledStatus[LEDPLAY] = 1;
}

void LedPlayOFF()
{
  ledStatus[LEDPLAY] = 0;
}

void LedPlayBLINK()
{
  ledStatus[LEDPLAY] = 2;
}

void LedMicON()
{
  ledStatus[LEDMIC] = 1;
}

void LedMicOFF()
{
  ledStatus[LEDMIC] = 0;
}

void LedMicBLINK()
{
  ledStatus[LEDMIC] = 2;
}

/*******************************************************/
//LCD stuff
void displayLCD()
{
  //first line
  if (playSdWav.isPlaying())
  {
    displayTimer();
    //displayInfo();
  } else {
    if ( flag_readyToPlay )
    {
      displayHome();
      //Serial1.print("Pret...             ");
      Serial1.print("Pret.. presser jouer");
      //displayInfo();
    } else {
      displayBoot();
      //displayInfo();
    }
  }

  //second line
  displaySetLine(1);
  displayInfo();
}

void displayBoot()
{
  //displayClear();
  displayHome();
  Serial1.print("Imperial-Trans-Kairo");
  //displaySetLine(1);
  //Serial1.print("********************");


}

void displayTimer()
{
  long elapsed;
  long tot;
  if (playSdWav.isPlaying())
  {
    elapsed = playSdWav.positionMillis() / 1000;
    tot = playSdWav.lengthMillis() / 1000;
    Serial1.printf("%02dm:%02ds/%02dm:%02ds     ", numberOfMinutes(elapsed), numberOfSeconds(elapsed), numberOfMinutes(tot), numberOfSeconds(tot));
  }

}

void displayBatt()
{
  int rawbattLevel;
  int  battLevel;

  rawbattLevel = constrain(analogRead(BATT_PIN), 565, 580);
  rawbattLevel = map(rawbattLevel, 565, 580, 0, 9);

  battLevel = int(getLPFSignalInput((float)rawbattLevel));

  //displaySetLine(1);
  if ( battLevel == 0 )
  {
    Serial1.print("BATT:!");

  }
  else
  {
    Serial1.print("Batt:");
    //Serial1.print(analogRead(BATT_PIN));
    
    Serial1.write(battLevel - 1);
  }


}

void displayMicLvl()
{
  int  micLevel;
  if ( ! mic_peak.available() )return;

  micLevel = int(getLPF(&SmoothMicLevel, mic_peak.read(), 0.9) * 9);

  Serial1.print("Mic:");

  if ( micLevel == 0 )
  {
    Serial1.print(" ");
    return;
  }

  if ( micLevel == 9 )
  {
    Serial1.print("#");
    return;
  }

  Serial1.write(micLevel - 1);

}

void displayInfo()
{
  displaySetLine(1);

  if ( flagPrioAnnonces )
  {
    refresh_Info = 0;
    displayInfoAnnonces();
    flagPrioAnnonces = false;
  }
  else if (flagPrioExt)
  {
    refresh_Info = 0;
    displayInfoExt();
    flagPrioExt = false;
  }

  if ( false && refresh_Info >= (REFRESH_INFO) && refresh_Info < (REFRESH_INFO * 2))
  {
    if ( ! displayInfoExt() )refresh_Info = REFRESH_INFO * 2;
  }
  else if ( false && refresh_Info >= (REFRESH_INFO * 2 ) && refresh_Info < (REFRESH_INFO * 3) )
  {
    if ( ! displayInfoAnnonces() )refresh_Info = REFRESH_INFO * 3;
  }
  else if (refresh_Info >= (REFRESH_INFO) )
  {
    displayInfoWagons();
  }

  if (refresh_Info >= (REFRESH_INFO * 4) )refresh_Info = REFRESH_INFO;
}

void displayInfoWagons()
{

  displayBatt();
  Serial1.print("  ");
  displayMicLvl();

  if (! flag_readyToPlay)
  {
    Serial1.print(" [PROM]");
  } else {
    Serial1.print(" [    ]");
  }

  Serial1.flush();
}

bool displayInfoAnnonces()
{
  if ( AnnoncesLCD[0] == '\0' || AnnoncesLCD[20] != '\0' ) //bad string
  {
    return false;
  }

  Serial1.print(AnnoncesLCD);
  Serial1.flush();
  return true;

}

bool displayInfoExt()
{
  if ( ExtLCD[0] == '\0' || ExtLCD[20] != '\0') //bad string
  {
    return false;
  }

  Serial1.print(ExtLCD);
  Serial1.flush();
  return true;
}

void  updateLCD()
{
  if ( refresh_lcd < REFRESHLCD) return;

  refresh_lcd = 0;

  displayHome();
  displayLCD();
}

void updateAll()
{
  updateBounce();
  updateLeds();
  updateLCD();
  serialRXannonces();
  serialRXext();
}

float getLPFSignalInput(float RawData)
{
  // LPF: Y(n) = (1-ß)*Y(n-1) + (ß*X(n))) = Y(n-1) - (ß*(Y(n-1)-X(n)));
  SmoothData = SmoothData - (LPF_Beta * (SmoothData - RawData));
  return float(SmoothData);
}

float getLPF( float *avgLPF , float rawData, float LPF_Coef)
{
  *avgLPF = *avgLPF - (LPF_Coef * ( *avgLPF - rawData));
  return float(*avgLPF);
}

/*
  void serialEvent1()
  {
  serialRXannonces();
  }

  void serialEvent3()
  {
  serialRXext();
  }
*/


void serialRXannonces()
{
  if ( ! Serial1.available() )return;

  char inChar;

  while (Serial1.available() > 0)
  {
    inChar = (char)Serial1.read();   // Read single available character, there may be more waiting

    if ( inChar == '\r' || inChar == '\n' )
    {
      snprintf ( AnnoncesLCD, 21, "%s                    ",  AnnoncesLCD);
       
      flagPrioAnnonces = true;

      AnnoncesLCDindex = 0;

    } else {

      AnnoncesLCD[AnnoncesLCDindex] = inChar;
      AnnoncesLCDindex++;
      AnnoncesLCD[AnnoncesLCDindex] = '\0';

      if ( AnnoncesLCDindex >= 21 ) //buffer underrun
      {
        AnnoncesLCDindex = 0;
      }
    }


  }
  //flagAnnoncesAvail=true;
  //flagPrioAnnonces=true;
  //Serial1.println(AnnoncesLCD);

}

void serialRXext()
{
  if ( ! Serial3.available() )return;

  char inChar;

  //  ExtLCD[0]='\0';
  //   ExtLCDindex=0;

  while (Serial3.available() > 0)
  {

    inChar = (char)Serial3.read();   // Read single available character, there may be more waiting

    if ( inChar == '\r' || inChar == '\n' )
    {

      snprintf ( ExtLCD, 21, "%s                    ",  ExtLCD);
     
      flagPrioExt = true;

      ExtLCDindex = 0;

    } else {
      ExtLCD[ExtLCDindex] = inChar;
      ExtLCDindex++;
      ExtLCD[ExtLCDindex] = '\0';

      if ( ExtLCDindex >= 20 )
      {
        ExtLCDindex = 0;
      }
    }
  }


}

void loop()
{
  updateAll();
}
