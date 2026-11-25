/*
 * servo.c
 *
 *  Created on: 2025?12?2?
 *      Author: moruohan
 */
#include "servo.h"
#include "motor.h"
#include "pid.h"
#include "fuzzy.h"

uint8 start = 1;
float servo_value;
int left_target, right_target;
float servomid = 720;
int error_s;
static float prev_error = 0;



extern float kp_m;
extern float kd_m;
extern float beid_spk;

float error;
float diff_value_error;
void servo(float mid)
{
    float servo_out;
    float P=kp_m;
    error_s = -MID_W + mid;
    error = (float)error_s;
    diff_value_error = error - prev_error;
    prev_error = error;
	
    if(road_type.straight)
			{
					servo_out = -((KP_Fuzzy(error, diff_value_error, 30,18, beid_spk-zhidao_p_yizhi)) * error / 10 +
											KD_Fuzzy(fabs(error), fabs(diff_value_error), 30, 18, kd_m) * diff_value_error / 10);
			}
		else if(road_type.ruwan)
			{
						servo_out = -(( KP_Fuzzy(error, diff_value_error, 30,18, beid_spk+ruwan_p_zengyi)) * error / 10 +
													KD_Fuzzy(fabs(error), fabs(diff_value_error), 30, 18, kd_m) * diff_value_error / 10);
			}
			else if(road_type.bend||road_type.RightCirque||road_type.LeftCirque){
					servo_out = -((P + KP_Fuzzy(error, diff_value_error, 30,18, beid_spk+wandao_p_zengyi)) * error / 10 +
														KD_Fuzzy(fabs(error), fabs(diff_value_error), 30, 18, kd_m) * diff_value_error / 10);
			}
			else{
					servo_out = -(( KP_Fuzzy(error, diff_value_error, 30,18, beid_spk-else_p_yizhi)) * error / 10 +
															KD_Fuzzy(fabs(error), fabs(diff_value_error), 30,18, kd_m) * diff_value_error / 10);
			}
				
    servo_value = Limit_int(-duoji_limit_R, servo_out, duoji_limit_L);
    pwm_set_duty(duoji_jiao, servo_value + SERVO_PWMMID);
}


/*
void servo(float mid)
{
    float servo_out;
    float P=kp_m;
    error_s = -MID_W + mid;
    error = (float)error_s;
    diff_value_error = error - prev_error;
    prev_error = error;
	
    if(road_type.straight)
			{
					servo_out =		-(kp_m * error / 10 +
												kd_m * diff_value_error / 10);
			}
		else if(road_type.ruwan)
			{
					servo_out =		-(kp_m * error / 10 +
												kd_m * diff_value_error / 10);
			}
			else if(road_type.bend||road_type.RightCirque||road_type.LeftCirque){
					servo_out =		-(kp_m * error / 10 +
												kd_m * diff_value_error / 10);
			}
			else{
					servo_out =		-(kp_m * error / 10 +
												kd_m * diff_value_error / 10);
			}
				
    servo_value = Limit_int(-duoji_limit_R, servo_out, duoji_limit_L);
    pwm_set_duty(duoji_jiao, servo_value + SERVO_PWMMID);
}*/