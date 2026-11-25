#include "key.h"

uint8 menu_show_flag = 0;
volatile uint16 menu_tick_30ms = 0;
extern void key_select(void);
extern void key_1(void);
extern void key_2(void);
extern void key_quit(void);
extern uint8 start;

// ========== 按键部分（只处理KEY1-KEY4） ==========
static const uint8 key_pin_map[KEY_NUM] = 
{
    KEY1_PIN,
    KEY2_PIN,
    KEY3_PIN,
    KEY4_PIN
};

static uint8 key_current_status[KEY_NUM];
static uint8 key_last_status[KEY_NUM];
static uint8 key_press_cnt[KEY_NUM];
static uint8 key_pressed_flag[KEY_NUM];
static key_state_enum key_event[KEY_NUM];

#define KEY_DEBOUNCE_THRESHOLD  5

void key_init(void)
{
    uint8 i;

    // 初始化4个按键
    for (i = 0; i < KEY_NUM; i++)
    {
        gpio_init(key_pin_map[i], GPI, 1, GPI_PULL_UP);
        key_current_status[i] = 1;
        key_last_status[i]    = 1;
        key_press_cnt[i]      = 0;
        key_pressed_flag[i]   = 0;
        key_event[i]          = KEY_NONE;
    }
    
    // 初始化2个拨码开关
    switch_init();
}

void key_scanner(void)
{
    uint8 i;

    for (i = 0; i < KEY_NUM; i++)
    {
        key_last_status[i] = key_current_status[i];
        key_current_status[i] = gpio_get_level(key_pin_map[i]);

        if (key_current_status[i] == 0)  // 按键按下（低电平）
        {
            if (key_pressed_flag[i] == 0)
            {
                key_press_cnt[i]++;
                if (key_press_cnt[i] >= KEY_DEBOUNCE_THRESHOLD)
                {
                    key_pressed_flag[i] = 1;
                    key_press_cnt[i] = 0;
                }
            }
        }
        else  // 按键释放（高电平）
        {
            if (key_pressed_flag[i] == 1)
            {
                key_event[i] = KEY_SHORT_PRESS;
                key_pressed_flag[i] = 0;
                key_press_cnt[i] = 0;
            }
            else
            {
                key_press_cnt[i] = 0;
            }
        }
    }
}

key_state_enum key_get_state(key_index_enum key)
{
    if (key >= KEY_NUM)
    {
        return KEY_NONE;
    }
    return key_event[key];
}

void key_clear_all_state(void)
{
    uint8 i;

    for (i = 0; i < KEY_NUM; i++)
    {
        key_event[i] = KEY_NONE;
    }
}

// ========== 拨码开关部分（独立处理） ==========
void switch_init(void)
{
    gpio_init(SWITCH1_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(SWITCH2_PIN, GPI, 1, GPI_PULL_UP);
}

uint8 switch1_get_state(void)
{
    return gpio_get_level(SWITCH1_PIN);  // 0=拨到ON，1=拨到OFF
}

uint8 switch2_get_state(void)
{
    return gpio_get_level(SWITCH2_PIN);  // 0=拨到ON，1=拨到OFF
}

// ========== 主处理函数 ==========
void key_process(void)
{
    menu_tick_30ms++;
    key_scanner();

    // ===== 按键处理（瞬时触发） =====
    if (key_get_state(KEY_4) == KEY_SHORT_PRESS)
    {
        key_select();
    }

    if (key_get_state(KEY_3) == KEY_SHORT_PRESS)
    {
        key_1();
    }

    if (key_get_state(KEY_2) == KEY_SHORT_PRESS)
    {
        key_2();
    }

    if (key_get_state(KEY_1) == KEY_SHORT_PRESS)
    {
        start++;
    }

    // ===== 拨码开关处理（状态保持） =====
    // 注意：这里每次都读取，所以menu_show_flag会实时跟随拨码开关状态
    if(switch1_get_state() == 0)  // 拨到ON
    {
        menu_show_flag = 1;
    }
    else  // 拨到OFF
    {
        menu_show_flag = 0;
    }
    
    // 拨码开关2的处理示例（按需使用）
     if(switch2_get_state() == 0)
     {
         // 拨码开关2拨到ON时的功能
			 huandao_stage=1;
     }
    
    key_clear_all_state();
}