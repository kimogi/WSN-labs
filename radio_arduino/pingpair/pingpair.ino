#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

RF24 radio(9,10);

const int role_pin = 7;
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
typedef enum { role_ping_out = 1, role_pong_back } role_e;
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};
role_e role;

void setup (void) {
	pinMode(role_pin, INPUT);

	if (!digitalRead(role_pin))
		role = role_ping_out;
	else
		role = role_pong_back;

	Serial.begin(57600);
	printf_begin();
	printf("ROLE: %s\n", role_friendly_name[role]);

	radio.begin();
	radio.setRetries(15,15);
	radio.setPayloadSize(sizeof(unsigned long));

	if (role == role_ping_out) {
		radio.openWritingPipe(pipes[0]);
    		radio.openReadingPipe(1,pipes[1]);
 	 } else {
    		radio.openWritingPipe(pipes[1]);
    		radio.openReadingPipe(1,pipes[0]);
  	}
	
	radio.startListening();
	radio.printDetails();
}

static unsigned long sent_count = 0;
static unsigned long rec_count = 0;
static unsigned long start_time = 0;
static unsigned long time = 0;

void loop (void) {
	if (start_time == 0) {
		start_time = millis();
		time = start_time;
	} else {
		time = millis();
	}
	if (time - start_time >= 10000) {
		printf("%d %d\n\r", sent_count, rec_count);
		while(true);
	} else {
		if (role == role_ping_out) {
			radio.stopListening();
	    		radio.write(&time, sizeof(unsigned long));
			sent_count++;		
	  	}
	  	if (role == role_pong_back) {
			if (radio.available()) {
	      			unsigned long got_time;
      				bool done = false;
      				while (!done) {
        				done = radio.read( &got_time, sizeof(unsigned long) );
      				}
      				rec_count++;
    			}
  		}
	}
}
