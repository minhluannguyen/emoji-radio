/* x20-radio/remote.c */
/* Copyright (c) 2020 J. M. Spivey */

#include "microbian.h"
#include "hardware.h"
#include "lib.h"

#define GROUP 17
#define INIT_POS_X 2
#define INIT_POS_Y 2
#define PRESSED_TIME 2500000
#define BLINK_TIME 400

#ifdef UBIT_V2

void clock_init(void)
{
    /* Enable the cycle counter, part of the data watchpoint and trace module */
    SET_BIT(DEBUG_DEMCR, DEBUG_DEMCR_TRCENA);
    SET_BIT(DWT_CTRL, DWT_CTRL_CYCCNTENA);
}

#define clock_start()  DWT_CYCCNT = 0
#define clock_stop()   DWT_CYCCNT

#define FUDGE 5
#define MULT 1
#define FREQ 64
#endif

// Initialisation of the drawing variables
image receive_drawing =
    IMAGE(0,0,0,0,0,
          0,0,0,0,0,
          1,1,1,0,0,
          0,0,0,0,0,
          0,0,0,0,0);

image current_drawing =
    IMAGE(0,0,0,0,0,
          0,0,0,0,0,
          0,0,1,0,0,
          0,0,0,0,0,
          0,0,0,0,0);

image saved_drawing = 
    IMAGE(0,0,0,0,0,
          0,0,0,0,0,
          0,0,0,0,0,
          0,0,0,0,0,
          0,0,0,0,0);

// Initialisation of the cursor position
int posX = INIT_POS_X;
int posY = INIT_POS_Y;

int display_mode_on = 0;
int play_sound = 0;

/* Update the cursor position*/
void update_board_pos(int i, int j) {
    // Copy the saved state to the current drawing state
    memcpy(current_drawing, saved_drawing, sizeof(image));
    
    // Check if the cursor is out of bounds
    if (posX + i < 0 || posX + i > 4) {
        posX = 0;
    } else if (posY + j < 0 || posY + j > 4) {
        posY = 0;
    } else {
        posX += i;
        posY += j;
    }

    // Turn off when the saved state is on and vice versa
    if (image_is_set(posX, posY, saved_drawing)) {
        image_off(posX, posY, current_drawing);
    } else {
        image_set(posX, posY, current_drawing);
    }
}

/* Save the pixel position to the saved state */
void save_pixel_pos() {
    printf("Is pixel on: %d\n", image_is_set(posX, posY, saved_drawing));
    if (image_is_set(posX, posY, saved_drawing)) {
        image_off(posX, posY, saved_drawing);
    } else {
        image_set(posX, posY, saved_drawing);
    }
}

/* Play the sound when the receiver receives a packet */
void play_receive_sound() {
    while (1) {
        if (play_sound == 1) {
            speaker_plays_note(DO_5, QUARTER_NOTE);
            speaker_plays_note(RE_5, QUARTER_NOTE);
            speaker_plays_note(MI_5, QUARTER_NOTE);
            speaker_plays_note(FA_5, QUARTER_NOTE);
            play_sound = 0;
        }
    }
}

/* Receiver task */
void receiver_task(int dummy)
{
    byte buf[RADIO_PACKET];
    int n;

    while (1) {
        n = radio_receive(buf);
        printf("Received %d bytes\n", n);
        if (n == sizeof(image)) {
            printf("Packet received\n");

            // Copy the received packet to the receive_drawing
            memcpy(receive_drawing, buf, sizeof(image));

            // Turn on the display mode and play the sound
            display_mode_on = 1;
            play_sound = 1;
        }
    }
}

/* Handle the button presses and send the packet */
void sender_task(int dummy)
{
    // Initialise the GPIO pins for the buttons
    gpio_connect(BUTTON_A);
    gpio_connect(BUTTON_B);

    // Button pressed flags
    int button_a = 0;
    int button_b = 0;

    // Time variable
    unsigned time = 0;

    while (1) {
        //Two buttons pressed simultaneously to send the packet
        if (gpio_in(BUTTON_A) == 0 && gpio_in(BUTTON_B) == 0) 
        {
            printf("Press A and B\n");
            radio_send(saved_drawing, sizeof(saved_drawing));
        } else if (gpio_in(BUTTON_A) == 0) //Button A pressed
        {
            printf("Press A\n");
            if (button_a == 0)
            {
                clock_start();
                button_a = 1;
            }

            // Turn off the display mode when the button is pressed
            if (display_mode_on == 1) {
                display_mode_on = 0;
            }
        } else if (gpio_in(BUTTON_A) != 0 && button_a == 1) //Button A released
        {
            button_a = 0;
            time = clock_stop();
            time -= FUDGE;
            time /= FREQ;
            printf("Button A time: %d\n", time);

            // Save the pixel position when the button is pressed for a long time
            if (time > PRESSED_TIME) {
                printf("Long press\n");
                save_pixel_pos();
            }
            update_board_pos(0, 1);
        } else if (gpio_in(BUTTON_B) == 0) //Button B pressed
        {
            printf("Press B\n");

            if (button_b == 0)
            {
                clock_start();
                button_b = 1;
            }

            // Turn off the display mode when the button is pressed
            if (display_mode_on == 1) {
                display_mode_on = 0;
            }
        } else if (gpio_in(BUTTON_B) != 0 && button_b == 1) //Button B released
        {
            button_b = 0;
            time = clock_stop();
            time -= FUDGE;
            time /= FREQ;
            printf("Button B time: %d\n", time);

            // Save the pixel position when the button is pressed for a long time
            if (time > PRESSED_TIME) {
                printf("Long press\n");
                save_pixel_pos();
            }
            update_board_pos(1, 0);
        }

        // Delay to prevent the task from hogging the CPU
        timer_delay(200);
    }
}

/* Handle the display LEDs*/
void blink_task(int dummy)
{
    while (1) {

        // Display the current drawing when the display mode is off and the receive drawing when the display mode is on
        if (display_mode_on == 0) {
            display_show(current_drawing);
            timer_delay(BLINK_TIME);
            display_show(saved_drawing);
            timer_delay(BLINK_TIME);
        } else {
            display_show(receive_drawing);
            timer_delay(BLINK_TIME);
            display_show(blank);
            timer_delay(BLINK_TIME);
        }
    }
}

void init(void)
{
    // Init
    serial_init();
    radio_init();
    timer_init();
    clock_init();
    display_init();

    // Define the tasks
    start("Sender", sender_task, 0, STACK);
    start("Blinker", blink_task, 0, STACK);
    start("Receiver", receiver_task, 0, STACK);
    start("Sound", play_receive_sound, 0, STACK);
}