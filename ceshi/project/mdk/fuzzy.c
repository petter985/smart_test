/*
 * fuzzy.c
 * C89/C51 兼容版
 */

#include "zf_common_headfile.h"   

float kp_m = 0;    // 基础KP+
float kd_m = 386;      // 模糊D强度系数
float beid_spk = 150;  // 模糊强度系数
float ruwan_p_zengyi=0;//入弯p的直接增强量 
float wandao_p_zengyi=40;//弯道p的直接增强量
float zhidao_p_yizhi=30;//直道p的直接抑制量
float else_p_yizhi=25;//其他p的直接抑制量


/*
float kp_m = 25;    // 基础KP+
float kd_m = 386
	;      // 模糊D强度系数
float beid_spk = 125 ;  // 模糊强度系数
float ruwan_p_zengyi=0;//入弯p的直接增强量 
float wandao_p_zengyi=40;//弯道p的直接增强量
float zhidao_p_yizhi=30;//直道p的直接抑制量
float else_p_yizhi=25;//其他p的直接抑制量
*/
// ============================================================
// 1. 模糊边界定义
// ============================================================
static const float PSnormErrorBounds[7] = {-1.0, -0.25, -0.075, 0.0, 0.075, 0.25, 1.0};
static const float PSnormErrorChangeBounds[7] = {-1.0, -0.07, -0.025, 0.0, 0.025, 0.07, 1.0};
static const float DnormAbsErrorBounds[7] = {0, 0.05, 0.075, 0.1, 0.25, 0.5, 1};
static const float DnormAbsErrorChangeBounds[7] = {0, 0.01, 0.025, 0.10, 0.22, 0.4, 1};

int E_max = 72;
int EC_max = 36;

float normE_SOLUrange[7];
float normEC_SOLUrange[7];
float normAbsE_SOLUrange[7];
float normAbsEC_SOLUrange[7];

// ============================================================
// 2. 辅助函数
// ============================================================
void independentvalue_turn(float EFUZZY_input_max, float ECFUZZY_input_max)
{
    int i;
    for(i = 0; i < 7; i++) {
        normE_SOLUrange[i] = PSnormErrorBounds[i] * EFUZZY_input_max;
        normEC_SOLUrange[i] = PSnormErrorChangeBounds[i] * ECFUZZY_input_max;
        normAbsE_SOLUrange[i] = DnormAbsErrorBounds[i] * EFUZZY_input_max;
        normAbsEC_SOLUrange[i] = DnormAbsErrorChangeBounds[i] * ECFUZZY_input_max;
    }
}

// ============================================================
// 3. KD_Fuzzy
// ============================================================
float KD_Fuzzy(float E, float EC, float E_RANGEMAX, float EC_RANGEMAX, float kd_m)
{
    float GENERuleKd[7][7] = {
        {3, 3, 4, 5, 5, 6, 6},
        {3, 3, 4, 4, 5, 5, 5},
        {2, 3, 3, 4, 5, 5, 5},
        {1, 2, 2, 3, 3, 4, 5},
        {1, 1, 2, 2, 3, 3, 3},
        {0, 1, 1, 1, 2, 2, 3},
        {0, 0, 1, 1, 1, 2, 2}
    };

    static const float outputLevelScales[7] = {0.60f, 0.60f, 0.65f, 0.75f, 0.85f, 0.95f, 1.0f};
    
    int i;
    uint8 i2;
    float EFF[7];
    float DFF[7];
    float UFF[7];
    float U = 0;
    float PF[2] = {0}, DF[2] = {0}, UF[4] = {0};
    int Pn = 0, Dn = 0, Un[4] = {0};
    float t1 = 0, t2 = 0, t3 = 0, t4 = 0, temp1 = 0, temp2 = 0;

    independentvalue_turn(E_RANGEMAX, EC_RANGEMAX);

    for(i = 0; i < 7; i++) {
        EFF[i] = normAbsE_SOLUrange[i];
        DFF[i] = normAbsEC_SOLUrange[i];
    }

    for(i2 = 0; i2 < 7; i2++) {
        UFF[i2] = kd_m * outputLevelScales[i2];
    }

    if(E > EFF[0] && E < EFF[6]) {
        if(E <= EFF[1]) {
            Pn = -2;
            PF[0] = (EFF[1] - E) / (EFF[1] - EFF[0]);
        } else if(E <= EFF[2]) {
            Pn = -1;
            PF[0] = (EFF[2] - E) / (EFF[2] - EFF[1]);
        } else if(E <= EFF[3]) {
            Pn = 0;
            PF[0] = (EFF[3] - E) / (EFF[3] - EFF[2]);
        } else if(E <= EFF[4]) {
            Pn = 1;
            PF[0] = (EFF[4] - E) / (EFF[4] - EFF[3]);
        } else if(E <= EFF[5]) {
            Pn = 2;
            PF[0] = (EFF[5] - E) / (EFF[5] - EFF[4]);
        } else if(E <= EFF[6]) {
            Pn = 3;
            PF[0] = (EFF[6] - E) / (EFF[6] - EFF[5]);
        }
    } else if(E <= EFF[0]) {
        Pn = -2;
        PF[0] = 1;
    } else if(E >= EFF[6]) {
        Pn = 3;
        PF[0] = 0;
    }

    PF[1] = 1 - PF[0];

    if(EC > DFF[0] && EC < DFF[6]) {
        if(EC <= DFF[1]) {
            Dn = -2;
            DF[0] = (DFF[1] - EC) / (DFF[1] - DFF[0]);
        } else if(EC <= DFF[2]) {
            Dn = -1;
            DF[0] = (DFF[2] - EC) / (DFF[2] - DFF[1]);
        } else if(EC <= DFF[3]) {
            Dn = 0;
            DF[0] = (DFF[3] - EC) / (DFF[3] - DFF[2]);
        } else if(EC <= DFF[4]) {
            Dn = 1;
            DF[0] = (DFF[4] - EC) / (DFF[4] - DFF[3]);
        } else if(EC <= DFF[5]) {
            Dn = 2;
            DF[0] = (DFF[5] - EC) / (DFF[5] - DFF[4]);
        } else if(EC <= DFF[6]) {
            Dn = 3;
            DF[0] = (DFF[6] - EC) / (DFF[6] - DFF[5]);
        }
    } else if(EC <= DFF[0]) {
        Dn = -2;
        DF[0] = 1;
    } else if(EC >= DFF[6]) {
        Dn = 3;
        DF[0] = 0;
    }

    DF[1] = 1 - DF[0];

    Un[0] = GENERuleKd[Pn + 2][Dn + 2];
    Un[1] = GENERuleKd[Pn + 3][Dn + 2];
    Un[2] = GENERuleKd[Pn + 2][Dn + 3];
    Un[3] = GENERuleKd[Pn + 3][Dn + 3];

    UF[0] = (PF[0] <= DF[0]) ? PF[0] : DF[0];
    UF[1] = (PF[1] <= DF[0]) ? PF[1] : DF[0];
    UF[2] = (PF[0] <= DF[1]) ? PF[0] : DF[1];
    UF[3] = (PF[1] <= DF[1]) ? PF[1] : DF[1];

    if(Un[0] == Un[1]) {
        if(UF[0] > UF[1]) UF[1] = 0;
        else UF[0] = 0;
    }
    if(Un[0] == Un[2]) {
        if(UF[0] > UF[2]) UF[2] = 0;
        else UF[0] = 0;
    }
    if(Un[0] == Un[3]) {
        if(UF[0] > UF[3]) UF[3] = 0;
        else UF[0] = 0;
    }
    if(Un[1] == Un[2]) {
        if(UF[1] > UF[2]) UF[2] = 0;
        else UF[1] = 0;
    }
    if(Un[1] == Un[3]) {
        if(UF[1] > UF[3]) UF[3] = 0;
        else UF[1] = 0;
    }
    if(Un[2] == Un[3]) {
        if(UF[2] > UF[3]) UF[3] = 0;
        else UF[2] = 0;
    }

    t1 = UF[0] * UFF[Un[0]];
    t2 = UF[1] * UFF[Un[1]];
    t3 = UF[2] * UFF[Un[2]];
    t4 = UF[3] * UFF[Un[3]];
    temp1 = t1 + t2 + t3 + t4;
    temp2 = UF[0] + UF[1] + UF[2] + UF[3];

    if(temp2 != 0) {
        U = temp1 / temp2;
    } else {
        U = 0;
    }

    return U;
}

// ============================================================
// 4. KP_Fuzzy（只保留函数，去掉C99语法）
// ============================================================
float KP_Fuzzy(float E, float EC, float E_RANGEMAX, float EC_RANGEMAX, float kp_m)
{
    float GENERuleKp[7][7] = {
        {6, 5, 4, 4, 3, 0, 0},
        {6, 4, 3, 3, 2, 0, 0},
        {4, 3, 2, 1, 0, 1, 2},
        {2, 1, 1, 0, 1, 1, 2},
        {2, 1, 0, 1, 2, 3, 4},
        {0, 0, 2, 3, 3, 4, 6},
        {0, 1, 3, 4, 4, 5, 6}
    };

    static const float outputLevelScales[7] = {0.25f, 0.45f, 0.7f, 0.8f, 0.88f, 0.92f, 1.0f};
    
    int i;
    uint8 i2;
    float EFF[7];
    float DFF[7];
    float UFF[7];
    float U = 0;
    float PF[2] = {0}, DF[2] = {0}, UF[4] = {0};
    int Pn = 0, Dn = 0, Un[4] = {0};
    float t1 = 0, t2 = 0, t3 = 0, t4 = 0, temp1 = 0, temp2 = 0;

    independentvalue_turn(E_RANGEMAX, EC_RANGEMAX);

    for(i = 0; i < 7; i++) {
        EFF[i] = normE_SOLUrange[i];
        DFF[i] = normEC_SOLUrange[i];
    }

    for(i2 = 0; i2 < 7; i2++) {
        UFF[i2] = kp_m * outputLevelScales[i2];
    }

    // 误差隶属度计算（同KD_Fuzzy一样的逻辑，但用normE_SOLUrange）
    if(E > EFF[0] && E < EFF[6]) {
        if(E <= EFF[1]) {
            Pn = -2;
            PF[0] = (EFF[1] - E) / (EFF[1] - EFF[0]);
        } else if(E <= EFF[2]) {
            Pn = -1;
            PF[0] = (EFF[2] - E) / (EFF[2] - EFF[1]);
        } else if(E <= EFF[3]) {
            Pn = 0;
            PF[0] = (EFF[3] - E) / (EFF[3] - EFF[2]);
        } else if(E <= EFF[4]) {
            Pn = 1;
            PF[0] = (EFF[4] - E) / (EFF[4] - EFF[3]);
        } else if(E <= EFF[5]) {
            Pn = 2;
            PF[0] = (EFF[5] - E) / (EFF[5] - EFF[4]);
        } else if(E <= EFF[6]) {
            Pn = 3;
            PF[0] = (EFF[6] - E) / (EFF[6] - EFF[5]);
        }
    } else if(E <= EFF[0]) {
        Pn = -2;
        PF[0] = 1;
    } else if(E >= EFF[6]) {
        Pn = 3;
        PF[0] = 0;
    }

    PF[1] = 1 - PF[0];

    // EC隶属度
    if(EC > DFF[0] && EC < DFF[6]) {
        if(EC <= DFF[1]) {
            Dn = -2;
            DF[0] = (DFF[1] - EC) / (DFF[1] - DFF[0]);
        } else if(EC <= DFF[2]) {
            Dn = -1;
            DF[0] = (DFF[2] - EC) / (DFF[2] - DFF[1]);
        } else if(EC <= DFF[3]) {
            Dn = 0;
            DF[0] = (DFF[3] - EC) / (DFF[3] - DFF[2]);
        } else if(EC <= DFF[4]) {
            Dn = 1;
            DF[0] = (DFF[4] - EC) / (DFF[4] - DFF[3]);
        } else if(EC <= DFF[5]) {
            Dn = 2;
            DF[0] = (DFF[5] - EC) / (DFF[5] - DFF[4]);
        } else if(EC <= DFF[6]) {
            Dn = 3;
            DF[0] = (DFF[6] - EC) / (DFF[6] - DFF[5]);
        }
    } else if(EC <= DFF[0]) {
        Dn = -2;
        DF[0] = 1;
    } else if(EC >= DFF[6]) {
        Dn = 3;
        DF[0] = 0;
    }

    DF[1] = 1 - DF[0];

    Un[0] = GENERuleKp[Pn + 2][Dn + 2];
    Un[1] = GENERuleKp[Pn + 3][Dn + 2];
    Un[2] = GENERuleKp[Pn + 2][Dn + 3];
    Un[3] = GENERuleKp[Pn + 3][Dn + 3];

    UF[0] = (PF[0] <= DF[0]) ? PF[0] : DF[0];
    UF[1] = (PF[1] <= DF[0]) ? PF[1] : DF[0];
    UF[2] = (PF[0] <= DF[1]) ? PF[0] : DF[1];
    UF[3] = (PF[1] <= DF[1]) ? PF[1] : DF[1];

    if(Un[0] == Un[1]) {
        if(UF[0] > UF[1]) UF[1] = 0;
        else UF[0] = 0;
    }
    if(Un[0] == Un[2]) {
        if(UF[0] > UF[2]) UF[2] = 0;
        else UF[0] = 0;
    }
    if(Un[0] == Un[3]) {
        if(UF[0] > UF[3]) UF[3] = 0;
        else UF[0] = 0;
    }
    if(Un[1] == Un[2]) {
        if(UF[1] > UF[2]) UF[2] = 0;
        else UF[1] = 0;
    }
    if(Un[1] == Un[3]) {
        if(UF[1] > UF[3]) UF[3] = 0;
        else UF[1] = 0;
    }
    if(Un[2] == Un[3]) {
        if(UF[2] > UF[3]) UF[3] = 0;
        else UF[2] = 0;
    }

    t1 = UF[0] * UFF[Un[0]];
    t2 = UF[1] * UFF[Un[1]];
    t3 = UF[2] * UFF[Un[2]];
    t4 = UF[3] * UFF[Un[3]];
    temp1 = t1 + t2 + t3 + t4;
    temp2 = UF[0] + UF[1] + UF[2] + UF[3];

    if(temp2 != 0) {
        U = temp1 / temp2;
    } else {
        U = 0;
    }

    return U;
}

// ============================================================
// 5. SPEED_Fuzzy（你自己补，同样去掉C99语法）
// ============================================================
float SPEED_Fuzzy(float E, float EC, float E_RANGEMAX, float EC_RANGEMAX, float SPEED_max)
{
    // 和KP_Fuzzy一样结构，用你自己的规则表
    // 你原来的代码里已经有了，把所有 for(int i=...) 改成 for(i=...)
    // 并在函数开头声明 int i;
    
    float GENERuleSPEED[7][7] = {
        {0, 0, 1, 1, 2, 3, 3},
        {0, 1, 1, 2, 3, 3, 3},
        {1, 1, 2, 2, 4, 4, 4},
        {1, 1, 3, 3, 4, 5, 5},
        {1, 2, 3, 4, 5, 5, 5},
        {2, 3, 4, 4, 5, 6, 6},
        {2, 3, 4, 4, 5, 6, 6}
    };

    static const float outputLevelScales[7] = {0.625f, 0.667f, 0.708f, 0.792f, 0.833f, 0.917f, 1.000f};

    int i;
    uint8 i2;
    float EFF[7];
    float DFF[7];
    float UFF[7];
    float U = 0;
    float PF[2] = {0}, DF[2] = {0}, UF[4] = {0};
    int Pn = 0, Dn = 0, Un[4] = {0};
    float t1 = 0, t2 = 0, t3 = 0, t4 = 0, temp1 = 0, temp2 = 0;

    independentvalue_turn(E_RANGEMAX, EC_RANGEMAX);

    for(i = 0; i < 7; i++) {
        EFF[i] = normAbsE_SOLUrange[i];
        DFF[i] = normAbsEC_SOLUrange[i];
    }

    for(i2 = 0; i2 < 7; i2++) {
        UFF[i2] = SPEED_max * outputLevelScales[i2];
    }

    // 后面照抄KP_Fuzzy的逻辑，但规则表用 SPEED 的
    // ...（省略，和KP_Fuzzy一样，只是规则表不同）
    
    return U;
}