/****************************************************************************************************
* 文件名称        Mymenu.c
* 适用平台        STC32G144K246（Ai8052U），MDK FOR C251
* 功能说明        菜单树构建、显示、按键操作，集成 Flash 断电保护
* 修改记录
* 2026-07-18      V1.1 集成 Flash 断电保护
* 2026-07-22      V1.2 每个参数独立步长；select 进入文件夹；第一个元素向上返回上级
****************************************************************************************************/

#include "Mymenu.h"
#include "flash.h"
#include <string.h>

/* 在 Menu_Item m24; 后面加 */

#define H 16
/* 在 Menu_Item m24; 后面加 */
Menu_Item m19;   /* plan 切换项 */
Menu_Item head;
Menu_Item *key;

Menu_Item m1;
Menu_Item m3;
Menu_Item m4;
Menu_Item m5;
Menu_Item m6;
Menu_Item m7;
Menu_Item m8;
Menu_Item m9;
Menu_Item m10;
Menu_Item m11;
Menu_Item m12;
Menu_Item m13;
Menu_Item m14;
Menu_Item m15;
Menu_Item m16;
Menu_Item m17;
Menu_Item m18;
Menu_Item m19;
Menu_Item m20;
Menu_Item m21;
Menu_Item m22;
Menu_Item m23;
Menu_Item m24;

/* image 子菜单节点 */
Menu_Item m2_1;
Menu_Item m2_2;
Menu_Item m2_3;


uint8 key_flag = 0;
volatile uint8 menu_save_request = 0;
uint8  menu_flash_status = 0xFF;    /* 本次保存结果 */
uint8  menu_load_status  = 0xFF;    /* 开机读取结果 */
uint16 menu_save_count = 0;
static uint8  menu_dirty = 0;
static uint16 menu_dirty_tick = 0;

/*----------------------------------------------------------------------------------------------------
* Flash 动作项回调
----------------------------------------------------------------------------------------------------*/
static void menu_save_params(void)
{
    menu_save_to_flash(&head);
}

static void menu_load_params(void)
{
    menu_load_from_flash(&head);
}

static void menu_erase_params(void)
{
    menu_erase_flash(&head);
}

void menu_init(void)
{
    FLASH_STA menu_sta;
    
    /* 原有 head 初始化... */
    head.dat = NULL;
    head.father = NULL;
    head.first_son = NULL;
    head.last_brother = NULL;
    head.next_brother = NULL;
    head.kind = MENU_Folder;
    head.nam = "MENU";
    head.No = 0;
    head.sons = 0;
    head.select = 0;
    head.flash_addr = 0;
    head.action = NULL;
    head.step = 1.0f;

    /* ========== 主菜单 ========== */
    Creat_Menu_Folder(&head, &m1, "menu");

    /* 原有参数项... */
    //Create_Menu_Number(&head, &m3,  "baoguang",    &MT9V03X_EXP_TIME_DEF, int16_Box, 5);
    
	Create_Menu_Number(&head, &m4,  "D",           &kd_m,                  float_Box, 100.0f);
	Create_Menu_Number(&head, &m6,  "Kp_m",        &kp_m,                  float_Box, 5.0f);
    //Create_Menu_Number(&head, &m7,  "wandao_p_zengyi",   &wandao_p_zengyi,             float_Box, 5);
    Create_Menu_Number(&head, &m8,  "beid_spk",   &beid_spk,             float_Box, 10);
    

    Create_Menu_Number(&head, &m11, "wan_p",       &wandao_p_zengyi,       float_Box, 1);
    Create_Menu_Number(&head, &m12, "zhi_p",       &zhidao_p_yizhi,        float_Box, 1.0f);
    Create_Menu_Number(&head, &m13, "else_p",      &else_p_yizhi,          float_Box, 1.0f);
    //Create_Menu_Number(&head, &m16, "ruwan_speed", &ruwan_speed,           float_Box, 2);
    Create_Menu_Number(&head, &m17, "huandao_speed",&huandao_speed,        float_Box, 2);
    Create_Menu_Number(&head, &m18, "else_speed",  &else_speed,            float_Box, 2);

    /* ========== Flash 参数恢复 ========== */
    Flash_Init(system_clock);                       /* 必须在 clock_init 之后 */

    menu_sta = menu_load_from_flash(&head);
    menu_load_status = (uint8)menu_sta;

    if (menu_sta != FLASH_OK)
    {
        menu_sta = menu_save_to_flash(&head);       /* 首次上电或数据损坏：写入默认参数 */
    }
    menu_flash_status = (uint8)menu_sta;
    menu_save_count = 0;

    key = head.first_son;
}

void menu_flash_task(void)
{
    static uint8 last_show = 0;

    /* 关闭菜单拨码开关时，把尚未确认的编辑落盘 */
    if (last_show && !menu_show_flag)
    {
        key->select = 0;
        menu_save_request = 1;
    }
    last_show = menu_show_flag;

    /* 编辑后 1.2s 内没有新按键就自动保存，防止没退出编辑就断电 */
    if (menu_dirty && ((uint16)(menu_tick_30ms - menu_dirty_tick) >= 40))
    {
        menu_dirty = 0;
        menu_save_request = 1;
    }

    if (!menu_save_request) return;

    menu_save_request = 0;
    menu_flash_status = (uint8)menu_save_to_flash(&head);
    if (menu_flash_status == FLASH_OK)
    {
        menu_save_count++;
    }
}

void show_number(void)
{
    int i;
    Menu_Item* h = key->father;
    Menu_Item* s = h->first_son;

    for(i = 0; i < h->sons; i++)
    {
        if(s->select)
            ips200_show_char(110, i * H + 10, '*');
        else
            ips200_show_char(110, i * H + 10, ' ');

        switch(s->kind)
        {
            case int32_Box:
                ips200_show_int32(120, i * H + 10, *(int32 *)s->dat, 10);
                break;
            case uint32_Box:
                ips200_show_int32(120, i * H + 10, (int32)(*(uint32 *)s->dat), 10);
                break;
            case int16_Box:
                ips200_show_int16(120, i * H + 10, *(int16 *)s->dat);
                break;
            case uint16_Box:
                ips200_show_uint16(120, i * H + 10, *(uint16 *)s->dat);
                break;
            case int8_Box:
                ips200_show_int8(120, i * H + 10, *(int8 *)s->dat);
                break;
            case uint8_Box:
                ips200_show_uint8(120, i * H + 10, *(uint8 *)s->dat);
                break;
            case float_Box:
                ips200_show_float(120, i * H + 10, *(float *)s->dat, 4,  4);
                break;
            case MENU_Action:
                ips200_show_string(120, i * H + 10, "[run]");
                break;
            default:
                break;
        }
        s = s->next_brother;
    }
}

void show_key(void)
{
    int i;
    Menu_Item* h = key->father;
    Menu_Item* s = h->first_son;

    for(i = 0; i < h->sons; i++)
    {
        if(s == key)
        {
            ips200_show_string(0, i * H + 10, "->");
        }
        else
        {
            ips200_show_string(0, i * H + 10, "  ");
        }
        s = s->next_brother;
    }
}

static void show_flash_status(void)
{
    ips200_show_string(17, 230, "L");
    if (menu_load_status == FLASH_OK)
    {
        ips200_show_string(25, 230, "OK ");
    }
    else if (menu_load_status == FLASH_NO_DATA)
    {
        ips200_show_string(25, 230, "NEW");
    }
    else
    {
        ips200_show_string(25, 230, "ERR");
    }

    ips200_show_string(57, 230, "S");
    ips200_show_int32(65, 230, (int32)menu_save_count, 3);

    if (menu_flash_status == FLASH_OK)
    {
        ips200_show_string(97, 230, "OK ");
    }
    else if (menu_flash_status == FLASH_NO_DATA)
    {
        ips200_show_string(97, 230, "NEW");
    }
    else
    {
        ips200_show_string(97, 230, "ERR");
    }
}

void menu_show(void)
{
    int i;
    Menu_Item* h;
    Menu_Item* s;

    if (key == NULL)
    {
        key = &head;
    }

    h = key->father;
    s = h->first_son;

    for(i = 0; i < h->sons; i++)
    {
        ips200_show_string(17, i * H + 10, s->nam);
        s = s->next_brother;
    }
    show_key();
    show_number();
    show_flash_status();
}

void key_down(void)
{
    if(key->next_brother != NULL)
        key = key->next_brother;
}

/*====================================================================================================
* 光标在第一个元素按向上 = 返回上级菜单
====================================================================================================*/
void key_up(void)
{
    if(key->last_brother != NULL)
        key = key->last_brother;
    else
        key_quit();
}

void key_enter(void)
{
    if(key->sons > 0)
		{
        key = key->first_son;
			 ips200_clear();
		}
}

void key_quit(void)
{
    if(key->father->father != NULL)
		{
        key = key->father;
			 ips200_clear();
		}
}

/*====================================================================================================
* 核心修改：文件夹按 select 进入子菜单
====================================================================================================*/
void key_select(void)
{
    /* 动作项：执行回调 */
    if(key->kind == MENU_Action)
    {
        if(key->action != NULL)
            key->action();
        return;
    }

    /* 文件夹：有子节点就进入子菜单 */
    if(key->kind == MENU_Folder)
    {
        if(key->sons > 0){
            key = key->first_son;
            ips200_clear();
        }
        return;
    }

    /* 数值项：切换编辑模式（0<->1） */
    key->select = !key->select;

    /* 退出编辑模式（select 从 1 变 0） */
    if(!key->select)
    {
        /* 退出编辑：请求主循环写 Flash（不能在定时器中断里擦写） */
        menu_save_request = 1;
        menu_dirty = 0;
    }
}

/*====================================================================================================
* 核心修改：使用 key->step 代替固定值
====================================================================================================*/
void key_plus(void)
{
    switch(key->kind)
    {
        case int32_Box:
            *(int32 *)key->dat += (int32)key->step;
            break;
        case uint32_Box:
            *(uint32 *)key->dat += (uint32)key->step;
            break;
        case int16_Box:
            *(int16 *)key->dat += (int16)key->step;
            break;
        case uint16_Box:
            *(uint16 *)key->dat += (uint16)key->step;
            break;
        case int8_Box:
            *(int8 *)key->dat += (int8)key->step;
            break;
        case uint8_Box:
            *(uint8 *)key->dat += (uint8)key->step;
            break;
        case float_Box:
            *(float *)key->dat += key->step;
            break;
        default:
            break;
    }

    menu_dirty = 1;
    menu_dirty_tick = menu_tick_30ms;
}

void key_sub(void)
{
    switch(key->kind)
    {
        case int32_Box:
            *(int32 *)key->dat -= (int32)key->step;
            break;
        case uint32_Box:
            *(uint32 *)key->dat -= (uint32)key->step;
            break;
        case int16_Box:
            *(int16 *)key->dat -= (int16)key->step;
            break;
        case uint16_Box:
            *(uint16 *)key->dat -= (uint16)key->step;
            break;
        case int8_Box:
            *(int8 *)key->dat -= (int8)key->step;
            break;
        case uint8_Box:
            *(uint8 *)key->dat -= (uint8)key->step;
            break;
        case float_Box:
            *(float *)key->dat -= key->step;
            break;
        default:
            break;
    }

    menu_dirty = 1;
    menu_dirty_tick = menu_tick_30ms;
}

void key_1(void)
{
    if(key->select == 0)
        key_up();
    else
        key_plus();
}

void key_2(void)
{
    if(key->select == 0)
        key_down();
    else
        key_sub();
}