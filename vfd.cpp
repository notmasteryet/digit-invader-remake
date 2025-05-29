#include <Arduino.h>

#include "vfd.h"

uint8_t vfd[12];
volatile uint8_t vfd_p;

const int BLANK = 2;
const int DIN = 3;
const int STROBE = 4;
const int CLK = 5;

void writeVFD(uint16_t grids, uint8_t segs) {
  shiftOut(DIN, CLK, MSBFIRST, (segs >> 6) | ((grids & 1) << 3)| ((grids & 2) << 1));
  shiftOut(DIN, CLK, MSBFIRST, ((segs << 2) & 0b11111100) | ((grids >> 10) & 3));  
  shiftOut(DIN, CLK, MSBFIRST, (grids >> 2) & 255);
  digitalWrite(STROBE, HIGH);
  digitalWrite(STROBE, LOW);  
}

uint8_t digitToBits(int8_t d) {
  switch(d) {
    case 0: return 0b11011110;
    case 1: return 0b110;
    case 2: return 0b11101100;
    case 3: return 0b10101110;
    case 4: return 0b00110110;
    case 5: return 0b10111010;
    case 6: return 0b11111010;
    case 7: return 0b00001110;
    case 8: return 0b11111110;
    case 9: return 0b10111110;
    case -1: return 1;
    case -2: return 0b00100000;
  }
  return 0;
}

ISR(TIMER1_COMPA_vect) 
{
  writeVFD((1 << vfd_p), vfd[vfd_p]);
  if (++vfd_p >= 12) vfd_p = 0;
}

void setupVfd() {
  pinMode(BLANK, OUTPUT); digitalWrite(BLANK, HIGH);
  pinMode(STROBE, OUTPUT);
  pinMode(DIN, OUTPUT);
  pinMode(CLK, OUTPUT);
  digitalWrite(CLK, LOW);
  digitalWrite(STROBE, LOW);
  digitalWrite(BLANK, LOW);

  clearVfd();
  vfd_p = 0;

	noInterrupts();
	TCCR1A = 0;
	TCCR1B = (1 << WGM12);
  TCNT1 = 0;  
	/*
		OCR1A = [16, 000, 000 / (prescaler * 1)] - 1
		OCR1A = [16, 000, 000 / (8 * 2000)] - 1 = 1000 - 1
	*/
	OCR1A  = 999;
  /*
	Prescaler:
			1    [CS10]
			8    [CS11]
			64   [CS11 + CS10]
			256  [CS12]
			1024 [CS12 + CS10]
	*/
	TCCR1B |= (1 << CS11);
	TIMSK1 |= (1 << OCIE1A); 
  interrupts();
}

void setDigit(uint8_t pos, int8_t n) {
  vfd[pos] = digitToBits(n);
}

void setSegments(uint8_t pos, uint8_t bits) {
  vfd[pos] = bits;
}

void clearVfd() {
  memset(vfd, 0, sizeof(vfd));
}