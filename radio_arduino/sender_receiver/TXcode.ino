#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
//{Write , Read}
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
int MyCounter=0;
int MyData=1234;

void setup(void)
{

  Serial.begin(57600);
  radio.begin();

  radio.setChannel(0x66);
  radio.setRetries(15,15);
  radio.setAutoAck(true);

  radio.stopListening();
  
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);

  
Serial.println("Starting in 30 seconds");
delay(30000);
Serial.println("Started TX");
}

void loop(void)
{
    while(MyCounter<1000)
    {
    radio.stopListening();
    MyCounter++;
    Serial.println(MyCounter);
    radio.write( &MyData, sizeof(int) );
    delay(10);
    radio.startListening();       
    delay(100);
    //this delay if for the buffer for
    //the radio and serial to clear
    };
}

