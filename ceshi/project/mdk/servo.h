/*
 * servo.h
 *
 *  Created on: 2026?1?23?
 *      Author: petter
 */

#ifndef CODE_SERVO_H_
#define CODE_SERVO_H_

#include "zf_common_headfile.h"

/* Servo PWM Definitions */
#define SERVO_PWMMID 724*6   /* Middle PWM value */
#define SERVO_PWMMAX 816*6   /* Max PWM value (+65) */
#define SERVO_PWMMIN 634*6   /* Min PWM value (-65) */

/* Servo Angle Limits */
#define duoji_limit_L 92*6   /* Left limit (PWM offset) */
#define duoji_limit_R 89*6   /* Right limit (PWM offset) */

/* Motor Pins (Verify consistency with motor.h) */
//#define PWM_L ATOM0_CH2_P21_4
//#define PWM_R ATOM0_CH0_P21_2
//#define GPIO_L P21_5

//#define GPIO_R P21_3

/* Servo Pin Definition */
//#define duoji_jiao PWME_CH4P_PA6
#define duoji_jiao        (PWME_CH3P_PA4)                         // 定义主板上舵机对应引脚
extern int left_target;
extern int right_target;
/* Function Prototypes */

void Servo_init(void);
void servo(float mid);

/* External Variables */
extern uint8 start;
extern float servo_value;
extern float servomid;
extern int error_s;

extern float error;
extern float diff_value_error;
#endif /* CODE_SERVO_H_ */
