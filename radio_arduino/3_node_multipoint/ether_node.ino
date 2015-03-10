#include <SPI.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

RF24 radio(9,10);
const int address_pins[3] = {5, 6, 7} ;
const uint64_t pipe = 0xF0F0F0F0E1LL;
typedef enum { wdt_16ms = 0, wdt_32ms, wdt_64ms, wdt_128ms, wdt_250ms, wdt_500ms, wdt_1s, wdt_2s, wdt_4s, wdt_8s };

void setup_watchdog(uint8_t prescalar);
short wait_cycles = 0;
volatile short wait_cycles_remaining = wait_cycles_remaining;

int address = 0;

void setup (void) {
	
	int i;
	for (i=0; i<3; i++) {
		pinMode(address_pins[i], INPUT);
	}  
	delay(100);
	for (i=0; i<3; i++) {
                if (!digitalRead(address_pins[i])) {
                        address = address_pins[i];
                }
        }

	Serial.begin(57600);
	Serial.println("Initializing...");
  	printf_begin();
  	printf("Node address: %d\n\r", address);

	wait_cycles = address-address_pins[0]+1;
	wait_cycles_remaining = wait_cycles;

	radio.begin();
	radio.setPayloadSize(sizeof(int));
	radio.setRetries(15, 15);

	setup_watchdog(wdt_1s);  	

    	radio.openReadingPipe(1, pipe);

  	radio.startListening();
  	radio.printDetails();
	delay(200);
}

bool waiting_message = false;
bool write_time = false;

void loop (void) {
	if (write_time) {
		 if (!radio.available()) {
                        radio.stopListening();
                        radio.openWritingPipe(pipe);

                        int i=0, dest_address=0, message=0; const char *ok;
                        for (i=0; i<3; i++) {
                                if (address_pins[i] != address) {
                                        dest_address = address_pins[i];
                                        printf("%d : Sending to  %d... ", address, dest_address);
                                        ok = radio.write(&dest_address, sizeof(int)) ? "Ok" : "Failed";
                                        printf("%s... \n\r", ok);
                                        
                                        message = random(10, 100);
                                        printf("%d : Sending message : %d... ", address, message);
                                        ok = radio.write(&message, sizeof(int)) ? "Ok" : "Failed";
                                        printf("%s... \n\r", ok);
                                }
                        }
                        radio.openReadingPipe(1, pipe);
                        radio.startListening();
			wait_cycles_remaining = wait_cycles;
			printf("\n\r");
                } else {
			printf("Collision detected. Wait random time...");
			wait_cycles_remaining = random(0, wait_cycles);
		}
		write_time = false;
	} else {
		if (radio.available()) {
        	        int payload;
        	        bool done = false;
        	        while (!done) {
        	                done = radio.read(&payload, sizeof(int));
        	                printf("%d : Got payload : %d ", address, payload);
        	        }
			if (payload == address || waiting_message) {
        	        	printf("... for me\n\r");
				waiting_message = !waiting_message;
			} else {
        	        	printf("...ignore\n\r");
        	   	}
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
	printf("\n\r");
	if (wait_cycles_remaining == 0) {
		write_time = true;
	} else {
        	wait_cycles_remaining--;
	}
}
