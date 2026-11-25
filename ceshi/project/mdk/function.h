/*
 * function.h
 *
 *  Created on: 2026?3?22?
 *      Author: petter
 */

#ifndef CODE_FUNCTION_H_
#define CODE_FUNCTION_H_

#include "zf_common_headfile.h"

/* ???: ???? */
#define Limit_Min_Max(data, min, max) (((data) > (max)) ? (max) : (((data) < (min)) ? (min) : (data)))

/* ???: ????? (??: ?????????,??if/else???) */
#define BUZZER_ON       gpio_set_level(BUZZER_PIN, 1)
#define BUZZER_OFF      gpio_set_level(BUZZER_PIN, 0)

/* ????????? */
struct FLAG
{
    int8 init_ok;            // ???????
    int8 start;              // ???????
    int8 strategy1;          // ??1??
    int8 strategy2;          // ??2??
    int8 strategy3;          // ??3??
    int8 strategy4;          // ??4??
    int8 adjust_ok;          // ????????
    int8 t_ms;               // ??????
    int8 msg_collect;        // ??????
    int8 speed;              // ????
    int8 direction;          // ????
    int8 stop;               // ????
    int8 image_show;         // ??????
};

extern struct FLAG flag;

void Buzzer_di(uint16 ms);
int16 Filter_first(int16 dat, int16 last_data, float k);
int Limit_int(int a, int b, int c);
int int_abs(int t);
#endif /* CODE_FUNCTION_H_ */
