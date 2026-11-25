#ifndef CODE_KEY_H_
#define CODE_KEY_H_

#include "zf_common_headfile.h"
extern uint8 menu_show_flag;
extern volatile uint16 menu_tick_30ms;
// 4个普通按键
#define KEY1_PIN    IO_PB2
#define KEY2_PIN    IO_PB3
#define KEY3_PIN    IO_PB4
#define KEY4_PIN    IO_P32

// 2个拨码开关
#define SWITCH1_PIN     IO_PB0
#define SWITCH2_PIN     IO_PB1

typedef enum
{
    KEY_1 = 0,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_NUM
} key_index_enum;

typedef enum
{
    KEY_NONE         = 0,
    KEY_SHORT_PRESS  = 1,
    KEY_LONG_PRESS   = 2
} key_state_enum;

// 函数声明
void key_init(void);
void key_scanner(void);
key_state_enum key_get_state(key_index_enum key);
void key_clear_all_state(void);

void switch_init(void);
uint8 switch1_get_state(void);
uint8 switch2_get_state(void);

void key_process(void);

#endif