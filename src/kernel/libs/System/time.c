#include "time.h"
#include <stddef.h>
#include <stdint.h>
#include "../Drivers/kernel.h"

// Global variable to store the current tick count
uint32_t ticks = 0;

// Initialize the PIT timer to trigger interrupts at regular intervals
void init_pit_timer() {
    // Send the command to PIT to set the mode (mode 3: square wave generator)
    outb(PIT_COMMAND_PORT, 0x36);  // 0x36: 0b00110110, set mode 3 and select binary mode
    
    // Set the PIT divisor to generate 1-second interrupts
    outb(PIT_DATA_PORT, PIT_DIVISOR & 0xFF);    // Low byte of the divisor
    outb(PIT_DATA_PORT, PIT_DIVISOR >> 8);      // High byte of the divisor
}

// Interrupt handler to increment the tick count on each timer interrupt
void timer_interrupt_handler() {
    ticks++;  // Increment the global tick counter each time the PIT triggers an interrupt
}

// System call to get the current time in ticks
int get_time() {
    return ticks;  // Return the current time in ticks
}

// Convert the ticks to seconds (based on 18.2 Hz PIT frequency)
int get_time_in_seconds() {
    return ticks / TICKS_PER_SECOND;  // Convert ticks to seconds
}

// Convert the ticks to minutes
int get_time_in_minutes() {
    return get_time_in_seconds() / 60;  // Convert seconds to minutes
}

// Handle system calls. For now, we only handle time-related system calls.
int syscall_handler(int syscall_number, int arg) {
    switch (syscall_number) {
        case 1:  // SYS_GET_TIME
            return get_time();  // Return current time in ticks
        case 2:  // SYS_GET_TIME_IN_SECONDS
            return get_time_in_seconds();  // Return time in seconds
        case 3:  // SYS_GET_TIME_IN_MINUTES
            return get_time_in_minutes();  // Return time in minutes
        default:
            return -1;  // Invalid syscall
    }
}

void sleep(duration) {
    int time = get_time_in_seconds();
    while (get_time_in_seconds!=time+duration/100) {
    }
}