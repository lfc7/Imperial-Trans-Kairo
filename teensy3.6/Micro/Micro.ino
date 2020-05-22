/* Trans Kairo
 * teensy 3.6 + 2 * sgtl5000 (audio chip)
 *  
 *  
 *  
 *  
 *  
 *  
 */

#define DUCK_VOL  0.10   // 0.0 to 1.0
#define MIC_VOL   1.0
#define MIX_VOL   0.50


#define BLINKTIME           800 // led blink speed
#define BOUNCE_MS           50 //buttons debounce time in ms
//#define MAXWAV 12 //max wav than selector can choose
#define REFRESHMICCMD       5 //ms

#define MICDUCKTHRES_OFF    0.02 // deactive duck under this lvl ; float between 0.0 & 1.0
#define MICDUCKTHRES_ON     0.03 // active duck under this lvl ; float between 0.0 & 1.0

#define DUCK_ATTACK_TIME    200 // in ms
#define DUCK_RELEASE_TIME   1500 // in ms

#define LPF_COEF_MIC_GAIN   0.1 //smoothing coef 
#define LPF_COEF_MIC_BASS   0.1
#define LPF_COEF_MIC_MID    0.1
#define LPF_COEF_MIC_TREBLE 0.1
#define LPF_COEF_VOLUME_G   0.1

//autoMix
#define LIMIT_OUT_THRESHOLD 0.8
#define LIMIT_OUT_LPFCOEF   0.3

//#include <Audio.h>
#include "Audio.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce2.h>



// GUItool: begin automatically generated code
AudioSynthWaveformDc     cmdVcaMix;      //xy=67,337
AudioInputI2SQuad        inputs;         //xy=76,253
AudioAmplifier           preAmpMic;      //xy=305,189
AudioEffectMultiply      vcaMixR;        //xy=450,323
AudioEffectMultiply      vcaMixL;        //xy=450,358
AudioEffectFade          fadeMic;        //xy=462,228
AudioAnalyzePeak         mic_peak;       //xy=483,139
AudioFilterStateVariable micfilter;        //xy=596.6666666666666,67.77777777777777
AudioMixer4              mixerPromenade; //xy=684,548
AudioMixer4              micfiltermix;         //xy=744.4444580078125,74.44445037841797
AudioEffectFade          fadeDirectMic;  //xy=759,189
AudioMixer4              mixerR;         //xy=761,319
AudioMixer4              mixerL;         //xy=762,248
AudioEffectFade          fadePromenade;  //xy=890,547
AudioAnalyzePeak         outputPeak;     //xy=1090,102
AudioOutputI2SQuad       outputs;        //xy=1100,182
AudioConnection          patchCord1(cmdVcaMix, 0, vcaMixR, 1);
AudioConnection          patchCord2(cmdVcaMix, 0, vcaMixL, 1);
AudioConnection          patchCord3(inputs, 0, preAmpMic, 0);
AudioConnection          patchCord4(inputs, 0, mixerR, 2);
AudioConnection          patchCord5(inputs, 1, mixerL, 2);
AudioConnection          patchCord6(inputs, 2, vcaMixR, 0);
AudioConnection          patchCord7(inputs, 3, vcaMixL, 0);
AudioConnection          patchCord8(preAmpMic, mic_peak);
AudioConnection          patchCord9(preAmpMic, 0, micfilter, 0);
AudioConnection          patchCord10(vcaMixR, 0, mixerL, 1);
AudioConnection          patchCord11(vcaMixR, 0, mixerPromenade, 1);
AudioConnection          patchCord12(vcaMixL, 0, mixerR, 1);
AudioConnection          patchCord13(vcaMixL, 0, mixerPromenade, 2);
AudioConnection          patchCord14(fadeMic, 0, mixerL, 0);
AudioConnection          patchCord15(fadeMic, 0, mixerR, 0);
AudioConnection          patchCord16(fadeMic, 0, mixerPromenade, 0);
AudioConnection          patchCord17(micfilter, 0, micfiltermix, 0);
AudioConnection          patchCord18(micfilter, 1, micfiltermix, 1);
AudioConnection          patchCord19(micfilter, 2, micfiltermix, 2);
AudioConnection          patchCord20(mixerPromenade, fadePromenade);
AudioConnection          patchCord21(micfiltermix, fadeDirectMic);
AudioConnection          patchCord22(micfiltermix, fadeMic);
AudioConnection          patchCord23(fadeDirectMic, 0, outputs, 2);
AudioConnection          patchCord24(mixerR, 0, outputs, 1);
AudioConnection          patchCord25(mixerL, 0, outputs, 0);
AudioConnection          patchCord26(mixerL, outputPeak);
AudioConnection          patchCord27(fadePromenade, 0, outputs, 3);
AudioControlSGTL5000     ctrl_sgtl_1;    //xy=259,62
AudioControlSGTL5000     ctrl_sgtl_2;    //xy=262,106
// GUItool: end automatically generated code

// Use these with the Teensy 3.5 & 3.6 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used


// pin assignations
#define TALKOVER_OUT_PIN  24
#define TALKOVER_IN_PIN   25
#define PROMENADE_PIN     26

#define UNMUTEMIC_PIN     28
#define UNMUTEPFL_PIN     29

#define ERREURCABLAGE     30

#define LEDMIC_PIN        31
#define LEDDUCK_PIN       32
#define MICGAIN_PIN       33
#define MICTREBLE_PIN     34
#define MICMID_PIN        35
#define MICBASS_PIN       36
#define VOLUME_EXT_PIN    37

#define RX_SGTL5000       38


//debounce
#define NUM_BUTTONS 4
const uint8_t BUTTONS_PINS[NUM_BUTTONS] = {UNMUTEMIC_PIN, UNMUTEPFL_PIN, TALKOVER_IN_PIN, PROMENADE_PIN};
Bounce * buttons = new Bounce[NUM_BUTTONS];

//void  unMuteMic();
void  micON();
void  unMutePFL();
void  duckON();
void  unMutePromenade();

typedef void (*buttonFellActionList[])();
buttonFellActionList buttonFellAction = { micON, unMutePFL, duckON, unMutePromenade};

//void  muteMic();
void  micOFF();
void  mutePFL();
void  duckOFF();
void  mutePromenade();

typedef void (*buttonRoseActionList[])();
buttonRoseActionList buttonRoseAction = { micOFF, mutePFL, duckOFF, mutePromenade };

#define NUM_LEDS  2
#define LEDMIC   0
#define LEDDUCK   1

const uint8_t LEDS_PINS[NUM_LEDS] = {LEDMIC_PIN, LEDDUCK_PIN};
uint8_t       ledStatus[NUM_LEDS] ={0,0}; // 0 off, 1 on, 2 blink
elapsedMillis ledTimer[NUM_LEDS];

elapsedMillis refreshMicCmd;

boolean flag_unMuteMic=false;
boolean flag_unMutePFL=false;
boolean flag_duck_ON=false;

float micBass=0.0;
float micMid=0.0;
float micTreble=0.0;

uint16_t last_analogPot;

//autoMix
float automixMicLvl=MIC_VOL;
float automixMixLvl=MIX_VOL;

//LPF
float micBass_LPF=0.0;
float micMid_LPF=0.0;
float micTreble_LPF=0.0;
float micGain_LPF=0.0;
float outputPeak_LPF=0.0;

float VolumeG_LPF=0.0;

float SmoothData;
float LPF_Beta = 0.05; // 0<ß<1 (0.025) 


/***********************************************************************************************************************************/

/********** SETTINGs *************/
void setup() {
  //pinMode(30,INPUT);
  //pinMode(38,INPUT);
  //pinMode(13,INPUT);
  
	//Serial.begin(9600);
  delay(250);
  
	// Audio connections require memory to work.  For more
	// detailed information, see the MemoryAndCpuUsage example
	
  
	ctrl_sgtl_1.setAddress(LOW);

	ctrl_sgtl_1.enable();
  

 //custom conf
  #define CHIP_ANA_POWER     0x0030
  ctrl_sgtl_1.write(CHIP_ANA_POWER, 0x40BB); // mono adc (left) / no capless headphone

  ctrl_sgtl_1.dacVolumeRamp();
  #define CHIP_ADCDAC_CTRL    0x000E
  ctrl_sgtl_1.write(CHIP_ADCDAC_CTRL, 0x0060); //ramp vol / expo
  
  #define CHIP_ANA_HP_CTRL    0x0022
  //ctrl_sgtl_1.adcHighPassFilterDisable(); // Disable(); //Freeze();  
   ctrl_sgtl_1.write(CHIP_ANA_HP_CTRL, 0x1818);
   
  #define CHIP_ANA_CTRL      0x0024
//  ctrl_sgtl_1.write(CHIP_ANA_CTRL, 0x0133);

  #define DAP_BASS_ENHANCE    0x0104
  ctrl_sgtl_1.write(DAP_BASS_ENHANCE, 0x0100); //HPF 100Hz

  #define CHIP_ANA_TEST2      0x003A
 // ctrl_sgtl_1.write(CHIP_ANA_TEST2, 0x0003); //drop ADC bias current ; turn off ADC dithering 
	
	ctrl_sgtl_1.muteHeadphone();
	ctrl_sgtl_1.muteLineout();
	
	ctrl_sgtl_1.volume(0.75,0.75);

	
	ctrl_sgtl_1.lineOutLevel(27); //lineOutLevel(bot,h) 13-31 default 29;
	
	ctrl_sgtl_1.muteHeadphone();
	ctrl_sgtl_1.muteLineout();
	
	ctrl_sgtl_1.audioPreProcessorEnable();
  
	ctrl_sgtl_1.autoVolumeControl(1, 1, 0, -18.0, 16.0, 4.0); //96.0 24.0autoVolumeControl(maxGain, response, hardLimit, threshold, attack, decay);
	ctrl_sgtl_1.autoVolumeEnable();
 
 /*
	ctrl_sgtl_1.eqSelect(3);
	ctrl_sgtl_1.eqBands(-1.0, 0.0, 0.0, 0.0, -0.7); //115Hz, 330Hz, 990Hz, 3kHz, and 9.9kHz. Each band has a range of adjustment from 1.00 (+12dB) to -1.00 (-11.75dB). 
	*/
  
  initMicParaEq();

  ctrl_sgtl_1.inputSelect(AUDIO_INPUT_MIC);
  
  ctrl_sgtl_1.micGain(40); 
  ctrl_sgtl_1.lineInLevel(0, 0); //lineInLevel(left, right)  0 - 15 default 5 //  !!! controled by micGain
  
  #define CHIP_MIC_CTRL      0x002A 
  ctrl_sgtl_1.write(CHIP_MIC_CTRL, 0x0312 ); //312 B1100010011 8k 1.50v 3 = 40db // 0x0003 no bias +40db
  
  
	
	
	//ctrl_sgtl_1.adcHighPassFilterDisable(); //Disable();//Freeze(); //Freeze(); //Disable(); //
	
	//ctrl_sgtl_1.unmuteHeadphone();
	//ctrl_sgtl_1.unmuteLineout();

  // ************ sgtl 2 **************
	
	ctrl_sgtl_2.setAddress(HIGH);

	ctrl_sgtl_2.enable();
	
	ctrl_sgtl_2.muteHeadphone();
	ctrl_sgtl_2.muteLineout();
	
	ctrl_sgtl_2.volume(0.8,0.8);
	ctrl_sgtl_2.inputSelect(AUDIO_INPUT_LINEIN);
	ctrl_sgtl_2.lineInLevel(7, 7); //lineInLevel(left, right)  0 - 15 default 5 //mix
	ctrl_sgtl_2.lineOutLevel(14); //lineOutLevel(both) 13-31 default 29;
	
	ctrl_sgtl_2.audioPostProcessorEnable();
	ctrl_sgtl_2.autoVolumeControl(1, 0, 0, -3.0, 48.0, 24.0);//autoVolumeControl(maxGain, response, hardLimit, threshold, attack, decay);
	ctrl_sgtl_2.autoVolumeEnable();
	ctrl_sgtl_2.eqSelect(3);
	ctrl_sgtl_2.eqBands(0.0, 0.0, 0.0, 0.0, 0.0); //115Hz, 330Hz, 990Hz, 3kHz, and 9.9kHz. Each band has a range of adjustment from 1.00 (+12dB) to -1.00 (-11.75dB). 
	ctrl_sgtl_2.dacVolumeRamp();


  // init pin and buttons
	for(int i=0; i < NUM_BUTTONS; i++)
	{
		// Setup the first button with an internal pull-up :
		pinMode(BUTTONS_PINS[i],INPUT_PULLUP);
		// After setting up the button, setup the Bounce instance :
		buttons[i].attach(BUTTONS_PINS[i]);
		buttons[i].interval(BOUNCE_MS); // interval in ms
	}

  pinMode(LEDMIC_PIN,OUTPUT);
  pinMode(LEDDUCK_PIN,OUTPUT);
  pinMode(TALKOVER_OUT_PIN,OUTPUT);

  //need to put all other pin to output 0

  //initial mix
  AudioMemory(64);
  preAmpMic.gain(1.0);

  micfilter.frequency(1800.0);
  micfilter.resonance(0.707);
  micfiltermix.gain(0,1.0);
  micfiltermix.gain(1,1.0);
  micfiltermix.gain(2,1.0);

  cmdVcaMix.amplitude(1.0);
  
  fadeDirectMic.fadeIn(100);
  
  mixerL.gain(0,MIC_VOL); //mic
  mixerL.gain(1,MIX_VOL); //mixL
  mixerL.gain(2,0.00);    //mic2
  mixerL.gain(3,0.00);
  
  mixerR.gain(0,MIC_VOL); //mic
  mixerR.gain(1,MIX_VOL); //mixR
  mixerR.gain(2,0.00);    // mic2
  mixerR.gain(3,0.00);


  mixerPromenade.gain(0,float(MIC_VOL ));  //mic
  mixerPromenade.gain(1,float(MIX_VOL / 2.0 ));  //mixL
  mixerPromenade.gain(2,float(MIX_VOL / 2.0 ));  //mixR
  fadePromenade.fadeOut(50);
  
  unMuteAllOutputs();
  
  initialCond();
  
}

void doNothing()
{
	return;
}

/****** SOUND *************/

void muteAllOutputs()
{
    ctrl_sgtl_1.muteHeadphone();
    ctrl_sgtl_1.muteLineout();
    
    ctrl_sgtl_2.muteHeadphone();
    ctrl_sgtl_2.muteLineout();
}

void unMuteAllOutputs()
{
    ctrl_sgtl_1.unmuteHeadphone();
    ctrl_sgtl_1.unmuteLineout();
    
    ctrl_sgtl_2.unmuteHeadphone();
    ctrl_sgtl_2.unmuteLineout();
}

void micON()
{
   LedMicON();
   unMuteMic();
}

void micOFF()
{
   LedMicOFF();
   muteMic();
}

void muteMic()
{
  fadeMic.fadeOut(250);
  flag_unMuteMic=false;
  restoreMix();
}

void unMuteMic()
{
	if( ! buttons[1].read() )
	{
		fadeMic.fadeIn(250);
		
	}
	
	flag_unMuteMic=true;
}

void mutePFL()
{
	ctrl_sgtl_1.muteHeadphone();
	flag_unMutePFL=false;
  muteMic();
  //fadeMic.f5adeOut(250);
  //flag_unMuteMic=true;
	//if(flag_unMuteMic)muteMic();
	//LedMicOFF();
}

void unMutePFL()
{
	ctrl_sgtl_1.unmuteHeadphone();
	flag_unMutePFL=true;
	
	if( ! buttons[0].read())unMuteMic();
}

void mutePromenade()
{
  fadePromenade.fadeOut(5000);
  //fadeDirectMic.fadeIn(2500);
}

void unMutePromenade()
{
  fadePromenade.fadeIn(250);
  //fadeDirectMic.fadeOut(250);
}

void micInGain()
{
  uint16_t analogPot=getLPF( &micGain_LPF , analogRead(MICGAIN_PIN), LPF_COEF_MIC_GAIN );
  //int micGain=map(analogRead(MICGAIN_PIN),0,1023,0,15);
  //ctrl_sgtl_1.lineInLevel(micGain, micGain); //lineInLevel(left, right)  0 - 15 default 5
  if(analogPot == last_analogPot)return;
  
  last_analogPot=analogPot;
  if( analogPot >= 512 )
  {
    preAmpMic.gain(mapf( analogPot, 512.0, 1023.0, 1.0, 4.0 ));
  }else{
    preAmpMic.gain(mapf( analogPot, 0.0, 512.0, 0.25, 1.0 ));
  }
  
}

/*
void micInTone()
{
  float newMicBass=getLPF( &micBass_LPF , mapf(analogRead(MICBASS_PIN),0.0,1023.0,-1.0,1.0), LPF_COEF_MIC_BASS );
  float newMicMid=getLPF( &micMid_LPF , mapf(analogRead(MICMID_PIN),0.0,1023.0,-1.0,1.0), LPF_COEF_MIC_MID );
  float newMicTreble=getLPF( &micTreble_LPF , mapf(analogRead(MICTREBLE_PIN),0.0,1023.0,-1.0,1.0), LPF_COEF_MIC_TREBLE );

  if( isLimit( newMicBass, micBass ) && ( isLimit( newMicTreble, micTreble ) ) && ( isLimit( newMicMid, micMid ) )) return;

  if( ! isLimit( newMicBass, micBass ) )
  {
    if( newMicBass > micBass )
    {
      micBass += 0.04;
      if( micBass > 1.0 )micBass=1.0;
    }
    
    if( newMicBass < micBass )
    {
      micBass -= 0.04;
      if( micBass < -1.0 )micBass=-1.0;
    }
  }

  if( ! isLimit( newMicMid, micMid ) )
  {
    if( newMicMid > micMid )
    {
      micMid += 0.04;
      if( micMid > 1.0 )micMid=1.0;
    }
    
    if( newMicMid < micMid )
    {
      micMid -= 0.04;
      if( micMid < -1.0 )micMid=-1.0;
    }
  }

  if( ! isLimit( newMicTreble, micTreble ) )
  {
      if( newMicTreble > micTreble )
      {
        micTreble += 0.04;
        if( micTreble > 1.0 )micTreble=1.0;
      }
      
      if( newMicTreble < micTreble )
      {
        micTreble -= 0.04;
        if( micTreble < -1.0 )micTreble=-1.0;
      } 
  }

  ctrl_sgtl_1.eqBands(-1.00,micBass, micMid ,micTreble,-0.7);
}
*/

void micInTone()
{
  float newMicBass=getLPF( &micBass_LPF , mapf(analogRead(MICBASS_PIN),0.0,1023.0,0.5,2.0), LPF_COEF_MIC_BASS );
  float newMicMid=getLPF( &micMid_LPF , mapf(analogRead(MICMID_PIN),0.0,1023.0,0.5,2.0), LPF_COEF_MIC_MID );
  float newMicTreble=getLPF( &micTreble_LPF , mapf(analogRead(MICTREBLE_PIN),0.0,1023.0,0.5,2.0), LPF_COEF_MIC_TREBLE );

  if( isLimit( newMicBass, micBass ) && ( isLimit( newMicTreble, micTreble ) ) && ( isLimit( newMicMid, micMid ) )) return;

  if( ! isLimit( newMicBass, micBass ) )
  {
    if( newMicBass > micBass )
    {
      micBass += 0.04;
      if( micBass > 2.0 )micBass=2.0;
    }
    
    if( newMicBass < micBass )
    {
      micBass -= 0.04;
      if( micBass < 0.5 )micBass=0.5;
    }
  }

  if( ! isLimit( newMicMid, micMid ) )
  {
    if( newMicMid > micMid )
    {
      micMid += 0.04;
      if( micMid > 2.0 )micMid=2.0;
    }
    
    if( newMicMid < micMid )
    {
      micMid -= 0.04;
      if( micMid < 0.5 )micMid=0.5;
    }
  }

  if( ! isLimit( newMicTreble, micTreble ) )
  {
      if( newMicTreble > micTreble )
      {
        micTreble += 0.04;
        if( micTreble > 2.0 )micTreble=2.0;
      }
      
      if( newMicTreble < micTreble )
      {
        micTreble -= 0.04;
        if( micTreble < 0.5 )micTreble=0.5;
      } 
  }
  micfiltermix.gain(0,micBass);
  micfiltermix.gain(1,micMid);
  micfiltermix.gain(2,micTreble);
  //ctrl_sgtl_1.eqBands(-1.00,micBass, micMid ,micTreble,-0.7);
}


void initMicParaEq() 
{
    //return;
    int updateFilter[5];
    
    ctrl_sgtl_1.eqSelect(1); //PARAMETRIC_EQUALIZER   
    ctrl_sgtl_1.eqFilterCount(7);
    
    //calcBiquad(FILTER_LOSHELF, 125.0, -24.0, 0.7, 524288, 44100, updateFilter);
    calcBiquad(FILTER_HIPASS, 150.0, -24.0, 0.707, 524288, 44100, updateFilter);
    ctrl_sgtl_1.eqFilter(0,updateFilter);

    calcBiquad(FILTER_PARAEQ, 200.0, +4.0, 0.7, 524288, 44100, updateFilter);
    ctrl_sgtl_1.eqFilter(1,updateFilter);
    
    //calcBiquad(FILTER_PARAEQ, 350.0, -3.0, 0.7, 524288, 44100, updateFilter);
    calcBiquad(FILTER_PARAEQ, 350.0, -4.0, 0.707, 524288, 44100, updateFilter);
    ctrl_sgtl_1.eqFilter(2,updateFilter);
    
    //calcBiquad(FILTER_PARAEQ, 2000.0, 5.0, 0.7, 524288, 44100, updateFilter);
    calcBiquad(FILTER_PARAEQ, 2000.0, 6.0, 0.9, 524288, 44100, updateFilter);
    ctrl_sgtl_1.eqFilter(3,updateFilter);
    
    calcBiquad(FILTER_PARAEQ, 5000.0, 3.0, 0.7, 524288, 44100, updateFilter);
    ctrl_sgtl_1.eqFilter(4,updateFilter);

    //calcBiquad(FILTER_HISHELF, 8000.0, -18.0, 0.5, 524288, 44100, updateFilter);
    calcBiquad(FILTER_HISHELF, 8000.0, -12.0, 0.707, 524288, 44100, updateFilter);
    ctrl_sgtl_1.eqFilter(5,updateFilter);

    //calcBiquad(FILTER_LOPASS, 15000.0, -12.0, 0.7, 524288, 44100, updateFilter); // tester 10000 -48
    calcBiquad(FILTER_LOPASS, 10000.0, -64.0, 0.707, 524288, 44100, updateFilter); // tester 10000 -48
    ctrl_sgtl_1.eqFilter(6,updateFilter);
}

void volumeExt()
{
   float volumeG = getLPF( &VolumeG_LPF , mapf( (float)analogRead(VOLUME_EXT_PIN),0.0,1023.0,0.60,1.0 ), LPF_COEF_VOLUME_G );
   
    //float volumeG=analogRead(VOLUME_EXT_PIN) * 0.0009765625;
   
    //ctrl_sgtl_1.unmuteLineout();
    
    ctrl_sgtl_1.dacVolume( volumeG );
   // ctrl_sgtl_1.lineOutLevel(volumeG); //lineInLevel(left, right)  13 - 31 default 29
}

void  micInUpdate()
{
  if(refreshMicCmd < REFRESHMICCMD ) return;
  refreshMicCmd=0;
  volumeExt();
  micInGain();
  micInTone();
}


/** ******** DUCK ****** */

void duckON()
{
	flag_duck_ON=true;
}

void duckOFF()
{
	flag_duck_ON=false;
}

void  duckMix()
{
  //mixerL.gain(1,DUCK_VOL); //mixL
  //mixerR.gain(1,DUCK_VOL); //mixR
  cmdVcaMix.amplitude(DUCK_VOL, DUCK_ATTACK_TIME);
}

void restoreMix()
{
    //mixerL.gain(1,MIX_VOL); //mixL
   // mixerR.gain(1,MIX_VOL); //mixR
   cmdVcaMix.amplitude(1.0, DUCK_RELEASE_TIME);
}

void updateDuck()
{
  if(flag_duck_ON)
  {
    if(mic_peak.available())
    {
      float smoothSignal = getLPFSignalInput(mic_peak.read());
      if(  smoothSignal > MICDUCKTHRES_ON )
      {
        digitalWrite(TALKOVER_OUT_PIN,false);
        if(flag_unMuteMic && flag_unMutePFL)duckMix();
        LedDuckON();
      }
      
      if( smoothSignal < MICDUCKTHRES_OFF )
      {
        digitalWrite(TALKOVER_OUT_PIN,true);
        restoreMix();
        LedDuckOFF();
      }
    }
    
  }else{
    digitalWrite(TALKOVER_OUT_PIN,true);
    restoreMix();
    LedDuckOFF();
  }
  
}

void autoMix()
  {
   
    if(outputPeak.available())
    {
      if( (getLPF( &outputPeak_LPF , outputPeak.read(), LIMIT_OUT_LPFCOEF)) >= LIMIT_OUT_THRESHOLD )
      {
        automixMicLvl-=0.04;
        automixMixLvl-=0.04;

       if(automixMicLvl < 0.0 ) automixMicLvl = 0.0;
       if(automixMixLvl < 0.0 ) automixMixLvl = 0.0;
        
      }else{
        automixMicLvl+=0.04;
        automixMixLvl+=0.04;

       if(automixMicLvl > MIC_VOL ) automixMicLvl = MIC_VOL;
       if(automixMixLvl > MIX_VOL ) automixMixLvl = MIX_VOL;
 
      }
      
        mixerL.gain(0,automixMicLvl); //mic
        mixerL.gain(1,automixMixLvl); //mixL
        
        mixerR.gain(0,automixMicLvl); //mic
        mixerR.gain(1,automixMixLvl); //mixR
    }
  }

/********* BUTTONs *********/

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

/************* LEDs **********/

void LedMicON()
{
  ledStatus[LEDMIC]=1;
}

void LedMicOFF()
{
  ledStatus[LEDMIC]=0;
}

void LedMicBLINK()
{
  ledStatus[LEDMIC]=2;
}

void LedDuckON()
{
  ledStatus[LEDDUCK]=1;
}

void LedDuckOFF()
{
  ledStatus[LEDDUCK]=0;
}

void LedDuckBLINK()
{
  ledStatus[LEDDUCK]=2;
}

void updateLeds()
{
 
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

/*
void volumeExt()
{
    int volumeG=map(analogRead(VOLUME_EXT_PIN),0,1023,31,13);
    if( volumeG == 31 )
    {
      ctrl_sgtl_1.lineOutLevel(31);
      ctrl_sgtl_1.muteLineout();
      return;
    }
    ctrl_sgtl_1.unmuteLineout();
    ct5rl_sgtl_1.lineOutLevel(volumeG); //lineInLevel(left, right)  13 - 31 default 29
}
*/

/******* USEFUL ***********/

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

float mapf(float value, float istart, float istop, float ostart, float ostop) {
return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

/****** UPDATEs **********/

void updateAll()
{
  updateBounce();
  updateLeds(); 
  updateDuck();
  micInUpdate();
  autoMix();
}

/******* MAIN ***********/

void loop() 
{
  updateAll();
}
 
