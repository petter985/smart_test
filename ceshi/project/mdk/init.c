/*
 * init.c
 *
 *  Created on: 2026?3?22?
 *      Author: Eric
 */

#include "init.h"  // ??1:??????????????

void init(void)
{
    // ??????
    mt9v03x_init();
    // ??
		//my_wifi_init();
	
    ips200_init();
    // ?????
    Motor_init();
    // ?????
    pwm_init(duoji_jiao, 300, SERVO_PWMMID);//可以提高
    
    // gpio???
    gpio_init(GPIO_L, GPO, 0, GPO_PUSH_PULL);
    gpio_init(GPIO_R, GPO, 0, GPO_PUSH_PULL);
    
    // pwm???
    pwm_init(PWM_L, 17000, 0);
    pwm_init(PWM_R, 17000, 0);
    
		key_init();
		
	  menu_init();
		//蜂鸣器初始化
		gpio_init(BEEP_PIN, GPO, 0, GPO_PUSH_PULL);
    //激光初始化
		gpio_init(LASER_HEAD_PIN, GPO, 0, GPO_PUSH_PULL);
    // ??????
    Encoder_Init();
    
    
    //pit_ms_init(CCU60_CH0); 
    //pit_ms_init(CCU60_CH1);
}