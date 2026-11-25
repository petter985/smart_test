#include "zf_common_headfile.h"




void pit_handler(void);
void servo_handler(void);
void encoder_handler(void);
void key_handler(void);
void car_start_handler(void);
extern int temp8;
uint8 time_test=0;
float wave_data[4];

void main(void)
{
    clock_init(SYSTEM_CLOCK_96M);                 // ??????ü????????<??????>
    debug_init();                               // ???????????????

    
    init();
		//my_vofa_init();

	vofa_register_param(0, "Kp",  1.5f,  0.0f, 160.0f, 0.1f);
    vofa_register_param(1, "Ki",  0.2f,  0.0f,  160.0f, 0.05f);
    vofa_register_param(2, "Kd",  0.5f,  0.0f,  165.0f, 0.05f);
    vofa_register_param(4, "Speed", 200.0f, 0.0f, 500.0f, 10.0f);
	vofa_register_param(5, "kp_l",50.0f, 0.0f, 100.0f, 0.5f);
	vofa_register_param(6, "ki_l",5.5f, 0.0f,10.0f,0.1f);
	vofa_register_param(7, "kp_r",50.0f, 0.0f, 100.0f, 0.5f);
	vofa_register_param(8, "ki_r",5.5f, 0.0f,10.0f,0.1f);
	
	
	pit_ms_init(TIM1_PIT, 5, encoder_handler);    // 1ms
	pit_ms_init(TIM2_PIT, 5, car_start_handler); //  10ms
	pit_ms_init(TIM3_PIT, 3, servo_handler);      // 1ms
	pit_ms_init(TIM4_PIT, 30, key_handler);        //5ms(???????
    
    while(1)
    {
		image_process();
		//menu_flash_task();          /* Flash 参数保存任务（主循环执行） */
		//my_wifi_process();
		if(menu_show_flag)
			menu_show();
    }
       
}


void servo_handler(void)
{
    servo(final_mid_line);
}

void encoder_handler(void)
{
	Encoder_Data_Get();

	if (get_distance) //用来作为距离判断的依据，辅助元素处理
	{ 
		annulus_s1 += Distance_Measure();//左右同时一圈60
	}
	else annulus_s1 = 0;
}
	
void key_handler(void)
{ 
    key_process();          
}

void car_start_handler(void)
{
	  car_start();            
}