/*
 * pid.h
 *
 *  
 */

#ifndef CODE_PID_H_
#define CODE_PID_H_

#include "zf_common_headfile.h"

#define PID_OUTPUT_LIMIT    10000.0f		

/* PID Structure Definition */
typedef struct PID
{
    float Kp;
    float Ki;
    float Kd;        /* PID Parameters */
    float Low_Pass;  /* Low Pass Filter coefficient */

    float Out_p;      /* Proportional output */
    float Out_i;      /* Integral output */
    float Out_d;      /* Derivative output */
    float Out_put;    /* Total output */

    float error;           /* Current error */
    float lasterror;       /* Last error */
    float lastlasterror;   /* Last last error */
    float LastData;        /* Last data */
} PID;

/* Macro for PID initialization (C89 Compatible) */
/* Order: Kp, Ki, Kd, Low_Pass, Out_p, Out_i, Out_d, Out_put, error, lasterror, lastlasterror, LastData */
#define PID_CREATE(_kp, _ki, _kd, _low_pass) \
{ \
    _kp, _ki, _kd, _low_pass, \
    0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f \
}

/* External PID Objects */
extern PID servo_pid;
extern PID motor_pid_l;
extern PID motor_pid_r;
extern float Kp_pro;
/* Function Prototypes */
float Positional_PID_Calc(PID *pid, float nowdata, float target);
float PID_Increase(PID *pid, float NowData, float target);

#endif /* CODE_PID_H_ */
