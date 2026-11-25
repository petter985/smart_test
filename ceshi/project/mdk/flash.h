/****************************************************************************************************
* 文件名称        flash.h
* 适用平台        STC32G144K246（Ai8052U），MDK FOR C251
* 功能说明        内部 Flash(EEPROM) 底层驱动头文件，C90(C98) 语法，不依赖任何库，可独立使用
*
* 使用前提        1. 在 STC-ISP(AiCube-ISP) 下载软件的硬件选项中“设置用户EEPROM大小”（例如 2K）
*                2. 取消勾选“下次下载用户程序时擦除用户EEPROM区”，否则每次下载程序参数都会丢失
*                3. 下面的 FLASH_EEPROM_SIZE 必须改成与 STC-ISP 中设置的大小一致
*
* 地址说明        驱动内所有地址均为 EEPROM 相对地址，从 0 开始（物理位置固定在 FC:2800h 起，
*                由硬件自动映射，用户无需关心，见手册 19.4 节）
*
* 修改记录
* 日期            备注
* 2026-07-18      V1.0 按 STC32G144K246 系列技术手册第 19 章（IAP/EEPROM）编写
****************************************************************************************************/

#ifndef __FLASH_H__
#define __FLASH_H__

/*---------------------------------- 用户配置区 ----------------------------------*/

#define FLASH_EEPROM_SIZE   (2UL * 1024UL)      /* EEPROM 总大小(字节)，必须与 STC-ISP 设置一致 */
#define FLASH_SECTOR_SIZE   (512UL)             /* 扇区大小，手册固定 512 字节，不可修改 */

/*---------------------------------- 返回状态 ----------------------------------*/

typedef enum
{
    FLASH_OK       = 0,                         /* 操作成功 */
    FLASH_FAIL     = 1,                         /* 操作失败，硬件 CMD_FAIL 置位 */
    FLASH_ADDR_ERR = 2,                         /* 地址或长度越界 */
    FLASH_NO_DATA  = 3,                         /* 没有找到有效参数（首次使用或被擦除过） */
    FLASH_LEN_ERR  = 4,                         /* 参数长度不一致（结构体改动过，需重新保存） */
    FLASH_DATA_ERR = 5                          /* 校验和错误，数据已损坏（可能写入时掉电） */
} FLASH_STA;

/*---------------------------------- 底层接口 ----------------------------------*/
/* 直接操作 IAP，使用前需要了解“写前必须先擦除所在扇区”的规则 */

void          Flash_Init(unsigned long sysclk);                     /* 初始化，sysclk 为系统时钟频率(Hz) */
void          Flash_Close(void);                                    /* 关闭 IAP 功能 */
unsigned char Flash_Iap_Read_Byte(unsigned long addr);              /* IAP 读一个字节 */
FLASH_STA     Flash_Iap_Write_Byte(unsigned long addr, unsigned char dat);  /* IAP 写一个字节，目标必须先擦除 */
FLASH_STA     Flash_Iap_Erase_Sector(unsigned long addr);           /* 擦除 addr 所在的一个扇区(512 字节) */

/*---------------------------------- 应用接口 ----------------------------------*/
/* 推荐使用：自动完成“读出-修改-擦除-写回”，自动处理跨扇区，内容没变就不擦写 */

FLASH_STA     Flash_Read_Buff(unsigned long addr, unsigned char *buf, unsigned long len);
FLASH_STA     Flash_Write_Buff(unsigned long addr, unsigned char *buf, unsigned long len);
FLASH_STA     Flash_Write_Byte(unsigned long addr, unsigned char dat);
FLASH_STA     Flash_Erase_All(void);                                /* 擦除整个 EEPROM 区，恢复出厂用 */

/*---------------------------------- 参数存取接口 ----------------------------------*/
/* 带帧头、长度和校验和，适合菜单参数整体保存与上电恢复 */
/* 记录占用 len+5 个字节：帧头(2) + 长度(2) + 数据(len) + 校验和(1)，len 最大 65535 */

FLASH_STA     Flash_Save_Param(unsigned long addr, unsigned char *buf, unsigned int len);
FLASH_STA     Flash_Load_Param(unsigned long addr, unsigned char *buf, unsigned int len);

#endif

/*************************************** 使用示例 ****************************************

#include "flash.h"

typedef struct
{
    float kp_m;
    float kd_m;
    float beid_spk;
    int   speed;
} Param;

Param param;

void main(void)
{
    clock_init(SYSTEM_CLOCK_96M);               // 逐飞库时钟初始化

    Flash_Init(system_clock);                   // 必须在时钟初始化之后再初始化 Flash

    if (Flash_Load_Param(0, (unsigned char *)&param, sizeof(param)) != FLASH_OK)
    {
        param.kp_m     = 1.0f;                  // 没有有效参数（首次使用或数据损坏），装入默认值
        param.kd_m     = 0.0f;
        param.beid_spk = 2.0f;
        param.speed    = 100;
        Flash_Save_Param(0, (unsigned char *)&param, sizeof(param));
    }

    while (1)
    {
        // 菜单里修改完参数后（建议停车时执行）：
        // Flash_Save_Param(0, (unsigned char *)&param, sizeof(param));
    }
}

注意事项：
1. 擦除一个扇区期间 CPU 会暂停约 4~6ms（中断同样被挂起），请在停车或菜单状态下保存参数，
   不要在控制周期中断里保存。
2. EEPROM 擦写寿命约 10 万次，不要放在循环里频繁保存。
3. 不同的参数记录建议放在不同的扇区（0、512、1024...），互不影响。
4. 程序超过 64K 时，需按手册 2.14 节在 Keil 中设置 Huge 模式并保留 EEPROM 区域。

*****************************************************************************************/
