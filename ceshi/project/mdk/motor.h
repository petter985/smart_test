/*
 * motor.h
 *
 *  Created on: 2026?3?8?
 *      Author: Eric
 */

#ifndef CODE_MOTOR_H_
#define CODE_MOTOR_H_

#include "zf_common_headfile.h"

/* Pin Definitions */
#define MotorL_pwm  PWMB_CH3_P76
#define MotorL_turn PWMB_CH4_P77

#define MotorR_pwm  PWMB_CH1_P74
#define MotorR_turn PWMB_CH2_P75

#define PWM_L     PWMB_CH3_P76  /* ??PWM??,???MotorL_pwm */
#define GPIO_L    PWMB_CH4_P77        /* ??????,???MotorL_turn */
#define PWM_R     PWMB_CH1_P74 /* ??PWM??,???MotorR_pwm */
#define GPIO_R    PWMB_CH2_P75       /* ??????,???MotorR_turn */

/* PID Limit */
#define pid_limit 6000

/* Global Variables */
extern uint16 L_speed;
extern uint16 R_speed;

/* Function Prototypes */
void Motor_init(void);
void MotorL_setspeed(int pwm);
void MotorR_setspeed(int pwm);

/* Motor Structure Definition */
typedef struct motor1
{
    int target_speed;
    int duty;
    float encoder_speed;
    int encoder_raw;
    int32 total_encoder;
} motor1;

/* External Motor Instances */
extern motor1 motor_l;
extern motor1 motor_r;

extern int g_base_speed;
extern float k;
/* Control Functions */
void car_start(void);
void Speed_Set(pwm_channel_enum pin1, gpio_pin_enum pin2, int speed, uint8 just, uint8 lose);
void Motor_Control(int Speed_L, int Speed_R);
void Encoder_Init(void);
void Encoder_Data_Get(void);
void Ackermann_Speed_Calc(int base_speed, float servo_value, int *left_speed, int *right_speed);

/*
		#define ENCODER_DIR_3                 	(TIM17_ENCODER)             // ????????????????x
		#define ENCODER_DIR_PULSE_3       		(TIM17_ENCODER_CH1_P80)     // PULSE ?????
		#define ENCODER_DIR_DIR_3           	(IO_P44)             	    // DIR ?????
*/
		#define ENCODER_DIR_1                 	(PWMA_ENCODER)              // 带方向编码器对应使用的编码器接口 
		#define ENCODER_DIR_PULSE_1            	(PWMA_ENCODER_CH1P_P60)     // PULSE 对应的引脚
		#define ENCODER_DIR_DIR_1              	(PWMA_ENCODER_CH2P_P62)     // DIR 对应的引脚

	/*	#define ENCODER_DIR_4                 	(TIM18_ENCODER)             // ????????????????
		#define ENCODER_DIR_PULSE_4       		(TIM18_ENCODER_CH1_P90)     // PULSE ?????
		#define ENCODER_DIR_DIR_4           	(IO_P46)             	    // DIR ?????
*/
		#define ENCODER_DIR_2                 	(PWMC_ENCODER)              // 带方向编码器对应使用的编码器接口
		#define ENCODER_DIR_PULSE_2       		(PWMC_ENCODER_CH1P_P40)     // PULSE 对应的引脚
		#define ENCODER_DIR_DIR_2           	(PWMC_ENCODER_CH2P_P42)     // DIR 对应的引脚
	

/* Encoder Variables */
extern int16 l_encoder_pulse;       // Left encoder pulse
extern int16 r_encoder_pulse;       // Right encoder pulse
extern int16 l_speed_now;           // Left current speed
extern int16 r_speed_now;           // Right current speed


extern float zhi_speed;
extern float wan_speed;
extern float ruwan_speed;
extern float huandao_speed;
extern float else_speed;
float Distance_Measure(void);

#endif /* CODE_MOTOR_H_ */
