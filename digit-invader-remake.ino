/****************
Copyright (c) 2013 kwx

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/****************
This is an Arduino project. With the source code only one may not get the game working properly without preparing the required electronics components and wire them up accordingly.
Please refer to the follow link for the wiring diagram and component list.

Wiring diagram:
https://code.google.com/p/digit-invader-remake/downloads/list

Component list:
https://code.google.com/p/digit-invader-remake/w/list
 */
#include "vfd.h"

/* Set the appropriate digital I/O pin connections */
//the aim button and fire button should be connected to pins 8,9 with resistors
//refer to wiring diagram for connection details
const byte AIM_PIN = 8;
const byte FIRE_PIN = 9;

//pin 10 is for speaker, note that you need to use a pin that has PWM support if you would like to use other pins
const byte SPEAKER_PIN = 10; 

const byte LED_PIN = 6;

const word HIT = 1157;
const word MISSED = 543;

const word COMMANDER_DEAD_1 = 2276;
const word COMMANDER_DEAD_2 = 2217;
const word COMMANDER_DEAD_3 = 2165;
const word COMMANDER_DEAD_4 = 2109;
const word COMMANDER_DEAD_5 = 2053;
const word COMMANDER_DEAD_6 = 2003;
const word COMMANDER_DEAD_7 = 1959;
const word COMMANDER_DEAD_8 = 1916;
const word COMMANDER_DEAD_9 = 1873;
const word COMMANDER_DEAD_10 = 1832;
const word COMMANDER_DEAD_11 = 1795;
const word COMMANDER_DEAD_12 = 1759;
const word COMMANDER_DEAD_13 = 1723;

const word DEFENSE_LINE_FALLING = 814;
const word ADVANCE_ENCOUNTER = 1210;

const word GAME_OVER_SOUND_1 = 2341;
const word GAME_OVER_SOUND_2 = 815; //2440

const int8_t COMMANDER_INVADER = 10;
const int COMMANDER_INVADER_SCORE = 300;
const int8_t BLANK = -1;
const byte COMMANDER_INVADER_CHAR_CODE = 0;
const byte defense_line_total = 3;
const byte encounter_invader_total = 16;
const byte encounter_missile_total = 30;
const byte max_stage_queue_length = 14;
const byte max_stage = 2; //only have stage 1 and 2
const byte max_encounter = 9; //only have encounter 1 up to 9 and then back to 1 again
const byte stage_queue_length[max_stage] = {
    6,5};
const byte encounter_delay_countdown[max_encounter] = {
    //    270,240,210,180,150,120,90,60,30};
    //    90,85,80,75,70,65,60,55,50};
    240,200,165,135,110,90,75,65,60};
const byte stage_position_score_factor[2] = {
    10,20};



//variables that needs to be initialized only once after power on
boolean aim_current_state = HIGH;
boolean fire_current_state = HIGH;
boolean aim_last_state = aim_current_state;
boolean fire_last_state = fire_current_state;
//int8_t invader_queue[max_stage_queue_length];
char invader_queue[max_stage_queue_length];
unsigned long player_score = 0;
boolean game_over = false;
byte current_encounter = 0;
byte current_stage = 0;
boolean game_over_tone_played = false;


//variables that needs not to be initialized
byte next_invader; //to hold the value of the next invader generated
byte missile_hit_position; // to record the position that an invader is hunted down
boolean invader_is_shot_down; //a Boolean to indicate if the missile has hit any invader
byte invader_killed; // to record the code of the invader got shot


//variables that needs to be initialized after each defense line falling
byte aim_current_value = 0;


//variables that needs to be initialized in each encounter
byte defense_line_remaining = defense_line_total;
byte encounter_invader_remaining = encounter_invader_total;
byte encounter_missile_remaining = encounter_missile_total;

byte killed_invader_sum = 0;
byte killed_invader_sum_remainder = 0;
byte invasion_delay_countdown_remaining = encounter_delay_countdown[0];
byte commander_invader_2b_generated = 0;
byte invader_killed_count = 0;


//variables that needs to be initialized in each STAGE

byte current_stage_queue_length = stage_queue_length[0];


//missing features:
//play sound when missed [DONE]
//play sound when hit [DONE]
//play sound when hit commander [DONE]
//play sound when advance level [DONE]
//play sound when game over [DONE]
//advance to next encounter [DONE]
//speed up [DONE]
//advance to next stage [DONE]
//game over when defense line all gone [DONE]
//game over when all missiles fired [DONE]
//show player score when defense line missing [DONE]
//show curent encounter and player score during advance encounter [DONE]


int lcd_cursor = 0;

void lcd_print(const char *p) {
    char ch;
    while (ch = *(p++)) {
        if (ch >= '0' && ch <= '9') {
            setDigit(lcd_cursor++, ch - '0');
        } else {
            switch (ch) {
                case 'a':
                case '-': setSegments(lcd_cursor++, 0b00100000); break;
                case 'b': setSegments(lcd_cursor++, 0b10100000); break;
                case 'c': setSegments(lcd_cursor++, 0b10101000); break;
                case 'n': setSegments(lcd_cursor++, 0b01100010); break;
                default:
                case ' ': setSegments(lcd_cursor++, 0); break;
            }
        }
    }    
}
void lcd_print(int i) {
    String s;
    s += i;
    lcd_print(s.c_str());
}

void playToneInHz(int Hz, long durationInMicroS) { //it seems that this function can be replaced with tone() function
    // long elapsed_time = 0;
    // double note = 1000000/Hz;
    // while (elapsed_time  < durationInMicroS) {
    //     digitalWrite(SPEAKER_PIN, HIGH);
    //     delayMicroseconds(note / 2);
    //     digitalWrite(SPEAKER_PIN, LOW);
    //     delayMicroseconds(note / 2);
    //     elapsed_time += (note);
    // }
    long d = durationInMicroS/1000;
    tone(SPEAKER_PIN, Hz, d); //although it is much more convenient to use the tone() function, yet it is too good in the sense that it is non-blocking and does not simulate exactly eh sound given in the calculator
    delay(d);
}

void playGameOverTone() {
    playToneInHz(GAME_OVER_SOUND_1, 195000);
    delay(210);
    playToneInHz(GAME_OVER_SOUND_2, 195000);
    playToneInHz(GAME_OVER_SOUND_1, 195000);
    delay(200);
    playToneInHz(GAME_OVER_SOUND_2, 200000);
    playToneInHz(GAME_OVER_SOUND_1, 200000);
    delay(200);
    playToneInHz(GAME_OVER_SOUND_2, 200000);
}

void playCommanderGotShotTone() {
    playToneInHz(COMMANDER_DEAD_1, 25000);
    playToneInHz(COMMANDER_DEAD_2, 25000);
    playToneInHz(COMMANDER_DEAD_3, 25000);
    playToneInHz(COMMANDER_DEAD_4, 25000);
    playToneInHz(COMMANDER_DEAD_5, 25000);
    playToneInHz(COMMANDER_DEAD_6, 25000);
    playToneInHz(COMMANDER_DEAD_7, 25000);
    playToneInHz(COMMANDER_DEAD_8, 25000);
    playToneInHz(COMMANDER_DEAD_9, 25000);
    playToneInHz(COMMANDER_DEAD_10, 25000);
    playToneInHz(COMMANDER_DEAD_11, 25000);
    playToneInHz(COMMANDER_DEAD_12, 25000);
    playToneInHz(COMMANDER_DEAD_13, 25000);
}

void playAdvanceEncounterTone() {
    delay(200);
    playToneInHz(ADVANCE_ENCOUNTER, 200000);
    delay(200);
    playToneInHz(ADVANCE_ENCOUNTER, 400000);
    delay(200);
    playToneInHz(ADVANCE_ENCOUNTER, 400000);
    delay(200);
    playToneInHz(ADVANCE_ENCOUNTER, 200000);	
}						

void playMissedTone() {
    playToneInHz(MISSED, 25000);
}

void playHitTone() {
    playToneInHz(HIT, 25000);
}

void playDefenceLineFallingTone() {
    playToneInHz(DEFENSE_LINE_FALLING, 1750000);
}


void reset() {
  aim_current_state = HIGH;
  fire_current_state = HIGH;
  aim_last_state = aim_current_state;
  fire_last_state = fire_current_state;
  player_score = 0;
  game_over = false;
  current_encounter = 0;
  current_stage = 0;
  game_over_tone_played = false;
  aim_current_value = 0;

  defense_line_remaining = defense_line_total;
  encounter_invader_remaining = encounter_invader_total;
  encounter_missile_remaining = encounter_missile_total;

  killed_invader_sum = 0;
  killed_invader_sum_remainder = 0;
  invasion_delay_countdown_remaining = encounter_delay_countdown[0];
  commander_invader_2b_generated = 0;
  invader_killed_count = 0;

  current_stage_queue_length = stage_queue_length[0];
  for (int i = 0; i < (current_stage_queue_length + 1); i++) {
    invader_queue[i] = BLANK;
  }
  clearVfd();
  digitalWrite(LED_PIN, LOW);
}

void setup()
{
    setupVfd();

    randomSeed(analogRead(0));
    pinMode(AIM_PIN, INPUT_PULLUP);
    pinMode(FIRE_PIN, INPUT_PULLUP);
    pinMode(SPEAKER_PIN, OUTPUT);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    reset();
    delay(200);
}

void loop()
{
    aim_current_state = digitalRead(AIM_PIN);
    fire_current_state = digitalRead(FIRE_PIN);

    if (game_over) {
        setSegments(10, 0b11011010); // "GAME OVER"
        setSegments(11, 0b11011110);
        digitalWrite(LED_PIN, HIGH);

        lcd_cursor = 1;
        lcd_print(player_score);
        if (!game_over_tone_played) {
            playGameOverTone();
            game_over_tone_played = true;
        }

        if (aim_current_state == LOW || fire_current_state == LOW) {
            delay(200);
            reset();
            delay(1000);
            return;
        }
    } 
    else {
        if (aim_current_state != aim_last_state) {

            if (aim_current_state == LOW) { //aim button has been pressed
                //handle the aim button press here
                aim_current_value++;
                aim_current_value %= 11;
            }

        }
        if (fire_current_state != fire_last_state) {

            if (fire_current_state == LOW) { //fire button has been pressed

                if (encounter_missile_remaining > 0) { //it is a valid missile press and a missile is fired

                    //handle the fire button press here
                    //deduct a missile, the max number of missile for each level is 30
                    //check if the missile hit any of the invaders
                    //if so, eliminate the invader
                    invader_is_shot_down = false;
                    for (int i = 0; i < current_stage_queue_length; i++) {
                        if ((invader_queue[i] == aim_current_value) && (!invader_is_shot_down)) {
                            invader_is_shot_down = true;
                            missile_hit_position = i;
                            invader_killed = aim_current_value;
                        }
                    }

                    //if invader_is_shot_down is true, there is monter got hit, 
                    //eliminate it from queue and 
                    //shift the monter rightwards, 
                    //put back a blank at the head of invader queue
                    if (invader_is_shot_down) {
                        //give score to player based on the shot position!
                        player_score += stage_position_score_factor[current_stage] * (missile_hit_position + 1);
                        invader_killed_count++;

                        //add the killed invader to the killed_invader_sum
                        if (invader_killed == COMMANDER_INVADER) {
                            //give bonus score 300 to player
                            player_score += COMMANDER_INVADER_SCORE;
                            clearVfd();

                            playCommanderGotShotTone();						
                        } 
                        else { //killed invader is a normal digit
                            killed_invader_sum += invader_killed;
                            killed_invader_sum_remainder = killed_invader_sum % 10;

                            if ((killed_invader_sum_remainder == 0) && (killed_invader_sum > 0)) {
                                commander_invader_2b_generated++;
                                //reset the killed_invader_sum after the commander invader is awarded
                                killed_invader_sum = 0;
                            }

                            playHitTone();
                        }

                        for (int i = missile_hit_position; i > 0; i--) {
                            invader_queue[i] = invader_queue[i-1]; 
                        }
                        //put back a blank to the starting of the quene
                        invader_queue[0] = BLANK;

                        //##### if all invaders in the encounter are killed, advance to the next encounter!!
                        if (invader_killed_count == encounter_invader_total) {
                            //display encounter number, hyphen and player score for a few seconds
                            //reset queue, initialize all encounter variables
                            clearVfd();
                            lcd_cursor = 1;
                            lcd_print(current_encounter+1);
                            lcd_print("-");
                            lcd_print(player_score);

                            playAdvanceEncounterTone();						

                            current_encounter++;
                            if (current_encounter == max_encounter) { //advance the stage if the encounter reaches 9
                                current_stage++;
                                current_stage %= max_stage;
                            }
                            current_encounter %= max_encounter;

                            aim_current_value = 0;
                            //variables that needs to be initialized in each encounter
                            defense_line_remaining = defense_line_total;
                            encounter_invader_remaining = encounter_invader_total;
                            encounter_missile_remaining = encounter_missile_total;
                            killed_invader_sum = 0;
                            killed_invader_sum_remainder = 0;
                            invasion_delay_countdown_remaining = encounter_delay_countdown[current_encounter];
                            commander_invader_2b_generated = 0;
                            invader_killed_count = 0;
                            //variables that needs to be initialized in each STAGE
                            current_stage_queue_length = stage_queue_length[current_stage];

                        }
                    } 
                    else { //no invader got hit! The missile misses
                        //flash the screen once
                        clearVfd();

                        playMissedTone();
                    }

                    encounter_missile_remaining--;
                } 
                else { //no more missiles are available
                    game_over = true;
                    clearVfd();
                }     


            }
        }

        if (invasion_delay_countdown_remaining > 0) {
            invasion_delay_countdown_remaining--;
        } 
        else {
            //generate the next invader only as long as the remaining encounter_invader_remaining > 0
            if (encounter_invader_remaining > 0) {
                //generate the next invader (unless the current dead invader sum is mod 10 = 0
                if (commander_invader_2b_generated > 0) {
                    next_invader = COMMANDER_INVADER;
                    commander_invader_2b_generated--;
                } 
                else {
                    next_invader = random(0,10);
                    //      next_invader = 5; //for debug about commander invader only
                }
                encounter_invader_remaining--;
            } 
            else {
                //there are no more invaders remaining, put back BLANK
                next_invader = BLANK;
            }

            //about to shift the queue, however, before doing that, check if the invader has hit the defensive shield
            if (invader_queue[0] == BLANK) {
                //shift the next invader into the end of the queue
                invader_queue[current_stage_queue_length] = next_invader;

                //shift the whole queue leftwards by 1
                for (int i = 0; i < current_stage_queue_length; i++) {
                    invader_queue[i] = invader_queue[i+1];      
                }
            } 
            else { //the head of the invader queue is a invader
                //deduct 1 live from player
                defense_line_remaining--;
                //check if there are any more defense line remaining
                if (defense_line_remaining > 0) {
                    //clear screen for 1 second and
                    //play losing noise for 1 second and
                    //reset almost everything

                    clearVfd();                    
                    //show player score
                    lcd_cursor = 1;
                    lcd_print(player_score);

                    playDefenceLineFallingTone();

                    //clear the queue
                    //set the invader to be generated count = invader total in each encounter - invader killed
                    encounter_invader_remaining = encounter_invader_total - invader_killed_count;
                    for (int i = 0; i < (current_stage_queue_length + 1); i++) {
                        invader_queue[i] = BLANK;
                    }
                    aim_current_value = 0;                
                    clearVfd();
                } 
                else { //defense line remaining == 0
                    game_over = true;
                    clearVfd();
                }
            }

            //reset the countdown timer
            invasion_delay_countdown_remaining = encounter_delay_countdown[current_encounter];
        }

        //update screen
        if (!game_over) {
            lcd_cursor = 1;
            if (aim_current_value < 10) {
                lcd_print((word)aim_current_value);
            } 
            else {
                lcd_print("n");
            }

            lcd_cursor = 2;
            if (defense_line_remaining > 0) {
                char ch[2] = {'a' + defense_line_remaining - 1, 0};
                lcd_print(ch);
            }

            for (int i = 0; i < current_stage_queue_length; i++) {
                lcd_cursor = 3+i;
                if (invader_queue[i] == BLANK) {
                    lcd_print(" ");
                } 
                else if (invader_queue[i] == COMMANDER_INVADER) {
                    lcd_print("n");
                } 
                else {
                    lcd_print((word)invader_queue[i]);
                }
            }

            //lcd.setCursor(0, 0);

            //save current button state to the last button state variable
            aim_last_state = aim_current_state;
            fire_last_state = fire_current_state;
        }
       delayMicroseconds(2000L);
    }
}

