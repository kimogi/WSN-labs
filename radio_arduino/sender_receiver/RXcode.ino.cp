#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
//{Write , Read}
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
int MyCounter=0;
int MyData=0;

void setup(void)
{

  Serial.begin(57600);

  radio.begin();

  radio.setChannel(0x66);
  radio.setRetries(15,15);
  radio.setAutoAck(true);

  radio.stopListening();
  
   radio.openWritingPipe(pipes[0]);
   radio.openReadingPipe(1,pipes[1]);
  
   radio.startListening();    

    Serial.println("Starting in 15 seconds");
    delay(15000);
    Serial.println("Started RX");
}

void loop(void)
{

    // if there is data ready
    if ( radio.available() )
    {
      MyData=0;
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( &MyData, sizeof(int) );	
      }
      if (MyData==1234)
      {MyCounter++;
      Serial.println(MyCounter);
      };
    }
   radio.stopListening();
   radio.startListening();
   delay(10);
}

