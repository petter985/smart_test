/*
 * function.c
 *
 *  Created on: 2026?3?22?
 *      Author: petter
 */

#include "function.h"
#include "math.h"
/* 
 * C90 ????????:????????????????????
 * ????:init_ok -> start -> ... -> image_show
 */
struct FLAG flag = {
    0,      /* init_ok      */
    0,      /* start        */
    0,      /* strategy1    */
    0,      /* strategy2    */
    0,      /* strategy3    */
    0,      /* strategy4    */
    0,      /* adjust_ok    */
    0,      /* t_ms         */
    0,      /* msg_collect  */
    0,      /* speed        */
    0,      /* direction    */
    0,      /* stop         */
    1       /* image_show   */
};

/************************************************
???: Buzzer_di
?  ?: ??????
?  ?: uint16 ms (?????)
???: void
************************************************/
void Buzzer_di(uint16 ms)
{
    // BUZZER_ON;    /* ???????????,????????? */
    //system_delay_ms(ms);
    //BUZZER_OFF;
    //system_delay_ms(ms);
}

/************************************************
???: Filter_first
?  ?: ??????
?  ?: int16 data (?????)
       int16 last_data (???????)
       float k (????, 0~1??)
???: int16 (??????)
************************************************/
int16 Filter_first(int16 dat, int16 last_data, float k)
{
    /* C90 ??: ???????????????? */
    int16 result;

    result = (int16)((1 - k) * dat + k * last_data);

    return result;
}

/************************************************
???: Limit_int
?  ?: ??????
?  ?: int a (??)
       int b (???)
       int c (??)
???: int (?????)
************************************************/
int Limit_int(int a, int b, int c)
{
    if (b >= a && b <= c) 
    {
        return b;
    }
    else if (b < a) 
    {
        return a;
    }
    else 
    {
        return c;
    }
}

int int_abs(int t) {
    if (t < 0)
        return -t;
    else
        return t;
}
