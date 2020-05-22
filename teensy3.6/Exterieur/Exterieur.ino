/* Trans Kairo
 * teensy 3.6 + sgtl_500 (audio chip)
 *  
 *  play "EXT%d.WAV" on sdcard ( // filenames are always uppercase 8.3 format )
 *  
 *  
 *  
 *  
 */

#define DUCK_VOL  0.10   // 0.0 to 1.0
#define WAV_VOL 0.90
#define AUX_VOL 0.80
#define BLINKTIME 800 // led blink speed
#define BOUNCE_MS 50 //buttons debounce time in ms

#define MAXWAV 12 //max wav than selector can choose

#define REFRESHAUXCMD 50 //ms
#define FILENAMEFORMAT  "EXT%d.WAV" // 8.1 upper name ; where %d is 1-12
#define TXT_FILENAME "INDEX.TXT"

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce2.h>

// GUItool: begin automatically generated code
AudioInputI2S            input;          //xy=137.75,105
AudioPlaySdWav           playSdWav;      //xy=139.75,237
AudioEffectFade          wavFaderL;      //xy=305.75,221
AudioEffectFade          wavFaderR;      //xy=306.75,255
AudioMixer4              mixerR;         //xy=537.75,212
AudioSynthWaveformDc     ducklvl;            //xy=537.75,302.75
AudioMixer4              mixerL;         //xy=539.75,119
AudioEffectMultiply      vcaR;      //xy=723.75,216.75
AudioEffectMultiply      vcaL;      //xy=725.75,124.75
AudioOutputI2S           output;         //xy=898.75,169
AudioConnection          patchCord1(input, 0, mixerL, 0);
AudioConnection          patchCord2(input, 1, mixerR, 0);
AudioConnection          patchCord3(playSdWav, 0, wavFaderL, 0);
AudioConnection          patchCord4(playSdWav, 1, wavFaderR, 0);
AudioConnection          patchCord5(wavFaderL, 0, mixerL, 2);
AudioConnection          patchCord6(wavFaderR, 0, mixerR, 2);
AudioConnection          patchCord7(mixerR, 0, vcaR, 0);
AudioConnection          patchCord8(ducklvl, 0, vcaR, 1);
AudioConnection          patchCord9(ducklvl, 0, vcaL, 1);
AudioConnection          patchCord10(mixerL, 0, vcaL, 0);
AudioConnection          patchCord11(vcaR, 0, output, 1);
AudioConnection          patchCord12(vcaL, 0, output, 0);
AudioControlSGTL5000     ctrl_sgtl;      //xy=528.75,444
// GUItool: end automatically generated code


// Use these with the Teensy 3.5 & 3.6 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used


#define UNMUTEAUX_PIN     25
#define TALKOVER_PIN      24
#define STARTPAUSE_PIN    26
#define STOP_PIN          27
#define SELECTOR_PIN      33
#define AUXGAIN_PIN       34
#define AUXTREBLE_PIN     35
#define AUXBASS_PIN       36

#define LEDPLAY_PIN       30

//debounce
#define NUM_BUTTONS 4
const uint8_t BUTTONS_PINS[NUM_BUTTONS] = {UNMUTEAUX_PIN, TALKOVER_PIN, STARTPAUSE_PIN, STOP_PIN};
Bounce * buttons = new Bounce[NUM_BUTTONS];

void  unMuteAux();
void  duckWav();
void  playAudio();
void  stopAudio();

typedef void (*buttonFellActionList[])();
buttonFellActionList buttonFellAction = { unMuteAux, duckWav, playAudio, stopAudio };

void  muteAux();
void  restoreWav();
void  pauseAudio();
void  readyToPlay();
void  doNothing();

typedef void (*buttonRoseActionList[])();
buttonRoseActionList buttonRoseAction = { muteAux, restoreWav, doNothing, readyToPlay };

/*
typedef void (*buttonReadActionList[])();
buttonReadActionList buttonReadAction = { ctrlMic, doNothing, ctrlPFL, doNothing, doNothing, doNothing };
*/

const uint8_t LEDS_PINS = LEDPLAY_PIN;
uint8_t       ledStatus =0; // 0 off, 1 on, 2 blink
elapsedMillis ledTimer;
elapsedMillis refreshAuxCmd;

boolean flag_unMuteAux=false;
boolean flag_readyToPlay=false;
boolean flag_newWavSelector=false;
boolean flag_pause=false;

int wavSelector;

float auxBass=0.0;
float auxTreble=0.0;

//wav filename
const size_t bufferLen = 13; 
char filename[bufferLen];

//wavname listing
const size_t LCD_Len = 21;
char wavname[MAXWAV][LCD_Len];


/***********************************************************************************************************************************/

void setup() {
	Serial1.begin(9600);

	// Audio connections require memory to work.  For more
	// detailed information, see the MemoryAndCpuUsage example
	AudioMemory(16);
  delay(250);
  
	ctrl_sgtl.enable();
  delay(10);
  
  ctrl_sgtl.muteHeadphone();
  ctrl_sgtl.muteLineout();
 
	ctrl_sgtl.volume(0.8,0.8);
	ctrl_sgtl.inputSelect(AUDIO_INPUT_LINEIN);
	ctrl_sgtl.lineInLevel(5, 5); //lineInLevel(left, right)  0 - 15 default 5
	ctrl_sgtl.lineOutLevel(14); //lineOutLevel(both) 13-31 default 29;

	ctrl_sgtl.audioPostProcessorEnable();
	ctrl_sgtl.autoVolumeControl(0, 0, 0, -6.0, 48.0, 12.0);//autoVolumeControl(maxGain, response, hardLimit, threshold, attack, decay);
	ctrl_sgtl.autoVolumeEnable();
	ctrl_sgtl.eqSelect(3);
	ctrl_sgtl.eqBands(0.0, 0.0, 0.0, 0.0, -0.5); //115Hz, 330Hz, 990Hz, 3kHz, and 9.9kHz. Each band has a range of adjustment from 1.00 (+12dB) to -1.00 (-11.75dB). 

//  ctrl_sgtl.adcHighPassFilterDisable();
   
  mixerL.gain(0,AUX_VOL); //aux in
  mixerL.gain(1,0.00);
  mixerL.gain(2,WAV_VOL); //wav
  mixerL.gain(3,0.00);
  
  mixerR.gain(0,AUX_VOL);
  mixerR.gain(1,0.00);
  mixerR.gain(2,WAV_VOL);
  mixerR.gain(3,0.00);

  pinMode(LEDPLAY_PIN, OUTPUT);
	// Setup the first button with an internal pull-up :
	pinMode(UNMUTEAUX_PIN,INPUT_PULLUP);
  
	// After setting up the button, setup the Bounce instance :
	buttons[0].attach(UNMUTEAUX_PIN);
	buttons[0].interval(BOUNCE_MS); // interval in ms

	pinMode(TALKOVER_PIN,INPUT);
	buttons[1].attach(TALKOVER_PIN);
	buttons[1].interval(BOUNCE_MS); // interval in ms

	pinMode(STARTPAUSE_PIN,INPUT_PULLUP);
	buttons[2].attach(STARTPAUSE_PIN);
	buttons[2].interval(BOUNCE_MS); // interval in ms

	pinMode(STOP_PIN,INPUT_PULLUP);
	buttons[3].attach(STOP_PIN);
	buttons[3].interval(BOUNCE_MS); // interval in ms

	//SPI.setMOSI(SDCARD_MOSI_PIN);
	//SPI.setSCK(SDCARD_SCK_PIN);
	if (!(SD.begin(SDCARD_CS_PIN))) {
	// stop here, but print a message repetitively
		while (1) {
		  //Serial.println("Unable to access the SD card");
      digitalWrite(LEDS_PINS,( ! digitalRead(LEDS_PINS) ) );
		  delay(250);
		}
	}

  //get wav titles
  parseTxtFile();
  
	//initial selector
	wavSelector= map(analogRead( SELECTOR_PIN ),0,1023,1,MAXWAV);
	selectWav(wavSelector);
 
  //initial conditions
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    // Update the Bounce instance :
    buttons[i].update();
    if(buttons[i].read())
    {
      buttonRoseAction[i]();
      
    }else{
      buttonFellAction[i]();
    }
  }
  ducklvl.amplitude(1.0, 250);

  ctrl_sgtl.unmuteHeadphone();
  ctrl_sgtl.unmuteLineout();
}

void doNothing()
{
  return;
}
/*
void playAudio()
{
  if( ! flag_readyToPlay || playSdWav.isPlaying() )return;
  
	// Start playing the file.  This sketch continues to
	// run while the file plays.
	playSdWav.play(filename); // filenames are always uppercase 8.3 format

	// A brief delay for the library read WAV info
	delay(5);

	// Simply wait for the file to finish playing.
	LedPlayON();
  
	while (playSdWav.isPlaying()) 
	{
		updateBounce();
		wavChkLastSeconds();
		updateLeds();
    auxInUpdate();
		
		if(flag_newWavSelector)
		{
			flag_newWavSelector=false;
			wavFaderL.fadeIn(2000);
			wavFaderR.fadeIn(2000);
			
			playSdWav.play(filename); // filenames are always uppercase 8.3 format
			// A brief delay for the library read WAV info
			delay(5);
			
			wavFaderL.fadeOut(2000);
			wavFaderR.fadeOut(2000);
		}
	}
 
	LedPlayOFF();
}
*/

void playAudio()
{
  //if( ! flag_readyToPlay )return;
  
  if( playSdWav.isPlaying() )
  {
      if( flag_newWavSelector )
      {
        flag_newWavSelector=false;
        wavFaderL.fadeOut(1000);
        wavFaderR.fadeOut(1000);
    
      }else{
        
          if( ! flag_pause )
          {
            wavFaderL.fadeOut(50);
            wavFaderR.fadeOut(50);
            playSdWav.pause();
            flag_pause=true;
            LedPlayBLINK();
            
            Serial1.print(F("Pause: "));
            Serial1.println(wavname[wavSelector-1]);
            return;
          }else{
            playSdWav.resume();
            wavFaderL.fadeIn(50);
            wavFaderR.fadeIn(50);
            flag_pause=false;
            LedPlayON();
            
            Serial1.print(F("Play: "));
            Serial1.println(wavname[wavSelector-1]);
            return;
          }
      }
  }

  stopAudio();
  readyToPlay(); 
  flag_newWavSelector=false; 
  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playSdWav.play(filename); // filenames are always uppercase 8.3 format
  
  // A brief delay for the library read WAV info
  delay(5);
   playSdWav.loop(true);
    
  wavFaderL.fadeIn(500);
  wavFaderR.fadeIn(500);

  Serial1.print(F("Play: "));
  Serial1.println(wavname[wavSelector-1]);
  
  // Simply wait for the file to finish playing.
  muteAux();
  LedPlayON();
  
}

void stopAudio()
{
	wavFaderL.fadeOut(50);
  wavFaderR.fadeOut(50);
  playSdWav.resume();
	playSdWav.stop();
	flag_readyToPlay=false;
  flag_pause=false;
  LedPlayOFF();
  Serial1.print(F("STOP: "));
  Serial1.println(wavname[wavSelector-1]);
  unMuteAux();
}

void readyToPlay()
{
  flag_readyToPlay=true;
  flag_pause=false;
}

void pauseAudio()
{
	return;
}

void muteAux()
{
	mixerL.gain(0,0.0);
	mixerR.gain(0,0.0);
	flag_unMuteAux=false;
}

void unMuteAux()
{
	mixerL.gain(0,AUX_VOL);
	mixerR.gain(0,AUX_VOL);
	flag_unMuteAux=true;
}

void duckWav()
{
	//mixerL.gain(2,DUCK_VOL);
	//mixerR.gain(2,DUCK_VOL);
	//ducklvl.amplitude(DUCK_VOL, 25);
  return;
}

void restoreWav()
{
	//mixerL.gain(2,1.0);
	//mixerR.gain(2,1.0);
	ducklvl.amplitude(1.00, 250);
}

void muteLineOut()
{
	ctrl_sgtl.muteLineout();
}

void UnMuteLineOut()
{
	ctrl_sgtl.unmuteLineout();
}

void updateBounce()
{
	int actualWavSelector;
  
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		// Update the Bounce instance :
		buttons[i].update();
		if(buttons[i].fell())buttonFellAction[i]();
		if(buttons[i].rose())buttonRoseAction[i]();
	}
	
	actualWavSelector= map(analogRead( SELECTOR_PIN ),0,1023,1,MAXWAV);
	if( actualWavSelector != wavSelector )
	{
		flag_newWavSelector=true;
		wavSelector=actualWavSelector;
		selectWav(wavSelector);
   
    Serial1.print(F("Select:"));
    Serial1.println(wavname[wavSelector - 1]);
	}
	
}

void updateLeds()
{
  	switch (ledStatus) 
  	{
    case 0:
      digitalWrite(LEDS_PINS,false);
      break;
    case 1:
      digitalWrite(LEDS_PINS,true);
      break;
    case 2:
      if( ledTimer > BLINKTIME )
      {
        digitalWrite(LEDS_PINS,true);
        ledTimer=0;
      }
      if( ledTimer > ( BLINKTIME / 2 ) )
      {
        digitalWrite(LEDS_PINS,false);
      }
      break;
      
      case 3:
      if( ledTimer > BLINKTIME / 2 )
      {
        digitalWrite(LEDS_PINS,true);
        ledTimer=0;
      }
      if( ledTimer > ( BLINKTIME / 4 ) )
      {
        digitalWrite(LEDS_PINS,false);
      }
      break;
      
    default:
      // if nothing else matches, do the default
      // default is optional
      break;
    }
  
}

void LedPlayON()
{
  ledStatus=1;
}

void LedPlayOFF()
{
  ledStatus=0;
}

void LedPlayBLINK()
{
  ledStatus=2;
}

void LedPlayBLINKFAST()
{
  ledStatus=3;
}

void selectWav(int filenb)
{
  if(filenb > MAXWAV)filenb=MAXWAV;
  sprintf(filename, FILENAMEFORMAT, filenb);
}

void wavChkLastSeconds()
{
	long elapsed; 
	long tot;
	if(playSdWav.isPlaying())
	 {
		elapsed= playSdWav.positionMillis() / 1000;
		tot= playSdWav.lengthMillis() / 1000;
		if( tot - elapsed < 10 )
		{
		  LedPlayBLINKFAST();
		}else{
      if(ledStatus==0)LedPlayON();
		}
		
	 }else{
		LedPlayOFF();
    //if(flag_readyToPlay)stopAudio();
	}
}

void auxInGain()
{
  /*
  int auxGain=map(analogRead(AUXGAIN_PIN),0,1023,0,15);
  ctrl_sgtl.lineInLevel(auxGain, auxGain); //lineInLevel(left, right)  0 - 15 default 5
  */
  
  float digitalGain=mapf( analogRead(AUXGAIN_PIN), 0.0, 1023.0, 0.01, 1.0 );
  mixerL.gain(2,digitalGain);
  mixerR.gain(2,digitalGain);
  //mixerL.gain(0,digitalGain);
  //mixerR.gain(0,digitalGain);
}

void auxInTone()
{
  float newAuxBass=mapf(analogRead(AUXBASS_PIN),0.0,1023.0,-1.0,1.0);
  float newAuxTreble=mapf(analogRead(AUXTREBLE_PIN),0.0,1023.0,-1.0,1.0);
  
  if( isEqual( newAuxBass, auxBass ) && ( isEqual( newAuxTreble, auxTreble ) ) ) return;
  
  if( newAuxBass > auxBass )
  {
    auxBass += 0.04;
    if( auxBass > 1.0 )auxBass=1.0;
  }
  
  if( newAuxBass < auxBass )
  {
    auxBass -= 0.04;
    if( auxBass < -1.0 )auxBass=-1.0;
  }
  
  if( newAuxTreble > auxTreble )
  {
    auxTreble += 0.04;
    if( auxTreble > 1.0 )auxTreble=1.0;
  }
  
  if( newAuxTreble < auxTreble )
  {
    auxTreble -= 0.04;
    if( auxTreble < -1.0 )auxTreble=-1.0;
  } 
   
  ctrl_sgtl.eqBands(auxBass, auxTreble);
}

void  auxInUpdate()
{
  if(refreshAuxCmd < REFRESHAUXCMD ) return;
  refreshAuxCmd=0;
  auxInGain();
  auxInTone();
}

void updateAll()
{
  updateBounce(); 
  wavChkLastSeconds();
  updateLeds(); 
  auxInUpdate();
  
}

boolean isEqual(float f1, float f2){
 return ( (int)(f1 *100)) == ((int)(f2 * 100) );
}

float mapf(float value, float istart, float istop, float ostart, float ostop) {
return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

bool parseTxtFile()
{
  File myFile;
  int ptrLine=0;

 // wavname[MAXWAV][bufferLen];
  char ch;
  uint8_t charIndex = 0;
  
  wavname[ptrLine][0] = '\0';

   // open the file for reading:
  myFile = SD.open(TXT_FILENAME);

  if (myFile)
  {
    Serial1.println("parsing txt file");

    while (myFile.available())
    {
      ch = myFile.read();
      if (ch == '\r' || ch == '\n') // looking for new line
      {
        
        wavname[ptrLine][charIndex] = '\0';
        charIndex = 0;
        ptrLine ++;
        if(ptrLine >= MAXWAV)
        {
          myFile.close();
          return true;
        }else{
          wavname[ptrLine][0] = '\0';
        }
        
      }
      else
      {
        wavname[ptrLine][charIndex] = ch;
        charIndex++;
        if (charIndex >= (LCD_Len - 1))
        {
          charIndex=LCD_Len - 1; //poor overflow handling 
          wavname[ptrLine][charIndex] = '\0';
        }
      }
    }

    // close the file:
    myFile.close();
    return true;

  } else {
    // if the file didn't open, print an error:
    Serial1.println(F("txt file not found"));
    
  }
  return false;
}

void loop() 
{
  updateAll();
}


