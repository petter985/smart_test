#include "pid.h"

PID motor_pid_l = {56, 4.9, 0, 1};      /* ??? PID */
PID motor_pid_r = {56, 4.8, 0, 1};      /* ??? PID */
int base_speed = 2000;                /* ???? */

PID servo_pid = {12.4, 0, 0, 1 };  /* ?? PID */
//5.5  					         2.015, 0, 7,      12.07*5, 0, 5.3*5, 0.8   10.7, 0, 27, 0.8  
/* ??? PID ?? */
float Kp_base = 0.7;
float Kd_base = 5;

float Kp_pro; 

float Positional_PID_Calc(PID *pid, float nowdata, float target)
{
			float abs_err = fabs(pid->error);
			float factor;
			pid->error = target - nowdata;
      factor = abs_err*abs_err;  // 二次函数 y = (x/max)2
    
			Kp_pro =4 + 0.135* factor;  // 映射到 [0.5Kp, 1.0Kp]
		

    //pid->Out_d = (pid->error - pid->Out_p) * pid->Low_Pass + pid->Out_d * (1 - pid->Low_Pass);
    
    pid->Out_p = pid->error;
    
    pid->Out_put = Kp_pro * pid->Out_p + pid->Out_d;

    return (pid->Out_put);
}

/*
float Positional_PID_Calc(PID *pid, float nowdata, float target)
{
		pid->error = target - nowdata;
		pid->Out_d = (pid->error - pid->Out_p) * pid->Low_Pass + pid->Out_d * (1 - pid->Low_Pass);
		pid->Out_p=pid->error;
		return (pid->Kp*pid->Out_p+pid->Kd*pid->Out_d);
}
*/

float PID_Increase(PID *pid, float nowdata, float target)
{
    float error = target - nowdata;
    float last_e = pid->lasterror;
    float last_last_e = pid->lastlasterror;
    float p_term, i_term, d_term, delta;

    p_term = pid->Kp * (error - last_e);
    i_term = pid->Ki * error;
    d_term = pid->Kd * (error - 2.0f * last_e + last_last_e);
    delta = p_term + i_term + d_term;

    pid->Out_put += delta;

    if(pid->Out_put > PID_OUTPUT_LIMIT) pid->Out_put = PID_OUTPUT_LIMIT;
    if(pid->Out_put < -PID_OUTPUT_LIMIT) pid->Out_put = -PID_OUTPUT_LIMIT;

    pid->lastlasterror = last_e;
    pid->lasterror = error;
	
    pid->Out_p = p_term;
    pid->Out_i = i_term;
    pid->Out_d = d_term;

    return pid->Out_put;
}