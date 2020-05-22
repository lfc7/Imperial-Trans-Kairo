/* Trans Kairo
 * teensy 3.6 + SGTL5000 (audio chip)
 *  
 *  play "JINGLE%d.WAV" on sdcard ( // filenames are always uppercase 8.3 format )
 *  
 *  
 *  
 *  
 */

#define DUCK_VOL  0.10   // 0.0 to 1.0
#define JINGLE_VOL 0.90
#define BLINKTIME 800 // led blink speed
#define BOUNCE_MS 50 //buttons debounce time in ms
#define MAXWAV 16 //max wav 
#define REFRESHAUXCMD 100 //ms
#define FILENAMEFORMAT  "JINGLE%d.WAV" // 8.1 upper name ; where %d is 1-4 5-8 9-12
#define WAVLASTSECOND   5 //led blink N seconds before end of sample
#define LED_ERROR_TIME  3000 //led blink fast N milliseconds on error

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce2.h>

// GUItool: begin automatically generated code
AudioInputI2S            input;           //xy=102,188
AudioPlaySdWav           playSdWav;     //xy=104.75,320.75
AudioEffectFade          wavFaderL;          //xy=270.75,304.75
AudioEffectFade          wavFaderR;          //xy=271.75,338.75
AudioMixer4              mixerR;         //xy=502.75,295.75
AudioMixer4              mixerL;         //xy=504.75,202.75
AudioOutputI2S           output;           //xy=703.75,250.75
AudioConnection          patchCord1(input, 0, mixerL, 0);
AudioConnection          patchCord2(input, 1, mixerR, 0);
AudioConnection          patchCord3(playSdWav, 0, wavFaderL, 0);
AudioConnection          patchCord4(playSdWav, 1, wavFaderR, 0);
AudioConnection          patchCord5(wavFaderL, 0, mixerL, 2);
AudioConnection          patchCord6(wavFaderR, 0, mixerR, 2);
AudioConnection          patchCord7(mixerR, 0, output, 1);
AudioConnection          patchCord8(mixerL, 0, output, 0);
AudioControlSGTL5000     ctrl_sgtl;     //xy=499.75,365.75
// GUItool: end automatically generated code

// Use these with the Teensy 3.5 & 3.6 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

#define UNMUTEWAG_PIN     26
#define UNMUTEEXT_PIN     25
#define TALKOVER_PIN      24
#define JINGLE1_PIN       28
#define JINGLE2_PIN       29
#define JINGLE3_PIN       30
#define JINGLE4_PIN       31
#define STOP_PIN          27
//#define SELECTOR_PIN      21
#define AUXGAIN_PIN       34  //opt
#define AUXTREBLE_PIN     35  //opt
#define AUXBASS_PIN       36  //opt

#define LEDJINGLE1_PIN       33
#define LEDJINGLE2_PIN       37
#define LEDJINGLE3_PIN       38
#define LEDJINGLE4_PIN       39
#define LEDEXT_PIN           32   //opt
  
//debounce & button action **********************************************************
#define NUM_BUTTONS 8
const uint8_t BUTTONS_PINS[NUM_BUTTONS] = {JINGLE1_PIN, JINGLE2_PIN, JINGLE3_PIN, JINGLE4_PIN, UNMUTEEXT_PIN, TALKOVER_PIN, STOP_PIN, UNMUTEWAG_PIN};
Bounce * buttons = new Bounce[NUM_BUTTONS];

// fell actions
void  playJingle1();
void  playJingle2();
void  playJingle3();
void  playJingle4();
void  unMuteExt();
void  unMuteWag();
void  duckWav();
void  stopAudio();

typedef void (*buttonFellActionList[])();
buttonFellActionList buttonFellAction = { playJingle1, playJingle2, playJingle3, playJingle4, unMuteExt, duckWav, stopAudio, unMuteWag };

//rise actions
void  muteExt();
void  muteWag();
void  restoreWav();
void  pauseAudio();
void  readyToPlay();
void  doNothing();

typedef void (*buttonRoseActionList[])();
buttonRoseActionList buttonRoseAction = { doNothing, doNothing, doNothing, doNothing, muteExt, restoreWav, readyToPlay, muteWag };

// led declarations ******************************************************************
#define NUM_LEDS  5
#define LEDJINGLE1   0
#define LEDJINGLE2   1
#define LEDJINGLE3   2
#define LEDJINGLE4   3
#define LEDEXT       4

const uint8_t LEDS_PINS[NUM_LEDS] = {LEDJINGLE1_PIN, LEDJINGLE2_PIN,LEDJINGLE3_PIN,LEDJINGLE4_PIN, LEDEXT_PIN};
uint8_t       ledStatus[NUM_LEDS] ={0,0,0,0,0}; // 0 off, 1 on, 2 blink
elapsedMillis ledTimer[NUM_LEDS];


//glob
elapsedMillis refreshAuxCmd;

boolean flag_unMuteExt=false;
boolean flag_readyToPlay=false;
boolean flag_newWavSelector=false;
boolean flag_pause=false;

int wavSelector;

float auxBass=0.0;
float auxTreble=0.0;

//wav filename
const size_t bufferLen = 13; 
char filename[bufferLen];

int selectedWav=1;
int selectedBank=0;

/***********************************************************************************************************************************/

void setup() 
{
	Serial1.begin(9600);

	// Audio connections require memory to work.  For more
	// detailed information, see the MemoryAndCpuUsage example
	AudioMemory(24);
  delay(250);
  
	ctrl_sgtl.enable();
	ctrl_sgtl.muteHeadphone();
	ctrl_sgtl.muteLineout();
 
	ctrl_sgtl.volume(0.8,0.8);
	ctrl_sgtl.inputSelect(AUDIO_INPUT_LINEIN);
	ctrl_sgtl.lineInLevel(0, 0); //lineInLevel(left, right)  0 - 15 default 5
	ctrl_sgtl.lineOutLevel(14); //lineOutLevel(both) 13-31 default 29;

	
	ctrl_sgtl.audioPostProcessorEnable();
	ctrl_sgtl.autoVolumeControl(0, 0, 0, -18.0, 48.0, 12.0);//autoVolumeControl(maxGain, response, hardLimit, threshold, attack, decay);
	ctrl_sgtl.autoVolumeEnable();
	
	ctrl_sgtl.eqSelect(2); //tone control
	ctrl_sgtl.eqBands(0.0, 0.0); //115Hz, 330Hz, 990Hz, 3kHz, and 9.9kHz. Each band has a range of adjustment from 1.00 (+12dB) to -1.00 (-11.75dB). 
  
  //ctrl_sgtl.adcHighPassFilterDisable();


  mixerL.gain(0,0.00);
  mixerL.gain(1,0.00);
  mixerL.gain(2,JINGLE_VOL);
  mixerL.gain(3,0.00);
  
  mixerR.gain(0,0.00);
  mixerR.gain(1,0.00);
  mixerR.gain(2,JINGLE_VOL);
  mixerR.gain(3,0.00);

	// Setup the first button with an internal pull-up :
  pinMode(JINGLE1_PIN,INPUT_PULLUP);
  buttons[0].attach(JINGLE1_PIN);
  buttons[0].interval(BOUNCE_MS); // interval in ms

  pinMode(JINGLE2_PIN,INPUT_PULLUP);
  buttons[1].attach(JINGLE2_PIN);
  buttons[1].interval(BOUNCE_MS); // interval in ms

  pinMode(JINGLE3_PIN,INPUT_PULLUP);
  buttons[2].attach(JINGLE3_PIN);
  buttons[2].interval(BOUNCE_MS); // interval in ms

  pinMode(JINGLE4_PIN,INPUT_PULLUP);
  buttons[3].attach(JINGLE4_PIN);
  buttons[3].interval(BOUNCE_MS); // interval in ms

	pinMode(UNMUTEEXT_PIN,INPUT_PULLUP);
	buttons[4].attach(UNMUTEEXT_PIN);
	buttons[4].interval(BOUNCE_MS); // interval in ms

	pinMode(TALKOVER_PIN,INPUT);
	buttons[5].attach(TALKOVER_PIN);
	buttons[5].interval(BOUNCE_MS); // interval in ms

	pinMode(STOP_PIN,INPUT_PULLUP);
	buttons[6].attach(STOP_PIN);
	buttons[6].interval(BOUNCE_MS); // interval in ms

  pinMode(UNMUTEWAG_PIN,INPUT_PULLUP);
  buttons[7].attach(UNMUTEWAG_PIN);
  buttons[7].interval(BOUNCE_MS); // interval in ms

  for(int i=0; i < NUM_LEDS; i++)
  {
    pinMode(LEDS_PINS[i],OUTPUT);
  }

	//SPI.setMOSI(SDCARD_MOSI_PIN);
	//SPI.setSCK(SDCARD_SCK_PIN);
 
	if (!(SD.begin(SDCARD_CS_PIN))) 
	{
	  // stop here, but print a message repetitively
		LedError(true); //forever
	}
	
  muteAux();
  initialCond();

  ctrl_sgtl.unmuteHeadphone();
  ctrl_sgtl.unmuteLineout();
}

void doNothing()
{
	return;
}

void playJingle1()
{
	//playJingles( 1 + (selectedBank * 4));
  if( ! buttons[6].read() )
  {
    selectBank(0);
  }
  else
  {
    playJingles( 1 + (selectedBank * 4));
  }
  
}

void playJingle2()
{
	if( ! buttons[6].read() )
	{
	  selectBank(1);
	}
	else
	{
	  playJingles( 2 + (selectedBank * 4));
	}
}

void playJingle3()
{
	if( ! buttons[6].read())
	{
	  selectBank(2);
	}
	else
	{
    playJingles( 3 + (selectedBank * 4));
	}
	
}

void playJingle4()
{
	if( ! buttons[6].read())
	{
	  selectBank(3);
	}
	else
  {
	  playJingles( 4 + (selectedBank * 4) );
  }
}

void playJingles( int nb )
{
	if( ! flag_readyToPlay )return;
 
	if( playSdWav.isPlaying() )
  {
    if( nb == selectedWav )
    {
       if( ! flag_pause )
          {
            wavFaderL.fadeOut(50);
            wavFaderR.fadeOut(50);
            playSdWav.pause();
            flag_pause=true;
            LedPlayBLINK(( (nb - (selectedBank * 4) )  - 1 ) );
            return;
          }else{
            playSdWav.resume();
            wavFaderL.fadeIn(50);
            wavFaderR.fadeIn(50);
            flag_pause=false;
            LedPlayON(( (nb - (selectedBank * 4) )  - 1 ) );
            return;
          } 
    }
    else
  	{
  		playSdWav.resume();
  		// new file stop gently the previous
  		wavFaderL.fadeOut(100);
  		wavFaderR.fadeOut(100);
  		playSdWav.stop();
  		LedPlayOFF();
  		
  	}
  }
  LedPlayOFF();
	selectWav(nb);
	LedPlayON( ( (nb - (selectedBank * 4) )  - 1 ) );
	playAudio();
}

void selectBank(int bank)
{
   // elapsedMillis ledtimer = 0;
    selectedBank=bank;

    /*
    LedPlayBLINK(  );
    
    while( ledtimer < 1000 )
    {
      updateLeds();
    }
    
    ledtimer = 0;
    
    LedPlayOFF();
    LedPlayBLINK( bank + 1 ); 
    
    while( ledtimer < 1000 )
    {    
      updateLeds();
    }
    */

    Serial1.print("Annonces: banque ");
    Serial1.println(selectedBank + 1);
    Serial1.flush();

  }
    
void playAudio()
{
	// Start playing the file.  This sketch continues to
	// run while the file plays.
	if( ! playSdWav.play(filename) ) // filenames are always uppercase 8.3 format
  {
    stopAudio();
    LedError();
  }
  else
  {
      // A brief delay for the library read WAV info
    delay(5);
  
    wavFaderL.fadeIn(50);
    wavFaderR.fadeIn(50);
  }
	
}

void stopAudio()
{
	playSdWav.resume();
	playSdWav.stop();
	flag_readyToPlay=false;
  flag_pause=false;
	LedPlayOFF();
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
	//flag_unMuteAux=false;
}

void unMuteAux()
{
	mixerL.gain(0,1.0);
	mixerR.gain(0,1.0);
	//flag_unMuteAux=true;
}

void duckWav() //duck in the mixer (micro board) now
{
	//mixerL.gain(2,DUCK_VOL);
	//mixerR.gain(2,DUCK_VOL);
  return;
}

void restoreWav()
{
	mixerL.gain(2,JINGLE_VOL);
	mixerR.gain(2,JINGLE_VOL);

}

void muteExt()
{
	ctrl_sgtl.muteLineout();
  LedExtOFF();
}

void unMuteExt()
{
	ctrl_sgtl.unmuteLineout();
  LedExtON();
}

void muteWag()
{
  ctrl_sgtl.muteHeadphone();
}

void unMuteWag()
{
  ctrl_sgtl.unmuteHeadphone();
}

void initialCond()
{
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
}

void updateBounce()
{
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		// Update the Bounce instance :
		buttons[i].update();
		if(buttons[i].fell())buttonFellAction[i]();
		if(buttons[i].rose())buttonRoseAction[i]();
	}
	
}

void updateLeds()
{
	if( ! playSdWav.isPlaying() )
	{
		LedPlayOFF();
	}
  for(int i=0; i < NUM_LEDS; i++)
  {	
  	switch (ledStatus[i]) 
  	{
    case 0:
      digitalWrite(LEDS_PINS[i],false);
      break;
    case 1:
      digitalWrite(LEDS_PINS[i],true);
      break;
    case 2:
      if( ledTimer[i] > BLINKTIME )
      {
        digitalWrite(LEDS_PINS[i],true);
        ledTimer[i]=0;
      }
      if( ledTimer[i] > ( BLINKTIME / 2 ) )
      {
        digitalWrite(LEDS_PINS[i],false);
      }
      break;
    default:
      // if nothing else matches, do the default
      // default is optional
      break;
    }
  }
  
}

void LedPlayON( int nb )
{
  if( nb < 4 )ledStatus[nb]=1;
}

void LedPlayON()
{
  ledStatus[LEDJINGLE1]=1;
  ledStatus[LEDJINGLE2]=1;
  ledStatus[LEDJINGLE3]=1;
  ledStatus[LEDJINGLE4]=1;
}

void LedPlayOFF()
{
  ledStatus[LEDJINGLE1]=0;
  ledStatus[LEDJINGLE2]=0;
  ledStatus[LEDJINGLE3]=0;
  ledStatus[LEDJINGLE4]=0;
}

void LedPlayOFF( int nb )
{
  if( nb < 4 )ledStatus[nb]=0;
}

void LedPlayBLINK()
{
  ledStatus[LEDJINGLE1]=2;
  ledStatus[LEDJINGLE2]=2;
  ledStatus[LEDJINGLE3]=2;
  ledStatus[LEDJINGLE4]=2;
}

void LedPlayBLINK( int nb )
{
  if( nb < 4 )ledStatus[nb]=2;
}

void LedExtON()
{
  ledStatus[LEDEXT]=1;
}

void LedExtOFF()
{
  ledStatus[LEDEXT]=0;
}

void LedError()
{
  LedError( false );
}

void LedError( bool forever )
{
  elapsedMillis lederrortime;
  
  while ( ( lederrortime < LED_ERROR_TIME ) || forever) 
  {
      //Serial.println("Error");
      digitalWrite(LEDS_PINS[0],( ! digitalRead(LEDS_PINS[0]) ) );
      digitalWrite(LEDS_PINS[1],( ! digitalRead(LEDS_PINS[1]) ) );
      digitalWrite(LEDS_PINS[2],( ! digitalRead(LEDS_PINS[2]) ) );
      digitalWrite(LEDS_PINS[3],( ! digitalRead(LEDS_PINS[3]) ) );
      delay(250);
   }
   
}

void selectWav(int filenb)
{
  if(filenb > MAXWAV)filenb=MAXWAV;
  if(filenb == 0)filenb=1;
  selectedWav=filenb;
  sprintf(filename, FILENAMEFORMAT, filenb);
}

void wavChkLastSeconds()
{
	long elapsed; 
	long tot;
	if(playSdWav.isPlaying())
	 {
		//LedPlayON();
		elapsed=playSdWav.positionMillis() / 1000;
		tot=playSdWav.lengthMillis() / 1000;
		if( tot - elapsed <= WAVLASTSECOND )LedPlayBLINK(selectedWav - 1);
	 }else{
		LedPlayOFF();
	}
}

void auxInGain()
{
  int auxGain=map(analogRead(AUXGAIN_PIN),0,1023,0,15);
  ctrl_sgtl.lineInLevel(auxGain, auxGain); //lineInLevel(left, right)  0 - 15 default 5
}

void auxInTone()
{
  float newAuxBass=mapf((float)analogRead(AUXBASS_PIN),0.0,1023.0,-1.0,1.0);
  float newAuxTreble=mapf((float)analogRead(AUXTREBLE_PIN),0.0,1023.0,-1.0,1.0);
  
  if( isLimit( newAuxBass, auxBass ) && ( isLimit( newAuxTreble, auxTreble ) ) ) return;
  
  if( ! isLimit( newAuxBass, auxBass ) )
  {
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
  }
  
  if( ! isLimit( newAuxTreble, auxTreble ) )
  {
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
  //auxInUpdate();
}

boolean isEqual(float f1, float f2)
{
 return ( (int)(f1 *100)) == ((int)(f2 * 100) );
}

boolean isLimit(float f1, float f2)
{
  int diff;
  diff=((int)(f1 *100)) - ((int)(f2 * 100)) ;
  if( (diff >= 0 && diff <= 4 ) || (diff < 0 && diff < -4)  )return true;
  return false;
}

float mapf(float value, float istart, float istop, float ostart, float ostop) {
return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

void loop() 
{
  updateAll();
}
 



