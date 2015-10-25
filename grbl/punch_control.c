/*
  punch_control.c - punch control methods
  Part of Grbl punch modification

  Copyright (c) 2015-2016 Freydiere P.
  Copyright (c) 2012-2015 Sungeun K. Jeon
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"

#define COMMAND_PUNCH_UNACTIVATED 0
#define COMMAND_PUNCH_DOWN 1
#define COMMAND_PUNCH_UP 2
void punch_activate_actuator(int direction)
{
   if (direction == COMMAND_PUNCH_UNACTIVATED) {
     PUNCH_PORT &= ~(1<<PUNCH_DOWN_ENABLE_BIT); // disable punch down
     PUNCH_PORT &= ~(1<<PUNCH_UP_ENABLE_BIT); // disable punch up
   } else if (direction == COMMAND_PUNCH_DOWN) {
     PUNCH_PORT &= ~(1<<PUNCH_UP_ENABLE_BIT); // disable punch up
     PUNCH_PORT |= (1<<PUNCH_DOWN_ENABLE_BIT);
   } else if (direction == COMMAND_PUNCH_UP) {
     PUNCH_PORT &= ~(1<<PUNCH_DOWN_ENABLE_BIT);
      PUNCH_PORT |= (1<<PUNCH_UP_ENABLE_BIT); // disable punch up
   }

}

void wait_a_bit() {
        unsigned int r = 4; 
        uint32_t i = 100000;
        while (i-- != 0){
       	__asm__ __volatile__ (
		"nop" "\n\t"
		"nop" "\n\t"
		"nop" "\n\t"
		"nop"); //just waiting 4 cycles
    }
}


void punch_init()
{    

  #ifndef CPU_MAP_ATMEGA328P
    #error("unsupported cpu for punch machine")
  #endif


  PUNCH_DDR |= (1<<PUNCH_DOWN_ENABLE_BIT); // Configure as output pin.
  PUNCH_DDR |= (1<<PUNCH_UP_ENABLE_BIT); // Configure as output pin.

  // configure the analog pins as input for sensor
  PUNCH_SENSOR_DDR = PUNCH_SENSOR_DDR & (PUNCH_SENSOR_DOWN_MASK | PUNCH_SENSOR_UP_MASK);
  punch_activate_actuator(COMMAND_PUNCH_UP); 
}



void punch_stop()
{
    punch_activate_actuator(COMMAND_PUNCH_UNACTIVATED);
}


void punch_wait_sensor_state(int punchbittowait) {

    uint8_t pin = (PUNCH_SENSOR_PIN & (PUNCH_SENSOR_DOWN_MASK | PUNCH_SENSOR_UP_MASK));

     while(bit_istrue(pin,bit(punchbittowait))) {
      // noop    
        	__asm__ __volatile__ (
		"nop" "\n\t"
		"nop" "\n\t"
		"nop" "\n\t"
		"nop"); //just waiting 4 cycles

         pin = (PUNCH_SENSOR_PIN & (PUNCH_SENSOR_DOWN_MASK | PUNCH_SENSOR_UP_MASK));
     }

}

void punch()
{

    punch_stop();

    // wait for the end of move
    do {
       protocol_execute_realtime();
       if (sys.abort) { return; }
    } while ( sys.state != STATE_IDLE );

    punch_activate_actuator(COMMAND_PUNCH_UNACTIVATED);

    punch_activate_actuator(COMMAND_PUNCH_DOWN);

    // wait_a_bit();

    punch_wait_sensor_state(PUNCH_SENSOR_DOWN_BIT);

    // wait_a_bit();

    punch_activate_actuator(COMMAND_PUNCH_UNACTIVATED);

    // wait_a_bit();
    
    // activate the punch up
    punch_activate_actuator(COMMAND_PUNCH_UP);

    // wait_a_bit();
    // activate the punch up, and release the down
    punch_wait_sensor_state(PUNCH_SENSOR_UP_BIT);

    // wait_a_bit();
}


