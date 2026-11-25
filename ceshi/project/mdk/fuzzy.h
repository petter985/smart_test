#ifndef FUZZY_H
#define FUZZY_H

#include "zf_common_headfile.h"

// ============================================================
// 函数声明
// ============================================================

float KP_Fuzzy(float E, float EC, float E_RANGEMAX, float EC_RANGEMAX, float kp_m);
float KD_Fuzzy(float E, float EC, float E_RANGEMAX, float EC_RANGEMAX, float kd_m);
float SPEED_Fuzzy(float E, float EC, float E_RANGEMAX, float EC_RANGEMAX, float SPEED_max);

// ============================================================
// 外部变量声明
// ============================================================
extern float kp_m;    // 基础KP
extern float kd_m;      // 模糊D强度系数
extern float beid_spk;  // 模糊强度系数 
extern int E_max;
extern int EC_max;
extern float normE_SOLUrange[7];
extern float normEC_SOLUrange[7];
extern float normAbsE_SOLUrange[7];
extern float normAbsEC_SOLUrange[7];
extern float ruwan_p_zengyi;
extern float wandao_p_zengyi;
extern float zhidao_p_yizhi;
extern float else_p_yizhi;
#endif /* FUZZY_H */