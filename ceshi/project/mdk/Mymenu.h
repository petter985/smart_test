#ifndef CODE_MYMENU_H_
#define CODE_MYMENU_H_

#include "menu.h"

void menu_init(void);
void menu_show(void);
void menu_flash_task(void);

extern volatile uint8 menu_save_request;

void key_1(void);
void key_2(void);

void key_enter(void);
void key_quit(void);
void key_select(void);

extern uint8 key_flag;

#endif