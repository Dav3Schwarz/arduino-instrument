//--ENUMS--
enum Mode {
  PLAY,
  RECORD,
  SELECT,
  PLAYBACK
};

enum Instrument {
  LEAD,
  DRUMS
};
//--------

//--STRUCTS--
struct Note {                       //stores note specific details for both leads and drums
  int16_t note;                     // -1 = rest, otherwise freq or drum ID 
  uint16_t start;                   // ms offset from recordStart
  uint16_t duration;                // ms length
  uint8_t type;                     // 0 = LEAD, 1 = DRUM
};                  
//--------

//--TRACK GLOBALS---
Note* timeline[160];                //contains pointers to memory addresses of both lead and drum tracks
int timelineLength = 0;             //tracks how much of the timeline we have consumed

const int MAX_TRACK_LENGTH = 60;    //the maximum length of our tracks

int trackLength = 0;                //tracks how much lead track we have consumed
int drumTrackLength = 0;            //tracks how much drum track we have consumed

Note leadTrack[MAX_TRACK_LENGTH];   //stores lead notes
Note drumTrack[MAX_TRACK_LENGTH];   //stores drum notes
int drumDelay = 250;                // delay added on to the drums to prevent it from firing off too fast
//--------

//--STATE GLOBALS --
Instrument instrument = LEAD;       //selected instrument
Mode mode = PLAY;                   //selected mode
int active = 4;                     //active button
Note currentNote;                   //active note
int pressedButton = 4;              //remembers which button is being pressed, 4 = no button
//--------

//--PINS--
const int trigPin = 11;             //connects to the trigger pin on the distance sensor
const int echoPin = 12;             //connects to the echo pin on the distance sensor
int button[] = {13, 4, 3, 2};       //red is button[0], yellow is button[1], green is button[2], blue is button[3]
int RedPin = 5;                     //red led
int GreenPin = 6;                   //green led
int BluePin = 9;                    //blue led
int buzzerPin = 10;                 //pin that the buzzer is connected to
int switchPin = 7;                  //pin that switch is connected to
//--------

//--DISTANCE SENSOR--
float distance = 0;                 //distance measured by distance sensor
unsigned long lastMeasurement = 0;  //timestamp of when distance sensor made a measurement
//--------

//--BUZZER--
int tones[] = {262, 330, 392, 494}; //tones to play with each button (c, e, g, b)
//--------

void setup() {
  pinMode(trigPin, OUTPUT);         //the trigger pin will output pulses of electricity
  pinMode(echoPin, INPUT);          //the echo pin will measure the duration of pulses coming back from the distance sensor

  
  pinMode(button[0], INPUT_PULLUP); //set all of the button pins to input_pullup (use the built-in pull-up resistors)
  pinMode(button[1], INPUT_PULLUP);
  pinMode(button[2], INPUT_PULLUP);
  pinMode(button[3], INPUT_PULLUP);

  
  pinMode(switchPin, INPUT_PULLUP); //switch

  
  pinMode(RedPin, OUTPUT);          //set the LED pins to output
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin, OUTPUT);

  pinMode(buzzerPin, OUTPUT);       //set the buzzer pin to output
}

/*
  mimics a drum kick by rapidly pitching down
*/
void kick(){
  static unsigned long lastHit = 0;
  if (millis() - lastHit < drumDelay) 
    return;                                           // set a timeout before we can use the kick again
  lastHit = millis();

  for (int freq = 200; freq > 0; freq -= 1){
    tone(buzzerPin, freq);
  }
  noTone(buzzerPin);
}

/*
  mimics a drum snare by playing random noise quickly
*/
void snare() {
  static unsigned long lastHit = 0;
  if (millis() - lastHit < drumDelay) 
    return;                                           // set a timeout before we can use the snare again
  lastHit = millis();

  tone(buzzerPin, 200, 40);

  for (int i = 0; i < 100; i++) {
    tone(buzzerPin, random(500,800));                // random noise
  }
  noTone(buzzerPin);
}

/*
  mimics a hihat by quickly playing through a set of frequencies
*/
void hiHat() {
  static unsigned long lastHit = 0;
  if (millis() - lastHit < drumDelay) 
    return;                                           // set a timeout before we can use the hihat again
  lastHit = millis();

  int freqs[] = {15000,3000, 4000,10000, 5000, 4000, 8000, 20000};
  for (int i = 0; i < sizeof(freqs) / sizeof(freqs[0]); i++) {
    tone(buzzerPin, freqs[i],90);
  }
  noTone(buzzerPin);
}

/*
  utility function maps the individual drums to an integer so they 
  can be interpreted as notes
*/
void playDrum(int drum){
  if(drum == 0){
    kick();
  }else if(drum == 1){
    snare();
  }else if(drum == 2){
    hiHat();
  }
}

/*
  construct a complete timeline including both instruments tracks.
  this allows us to playback the two tracks together in the correct order.
  since the drums play rather fast we mimic them playing together, 
  when in reality they take turns using the buzzer.
*/
void buildTimeline() {
  timelineLength = 0;

  for (int i = 0; i < trackLength; i++) {                // add pointers to lead track notes and store them in our timeline
    leadTrack[i].type = 0;                               // set type to lead
    timeline[timelineLength] = &leadTrack[i];            // store the memory address
    timelineLength++;
  }
  
  for (int j = 0; j < drumTrackLength; j++) {            // add pointers to drum track notes and store them in our timeline
    drumTrack[j].type = 1;                               // set type to drum
    timeline[timelineLength] = &drumTrack[j];            // store the memory address
    timelineLength++;
  }

  for (int i = 0; i < timelineLength - 1; i++) {         // sort the pointers by their start times
    for (int j = i + 1; j < timelineLength; j++) {
      if (timeline[j]->start < timeline[i]->start) {
        Note* tmp = timeline[i];                         // swap using temporary note pointer
        timeline[i] = timeline[j];
        timeline[j] = tmp;
      }
    }
  }
}

/*
  for this circuit we utilize a simple loop that will check states,
  and distribute the work to dedicated functions which will handle state specific requirements.
*/
void loop() {
  int switchState = digitalRead(switchPin); 

  if(mode == PLAY && switchState == LOW)
    play();
  else if((mode == PLAY && switchState == HIGH) || mode == SELECT)
    handleSelect();
  else if(mode == RECORD)
    handleRecord();
  else if(mode == PLAYBACK)
    handlePlayback();
}

/*
  utility function used to completely reset the lead track
*/
void resetLeadTrack() {
  trackLength = 0;
  for (int i = 0; i < MAX_TRACK_LENGTH; i++) {
    leadTrack[i] = { -1, 0, 0 };
  }
}

/*
  utility function used to completely reset the drum track
*/
void resetDrumTrack() {
  drumTrackLength = 0;
  for (int i = 0; i < MAX_TRACK_LENGTH; i++) {
    drumTrack[i] = { -1, 0, 0 };
  }
}

/*
  constructs a timeline and starts a clock to then
  execute all recorded notes in the correct order
*/
void handlePlayback() {
  timelineLength = 0;                                   //reset the timeline length before building it
  buildTimeline();                                      //rebuild timeline eachtime playback is called
  
  while(digitalRead(switchPin) == HIGH){                //continue playback until switch is moved to the LOW state
    int i = 0;
    uint16_t start = millis();                          // start a clock so we can properly traverse the timeline
    
    Note* interruptedLead = NULL;                       // tracks lead notes so they can be restored if drums interrupt them

    while (i < timelineLength) {
      if(digitalRead(switchPin) == LOW){                // check if the switch has moved, if so, break out of the loop
        break;
      }

      uint16_t now = millis() - start;
      if (now >= timeline[i]->start) {                  // play notes as they were recorded by comparing our current clock

        if (timeline[i]->type == 1) {                   // play a drum note
          playDrum(timeline[i]->note);
                                                        // restore the interrupted lead if there was one
          if (interruptedLead != NULL && interruptedLead->note != -1) {
            tone(buzzerPin, interruptedLead->note);
          }
        }
       
        else {                                          // play a lead note
          if (timeline[i]->note != -1) {
            interruptedLead = timeline[i];              // store the lead note incase it gets interrupted
            tone(buzzerPin, interruptedLead->note);
          } else {
            interruptedLead = NULL;                     // if the lead note is -1 then it should be silent
            noTone(buzzerPin);
          }
        }

        i++;                                            // move into the next timeline note 
      }
    }
  }

  
  noTone(buzzerPin);                                    // return everything to defaults before leaving playback
  active = 4;
  currentNote.note = -1;
  currentNote.duration = 0;
  distance = 0;
  lastMeasurement = millis(); 

  mode = PLAY;                                          // set state to play
}

uint16_t recordStart = 0;                               // used as a clock when recording notes

/*
  prompts user to select an instrument and then
  records all notes played by user into a dedicated track
  depending on the instrument selected
*/
void handleRecord(){

  handleInstrument();                                 // prompt user to select an instrument
  
  if(instrument == LEAD)                              // depending on which instrument the user selected, reset the track
    resetLeadTrack();
  else
    resetDrumTrack();

  while(digitalRead(switchPin) == HIGH){              // wait until user moves the swtich to LOW before recording
    red();
    delay(200);
  }

  buildTimeline();                                    //build the timeline so that we can hear the drums playing
                                                    
  hiHat();                                            // count in the user 
  delay(1000);
  hiHat(); 
  delay(1000);
  hiHat(); 
  delay(1000);
  snare();


  recordStart = millis();                             // start the clock to record timestamps
  active = 4;                                         // set active to 4 indicating no active note
  pressedButton = 4;                                  // set to 4 indicating no button is being pressed
  noTone(buzzerPin);                                  // set buzzer to be silent
  
  currentNote.note = -1;                              // reset the current note in case something was still lingering in it
  currentNote.start = 0;
  currentNote.duration = 0;

  int timelineIndex = 0;                              // tracks where on the timeline we currently are

  while(digitalRead(switchPin) == LOW){
    red();
    play();                                           // capture input
    
    if (instrument == LEAD) {                         // if we are recording the leads, we play the drums in the background using our timeline
      uint16_t now = millis() - recordStart;
      if (timelineIndex < timelineLength && now >= timeline[timelineIndex]->start) {
        if (timeline[timelineIndex]->type == 1) {
          playDrum(timeline[timelineIndex]->note);
          if (active != 4) {                          // if we interrupted a lead, make sure to recover it after playing the drums
            tone(buzzerPin, tones[active] - distance);
          }
        }
        timelineIndex++;                              // step forward in our timeline
      }
    }
  }

  uint16_t now = millis() - recordStart;              // finalize the last note
  currentNote.note = -1;
  currentNote.start = now;
  currentNote.duration = 0;

  turnOff();                                          // turn led off and return the select mode
  mode = SELECT;
}

/*
  handles instrument selection by awaiting for the user to 
  hold a corresponding button and then updating the instrument state
*/
void handleInstrument(){
  blue();
  bool await = true;                                  // used to wait for user to make a selection
  unsigned long held = 0;                             // timer we use to check how long a user has held a button for
  while(await){                                       // wait for user selection
    if(digitalRead(button[3]) == LOW){                // drums are selected
      kick();                                         // play a drum sample so the user is aware of which instrument they are selecting
      delay(200);
      hiHat(); 
      delay(200);
      snare(); 
      delay(200);

      if (held == 0) 
        held = millis();                              // start a timer
      
      if (millis() - held >= 2000){                   // if the button has been held for 2 seconds or more select the drums and stop waiting
        await = false;
        instrument = DRUMS;
      }
    }
    else if(digitalRead(button[0]) == LOW){          // leads are selected
      tone(buzzerPin, tones[0]);                     // play a sample of the leads so the user is aware of which instrument they are selecting
      delay(200); 
      noTone(buzzerPin); 
      delay(200);
      tone(buzzerPin, tones[0]); 
      delay(200); 
      noTone(buzzerPin); 
      delay(200);
      tone(buzzerPin, tones[1]); 
      delay(200); 
      noTone(buzzerPin); 
      delay(200);

      if (held == 0) 
        held = millis();                            // start a timer
      
      if (millis() - held >= 2000){                 // if the button has been held for 2 seconds or more select the leads and stop waiting
        await = false;
        instrument = LEAD;
      }
    }
    else 
      held = 0;                                     // reset timer
  }
  turnOff();                                        // turn off the led
}

/*
  handles the selection mode, where a user can choose
  which mode to enter
*/
void handleSelect(){
  mode = SELECT; 
  yellow();                                         // flash the led yellow so that the user is aware they are in selection mode 
  delay(200); 
  turnOff(); 
  delay(200); 
  yellow();

  if (digitalRead(button[0]) == LOW){               // update the mode corresponding to the button pressed
    mode = RECORD;
  }
  else if (digitalRead(button[1]) == LOW){
    mode = PLAYBACK;
  }
  else if (digitalRead(button[3]) == LOW){
    handleInstrument();
    mode = PLAY;
  }
}

/*
  this function serves two purposes:
    1: allow the user to play an instrument
    2: allow the user to play and instrument and record what they are playing
  because of this, the function will behave slightly differently when in play mode and when in record mode
*/
void play()
{

  int pressed = 4;                                  // tracks which button was last pressed

  turnOff();                                        // turn off the led

  if(digitalRead(button[0]) == LOW){                // update pressed corresponding to the button pressed, and change the color of the led to represent that
    red(); 
    pressed = 0; 
  }
  else if(digitalRead(button[1]) == LOW){ 
    yellow(); 
    pressed = 1; 
  }
  else if(digitalRead(button[2]) == LOW){ 
    green(); 
    pressed = 2; 
  }
  else if(digitalRead(button[3]) == LOW){ 
    blue(); 
    pressed = 3; 
  }

  uint16_t now = 0;                                // clock used in record mode, so that accurate timestamps can be taken
  if(mode == RECORD){
    now = millis() - recordStart;
  } 

  if(active == 4)                                  // if there is no active button
  {
    if(pressed != 4)                               // if a button is being pressed
    {
      if(mode == RECORD && currentNote.note == -1) // if we are recording and the current note is silence end the current note, start a new one
      {
        currentNote.duration = now - currentNote.start;

        if(instrument == LEAD && trackLength < MAX_TRACK_LENGTH){
          leadTrack[trackLength] = currentNote;
          trackLength++;
        }
        else if (instrument == DRUMS && drumTrackLength < MAX_TRACK_LENGTH){
          drumTrack[drumTrackLength] = currentNote;
          drumTrackLength++;
        }
      }

      active = pressed;

      if(instrument == LEAD) {
        currentNote.note = tones[pressed] - distance;
      }
      else {
        currentNote.note = pressed;
      }
      currentNote.start = now;
      currentNote.duration = 0;

      if(instrument == LEAD)
        tone(buzzerPin, currentNote.note);
      else {
        playDrum(currentNote.note);
      }
    }                                           // if there is no active button and no button is being pressed do nothing
  }
  else                                          // there exists an active button
  {
    if(digitalRead(button[active]) == LOW)      // active button is still being held, continue playing it on the buzzer
    {
      if(instrument == LEAD) {
        tone(buzzerPin, tones[active] - distance);
      }
      else {
        playDrum(active);
      }
    }
    else                                        // active button is not being held anymore
    {
      noTone(buzzerPin);                        // silence the buzzer

      if(mode == RECORD)                        // if we are recording capture the note
      {
        currentNote.duration = now - currentNote.start;

        if(instrument == LEAD && trackLength < MAX_TRACK_LENGTH){
          leadTrack[trackLength] = currentNote;
          trackLength++;
        }
        else if(instrument == DRUMS && drumTrackLength < MAX_TRACK_LENGTH){
          drumTrack[drumTrackLength] = currentNote;
          drumTrackLength++;
        }

        currentNote.note = -1;                 // reset the current note after we complete capturing
        currentNote.start = now;
        currentNote.duration = 0;
      }
      active = 4;                              // set active to none
    }
  }

  if (millis() - lastMeasurement > 50)         // check if we should have the distance sensor make a measurement
  {
    distance = getDistance();
    distance *= 8;                             // 8 is a bit of a magic number here, its what I felt sounded best when offsetting the notes with distance
    lastMeasurement = millis();
  }
}

/*
  makes a distance measurement using the distance sensor and returns it as a float
*/
float getDistance()
{
  float echoTime;                 
  float calculatedDistance;         

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  echoTime = pulseIn(echoPin, HIGH, 20000);    // I gave it a timeout because it seemed to be hanging up the rest of the program at times      

  if (echoTime == 0)                           // default return if the distance sensor is taking too long
    return 30;

  return echoTime / 148.0;              
}

/*
  utility functions used to change the color of the led
*/
void red () { 
  analogWrite(RedPin, 100); 
  analogWrite(GreenPin, 0); 
  analogWrite(BluePin, 0); 
}
void orange () { 
  analogWrite(RedPin, 100); 
  analogWrite(GreenPin, 50); 
  analogWrite(BluePin, 0); 
}
void yellow () { 
  analogWrite(RedPin, 100); 
  analogWrite(GreenPin, 100); 
  analogWrite(BluePin, 0); 
}
void green () { 
  analogWrite(RedPin, 0); 
  analogWrite(GreenPin, 100); 
  analogWrite(BluePin, 0); 
}
void cyan () { 
  analogWrite(RedPin, 0); 
  analogWrite(GreenPin, 100); 
  analogWrite(BluePin, 100); 
}
void blue () { 
  analogWrite(RedPin, 0); 
  analogWrite(GreenPin, 0); 
  analogWrite(BluePin, 100); 
}
void magenta () { 
  analogWrite(RedPin, 100); 
  analogWrite(GreenPin, 0); 
  analogWrite(BluePin, 100);
}
void turnOff () { 
  analogWrite(RedPin, 0); 
  analogWrite(GreenPin, 0); 
  analogWrite(BluePin, 0); 
}