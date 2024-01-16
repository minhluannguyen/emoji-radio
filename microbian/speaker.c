#include "microbian.h"
#include "hardware.h"

unsigned int period_us[] = {3822, 3405, 3033, 2863, 2551, 2272, 2024, 1911,
                             1702, 1516, 1431, 1275, 1136, 1012, 955};

/**
 * @brief Plays a tone with a frequency equal to the inverse of the period
 * corresponding to the indicated note "n", for t_ms milliseconds.
 *
 * @param n note to play
 * @param t_ms the duration of the note
 *
 */
void speaker_plays_note(note_t n, int t_ms)
{
    int p = period_us[n] / 2;
    int t_cc = 1000 * t_ms / (p * 2);

    for (int i = 0; i < t_cc; i++)
    {
        gpio_out(SPEAKER, 1);
        timer_delay(p);
        gpio_out(SPEAKER, 0);
        timer_delay(p);
    }
}
