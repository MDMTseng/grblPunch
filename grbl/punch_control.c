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

typedef struct {
     void (*punch_ext_init)();
     void (*punch_ext_homing)();
     void (*punch_ext)();
     void (*punch_ext_stop)();
} punch_command_flavor ;

// object oriented punch command
// depending on configuration, the punch style may
// be adjust with a vtble referring internal specific 
// functions
//

// forward
void v_punch_init();
void v_punch_homing();
void v_punch();
void v_punch_stop();

punch_command_flavor PUNCH_ACTUATORS;

punch_command_flavor *PUNCH_CURRENT; 


//////////////////////////////////////////////////////////////////////
// generic methods

void set_punch_bit(uint8_t bit, uint8_t value) 
{
    if(value) {
        PUNCH_PORT |= (1 << bit);
    } else {
        PUNCH_PORT &= ~(1<< bit);
    }
}


//////////////////////////////////////////////////////////////////////
// 
void v_set_punch_bit_with_inverted(uint8_t bit, uint8_t value) 
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


void v_punch_activate_actuator(int direction)
{
   if (direction == COMMAND_PUNCH_UNACTIVATED) {
     v_set_punch_bit_with_inverted(PUNCH_DOWN_ENABLE_BIT, 0);
     v_set_punch_bit_with_inverted(PUNCH_UP_ENABLE_BIT, 0);
   } else if (direction == COMMAND_PUNCH_DOWN) {
     v_set_punch_bit_with_inverted(PUNCH_UP_ENABLE_BIT, 0);
     v_set_punch_bit_with_inverted(PUNCH_DOWN_ENABLE_BIT, 1);
   } else if (direction == COMMAND_PUNCH_UP) {
     v_set_punch_bit_with_inverted(PUNCH_DOWN_ENABLE_BIT, 0);
     v_set_punch_bit_with_inverted(PUNCH_UP_ENABLE_BIT, 1);
   }
}

void wait_a_bit() {
        uint32_t i = 100000;
        while (i-- != 0){
       	__asm__ __volatile__ (
		"nop" "\n\t"
		"nop" "\n\t"
		"nop" "\n\t"
		"nop"); //just waiting 4 cycles
    }
}


void v_punch_init()
{    
  PUNCH_DDR |= (1<<PUNCH_DOWN_ENABLE_BIT); // Configure as output pin.
  PUNCH_DDR |= (1<<PUNCH_UP_ENABLE_BIT); // Configure as output pin.

  // configure the analog pins as input for sensor
  PUNCH_SENSOR_DDR = PUNCH_SENSOR_DDR & (PUNCH_SENSOR_DOWN_MASK | PUNCH_SENSOR_UP_MASK);
  v_punch_activate_actuator(COMMAND_PUNCH_UP); 

}

void v_punch_homing() {
    // nothing to do for the moment  
}

void v_punch_stop()
{
    v_punch_activate_actuator(COMMAND_PUNCH_UNACTIVATED);
}


uint8_t v_get_punch_sensor_value(uint8_t thebit) 
{
    volatile uint8_t v = PUNCH_SENSOR_PIN & (PUNCH_SENSOR_DOWN_MASK 
                            | PUNCH_SENSOR_UP_MASK);
    uint8_t value = bit_istrue(v, bit( thebit));

    uint8_t inverted = 0;
    if (thebit == PUNCH_SENSOR_DOWN_BIT) {
        inverted = BITFLAG_PUNCH_SENSOR_DOWN;
    } else if (thebit == PUNCH_SENSOR_UP_BIT) {
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


void v_punch_wait_not_sensor_state(int punchbittowait) {

     while(v_get_punch_sensor_value(punchbittowait) != 0) {
        // noop    
       	__asm__ __volatile__ (
		 "nop" "\n\t"
		 "nop" "\n\t"
		 "nop" "\n\t"
		 "nop"); //just waiting 4 cycles
     }

}

void v_punch_wait_sensor_state(int punchbittowait) {

     while(v_get_punch_sensor_value(punchbittowait) == 0) {
        // noop    
       	__asm__ __volatile__ (
		 "nop" "\n\t"
		 "nop" "\n\t"
		 "nop" "\n\t"
		 "nop"); //just waiting 4 cycles
     }

}

void v_punch()
{
    // punch logic for actuator
    v_punch_activate_actuator(COMMAND_PUNCH_UNACTIVATED);

    v_punch_activate_actuator(COMMAND_PUNCH_DOWN);

    v_punch_wait_sensor_state(PUNCH_SENSOR_DOWN_BIT);

    v_punch_activate_actuator(COMMAND_PUNCH_UNACTIVATED);

    // activate the punch up
    v_punch_activate_actuator(COMMAND_PUNCH_UP);

    // activate the punch up, and release the down
    v_punch_wait_sensor_state(PUNCH_SENSOR_UP_BIT);
}

void v_punch_one_way_with_only_one_sensor() {

    // activate motor
    v_punch_activate_actuator(COMMAND_PUNCH_DOWN);

    // wait for the motor not up
    v_punch_wait_not_sensor_state(PUNCH_SENSOR_UP_BIT);

    // wait for the motor up again
    v_punch_wait_sensor_state(PUNCH_SENSOR_UP_BIT);

    // disable motor 
    v_punch_activate_actuator(COMMAND_PUNCH_UNACTIVATED);
}

//////////////////////////////////////////////////////////
// stepper control

void s_punch_init() {
    v_punch_init();
}

void s_punch_homing() {

}

void s_punch() {

     uint8_t value = 1;

     uint16_t micros = settings.punch_stepper_delay; // in micros
     uint16_t ticks = micros * 4;   // (micros / 1000000) / (1.0f / 16000000); 
     
     uint32_t delayLoop = ticks; // (uint32_t)ticks / 4 ; // 4 cycles wait
     uint32_t nbDemiPeriod = 0;
     uint16_t tickDecrementPer10SemiPeriods = 1;
     int start = 1;

     while (start >= 0) {
        while(v_get_punch_sensor_value(PUNCH_SENSOR_UP_BIT) == start) {
            set_punch_bit(PUNCH_STEPPER_CLK_BIT, value);

            // noop    
            uint32_t i = delayLoop;
            while (i-- != 0){
                __asm__ __volatile__ (
                 "nop" "\n\t"
                 "nop" "\n\t"
                 "nop" "\n\t"
                 "nop"); //just waiting 4 cycles
            }
            // reverse bit
            value = value ^ 1;

            if (start == 0) {
                nbDemiPeriod ++;
                if (nbDemiPeriod < 300 && ((nbDemiPeriod % 30) == 0) ) {
                    // adjust the delay Loop
                    //
                    delayLoop -= tickDecrementPer10SemiPeriods;
                }
            }
        }
        start --;
     }

}

void s_punch_stop() {
    
}

//////////////////////////////////////////////////////////
// external interface

void punch_init() {

  #ifndef CPU_MAP_ATMEGA328P
    #error("unsupported cpu for punch machine, only arduino supported")
  #endif

  // default definition, on actuator logic
  PUNCH_ACTUATORS 
      .punch_ext_init = v_punch_init;
  PUNCH_ACTUATORS 
      .punch_ext_homing = v_punch_homing;
  PUNCH_ACTUATORS 
      .punch_ext = v_punch;
  PUNCH_ACTUATORS 
      .punch_ext_stop = v_punch_stop;


  // vtable handling, without static init
  switch(settings.punch_mode) {
      case 1:
        PUNCH_ACTUATORS.punch_ext = 
           v_punch_one_way_with_only_one_sensor;
      break;
      case 10: // stepper
        PUNCH_ACTUATORS 
          .punch_ext_init = s_punch_init;
        PUNCH_ACTUATORS 
          .punch_ext_homing = s_punch_homing;
        PUNCH_ACTUATORS 
          .punch_ext = s_punch;
        PUNCH_ACTUATORS 
          .punch_ext_stop = s_punch_stop;
      break;
   }

   PUNCH_CURRENT = &PUNCH_ACTUATORS;

   // launch init
   PUNCH_CURRENT->punch_ext_init();
}

void punch_homing() {
    PUNCH_CURRENT->punch_ext_homing();
}

void punch() {
   if (sys.state == STATE_CHECK_MODE) { return ;}

    PUNCH_CURRENT->punch_ext_stop();

    // wait for current awaited commands
    protocol_buffer_synchronize();
    // wait for the end of move
    do {
       protocol_execute_realtime();
       if (sys.abort) { return; }
    } while ( sys.state != STATE_IDLE );

    PUNCH_CURRENT->punch_ext();
}

void punch_stop() {
    PUNCH_CURRENT->punch_ext_stop();
}
