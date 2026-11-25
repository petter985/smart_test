/****************************************************************************************************
* 文件名称        flash.c
* 适用平台        STC32G144K246（Ai8052U），MDK FOR C251
* 功能说明        内部 Flash(EEPROM) 底层驱动，C90(C98) 语法
*
* 实现依据        STC32G144K246 系列技术手册 第 19 章（IAP/EEPROM）：
*                1. IAP 地址为 EEPROM 相对地址，从 0 开始（物理位置固定在 FC:2800h 起，19.4 节）
*                2. 写操作只能把 1 写成 0，改写前必须先擦除所在扇区，每扇区 512 字节（19 章开头）
*                3. IAP 命令执行完成前 CPU 不会继续执行程序，擦除后无需软件延时（19.2 节）
*                4. 主循环和中断中都可能操作 EEPROM 时，操作过程必须关总中断（19.2 节）
*                5. IAP_TPS = 系统频率 / 1000000，四舍五入（19.1 节）
*
* 注意            本文件自带 IAP 寄存器定义以保持独立，请勿在本文件中再包含 stc32g144k246.h，
*                否则寄存器会重复定义。
*
* 修改记录
* 日期            备注
* 2026-07-18      V1.0 首次编写
****************************************************************************************************/

#include "flash.h"
#include <intrins.h>                                /* _nop_() 声明 */

/*---------------------------------- IAP 相关寄存器 ----------------------------------*/
/* 寄存器地址见手册 19.3 节 */

sfr  IAP_DATA  = 0xC2;                              /* IAP 数据寄存器 */
sfr  IAP_ADDRH = 0xC3;                              /* IAP 地址寄存器 ADDR[15:8] */
sfr  IAP_ADDRL = 0xC4;                              /* IAP 地址寄存器 ADDR[7:0] */
sfr  IAP_CMD   = 0xC5;                              /* IAP 命令寄存器 */
sfr  IAP_TRIG  = 0xC6;                              /* IAP 触发寄存器 */
sfr  IAP_CONTR = 0xC7;                              /* IAP 控制寄存器：IAPEN SWBS SWRST CMD_FAIL SWBS2 */
sfr  IAP_TPS   = 0xF5;                              /* IAP 等待时间控制寄存器 */
sfr  IAP_ADDRE = 0xF6;                              /* IAP 扩展地址寄存器 ADDR[23:16]，复位值 0xFF */
sfr  IE        = 0xA8;                              /* 中断使能寄存器 */
sbit EA        = IE ^ 7;                            /* 总中断使能位 */

/*---------------------------------- 内部常量 ----------------------------------*/

#define IAP_CMD_READ    0x01                        /* 单字节读取，手册 19.3.4 */
#define IAP_CMD_PROG    0x02                        /* 单字节编程 */
#define IAP_CMD_ERASE   0x03                        /* 扇区擦除，512 字节 */

#define IAP_EN          0x80                        /* IAP_CONTR 的 IAPEN 位 */
#define IAP_FLAG_FAIL   0x10                        /* IAP_CONTR 的 CMD_FAIL 位（B4，需软件清零） */

#define FLASH_HEAD0     0x5A                        /* 参数记录帧头第 1 字节 */
#define FLASH_HEAD1     0xA5                        /* 参数记录帧头第 2 字节 */

/* 扇区“读出-修改-写回”的暂存区，静态分配，注意本驱动不可重入（不能在中断和主循环里同时调用） */
static unsigned char flash_sector_buf[FLASH_SECTOR_SIZE];

/*----------------------------------------------------------------------------------------------------
* 函数功能    IAP 触发：先送 5Ah 再送 A5h
* 说明        送完 A5h 后命令立即启动，CPU 等待 IAP 完成后才继续执行；
*            多级流水线内核，官方建议触发后补 4 个 NOP
----------------------------------------------------------------------------------------------------*/
static void Flash_Trig(void)
{
    IAP_TRIG = 0x5A;
    IAP_TRIG = 0xA5;
    _nop_();
    _nop_();
    _nop_();
    _nop_();
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    检查并清除 CMD_FAIL 失败标志
* 返回参数    FLASH_OK / FLASH_FAIL
* 说明        清标志时 IAP_CONTR 只能写 0x80：保持 IAPEN=1 且 SWBS/SWRST 必须为 0，
*            否则会触发软件复位（手册 19.3.6）
----------------------------------------------------------------------------------------------------*/
static FLASH_STA Flash_Check(void)
{
    if (IAP_CONTR & IAP_FLAG_FAIL)
    {
        IAP_CONTR = IAP_EN;
        return FLASH_FAIL;
    }
    return FLASH_OK;
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    设置 24 位 IAP 目标地址（EEPROM 相对地址）
* 说明        IAP_ADDRE 复位值为 0xFF，每次操作都必须重新写入
----------------------------------------------------------------------------------------------------*/
static void Flash_Set_Addr(unsigned long addr)
{
    IAP_ADDRL = (unsigned char)(addr);
    IAP_ADDRH = (unsigned char)(addr >> 8);
    IAP_ADDRE = (unsigned char)(addr >> 16);
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    初始化 Flash(IAP) 功能
* 参数说明    sysclk      系统时钟频率，单位 Hz（可填逐飞库的 system_clock 或 SYSTEM_CLOCK_96M 等）
* 说明        按手册 19.1 设置 IAP_TPS = 主频(MHz) 四舍五入，必须在时钟初始化之后调用
----------------------------------------------------------------------------------------------------*/
void Flash_Init(unsigned long sysclk)
{
    unsigned long tps;

    tps = (sysclk + 500000UL) / 1000000UL + 1UL;    /* 四舍五入后加 1，留足 IAP 等待时间 */
    if (tps > 255UL)
    {
        tps = 255UL;
    }
    IAP_TPS   = (unsigned char)tps;
    IAP_CONTR = IAP_EN;                             /* 使能 EEPROM 操作，同时清 CMD_FAIL */
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    关闭 Flash(IAP) 功能，防止误操作
----------------------------------------------------------------------------------------------------*/
void Flash_Close(void)
{
    IAP_CONTR = 0x00;
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    IAP 读一个字节
* 参数说明    addr        EEPROM 相对地址
* 返回参数    读到的字节
----------------------------------------------------------------------------------------------------*/
unsigned char Flash_Iap_Read_Byte(unsigned long addr)
{
    unsigned char dat;
    unsigned char ea_bak;

    ea_bak = EA;
    EA     = 0;

    IAP_CMD = IAP_CMD_READ;
    Flash_Set_Addr(addr);
    Flash_Trig();
    dat = IAP_DATA;

    EA = ea_bak;
    return dat;
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    IAP 写一个字节（只能把 1 写成 0，目标单元必须先擦除）
* 参数说明    addr        EEPROM 相对地址
* 参数说明    dat         要写入的数据
* 返回参数    FLASH_OK / FLASH_FAIL
----------------------------------------------------------------------------------------------------*/
FLASH_STA Flash_Iap_Write_Byte(unsigned long addr, unsigned char dat)
{
    unsigned char ea_bak;
    FLASH_STA     sta;

    ea_bak = EA;
    EA     = 0;

    IAP_CMD  = IAP_CMD_PROG;
    Flash_Set_Addr(addr);
    IAP_DATA = dat;
    Flash_Trig();
    sta = Flash_Check();

    EA = ea_bak;
    return sta;
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    擦除 addr 所在的一个扇区（512 字节，擦除后全部为 0xFF）
* 参数说明    addr        EEPROM 相对地址，只需落在目标扇区内
* 返回参数    FLASH_OK / FLASH_FAIL
* 说明        CPU 会等待擦除完成（约 4~6ms，手册 19.1/19.2），无需软件延时
----------------------------------------------------------------------------------------------------*/
FLASH_STA Flash_Iap_Erase_Sector(unsigned long addr)
{
    unsigned char ea_bak;
    FLASH_STA     sta;

    ea_bak = EA;
    EA     = 0;

    IAP_CMD = IAP_CMD_ERASE;
    Flash_Set_Addr(addr);
    Flash_Trig();
    sta = Flash_Check();

    EA = ea_bak;
    return sta;
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    连续读多个字节
* 参数说明    addr        EEPROM 相对地址
* 参数说明    buf         读出数据存放的缓冲区
* 参数说明    len         读取长度
* 返回参数    FLASH_OK / FLASH_ADDR_ERR
----------------------------------------------------------------------------------------------------*/
FLASH_STA Flash_Read_Buff(unsigned long addr, unsigned char *buf, unsigned long len)
{
    if (len == 0UL)
    {
        return FLASH_OK;
    }
    if (addr >= FLASH_EEPROM_SIZE)
    {
        return FLASH_ADDR_ERR;
    }
    if (len > FLASH_EEPROM_SIZE - addr)
    {
        return FLASH_ADDR_ERR;
    }

    while (len--)
    {
        *buf++ = Flash_Iap_Read_Byte(addr++);
    }
    return FLASH_OK;
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    写入核心：把三段数据看作一个连续数据流，自动完成跨扇区的“读出-修改-擦除-写回”
* 参数说明    addr        EEPROM 相对地址
* 参数说明    d1/l1       第 1 段数据及长度
* 参数说明    d2/l2       第 2 段数据及长度（无则长度填 0）
* 参数说明    d3/l3       第 3 段数据及长度（无则长度填 0）
* 返回参数    FLASH_OK / FLASH_FAIL / FLASH_ADDR_ERR
* 说明        1. 每个扇区最多只擦写一次；目标区域内容没变则整个扇区跳过，减少擦写次数
*            2. 写回时跳过 0xFF 字节（擦除后即为 0xFF，无需编程），节省时间
----------------------------------------------------------------------------------------------------*/
static FLASH_STA Flash_Write_Core(unsigned long addr,
                                  unsigned char *d1, unsigned long l1,
                                  unsigned char *d2, unsigned long l2,
                                  unsigned char *d3, unsigned long l3)
{
    unsigned long total;
    unsigned long pos;                              /* 已处理的数据流字节数 */
    unsigned long wi;                               /* 当前字节在数据流中的序号 */
    unsigned long sector_base;                      /* 当前扇区首地址 */
    unsigned int  offset;                           /* 写入起点在扇区内的偏移 */
    unsigned int  num;                              /* 本扇区内要写入的字节数 */
    unsigned int  i;
    unsigned char dat;
    unsigned char changed;
    FLASH_STA     sta;

    total = l1 + l2 + l3;
    if (total == 0UL)
    {
        return FLASH_OK;
    }
    if (addr >= FLASH_EEPROM_SIZE)
    {
        return FLASH_ADDR_ERR;
    }
    if (total > FLASH_EEPROM_SIZE - addr)
    {
        return FLASH_ADDR_ERR;
    }

    pos = 0;
    while (pos < total)
    {
        sector_base = (addr + pos) & ~(FLASH_SECTOR_SIZE - 1UL);
        offset      = (unsigned int)((addr + pos) - sector_base);
        num         = (unsigned int)(FLASH_SECTOR_SIZE - offset);
        if ((unsigned long)num > (total - pos))
        {
            num = (unsigned int)(total - pos);
        }

        /* 1. 读出整个扇区到暂存区 */
        for (i = 0; i < FLASH_SECTOR_SIZE; i++)
        {
            flash_sector_buf[i] = Flash_Iap_Read_Byte(sector_base + i);
        }

        /* 2. 把本扇区内的目标数据改到暂存区，同时判断内容是否有变化 */
        changed = 0;
        for (i = 0; i < num; i++)
        {
            wi = pos + i;
            if (wi < l1)
            {
                dat = d1[wi];
            }
            else if (wi < (l1 + l2))
            {
                dat = d2[wi - l1];
            }
            else
            {
                dat = d3[wi - l1 - l2];
            }

            if (flash_sector_buf[offset + i] != dat)
            {
                flash_sector_buf[offset + i] = dat;
                changed = 1;
            }
        }

        /* 3. 内容有变化才擦除并写回 */
        if (changed)
        {
            sta = Flash_Iap_Erase_Sector(sector_base);
            if (sta != FLASH_OK)
            {
                return sta;
            }
            for (i = 0; i < FLASH_SECTOR_SIZE; i++)
            {
                if (flash_sector_buf[i] != 0xFF)
                {
                    sta = Flash_Iap_Write_Byte(sector_base + i, flash_sector_buf[i]);
                    if (sta != FLASH_OK)
                    {
                        return sta;
                    }
                }
            }

            /* 写后读回校验，防止掉电/时序问题导致数据没写进去 */
            for (i = 0; i < FLASH_SECTOR_SIZE; i++)
            {
                if (Flash_Iap_Read_Byte(sector_base + i) != flash_sector_buf[i])
                {
                    return FLASH_DATA_ERR;
                }
            }
        }

        pos += num;
    }
    return FLASH_OK;
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    连续写多个字节（自动擦除，可跨扇区）
* 参数说明    addr        EEPROM 相对地址
* 参数说明    buf         要写入的数据缓冲区
* 参数说明    len         写入长度
* 返回参数    FLASH_OK / FLASH_FAIL / FLASH_ADDR_ERR
----------------------------------------------------------------------------------------------------*/
FLASH_STA Flash_Write_Buff(unsigned long addr, unsigned char *buf, unsigned long len)
{
    return Flash_Write_Core(addr, buf, len, buf, 0UL, buf, 0UL);
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    安全写一个字节（自动处理所在扇区的擦除，不影响扇区内其它数据）
* 参数说明    addr        EEPROM 相对地址
* 参数说明    dat         要写入的数据
* 返回参数    FLASH_OK / FLASH_FAIL / FLASH_ADDR_ERR
----------------------------------------------------------------------------------------------------*/
FLASH_STA Flash_Write_Byte(unsigned long addr, unsigned char dat)
{
    return Flash_Write_Core(addr, &dat, 1UL, &dat, 0UL, &dat, 0UL);
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    擦除整个 EEPROM 区（恢复出厂用）
* 返回参数    FLASH_OK / FLASH_FAIL
----------------------------------------------------------------------------------------------------*/
FLASH_STA Flash_Erase_All(void)
{
    unsigned long addr;
    FLASH_STA     sta;

    for (addr = 0; addr < FLASH_EEPROM_SIZE; addr += FLASH_SECTOR_SIZE)
    {
        sta = Flash_Iap_Erase_Sector(addr);
        if (sta != FLASH_OK)
        {
            return sta;
        }
    }
    return FLASH_OK;
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    保存一帧参数：帧头(2) + 长度(2) + 数据(len) + 校验和(1)
* 参数说明    addr        EEPROM 相对地址，建议按扇区对齐（0、512、1024...）
* 参数说明    buf         参数数据（通常是一个结构体强转）
* 参数说明    len         参数长度，最大 65535
* 返回参数    FLASH_OK / FLASH_FAIL / FLASH_ADDR_ERR
* 说明        整个记录只按涉及的扇区各擦写一次
----------------------------------------------------------------------------------------------------*/
FLASH_STA Flash_Save_Param(unsigned long addr, unsigned char *buf, unsigned int len)
{
    unsigned char head[4];
    unsigned char sum;
    unsigned int  i;

    sum = 0;
    for (i = 0; i < len; i++)
    {
        sum = (unsigned char)(sum + buf[i]);
    }

    head[0] = FLASH_HEAD0;
    head[1] = FLASH_HEAD1;
    head[2] = (unsigned char)(len);
    head[3] = (unsigned char)(len >> 8);

    return Flash_Write_Core(addr, head, 4UL, buf, (unsigned long)len, &sum, 1UL);
}

/*----------------------------------------------------------------------------------------------------
* 函数功能    读取一帧参数，自动验证帧头、长度和校验和
* 参数说明    addr        EEPROM 相对地址，与保存时一致
* 参数说明    buf         参数数据存放区
* 参数说明    len         参数长度，与保存时一致
* 返回参数    FLASH_OK        读取成功
*             FLASH_NO_DATA   首次使用或已擦除，请装入默认值
*             FLASH_LEN_ERR   长度不一致（结构体改动过），请装入默认值并重新保存
*             FLASH_DATA_ERR  校验失败（可能保存时掉电），请装入默认值
----------------------------------------------------------------------------------------------------*/
FLASH_STA Flash_Load_Param(unsigned long addr, unsigned char *buf, unsigned int len)
{
    unsigned char head[4];
    unsigned char sum;
    unsigned char chk;
    unsigned int  i;
    FLASH_STA     sta;

    sta = Flash_Read_Buff(addr, head, 4UL);
    if (sta != FLASH_OK)
    {
        return sta;
    }

    if ((head[0] != FLASH_HEAD0) || (head[1] != FLASH_HEAD1))
    {
        return FLASH_NO_DATA;
    }
    if ((head[2] != (unsigned char)(len)) || (head[3] != (unsigned char)(len >> 8)))
    {
        return FLASH_LEN_ERR;
    }

    sta = Flash_Read_Buff(addr + 4UL, buf, (unsigned long)len);
    if (sta != FLASH_OK)
    {
        return sta;
    }
    sta = Flash_Read_Buff(addr + 4UL + (unsigned long)len, &chk, 1UL);
    if (sta != FLASH_OK)
    {
        return sta;
    }

    sum = 0;
    for (i = 0; i < len; i++)
    {
        sum = (unsigned char)(sum + buf[i]);
    }
    if (sum != chk)
    {
        return FLASH_DATA_ERR;
    }

    return FLASH_OK;
}
