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

void set_punch_bit(uint8_t bit, uint8_t value) 
{
    uint8_t inverted = 0;
    if (bit == PUNCH_DOWN_ENABLE_BIT) {
        inverted = BITFLAG_PUNCH_ACTUATOR_DOWN;
    } else if (bit == PUNCH_UP_ENABLE_BIT) {
        inverted = BITFLAG_PUNCH_ACTUATOR_UP;
    }
   
    value = value ^ bit_istrue(settings.punch_actuator_invert_mask, inverted); 
    
    if(value) {
        PUNCH_PORT |= (1 << bit);
    } else {
        PUNCH_PORT &= ~(1<< bit);
    }
}


void punch_activate_actuator(int direction)
{
   if (direction == COMMAND_PUNCH_UNACTIVATED) {
     set_punch_bit(PUNCH_DOWN_ENABLE_BIT, 0);
     set_punch_bit(PUNCH_UP_ENABLE_BIT, 0);
   } else if (direction == COMMAND_PUNCH_DOWN) {
     set_punch_bit(PUNCH_UP_ENABLE_BIT, 0);
     set_punch_bit(PUNCH_DOWN_ENABLE_BIT, 1);
   } else if (direction == COMMAND_PUNCH_UP) {
     set_punch_bit(PUNCH_DOWN_ENABLE_BIT, 0);
     set_punch_bit(PUNCH_UP_ENABLE_BIT, 1);
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
    #error("unsupported cpu for punch machine, only arduino supported")
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


uint8_t get_punch_sensor_value(uint8_t bit) 
{
    volatile uint8_t v = PUNCH_SENSOR_PIN & (PUNCH_SENSOR_DOWN_MASK | PUNCH_SENSOR_UP_MASK);
    uint8_t value = bit_istrue(v, bit( bit));

    uint8_t inverted = 0;
    if (bit == PUNCH_SENSOR_DOWN_BIT) {
        inverted = BITFLAG_PUNCH_SENSOR_DOWN;
    } else if (bit == PUNCH_SENSOR_UP_BIT) {
        inverted = BITFLAG_PUNCH_SENSOR_UP;
    }

    if (bit_istrue(settings.punch_sensor_invert_mask , inverted) != 0)
    {
        if (value)
            return 1;
        return 0; 
    }
    return value == 0;
}



void punch_wait_sensor_state(int punchbittowait) {


     while(get_punch_sensor_value(punchbittowait) == 0) {
        // noop    
       	__asm__ __volatile__ (
		 "nop" "\n\t"
		 "nop" "\n\t"
		 "nop" "\n\t"
		 "nop"); //just waiting 4 cycles
        
         protocol_execute_realtime();

         if (sys.abort) { return; }

     }

}

void punch()
{

    if (sys.state == STATE_CHECK_MODE) { return ;}

    punch_stop();

    // wait for current awaited commands
    protocol_buffer_synchronize();
    // wait for the end of move
    do {
       protocol_execute_realtime();
       if (sys.abort) { return; }
    } while ( sys.state != STATE_IDLE );

	bit_true_atomic(sys_rt_exec_state, EXEC_FEED_HOLD); // Use feed hold for program pause.

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

    bit_false_atomic(sys_rt_exec_state, EXEC_FEED_HOLD); // Use feed hold for program pause.

}


