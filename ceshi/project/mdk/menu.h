#ifndef CODE_MENU_H_
#define CODE_MENU_H_

#include "zf_common_headfile.h"
#include "flash.h"

/* ==================== Flash 断电保护配置 ==================== */
#define MENU_FLASH_BASE_ADDR    0x0000     /* EEPROM起始地址（与STC-ISP设置一致） */
#define MENU_FLASH_MAGIC        0xAA55     /* 魔术字 */
#define MENU_FLASH_VERSION      0x02       /* 版本号 */
#define MENU_FLASH_HEADER_SIZE  4          /* 头部字节(magic+version) */
#define MENU_FLASH_SECTOR_SIZE  512        /* 扇区大小 */
/* ========================================================== */

typedef enum MENU_KIND {
    MENU_Folder = 0,
    int32_Box,
    uint32_Box,
    int16_Box,
    uint16_Box,
    int8_Box,
    uint8_Box,
    float_Box,
    MENU_Action
} MENU_KIND;

typedef struct Menu_Item {
    const char *nam;
    void *dat;
    MENU_KIND kind;

    uint8 No;
    uint8 sons;
    uint8 select;

    struct Menu_Item *father;
    struct Menu_Item *first_son;
    struct Menu_Item *next_brother;
    struct Menu_Item *last_brother;

    uint32 flash_addr;          /* EEPROM保存地址 */
    void (*action)(void);       /* 动作回调 */
    
    float step;                 /* 加减步长，每个参数独立配置 */
} Menu_Item;

/* 菜单创建接口 */
void Creat_Menu_Folder (Menu_Item *father, Menu_Item *me, const char nam[]);
void Create_Menu_Number(Menu_Item *father, Menu_Item *me, const char nam[], void *dat, MENU_KIND kind, float step);
void Create_Menu_Action(Menu_Item *father, Menu_Item *me, const char nam[], void (*action)(void));

/* Flash断电保护接口 */
FLASH_STA menu_load_from_flash(Menu_Item *root);
FLASH_STA menu_save_to_flash(Menu_Item *root);
void menu_erase_flash    (Menu_Item *root);
uint8 menu_flash_get_size(MENU_KIND kind);

#endif