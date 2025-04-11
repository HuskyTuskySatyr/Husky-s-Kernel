#ifndef PROGRAMS_H
#define PROGRAMS_H

const char* calc = 
    "x = 42\n"
    "if x < 10:\n"
    "    printk('Smthng went wrong, check your ram, or ram management in the system.')\n"
    "port = 0x3F8\n"
    "status = inw(port)\n"
    "printk('UART status: ')\n"
    "printk(status)\n";

#endif // PROGRAMS_H