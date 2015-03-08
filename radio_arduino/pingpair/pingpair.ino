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
	radio.setPayloadSize(sizeof(long));

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

void loop(void) {
	if (role == role_ping_out) {
    		radio.stopListening();
    		unsigned long time = millis();
    		printf("Now sending %lu... ", time);
    		bool ok = radio.write( &time, sizeof(unsigned long) );
    
    		if (ok)
      			printf("ok...\n\r");
    		else
      			printf("failed.\n\r");

    		radio.startListening();
    		unsigned long started_waiting_at = millis();
    		bool timeout = false;
    		while (!radio.available() && !timeout)
      			if (millis() - started_waiting_at > 200)
        			timeout = true;
    		if (timeout) {
      			printf("Failed, response timed out.\n\r");
    		} else {
      			unsigned long got_time;
      			radio.read(&got_time, sizeof(unsigned long));
      			printf("Got response %lu, round-trip delay: %lu\n\r", got_time, millis()-got_time);
    		}
    		delay(1000);
  	}
  	if (role == role_pong_back) {
    		if (radio.available()) {
      			unsigned long got_time;
      			bool done = false;
      			while (!done) {
        			done = radio.read( &got_time, sizeof(unsigned long) );
        			printf("Got payload %lu... ", got_time);
				delay(20);
      			}
      			
			radio.stopListening();
      			radio.write( &got_time, sizeof(unsigned long) );
      			printf("Sent response.\n\r");
      			radio.startListening();
    		}
  	}
}
