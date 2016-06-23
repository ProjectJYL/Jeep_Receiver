/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Example for Getting Started with nRF24L01+ radios.
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two
 * different nodes.  Put one of the nodes into 'transmit' mode by connecting
 * with the serial monitor and sending a 'T'.  The ping node sends the current
 * time to the pong node, which responds by sending the value back.  The ping
 * node can then see how long the whole cycle took.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"



//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9, 10);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.
//

// The various roles supported by this sketch
typedef enum { role_ping_out = 1, role_pong_back } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};

// The role of the current running sketch
role_e role = role_pong_back;

#define LED13 13
#define JEEP_ID 2

typedef struct RF_buf {
  uint16_t JeepID;
  unsigned long timestamp;
  };

  //create the buf here
static RF_buf tx_buf; //transmit buffer
static RF_buf rx_buf; //receive buffer


void setup(void)
{
  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();
  printf("\n\rRF24/examples/GettingStarted/\n\r");
  printf("ROLE: %s\n\r", role_friendly_name[role]);
  printf("*** PRESS 'T' to begin transmitting to the other node\n\r");

  //
  // Setup and configure rf radio
  //

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15, 15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  //radio.setPayloadSize(8);

  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

  if ( role == role_ping_out )
  {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1, pipes[1]);
  }
  else
  {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
  }

  //
  // Start listening
  //

  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_2MBPS);
  radio.printDetails();
  tx_buf.JeepID = JEEP_ID; //assigned jeep id
}

void loop(void)
{
  //
  // Ping out role.  Repeatedly send the current time
  //
  if (role == role_ping_out)
  {
    // First, stop listening so we can talk.
    radio.stopListening();

    // Take the time, and send it.  This will block until complete
    //send timestamp and jeep name
    tx_buf.timestamp = millis();
    bool ok = radio.write( &tx_buf, sizeof(RF_buf) );
    //print the output
    if(ok)
    {
      printf("Data SENT! Jeep id: %d. Timestamp: %lu. ", tx_buf.JeepID, tx_buf.timestamp);
    }
    else{
      printf("FAILED to send the data :(. ");
    }
    
    // Now, continue listening. switch to listening mode
    radio.startListening();

    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 250 ) //increase timeout period
        timeout = true;

    
    // Describe the results
    if ( timeout ) //response timeout
    {
      printf("Failed, response timed out.\n\r");
    } 
    /*
    else //proceed to print the result received
    {
      char *long_str; //for display long int as char array
      long_str = (char *) malloc(10); 
      bool ok = radio.read(&rx_buf, sizeof(RF_buf) );       // Grab the response, compare, and send to debugging spew
      uint16_t response_time = millis() - tx_buf.timestamp; //calculate response time
      rx_buf.timestamp = millis(); //timestamp for receiving the response
//      clearDisplay(WHITE); //flash the LED13 write on display
//      //display the received message and additional info
//      setStr("Jeep ID:", 0, 0, BLACK);
//      setStr(itoa(rx_buf.JeepID, NULL ,10), 45, 0, BLACK);
//      setStr("Time: ", 0, 8, BLACK);
//      //use sprintf to output long int to a char array buffer then display it
//      sprintf(long_str, "%lu", rx_buf.timestamp);
//      setStr(long_str, 30, 8, BLACK);
//      //display response time
//      sprintf(long_str, "%d", response_time);
//      setStr("Elapsed: ", 0, 16, BLACK);
//      setStr(long_str, 50, 16, BLACK);
//      updateDisplay();
      if(ok){
         printf("Response RECEIVED! Jeep id: %d. Timestamp: %lu. \n\r", rx_buf.JeepID, rx_buf.timestamp);
      }
      else{
        printf("FAILED to receive the response. :(\n\r");
      }
      digitalWrite(LED13, HIGH);
      delay(200);
      digitalWrite(LED13, LOW);
    }
    */
    // Try again 1s later
    //delay(1000); //to allow faster role transition reduce the time. 
  }

  //
  // Pong back role.  Receive each packet, dump it out, and send it back
  //

  if ( role == role_pong_back )
  {
    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 250 ) //increase timeout period
        timeout = true;
    // if there is data ready
    if(!timeout)
    {
      // Dump the payloads until we've gotten everything
      bool done = false;
      while(!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read(&rx_buf, sizeof(RF_buf) );
        //write on display once
//        clearDisplay(WHITE);
//        setStr("Jeep ID:", 0, 0, BLACK);
//        setStr(itoa(rx_buf.JeepID, NULL ,10), 40, 0, BLACK);
//        setStr("Time: ", 0, 8, BLACK);
//        //use sprintf to output long int to a char array buffer then display it
//        sprintf(long_str, "%lu", rx_buf.timestamp);
//        setStr(long_str, 6, 8, BLACK);
//        updateDisplay();
        digitalWrite(LED13, HIGH); //flash the lED
        delay(200);
        digitalWrite(LED13, LOW);
        // Delay just a little bit to let the other unit and make the transition to receiver
        delay(20);
      }
      //let's print out the data received
      printf("Data RECEIVED! Jeep id: %d. Timestamp: %lu. \n\r", rx_buf.JeepID, rx_buf.timestamp);
      //stop listening and trasmit data back here
      radio.stopListening();
      //Send the final one back 
      bool ok = radio.write(&tx_buf, sizeof(RF_buf) ); //send back the response with tx_buf
      //check transmit status
      if(ok)
      {
        printf("Sent response Jeep: %d and timestamp: %lu", rx_buf.JeepID, rx_buf.timestamp);
      }
      else
      {
        printf("Can't send response. \n\r");
      }
      
      radio.startListening(); //start listening again
    }
  }

  //swtich role back and forth
  if(role == role_ping_out)
  {
    printf("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n\r");
    role = role_pong_back;
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);
  }else{
    printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n\r");
    role = role_ping_out;
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1, pipes[1]);
  }

  //
  // Change roles
  //

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' && role == role_pong_back )
    {
      printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n\r");

      // Become the primary transmitter (ping out)
      role = role_ping_out;
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1, pipes[1]);
    }
    else if ( c == 'R' && role == role_ping_out )
    {
      printf("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n\r");

      // Become the primary receiver (pong back)
      role = role_pong_back;
      radio.openWritingPipe(pipes[1]);
      radio.openReadingPipe(1, pipes[0]);
    }
  }
}

//char * convertLtoStr(unsigned long value)
//{
//  //max is 4,294,967,295
//  char * converted; //temp string for manipulation and return
//  converted = (char *)malloc(10);
//  //converted = converted + 9;
//  //start with most significant digit
//  //reduce one zero from the divider
//  //use a for loop
//  for(long divider = 1000000000, converted = converted+9 ; divider > 0; divider/10, converted--)
//  {
//    if(value/divider)
//    {
//      *converted = value/divider + 0x30;
//    }
//    else
//    {
//      *converted = 0x30;
//    }
//  }
//  return converted;
//}
//// vim:cin:ai:sts=2 sw=2 ft=cpp
