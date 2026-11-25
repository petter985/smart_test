/****************************************************************************************************
* 文件名称        menu.c
* 适用平台        STC32G144K246（Ai8052U），MDK FOR C251
* 功能说明        菜单框架 + Flash 断电保护实现
* 修改记录
* 2026-07-18      V1.1 集成 Flash 断电保护
* 2026-07-22      V1.2 Create_Menu_Number 增加 step 参数
****************************************************************************************************/

#include "menu.h"
#include "flash.h"
#include <string.h>

#define MENU_FLASH_INVALID  (0xFFFFFFFFUL)

#ifndef MENU_FLASH_BUF_SIZE
#define MENU_FLASH_BUF_SIZE (512)
#endif

static uint8 menu_flash_buf[MENU_FLASH_BUF_SIZE];

/*----------------------------------------------------------------------------------------------------
* 获取某种参数类型占用的字节数
----------------------------------------------------------------------------------------------------*/
uint8 menu_flash_get_size(MENU_KIND kind)
{
    switch (kind)
    {
        case int32_Box:  return (uint8)sizeof(int32);
        case uint32_Box: return (uint8)sizeof(uint32);
        case int16_Box:  return (uint8)sizeof(int16);
        case uint16_Box: return (uint8)sizeof(uint16);
        case int8_Box:   return (uint8)sizeof(int8);
        case uint8_Box:  return (uint8)sizeof(uint8);
        case float_Box:  return (uint8)sizeof(float);
        default:         return 0;
    }
}

/*----------------------------------------------------------------------------------------------------
* 创建菜单项（内部函数）
----------------------------------------------------------------------------------------------------*/
static void Creat_Menu_Item(Menu_Item* father, Menu_Item* me, const char nam[], void *dat, MENU_KIND kind)
{
    Menu_Item* p;

    if(father->kind != MENU_Folder) return;

    me->nam = nam;
    me->sons = 0;
    me->select = 0;

    me->first_son = NULL;
    me->next_brother = NULL;
    me->last_brother = NULL;

    me->dat = dat;
    me->kind = kind;

    me->father = father;

    me->flash_addr = MENU_FLASH_INVALID;
    me->action = NULL;
    me->step = 1.0f;

    if (father->sons == 0)
    {
        father->first_son = me;
    }
    else
    {
        p = father->first_son;
        while (p->next_brother != NULL)
        {
            p = p->next_brother;
        }
        p->next_brother = me;
        me->last_brother = p;
    }
    me->No = father->sons;
    father->sons++;
}

void Creat_Menu_Folder(Menu_Item* father, Menu_Item* me, const char nam[])
{
    Creat_Menu_Item(father, me, nam, NULL, MENU_Folder);
}

void Create_Menu_Number(Menu_Item* father, Menu_Item* me, const char nam[], void *dat, MENU_KIND kind, float step)
{
    Creat_Menu_Item(father, me, nam, dat, kind);
    me->step = step;
}

void Create_Menu_Action(Menu_Item* father, Menu_Item* me, const char nam[], void (*action)(void))
{
    Creat_Menu_Item(father, me, nam, NULL, MENU_Action);
    me->action = action;
}

/*================================== 以下为 Flash 断电保护内部实现 ==================================*/

static void menu_walk(Menu_Item *item, void (*cb)(Menu_Item *, void *), void *arg)
{
    Menu_Item *s;

    s = item->first_son;
    while (s != NULL)
    {
        cb(s, arg);
        if (s->sons > 0)
        {
            menu_walk(s, cb, arg);
        }
        s = s->next_brother;
    }
}

static void menu_layout_cb(Menu_Item *item, void *arg)
{
    uint32 *offset;
    uint8  size;

    offset = (uint32 *)arg;
    size = menu_flash_get_size(item->kind);

    if ((item->dat != NULL) && (size > 0))
    {
        item->flash_addr = MENU_FLASH_BASE_ADDR + MENU_FLASH_HEADER_SIZE + *offset;
        *offset += size;
    }
    else
    {
        item->flash_addr = MENU_FLASH_INVALID;
    }
}

static uint32 menu_layout(Menu_Item *root)
{
    uint32 offset;

    offset = 0;
    menu_walk(root, menu_layout_cb, &offset);
    return offset;
}

static void menu_pack_cb(Menu_Item *item, void *arg)
{
    uint8  *buf;
    uint8  *src;
    uint8  size;
    uint8  i;
    uint32 pos;

    if ((item->dat == NULL) || (item->flash_addr == MENU_FLASH_INVALID)) return;

    buf  = (uint8 *)arg;
    src  = (uint8 *)item->dat;
    size = menu_flash_get_size(item->kind);
    pos  = item->flash_addr - MENU_FLASH_BASE_ADDR;

    for (i = 0; i < size; i++)
    {
        buf[pos + i] = src[i];
    }
}

static void menu_unpack_cb(Menu_Item *item, void *arg)
{
    uint8  *buf;
    uint8  *dst;
    uint8  size;
    uint8  i;
    uint32 pos;

    if ((item->dat == NULL) || (item->flash_addr == MENU_FLASH_INVALID)) return;

    buf  = (uint8 *)arg;
    dst  = (uint8 *)item->dat;
    size = menu_flash_get_size(item->kind);
    pos  = item->flash_addr - MENU_FLASH_BASE_ADDR;

    for (i = 0; i < size; i++)
    {
        dst[i] = buf[pos + i];
    }
}

static void menu_sum_cb(Menu_Item *item, void *arg)
{
    uint32 *sum;
    uint8  *src;
    uint8  size;
    uint8  i;

    if ((item->dat == NULL) || (item->flash_addr == MENU_FLASH_INVALID)) return;

    sum  = (uint32 *)arg;
    src  = (uint8 *)item->dat;
    size = menu_flash_get_size(item->kind);

    for (i = 0; i < size; i++)
    {
        *sum += src[i];
    }
}

static void menu_sum_read_cb(Menu_Item *item, void *arg)
{
    uint32 *sum;
    uint8  temp[8];
    uint8  size;
    uint8  i;

    if ((item->dat == NULL) || (item->flash_addr == MENU_FLASH_INVALID)) return;

    sum  = (uint32 *)arg;
    size = menu_flash_get_size(item->kind);

    if (Flash_Read_Buff(item->flash_addr, temp, size) != FLASH_OK) return;
    for (i = 0; i < size; i++)
    {
        *sum += temp[i];
    }
}

static void menu_read_item_cb(Menu_Item *item, void *arg)
{
    uint8 size;

    if ((item->dat == NULL) || (item->flash_addr == MENU_FLASH_INVALID)) return;

    size = menu_flash_get_size(item->kind);
    Flash_Read_Buff(item->flash_addr, (uint8 *)item->dat, size);
    (void)arg;
}

static void menu_save_item_cb(Menu_Item *item, void *arg)
{
    uint8 size;

    if ((item->dat == NULL) || (item->flash_addr == MENU_FLASH_INVALID)) return;

    size = menu_flash_get_size(item->kind);
    Flash_Write_Buff(item->flash_addr, (uint8 *)item->dat, size);
    (void)arg;
}

FLASH_STA menu_load_from_flash(Menu_Item *root)
{
    uint32 total;
    uint32 sum;
    uint32 i;
    FLASH_STA sta;

    total = menu_layout(root);
    if (total == 0) return FLASH_NO_DATA;

    if ((total + MENU_FLASH_HEADER_SIZE) <= MENU_FLASH_BUF_SIZE)
    {
        sta = Flash_Read_Buff(MENU_FLASH_BASE_ADDR, menu_flash_buf,
                              total + MENU_FLASH_HEADER_SIZE);
        if (sta != FLASH_OK) return sta;

        if ((menu_flash_buf[0] != (uint8)(MENU_FLASH_MAGIC & 0xFF)) ||
            (menu_flash_buf[1] != (uint8)(MENU_FLASH_MAGIC >> 8)))  return FLASH_NO_DATA;
        if (menu_flash_buf[2] != MENU_FLASH_VERSION)                return FLASH_NO_DATA;

        sum = 0;
        for (i = MENU_FLASH_HEADER_SIZE; i < total + MENU_FLASH_HEADER_SIZE; i++)
        {
            sum += menu_flash_buf[i];
        }
        if (menu_flash_buf[3] != (uint8)(sum & 0xFF))               return FLASH_DATA_ERR;

        menu_walk(root, menu_unpack_cb, menu_flash_buf);
    }
    else
    {
        uint8 head[MENU_FLASH_HEADER_SIZE];

        sta = Flash_Read_Buff(MENU_FLASH_BASE_ADDR, head, MENU_FLASH_HEADER_SIZE);
        if (sta != FLASH_OK) return sta;
        if ((head[0] != (uint8)(MENU_FLASH_MAGIC & 0xFF)) ||
            (head[1] != (uint8)(MENU_FLASH_MAGIC >> 8)))              return FLASH_NO_DATA;
        if (head[2] != MENU_FLASH_VERSION)                            return FLASH_NO_DATA;

        sum = 0;
        menu_walk(root, menu_sum_read_cb, &sum);
        if (head[3] != (uint8)(sum & 0xFF))                           return FLASH_DATA_ERR;

        menu_walk(root, menu_read_item_cb, NULL);
    }

    return FLASH_OK;
}

FLASH_STA menu_save_to_flash(Menu_Item *root)
{
    uint32 total;
    uint32 sum;
    uint32 i;
    uint8  head[MENU_FLASH_HEADER_SIZE];
    FLASH_STA sta;

    total = menu_layout(root);
    if (total == 0) return FLASH_NO_DATA;

    sum = 0;
    menu_walk(root, menu_sum_cb, &sum);

    head[0] = (uint8)(MENU_FLASH_MAGIC & 0xFF);
    head[1] = (uint8)(MENU_FLASH_MAGIC >> 8);
    head[2] = MENU_FLASH_VERSION;
    head[3] = (uint8)(sum & 0xFF);

    if ((total + MENU_FLASH_HEADER_SIZE) <= MENU_FLASH_BUF_SIZE)
    {
        for (i = 0; i < MENU_FLASH_HEADER_SIZE; i++)
        {
            menu_flash_buf[i] = head[i];
        }
        menu_walk(root, menu_pack_cb, menu_flash_buf);
        sta = Flash_Write_Buff(MENU_FLASH_BASE_ADDR, menu_flash_buf, total + MENU_FLASH_HEADER_SIZE);
        return sta;
    }
    else
    {
        sta = Flash_Write_Buff(MENU_FLASH_BASE_ADDR, head, MENU_FLASH_HEADER_SIZE);
        if (sta != FLASH_OK) return sta;
        menu_walk(root, menu_save_item_cb, NULL);
        return FLASH_OK;
    }
}

void menu_erase_flash(Menu_Item *root)
{
    uint32 total;
    uint32 addr;
    uint32 end;

    total = menu_layout(root) + MENU_FLASH_HEADER_SIZE;
    end   = MENU_FLASH_BASE_ADDR + total;

    for (addr = MENU_FLASH_BASE_ADDR; addr < end; addr += MENU_FLASH_SECTOR_SIZE)
    {
        Flash_Iap_Erase_Sector(addr);
    }
}