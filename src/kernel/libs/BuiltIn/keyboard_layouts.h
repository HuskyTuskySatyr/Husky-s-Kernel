#ifndef KEYBOARD_LAYOUT_H
#define KEYBOARD_LAYOUT_H

#include <stdint.h>

// Supported keyboard layouts
typedef enum {
    LAYOUT_US,    // US QWERTY
    LAYOUT_DE,    // German QWERTZ
    LAYOUT_FR,    // French AZERTY
    LAYOUT_ES,    // Spanish QWERTY
    LAYOUT_RU,    // Russian JCUKEN
    LAYOUT_COUNT
} KeyboardLayout;

// Current active layout
extern KeyboardLayout current_layout;

// First convert scancodes to US layout ASCII (base conversion)
static const char scancode_to_us_ascii[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x10] = 'q', [0x11] = 'w', [0x12] = 'e',
    [0x13] = 'r', [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']', [0x1E] = 'a',
    [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g', [0x23] = 'h',
    [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x27] = ';', [0x28] = '\'',
    [0x29] = '`', [0x2B] = '\\', [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c',
    [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',',
    [0x34] = '.', [0x35] = '/', [0x39] = ' '
};

// Then convert US ASCII to other layouts
static const char us_ascii_to_layout[LAYOUT_COUNT][128] = {
    /* US QWERTY (identity mapping) */
    [LAYOUT_US] = {
        ['1'] = '1', ['2'] = '2', ['3'] = '3', ['4'] = '4', ['5'] = '5',
        ['6'] = '6', ['7'] = '7', ['8'] = '8', ['9'] = '9', ['0'] = '0',
        ['-'] = '-', ['='] = '=', ['q'] = 'q', ['w'] = 'w', ['e'] = 'e',
        ['r'] = 'r', ['t'] = 't', ['y'] = 'y', ['u'] = 'u', ['i'] = 'i',
        ['o'] = 'o', ['p'] = 'p', ['['] = '[', [']'] = ']', ['a'] = 'a',
        ['s'] = 's', ['d'] = 'd', ['f'] = 'f', ['g'] = 'g', ['h'] = 'h',
        ['j'] = 'j', ['k'] = 'k', ['l'] = 'l', [';'] = ';', ['\''] = '\'',
        ['`'] = '`', ['\\'] = '\\', ['z'] = 'z', ['x'] = 'x', ['c'] = 'c',
        ['v'] = 'v', ['b'] = 'b', ['n'] = 'n', ['m'] = 'm', [','] = ',',
        ['.'] = '.', ['/'] = '/', [' '] = ' '
    },

    /* German QWERTZ */
    [LAYOUT_DE] = {
        ['1'] = '1', ['2'] = '2', ['3'] = '3', ['4'] = '4', ['5'] = '5',
        ['6'] = '6', ['7'] = '7', ['8'] = '8', ['9'] = '9', ['0'] = '0',
        ['-'] = 'ß', ['='] = '´', ['q'] = 'q', ['w'] = 'w', ['e'] = 'e',
        ['r'] = 'r', ['t'] = 't', ['y'] = 'z', ['u'] = 'u', ['i'] = 'i',
        ['o'] = 'o', ['p'] = 'p', ['['] = 'ü', [']'] = '+', ['a'] = 'a',
        ['s'] = 's', ['d'] = 'd', ['f'] = 'f', ['g'] = 'g', ['h'] = 'h',
        ['j'] = 'j', ['k'] = 'k', ['l'] = 'l', [';'] = 'ö', ['\''] = 'ä',
        ['`'] = '^', ['\\'] = '#', ['z'] = 'y', ['x'] = 'x', ['c'] = 'c',
        ['v'] = 'v', ['b'] = 'b', ['n'] = 'n', ['m'] = 'm', [','] = ',',
        ['.'] = '.', ['/'] = '-', [' '] = ' '
    },
};

// Shifted characters for each layout
static const char shifted_layout[LAYOUT_COUNT][128] = {
    /* US QWERTY Shifted */
    [LAYOUT_US] = {
        ['1'] = '!', ['2'] = '@', ['3'] = '#', ['4'] = '$', ['5'] = '%',
        ['6'] = '^', ['7'] = '&', ['8'] = '*', ['9'] = '(', ['0'] = ')',
        ['-'] = '_', ['='] = '+', ['q'] = 'Q', ['w'] = 'W', ['e'] = 'E',
        ['r'] = 'R', ['t'] = 'T', ['y'] = 'Y', ['u'] = 'U', ['i'] = 'I',
        ['o'] = 'O', ['p'] = 'P', ['['] = '{', [']'] = '}', ['a'] = 'A',
        ['s'] = 'S', ['d'] = 'D', ['f'] = 'F', ['g'] = 'G', ['h'] = 'H',
        ['j'] = 'J', ['k'] = 'K', ['l'] = 'L', [';'] = ':', ['\''] = '"',
        ['`'] = '~', ['\\'] = '|', ['z'] = 'Z', ['x'] = 'X', ['c'] = 'C',
        ['v'] = 'V', ['b'] = 'B', ['n'] = 'N', ['m'] = 'M', [','] = '<',
        ['.'] = '>', ['/'] = '?', [' '] = ' '
    },

    /* German QWERTZ Shifted */
    [LAYOUT_DE] = {
        ['1'] = '!', ['2'] = '"', ['3'] = '§', ['4'] = '$', ['5'] = '%',
        ['6'] = '&', ['7'] = '/', ['8'] = '(', ['9'] = ')', ['0'] = '=',
        ['-'] = '?', ['='] = '`', ['q'] = 'Q', ['w'] = 'W', ['e'] = 'E',
        ['r'] = 'R', ['t'] = 'T', ['y'] = 'Z', ['u'] = 'U', ['i'] = 'I',
        ['o'] = 'O', ['p'] = 'P', ['['] = 'Ü', [']'] = '*', ['a'] = 'A',
        ['s'] = 'S', ['d'] = 'D', ['f'] = 'F', ['g'] = 'G', ['h'] = 'H',
        ['j'] = 'J', ['k'] = 'K', ['l'] = 'L', [';'] = 'Ö', ['\''] = 'Ä',
        ['`'] = '°', ['\\'] = '\'', ['z'] = 'Y', ['x'] = 'X', ['c'] = 'C',
        ['v'] = 'V', ['b'] = 'B', ['n'] = 'N', ['m'] = 'M', [','] = ';',
        ['.'] = ':', ['/'] = '_', [' '] = ' '
    },
};

#endif // KEYBOARD_LAYOUT_H