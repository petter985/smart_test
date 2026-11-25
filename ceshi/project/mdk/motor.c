#include "zf_common_headfile.h"
#include "motor.h"
#include "pid.h"
#include "servo.h"
#include "flash.h"

motor1 motor_l = {0};  /* ?????? */
motor1 motor_r = {0};  /* ?????? */

int g_base_speed = 165;
float zhi_speed=180;
float wan_speed=170;
float ruwan_speed=170;
float huandao_speed=150;
float else_speed=170;
float norm_steer;
float k=0.356;
float stop_distance=0;
void Motor_init()
{
    gpio_init(MotorR_turn, GPO, 1, GPO_PUSH_PULL); /* ??????? */
    pwm_init(MotorR_pwm, 17000, 0);                /* ???PWM??? */
    gpio_init(MotorL_turn, GPO, 1, GPO_PUSH_PULL); /* ??????? */
    pwm_init(MotorL_pwm, 17000, 0);                /* ???PWM??? */
}

void MotorR_setspeed(int pwm)
{
    if(pwm >= 0) /* ?? */
    {
        gpio_set_level(MotorR_turn, 0);   /* ???? */
        pwm_set_duty(MotorR_pwm, pwm);    /* PWM????? */
    }
    else /* ?? */
    {
        gpio_set_level(MotorR_turn, 1);   /* ???? */
        pwm_set_duty(MotorR_pwm, -pwm);    /* PWM????? */
    }
}

void MotorL_setspeed(int pwm)
{
    if(pwm >= 0) /* ?? */
    {
        gpio_set_level(MotorL_turn, 0);   /* ???? */
        pwm_set_duty(MotorL_pwm, pwm);    /* PWM????? */
    }
    else /* ?? */
    {
        gpio_set_level(MotorL_turn, 1);    /* ???? */
        pwm_set_duty(MotorL_pwm, -pwm);    /* PWM????? */
    }
}

uint16 L_speed=70;
uint16 R_speed=70;
extern uint8 ban_count;
void car_start(void)
{
    if(start != 0)
    {
        if(start % 2 == 1)
        {
			if(ban_count==2)
				{
					get_distance=1;
					if(annulus_s1>60){
						annulus_s1=0;
						get_distance=0;
						flag.stop=1;
					}
				}
			if(flag.stop==1)
			{
				
				Motor_Control(0, 0);
				//MotorR_setspeed(0);
				//MotorL_setspeed(0);
			}
			else
				{
					if (road_type.straight)
					{
							g_base_speed = zhi_speed;
					}
					else if(road_type.ruwan)
					{
							g_base_speed = ruwan_speed;
					}
					else if(road_type.bend)
					{
							g_base_speed=wan_speed;
					}
					else if(road_type.LeftCirque||road_type.RightCirque){
							g_base_speed=huandao_speed;
					}
					else
					{
							g_base_speed=else_speed;
					}
					Ackermann_Speed_Calc(g_base_speed, servo_value, &left_target, &right_target);
					Motor_Control(left_target, right_target);
				 
			}
        }
        else
        {
            flag.stop=0;
            //Motor_Control(0, 0);
			MotorR_setspeed(0);
			MotorL_setspeed(0);
        }
    }
}

void Speed_Set(pwm_channel_enum pin1, gpio_pin_enum pin2, int speed, uint8 just, uint8 lose)
{
    if(speed > 0)
    {
        pwm_set_duty(pin1, speed);
        gpio_set_level(pin2, just);
    }
    else
    {
        pwm_set_duty(pin1, -speed);
        gpio_set_level(pin2, lose);
    }
}
/*
void Ackermann_Speed_Calc(int base_speed, float servo_value, int *left_speed, int *right_speed)
{
		float x;
		float y;
		if(road_type.bend||road_type.RightCirque||road_type.LeftCirque){
			k=1.12;
		} 
		else{
			k=0.4;
		}
		if(servo_value<0)
		{
				x=-servo_value/6;
				y=20/tanh(x*3.14/180);
				*left_speed=base_speed*(1-8/y*k);
				*right_speed=base_speed;
		}
		else
		{
				x=servo_value/6;
				y=20/tanh(x*3.14/180);
				*left_speed=base_speed;
				*right_speed=base_speed*(1-8/y*k);
		}
		if (*left_speed  < 0.0f) *left_speed  = 0.0f;
    if (*right_speed < 0.0f) *right_speed = 0.0f;
}*/
void Ackermann_Speed_Calc(int base_speed, float servo_value, int *left_speed, int *right_speed)
{
		float x;
		float y;
		k=0.356;
		
		if(servo_value<0)
		{
				x=-servo_value*0.0815;
				y=k*tan(x*3.14/180);
				*left_speed=base_speed*(1-y);
				*right_speed=base_speed;
		}
		else
		{
				x=servo_value*0.0815;
				y=k*tan(x*3.14/180);
				*left_speed=base_speed;
				*right_speed=base_speed*(1-y);
		}
		if (*left_speed  < 0.0f) *left_speed  = 0.0f;
    if (*right_speed < 0.0f) *right_speed = 0.0f;
}

void Motor_Control(int Speed_L, int Speed_R)
{
    motor_l.target_speed = Speed_L;
    motor_r.target_speed = Speed_R;

    motor_l.duty = Limit_int(-pid_limit, PID_Increase(&motor_pid_l, (float)motor_l.encoder_speed, (float)motor_l.target_speed), pid_limit);
    motor_r.duty = Limit_int(-pid_limit, PID_Increase(&motor_pid_r, (float)motor_r.encoder_speed, (float)motor_r.target_speed), pid_limit);

    Speed_Set(PWM_L, GPIO_L, motor_l.duty, 0, 1);
    Speed_Set(PWM_R, GPIO_R, motor_r.duty, 0, 1);
}

void Encoder_Init(void)
{
		encoder_dir_init(ENCODER_DIR_1, ENCODER_DIR_PULSE_1, ENCODER_DIR_DIR_1);
	
		encoder_dir_init(ENCODER_DIR_2, ENCODER_DIR_PULSE_2, ENCODER_DIR_DIR_2);
}

void Encoder_Data_Get(void)
{
    motor_l.encoder_raw = -encoder_get_count(ENCODER_DIR_1);  /* ????????? */
    motor_l.encoder_speed = motor_l.encoder_speed * 0.2 + (float)motor_l.encoder_raw * 0.8; /* ???? */
    motor_l.total_encoder += motor_l.encoder_raw;
    encoder_clear_count(ENCODER_DIR_1);

    motor_r.encoder_raw = encoder_get_count(ENCODER_DIR_2);  /* ????????? */
    motor_r.encoder_speed = motor_r.encoder_speed * 0.2 + (float)motor_r.encoder_raw * 0.8; /* ???? */
    motor_r.total_encoder += motor_r.encoder_raw;
    encoder_clear_count(ENCODER_DIR_2);
}

float Distance_Measure(void)
{
    float distance;
    distance = (float)(motor_l.encoder_speed + motor_r.encoder_speed) * 0.01f;
    return distance;
}
