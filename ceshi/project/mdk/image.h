/*
 * image.h
 *
 * Created on: 2026?3?22?
 * Author: petter
 */
#ifndef CODE_IMAGE_H_
#define CODE_IMAGE_H_
#include "zf_common_headfile.h"
extern uint8 MID_W;
extern uint8 l_line_x[MT9V03X_H];
extern uint8 r_line_x[MT9V03X_H];
extern float final_mid_line;
extern uint8 m_line_x[MT9V03X_H];
extern uint8 search_line_end;
//extern uint8 image_01[MT9V03X_H][MT9V03X_W];
extern uint8 start_col_for_search;  /* 搜线起始列（由最长白列决定） */
extern uint8 kuandu_saidao[MT9V03X_H-1];
extern uint8 get_distance;
struct ROAD_TYPE
{
    int8 straight;
		int8 short_straight;
    int8 bend;
    int8 Ramp;
    int8 Cross;
    int8 LeftCirque;
    int8 RightCirque;
    int8 ruwan;
		int8 banmaxian;
};

extern struct ROAD_TYPE road_type;

/* ==================== 阈值参数 ==================== */
extern uint8 reference_point;       /* 参考点 */
extern uint8 white_max_point;       /* 白色上限 */
extern uint8 white_min_point;       /* 白色下限 */

/* ==================== 图像处理函数 ==================== */
void get_reference_point(const uint8 *image);
uint8 search_left_edge(uint8 index[][MT9V03X_W], uint8 row, uint8 start);
uint8 search_right_edge(uint8 index[][MT9V03X_W], uint8 row, uint8 start);
void Search_edge(void);

/* 元素检测处理 */
void image_process(void);
void Element_Test(void);
void Element_Handle(void);

extern uint16 distance_L_Cross_out;
extern uint16 distance_R_Cross_out;
extern uint8 huandao_stage;
extern float annulus_s1;
extern int16 jiaodu_jifen;
extern int16 annulus_jinku_s;

void Handle_Left_Cirque(void);
void Handle_Right_Cirque(void);
void Diuxian_weizhi_test(uint8 type, uint8 startline, uint8 endline);
extern uint8 diuxian_hang, budiuxian_hang;

void Handle_Cross(void);
void Check_Cross_Guaidian(uint8 type);
extern uint16 cross_s;
extern uint8 cross_left[2];
extern uint8 cross_right[2];
extern uint8 found_left_corner[2];
extern uint8 found_right_corner[2];

void Handle_Bend(void);
uint8 RoundaboutGetArc(uint8 type, uint8 num);
extern uint8 bianxian_guaidian_l;
extern uint8 bianxian_guaidian_r;

void Transfer_Camera(void);
uint8 Threshold(uint8 x, uint8 y);
void Blacking(void);
void Regression(uint8 type, uint8 startline, uint8 endline);
extern float parameterA, parameterB;
void Hua_Xian(uint8 type, uint8 startline, uint8 endline, float parameterB, float parameterA);
void La_zhixian(uint8 x_down, uint8 y_down, uint8 x_up, uint8 y_up, uint8 *array);

float Process_Curvity(uint8 x1, uint8 y1, uint8 x2, uint8 y2, uint8 x3, uint8 y3);
uint8 My_Sqrt(int16 x);
void Bianxian_guaidian(uint8 type, uint8 startline, uint8 endline);
extern uint8 l_guaidain_x, l_guaidain_y;
extern uint8 r_guaidain_x, r_guaidain_y;
uint8 Bianxian_guaidian_num(uint8 type, uint8 startline, uint8 endline);
extern uint8 l_guaidain_x1, l_guaidain_y1;
extern uint8 r_guaidain_x1, r_guaidain_y1;
extern uint8 l_guaidain_x2, l_guaidain_y2;
extern uint8 r_guaidain_x2, r_guaidain_y2;
void Qianzhang(void);
extern uint8 qianzhang;
uint8 Cirque_or_Cross(uint8 type, uint8 startline);
extern uint8 Num;
void Mid_Col(uint8 index[][MT9V03X_W]);  /* 修改：添加参数 */
extern uint8 length;
void Outside_protect(uint8 index[][MT9V03X_W]);
void HDPJ_lvbo(uint8 dat[], uint8 N, uint8 siz);
void Check_Zhidao(void);
extern uint8 sudu_yingzi;
void turn_pro();
extern int turn_propect;
extern int turn_proline;
void zebra_test(void);
void Handle_Zebra(void);
uint8 check_zhibian(uint8 type);
uint8 check_circle_jump_point(uint8 type);

extern uint8 circle_x;
extern uint8 circle_y;
extern uint8 circle_point_flag;
void find_circle_point(uint8 type,uint8 kaishi_y,uint8 jiesu_y);
uint8 zebra_calc_black_center(void);
extern uint8 circle_x2;
extern uint8 circle_y2;
extern uint8 circle_point_flag2;
void find_circle_point2(uint8 type,uint8 down_y,uint8 up_y);
void Check_Out(void);
void qulv_characteristic();

uint8 refine_right(uint8 row, uint8 start);
uint8 refine_left(uint8 row, uint8 start);
void refine_turn_edges(void);

extern float l_jifen;
extern float r_jifen;
#endif /* CODE_IMAGE_H_ */