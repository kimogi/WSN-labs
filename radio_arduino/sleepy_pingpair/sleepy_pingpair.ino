#include <SPI.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

RF24 radio(9,10);
const int role_pin = 7;
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
typedef enum { role_ping_out = 1, role_pong_back } role_e;
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};
role_e role;
typedef enum { wdt_16ms = 0, wdt_32ms, wdt_64ms, wdt_128ms, wdt_250ms, wdt_500ms, wdt_1s, wdt_2s, wdt_4s, wdt_8s } wdt_prescalar_e;
const short sleep_cycles_per_transmission = 4;
volatile short sleep_cycles_remaining = sleep_cycles_per_transmission;

void setup_watchdog(uint8_t prescalar);
void do_sleep(void);

void setup (void) {
	pinMode(role_pin, INPUT);
  
	if (digitalRead(role_pin))
    		role = role_ping_out;
  	else
    		role = role_pong_back;
  
	Serial.begin(57600);
  	printf_begin();
  	printf("ROLE: %s\n\r", role_friendly_name[role]);

  	if (role == role_ping_out)
    		setup_watchdog(wdt_1s);
  
	radio.begin();
	radio.setPayloadSize(sizeof(long));
	radio.setRetries(15, 15);
  	
	if (role == role_ping_out) {
    		radio.openWritingPipe(pipes[0]);
    		radio.openReadingPipe(1,pipes[1]);
  	} else {
    		radio.openWritingPipe(pipes[1]);
    		radio.openReadingPipe(1,pipes[0]);
  	}

  	radio.startListening();
  	radio.printDetails();
	delay(500);
}

void loop (void) {
	if (role == role_ping_out) {
    		
		radio.stopListening();
    		unsigned long time = millis();
    		printf("Now sending %lu... ", time);
    		delay(500);
		bool ok = radio.write(&time, sizeof(long));
	
		if (ok)
			printf("Ok... ");
		else
			printf("Failed...");    		

		radio.startListening();
		unsigned long started_waiting_at = millis();
    		bool timeout = false;
   		while (!radio.available() && !timeout)
      			if (millis() - started_waiting_at > 500)
        			timeout = true;
    		if (timeout) {
      			printf("Response timed out.\n\r");
    		} else {	
			unsigned long got_time;
               	 	radio.read(&got_time, sizeof(long));
                	printf("Got response %lu, round-trip delay: %lu\n\r", got_time, millis()-got_time);
		}
		delay(500);

		radio.powerDown();
		while (sleep_cycles_remaining)
                	do_sleep();
           	sleep_cycles_remaining = sleep_cycles_per_transmission;
             	radio.powerUp();
	}
  	if (role == role_pong_back) {
    		if (radio.available()) {
      			unsigned long got_time;
      			bool done = false;
      			while (!done) {
        			done = radio.read( &got_time, sizeof(long));
        			printf("Got payload %lu at %lu...", got_time, millis());
      			}
      			
			radio.stopListening();
      			radio.write(&got_time, sizeof(long));
      			printf("Sent response.\n\r");
      			radio.startListening();
		} 
  	}
}

void setup_watchdog(uint8_t prescalar) {
	prescalar = min(9, prescalar);
	uint8_t wdtcsr = prescalar & 7;
  	if (prescalar & 8)
		wdtcsr |= _BV(WDP3);

	MCUSR &= ~_BV(WDRF);
  	WDTCSR = _BV(WDCE) | _BV(WDE);
  	WDTCSR = _BV(WDCE) | wdtcsr | _BV(WDIE);
}

ISR(WDT_vect) {
	--sleep_cycles_remaining;
}

void do_sleep (void) {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	sleep_mode();
	sleep_disable();
}
