#include <dos.h>
#include <conio.h>
#include <stdio.h>

#define MIDI_DATA   0x330
#define MIDI_STATUS 0x331
#define IRQ_NUM 9
#define IRQ_VECTOR 0x0D
#define PIT_CONTROL 0x43
#define PIT_CHANNEL2 0x42
#define SPEAKER_CTRL 0x61

// Lookup table for PIT divisors mapped from MIDI note numbers
// (frequency = 440 * 2^((note - 69)/12), divisor = 1193180 / frequency)
const unsigned int pitDivisors[128] = {
145955, 137741, 130047, 122692, 115843, 109341, 103194, 97402,
91960, 86777, 81865, 77354, 72977, 68870, 65023, 61346,
57921, 54670, 51597, 48701, 45980, 43388, 40932, 38677,
36489, 34435, 32512, 30673, 28961, 27335, 25798, 24351,
22990, 21694, 20466, 19338, 18244, 17218, 16256, 15337,
14480, 13668, 12899, 12175, 11495, 10847, 10242, 9661,
9122, 8609, 8128, 7668, 7240, 6834, 6450, 6088,
5745, 5424, 5119, 4833, 4561, 4304, 4063, 3835,
3620, 3417, 3225, 3044, 2873, 2712, 2559, 2416,
2280, 2152, 2032, 1917, 1810, 1708, 1612, 1522,
1437, 1356, 1280, 1208, 1140, 1076, 1016,  959,
 905,  854,  806,  761,  718,  678,  640,  604,
 570,  538,  508,  452,  427,  403,  380,  359,
 339,  320,  302,  285,  269,  254,  226,  214,
 202,  190,  180,  169,  106,  151,  143,  135,
 127,  113,  107,  101,   95,   90,   85,   80
};

volatile unsigned char last_byte=0;
volatile int byte_ready = 0;

void interrupt (*old_handler)(void);

void interrupt new_handler() {
 if(!(inp(MIDI_DATA) & 0x80)) {
 last_byte = inp(MIDI_DATA);
 byte_ready = 1;
	}
}
void toggleSpeaker(int on, unsigned char midiNote) {
    unsigned char tmp;
    unsigned int divisor;

    if (on) {
	if (midiNote > 127) return; // Sanity check

	divisor = pitDivisors[midiNote];

	outp(PIT_CONTROL, 0xB6);
	outp(PIT_CHANNEL2, divisor & 0xFF);         // Low byte
	outp(PIT_CHANNEL2, (divisor >> 8) & 0xFF);  // High byte

	tmp = inp(SPEAKER_CTRL);
	outp(SPEAKER_CTRL, tmp | 0x03);             // Turn speaker on
    } else {
	tmp = inp(SPEAKER_CTRL);
	outp(SPEAKER_CTRL, tmp & ~0x03);            // Turn speaker off
    }
}

void main() {
    int noteOn = 0;
    int noteOff= 0;
    int timebuf=10000;
    clrscr();
    printf("MIDI Sniffer - Listening on 0x330\nPress ESC to quit\n");

    // Wake up MPU-401 into UART mode
    outp(MIDI_STATUS, 0x3F);
    delay(100);

    /*
    disable();
    old_handler = getvect(IRQ_VECTOR);
    setvect(IRQ_VECTOR, new_handler);
    enable();
      */
while (1){ // ESC to quit
	if(!(inp(MIDI_STATUS) & 0x80)) {
	last_byte = inp(MIDI_DATA);
	byte_ready = 1;
		}
	if(byte_ready) {
		byte_ready = 0;
		if(last_byte == 0xFE) continue;

		gotoxy(0,1);printf("0x%02x ", last_byte);
		if(noteOn)
			{
			toggleSpeaker(1, last_byte);
			noteOn = 0;
			}
		if(noteOff)
			{
			toggleSpeaker(0, last_byte);
			noteOff = 0;
			}

		if((last_byte & 0xF0) == 0x90 && noteOn == 0 && noteOff == 0) noteOn = 1;
		else if((last_byte & 0xF0) == 0x80 && noteOn == 0 && noteOff == 0) noteOff = 1;

		//outp(MIDI_DATA,last_byte);
		}
	if(kbhit()) break;
	}
    /*
    disable();
    setvect(IRQ_VECTOR, old_handler);
    enable();
    */
}
