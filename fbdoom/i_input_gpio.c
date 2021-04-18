//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2021 Stefan Krupop
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DOOM keyboard input via GPIO pins
//


#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "config.h"
#include "deh_str.h"
#include "doomtype.h"
#include "doomkeys.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_swap.h"
#include "i_timer.h"
#include "i_video.h"
#include "i_scale.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

int vanilla_keyboard_mapping = 1;

#define GPIO_KEY_COUNT		5
#define GPIO_PRESSED_VALUE	0

uint16_t GPIO_PINS[GPIO_KEY_COUNT] = {164, 166, 165, 168, 169};
char     GPIO_KEYS[GPIO_KEY_COUNT] = {KEY_UPARROW, KEY_DOWNARROW, KEY_LEFTARROW, KEY_RIGHTARROW, KEY_ENTER/*KEY_FIRE*/};

FILE* _gpioHandles[GPIO_KEY_COUNT];
uint8_t _gpioLastState[GPIO_KEY_COUNT];

void I_GetEvent(void)
{
    event_t event;

    for (int i = 0; i < GPIO_KEY_COUNT; ++i) {
        char buffer[1];
        if (_gpioHandles[i] == NULL) continue;
        rewind(_gpioHandles[i]);
        fread(&buffer[0], sizeof(char), 1, _gpioHandles[i]);
        uint8_t gpioState = (buffer[0] == '1');

        if (gpioState != _gpioLastState[i]) {

            if (gpioState == GPIO_PRESSED_VALUE) {
                event.type = ev_keydown;
                event.data1 = GPIO_KEYS[i];
                event.data2 = 0;

                D_PostEvent(&event);
                // As we only have one non-directional key, fake fire and use keys for each enter
                if (event.data1 == KEY_ENTER) {
                    event.data1 = KEY_FIRE;
                    D_PostEvent(&event);
                    event.data1 = KEY_USE;
                    D_PostEvent(&event);
                }
            } else {
                event.type = ev_keyup;
                event.data1 = GPIO_KEYS[i];
                event.data2 = 0;

                D_PostEvent(&event);
                // As we only have one non-directional key, fake fire and use keys for each enter
                if (event.data1 == KEY_ENTER) {
                    event.data1 = KEY_FIRE;
                    D_PostEvent(&event);
                    event.data1 = KEY_USE;
                    D_PostEvent(&event);
                }
            }

            _gpioLastState[i] = gpioState;
        }
    }
}

void I_InitInput(void) {
    printf("Trying to open %d GPIOs...\n", GPIO_KEY_COUNT);
    fflush(stdout);
    for (int i = 0; i < GPIO_KEY_COUNT; ++i) {
        char gpioFileName[32];
        sprintf(gpioFileName, "/sys/class/gpio/gpio%d/value", GPIO_PINS[i]);
        if ((_gpioHandles[i] = fopen(gpioFileName, "rb")) == NULL) {
            printf("Could not open GPIO file %s: %s\n", gpioFileName, strerror(errno));
            fflush(stdout);
            return;
        }

        setvbuf(_gpioHandles[i], NULL, _IONBF, 0); // Disable buffering to get current value
        char buffer[1];
        fseek(_gpioHandles[i] , 0 , SEEK_SET);
        fread(&buffer[0], sizeof(char), 1, _gpioHandles[i]);
        _gpioLastState[i] = (buffer[0] == '1');
    }
    printf("GPIOs initialized\n");
    fflush(stdout);
}

