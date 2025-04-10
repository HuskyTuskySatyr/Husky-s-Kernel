#include <stdint.h>

#ifndef TIME_H
#define TIME_H

// Time constants
#define PIT_COMMAND_PORT 0x43       // PIT command port
#define PIT_DATA_PORT 0x40          // PIT data port
#define PIT_DIVISOR 3600            // Divisor for a 1 Hz interrupt (1 tick per second)
#define TICKS_PER_SECOND 18         // Timer frequency for PIT (approx 18.2 Hz)

// Global variable to track ticks
extern uint32_t ticks;

// Function prototypes
void sleep(int duration);
void init_pit_timer();               // Initialize the PIT timer
void timer_interrupt_handler();       // Interrupt handler for the PIT
int get_time();                      // Get current time in ticks
int get_time_in_seconds();           // Get time in seconds
int get_time_in_minutes();           // Get time in minutes
int syscall_handler(int syscall_number, int arg); // Handle system calls

#endif // TIME_H
