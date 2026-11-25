#include "image.h"
#include "string.h"
#include "math.h"
uint8 diuxian_sum = 0;
uint8 MID_W=91;/*改这个下面3个都要改动！！！！*/
float final_mid_line = 91;
uint8 start_col_for_search = 91;  /* 搜线起始列（由最长白列决定） */
uint8 last_mid_line = 91;

uint8 get_distance = 0;
uint8 zebra_center_x = 91;      /* 斑马线中心横坐标 */
uint8 zebra_center_valid = 0;   /* 斑马线中心有效标志 */
uint8 type2=0;//左环岛避免越界的操作
#define LIMIT_AB(value, min, max)  ((value)<(min) ? (min) : ((value)>(max) ? (max) : (value)))
#define ABS_WHITE_MUL       13.5
#define ABS_BLACK_MUL       6.5
#define BLACKPOINT          50
#define DIFF_RATIO_THRESHOLD 20
#define SEARCH_OFFSET       5
#define STOP_BLACK_ROWS     3
#define BINARY_OTSU_CROP_TOP       25
#define BINARY_OTSU_CROP_LEFT      44
#define BINARY_OTSU_CROP_HEIGHT    70
#define BINARY_OTSU_CROP_WIDTH     110
#define BINARY_OTSU_ROW_STEP       3
#define BINARY_OTSU_COL_STEP       5
#define BINARY_OTSU_DARK_MIN       40
#define BINARY_OTSU_MIN_THRESHOLD  100
#define BINARY_OTSU_MIN_DIFF       25
#define BINARY_OTSU_GRAY_STEP      4

void Calculate_Offset_1(void);

int turn_proline=MT9V03X_H-50;//最终前瞻行**********************************************************************

uint8 r_lose_value = 0;
uint8 l_lose_value = 0;

uint8 search_line_end = 20;
uint8 l_line_x[MT9V03X_H] = {0};
uint8 r_line_x[MT9V03X_H] = {0};

uint8 kuan[MT9V03X_H];
uint8 m_line_x[MT9V03X_H];
uint8 r_line_y[MT9V03X_H], l_line_y[MT9V03X_H];
/* uint8 r_line_x[MT9V03X_H], l_line_x[MT9V03X_H]; */
uint8 r_second_line_x[MT9V03X_H], l_second_line_x[MT9V03X_H];
uint8 m_second_line_x[MT9V03X_H];
/* uint8 r_lose_value=0, l_lose_value=0; */
uint8 r_search_flag[MT9V03X_H], l_search_flag[MT9V03X_H];
uint8 height, r_width, l_width;
uint8 r_losemax,l_losemax;

uint8 reference_point = 128;
uint8 white_max_point = 200;
uint8 white_min_point = 70;
uint8 image_gamma_enable = 0;
uint8 image_gamma_value = 100;

//uint8 image_01[MT9V03X_H][MT9V03X_W];
uint8 kuandu_saidao[MT9V03X_H - 1] = {
    170, 169, 168, 167, 167, 166, 165, 165, 164, 163,
    162, 162, 161, 160, 160, 159, 158, 157, 157, 156,
    155, 154, 150, 149, 147, 145, 144, 142, 141, 139,
    138, 136, 134, 133, 132, 130, 128, 126, 126, 124,
    122, 120, 119, 117, 116, 114, 113, 111, 109, 107,
    105, 105, 103, 101, 99, 98, 96, 94, 92, 91,
    90, 88, 86, 84, 83, 81, 79, 78, 76, 75,
    73, 71, 69, 68, 66, 64, 63, 61, 60, 58,
    56, 54, 53, 51, 49, 48, 46, 43, 41, 42,
    39, 38, 36, 34, 33, 30, 29, 31,
    30, 29, 28, 27, 26, 25, 24, 23, 22, 21,
    20, 19, 18, 17, 16, 15, 14, 13, 12
};
struct ROAD_TYPE road_type = {
    0,  /* straight */
    0,	/*short_straight*/
    0,  /* bend     */
    0,  /* Ramp     */
    0,  /* Cross    */
    0,  /* LeftCirque    */
    0,  /* RightCirque   */
    0,  /* ruwan         */
    0   /* banmaxian     */
};

void clean()
{
	memset(l_line_x, 0, 119);
	memset(r_line_x, MT9V03X_W-1, 119);
	memset(m_line_x, 91, 119);
	memset(l_search_flag, 0, 119);
	memset(r_search_flag, 0, 119);
}
void get_mid_line()//计算中值函数
{
	int i;
	if(type2){
		for (i = MT9V03X_H - 1; i>search_line_end; i--)
			{ 
					 m_line_x[i] = r_line_x[i] - kuandu_saidao[MT9V03X_H - 1 - i]*0.5-8;
			}
			return;
	}
	for (i = MT9V03X_H - 1; i>search_line_end; i--)
		{ 
				m_line_x[i] = (l_line_x[i] + r_line_x[i]) / 2;
		}
}

#define JIUJI_EXIT_DISTANCE  240.0f   /* 救急退出距离，根据实际调整（建议80~150） */

//变中值
void transfor_mid(void)
{
    /* 救急状态标志：0=无，1=左侧丢线救急，2=右侧丢线救急 */
    static uint8 jiuji_flag = 0;
    
    uint8 r_diuxian;
    uint8 l_diuxian;
    
    /* 0. 如果处于环岛/十字/斑马线，强制清零救急状态（防止残留干扰） */
    if (road_type.RightCirque || road_type.LeftCirque || 
        road_type.Cross || road_type.banmaxian)
    {
        jiuji_flag = 0;
        /* 注意：不清除 get_distance/annulus_s1，由元素处理函数自行管理 */
    }

    /* 1. 处理右环岛 */
    if (road_type.RightCirque) 
    {
        if (huandao_stage == 2) { MID_W = 82; return; }
        if (huandao_stage == 3) { MID_W = 76; return; }
        if (huandao_stage == 5) { MID_W = 83; return; }
        if (huandao_stage == 6) { MID_W = 91; return; }
    }

    /* 2. 处理左环岛 */
    if (road_type.LeftCirque) 
    {
        if (huandao_stage == 2) { MID_W = 100; return; }
        if (huandao_stage == 3) { MID_W = 106; return; }
        if (huandao_stage == 5) { MID_W = 99;  return; }
        if (huandao_stage == 6) { MID_W = 91;  return; }
    }

    if (road_type.bend)
    {
        if (jiuji_flag != 0)
        {
            if (annulus_s1 > JIUJI_EXIT_DISTANCE)
            {
                jiuji_flag = 0;
                get_distance = 0;
                annulus_s1 = 0;
            }
            else
            {
                if (jiuji_flag == 1)
                    MID_W = 145;  
                else
                    MID_W = 35;   
                return;
            }
        }
        if (road_type.bend)
        {
            Diuxian_weizhi_test(2, MT9V03X_H - 3, MT9V03X_H - 17);
            r_diuxian = diuxian_sum;
            Diuxian_weizhi_test(1, MT9V03X_H - 3, MT9V03X_H - 17);
            l_diuxian = diuxian_sum;
            

            if (r_diuxian > 13 && l_diuxian < 2 && search_line_end > MT9V03X_H - 42)
            {
                jiuji_flag = 2;         
                get_distance = 1;        
                annulus_s1 = 0;          
                MID_W = 35;
                return;
            }
            else if (l_diuxian > 13 && r_diuxian < 2 && search_line_end > MT9V03X_H - 42)
            {
                jiuji_flag = 1;          
                get_distance = 1;         
                annulus_s1 = 0;           
                MID_W = 145;
                return;
            }
        }
    }
    /* 4. 默认值（非环岛/非救急，或救急已退出） */
    MID_W = 91;
}


//图像处理主函数
void image_process(void)
{    
    if (mt9v03x_finish_flag == 1)
    {
        //Transfer_Camera();
        mt9v03x_finish_flag = 0;
        clean();//把上次搜线结果归零，避免影响环岛等处理
		
        //ips200_show_int8(0, 0, white_min_point);
		
        Search_edge();
        Element_Test();
		
        Element_Handle();
		get_mid_line();
		qulv_characteristic();//曲率计算，辅助路况判断
		transfor_mid();//变中线
		//turn_pro();//变前瞻
		Calculate_Offset_1();
		Check_Out();
        if (flag.image_show)
        {
            //Blacking();
            //ips200_show_int8(0, 0, final_mid_line - MID_W);
        }
    }
}

uint16 distance_L_Cross_out = 0;
uint16 distance_R_Cross_out = 0;
uint8 flag_L_shizi_R_turn = 0;
uint8 flag_R_shizi_L_turn = 0;
float annulus_s1 = 0;
int16 jiaodu_jifen = 0;
int16 annulus_jinku_s = 0;



int16 l_line_qulv = 0, r_line_qulv = 0;
/****************************************************元素检测主函数***********************************************************/
void Element_Test(void)
{	
	uint8 right_zhibian=check_zhibian(2);
	uint8 left_zhibian=check_zhibian(1);
	uint8 right_has_circle_point=check_circle_jump_point(2);
	uint8 left_has_circle_point=check_circle_jump_point(1);
	//检测斑马线
	zebra_test();

    //检测右环岛
	/*
    if (length > 98 && r_lose_value >= 18 && l_lose_value < 20
        && left_zhibian && right_has_circle_point
		&&!right_zhibian&&!left_has_circle_point)
    {
        if (road_type.RightCirque == 0)
        {
            road_type.RightCirque = 1;
            road_type.straight = 0;
            road_type.bend = 0;
        }
    }*/
	//检测左环岛
	/*if (length > 98 && l_lose_value >= 18 && r_lose_value < 20
		&& !left_zhibian && !right_has_circle_point
		&& right_zhibian&&left_has_circle_point)
    {
        if (road_type.LeftCirque == 0)
        {
            road_type.LeftCirque = 1;
            road_type.straight = 0;
            road_type.bend = 0;
        }
    }*/
	 //检测十字
	if(!road_type.RightCirque&&!road_type.LeftCirque)
   if(length > 95 && r_lose_value >18 && l_lose_value > 18
        &&!right_zhibian
        &&!left_zhibian) {
         road_type.Cross = 1;
         road_type.straight = 0;
         road_type.bend = 0;
    }
}

//变前瞻
void turn_pro()
{
		if(road_type.Cross){
				turn_proline=120-48;
			if(search_line_end>turn_proline-7)
				turn_proline=search_line_end;
				return;
		}
		if(road_type.straight||road_type.ruwan){
				turn_proline=120-65;
			if(search_line_end>turn_proline-7)
				turn_proline=search_line_end;
				return;
		}
		
		if(road_type.banmaxian){
				turn_proline=120-70;
				return;
		}
		if(road_type.LeftCirque||road_type.RightCirque){
				turn_proline=120-45;
				return;
		}

		if(road_type.bend){
				turn_proline=120-48;
			if(search_line_end>turn_proline-6)
				turn_proline=search_line_end;
			return;
		}
		turn_proline=120-50;
		if(search_line_end>turn_proline-6)
				turn_proline=search_line_end;
}

/*********************************************元素处理主函数************************************************************/
void Element_Handle(void)
{
	if (road_type.banmaxian)
    {
        Handle_Zebra();
        return;              /* 跳过其他元素，保持当前方向 */
    }
    if (road_type.LeftCirque)
    {
        Handle_Left_Cirque();
    }
    else if (road_type.RightCirque)
    {
        Handle_Right_Cirque();
    }
		

    if (road_type.Cross)
    {
        Handle_Cross();
				
    }

    //速度决策
   
}


void Transfer_Camera(void)
{
    //memcpy(image_01, mt9v03x_image, MT9V03X_IMAGE_SIZE);
}

/***********************************************************END*****************************************************************/


/****************************************************元素处理辅助函数**********************************************/

void qulv_characteristic(void)//对弯道，入弯，直道的检测函数
{
	
    int qulv_saver[20] = { 0 };
    int k = 0;
    uint8 i, j;
    int qulv_left, qulv_right;
    int jin_qu = 0, yuan_qu = 0;
    int max_value = 0, max_value_position = 0;
    int max_k = 0;

    /* 非普通赛道不判断 */
    if (road_type.Cross || road_type.LeftCirque || road_type.RightCirque)
    {
        road_type.straight = 0;
        road_type.ruwan = 0;
        road_type.bend = 0;
        return;
    }

    /* 计算曲率窗口 */
    for (i = search_line_end + 10; i < MT9V03X_H - 6; i += 4)
    {
        qulv_left = 0;
        qulv_right = 0;
        for (j = i; j < i + 5; j++)
        {
            if (m_line_x[j] > m_line_x[j + 1])
                qulv_right += m_line_x[j] - m_line_x[j + 1];
            else if (m_line_x[j + 1] > m_line_x[j])
                qulv_left += m_line_x[j + 1] - m_line_x[j];
        }
        qulv_saver[k++] = int_abs(qulv_right - qulv_left);
        if (k >= 20) break;
    }

    if (k < 3) return;

    /* 近处和远处曲率 */
    for (i = 0; i < k / 2; i++)
        yuan_qu += qulv_saver[i];
    for (i = k / 2; i < k; i++)
        jin_qu += qulv_saver[i];

    /* 找最大曲率及位置、最大变化率 */
    for (i = 0; i < k - 1; i++)
    {
        if (qulv_saver[i + 1] > max_value)
        {
            max_value = qulv_saver[i + 1];
            max_value_position = i + 1;
        }
        if (int_abs(qulv_saver[i + 1] - qulv_saver[i]) > max_k)
            max_k = int_abs(qulv_saver[i + 1] - qulv_saver[i]);
    }

    /* === 判断赛道类型 === */

		/* 入弯优先：近处曲率明显小于远处（还在入弯过程中） */
		if (yuan_qu > jin_qu * 2 &&      // 远处是近处的2倍以上（入弯特征）
				yuan_qu > 15 &&              // 远处确实有弯
				jin_qu < 15 &&               // 近处还没完全弯
				max_k > 3 &&                 // 曲率在变化
				search_line_end >= 28)
		{
				road_type.ruwan = 1;
				road_type.straight = 0;
				road_type.bend = 0;
		}
		/* 弯道：近处曲率已经接近或超过远处的一半（真正进入弯道） */
		else if ((search_line_end >= 35) ||
						 (search_line_end < 35 && jin_qu + yuan_qu > 25 && jin_qu >= 12 && yuan_qu <= jin_qu * 3))
		{
				road_type.bend = 1;
				road_type.straight = 0;
				road_type.ruwan = 0;
		}
		/* 直道 */
		else if (search_line_end <= 25 && max_value <= 3 && max_k <= 2)
		{
				road_type.straight = 1;
				road_type.ruwan = 0;
				road_type.bend = 0;
		}
		else
		{
				road_type.straight = 0;
				road_type.ruwan = 0;
				road_type.bend = 0;
		}
			/*printf("=== 曲率调试 ===\n");
			printf("k=%d, jin_qu=%d, yuan_qu=%d, total=%d\n", k, jin_qu, yuan_qu, jin_qu+yuan_qu);
			printf("max_value=%d, position=%d, max_k=%d\n", max_value, max_value_position, max_k);
			printf("search_line_end=%d\n", search_line_end);
			printf("曲率数组: ");
			for(i=0; i<k; i++) printf("%d ", qulv_saver[i]);
			printf("\n\n");*/
		}

/*******************************搜线函数***********************************************/

uint8 length = 0;
void Mid_Col(uint8 index[][MT9V03X_W])
{
    uint8 col_list[3] = { 48, 94, 140 };
    uint8 i;
    uint8 row;
    uint8 best_col = MID_W;
    uint8 best_len = 0;
    uint8 best_end_row = 10;
    uint8 white_len;
    uint8 black_count;
    uint8 is_mid_col;
    uint8 min_row;

    for (i = 0; i < 3; i++)
    {
        uint8 col = col_list[i];
        is_mid_col = (col == 94);

        white_len = 0;
        black_count = 0;

        /* 中间列+斑马线：留62行余量，保证能安全访问 row-52 */
        min_row = (is_mid_col && road_type.banmaxian) ? 62 : 10;

        for (row = MT9V03X_H - 1; row > min_row; row--)
        {
            if (index[row][col] > white_min_point)
            {
                white_len++;
                black_count = 0;
            }
            else
            {
                black_count++;
                if (black_count >= 3)
                {
                    /* 中间列+斑马线：向上偏移50行验证 */
                    if (is_mid_col && road_type.banmaxian &&
                        row > 62 &&
                        index[row - 50][col] < white_min_point &&
                        index[row - 51][col] < white_min_point &&
                        index[row - 52][col] < white_min_point)
                    {
                        break;
                    }
                    else if (is_mid_col && road_type.banmaxian)
                    {
                        black_count = 0;
                        continue;
                    }
                    break;
                }
            }
        }

        if (white_len > best_len)
        {
            best_len = white_len;
            best_col = col;
            best_end_row = row + 3;
        }
    }

    length = best_len;
    search_line_end = best_end_row;
    start_col_for_search = best_col;
}
void get_reference_point(const uint8 *image)
{
    uint8 row, col;
    uint16 valid_count = 0;
    uint16 sum = 0;
    uint8 pixel;
    uint8 start_row = MT9V03X_H - 10;
    uint8 end_row   = MT9V03X_H - 1;
    uint8 start_col = 90;
    uint8 end_col   = 100;

    for (row = start_row; row <= end_row; row++)
    {
        for (col = start_col; col <= end_col; col++)
        {
            pixel = image[row * MT9V03X_W + col];
            if (pixel > 30)
            {
                sum += pixel;
                valid_count++;
            }
        }
    }

    if (valid_count > 0)
        reference_point = (uint8)(sum / valid_count);
    else
        reference_point = 128;

    white_max_point = (uint8)LIMIT_AB((int)reference_point * ABS_WHITE_MUL / 10, BLACKPOINT, 255);
    white_min_point = (uint8)LIMIT_AB((int)reference_point * ABS_BLACK_MUL / 10, BLACKPOINT, 255);
}

static void applyGammaCorrection_C(uint8 image[MT9V03X_H][MT9V03X_W], uint8 gamma_value)
{
    static uint8 lookup_table[256];
    static uint8 cached_gamma_value = 0;
    static uint8 lookup_table_ready = 0;
    int i;
    int row;
    int col;
    double gamma;
    double inv_gamma;
    double corrected_value;

    if (gamma_value == 0)
    {
        return;
    }

    if (!lookup_table_ready || cached_gamma_value != gamma_value)
    {
        gamma = (double)gamma_value / 100.0;
        inv_gamma = 1.0 / gamma;

        for (i = 0; i < 256; i++)
        {
            corrected_value = pow((double)i / 255.0, inv_gamma) * 255.0;
            if (corrected_value > 255.0)
            {
                corrected_value = 255.0;
            }
            lookup_table[i] = (uint8)corrected_value;
        }

        cached_gamma_value = gamma_value;
        lookup_table_ready = 1;
    }

    for (row = 0; row < MT9V03X_H; row++)
    {
        for (col = 0; col < MT9V03X_W; col++)
        {
            image[row][col] = lookup_table[image[row][col]];
        }
    }
}

static uint8 calculate_binary_threshold(void)
{
    int i;
    int row;
    int col;
    uint8 gray;
    uint8 min_gray;
    uint8 max_gray;
    uint8 threshold;
    uint8 small_count;
    uint16 sample_count;
    uint16 w0;
    uint16 w1;
    uint16 mean0;
    uint16 mean1;
    uint16 diff;
    static uint16 pixel_count[256];
    uint32 gray_sum;
    uint32 sum0;
    uint32 sum1;
    uint32 score;
    uint32 best_score;
    static uint8 last_threshold = 90;
    static uint8 threshold_frame_div = 1;

    threshold_frame_div++;
    if (threshold_frame_div < 2)
    {
        return last_threshold;
    }
    threshold_frame_div = 0;

    min_gray = 255;
    max_gray = 0;
    sample_count = 0;
    gray_sum = 0;
    memset(pixel_count, 0, sizeof(pixel_count));

    for (row = BINARY_OTSU_CROP_TOP;
         row < BINARY_OTSU_CROP_TOP + BINARY_OTSU_CROP_HEIGHT;
         row += BINARY_OTSU_ROW_STEP)
    {
        for (col = BINARY_OTSU_CROP_LEFT;
             col < BINARY_OTSU_CROP_LEFT + BINARY_OTSU_CROP_WIDTH;
             col += BINARY_OTSU_COL_STEP)
        {
            gray = mt9v03x_image[row][col];
            pixel_count[gray]++;
            gray_sum += gray;
            sample_count++;

            if (gray < min_gray)
            {
                min_gray = gray;
            }
            if (gray > max_gray)
            {
                max_gray = gray;
            }
        }
    }

    threshold = last_threshold;
    if (sample_count > 0 && (uint8)(max_gray - min_gray) >= BINARY_OTSU_MIN_DIFF)
    {
        best_score = 0;
        small_count = 0;
        w0 = 0;
        sum0 = 0;

        for (i = 0; i < BINARY_OTSU_DARK_MIN; i++)
        {
            w0 += pixel_count[i];
            sum0 += (uint32)i * pixel_count[i];
        }

        for (i = BINARY_OTSU_DARK_MIN; i < max_gray && i < 255; i++)
        {
            w0 += pixel_count[i];
            sum0 += (uint32)i * pixel_count[i];

            if ((i & (BINARY_OTSU_GRAY_STEP - 1)) != 0)
            {
                continue;
            }

            if (w0 == 0 || w0 >= sample_count)
            {
                continue;
            }

            w1 = sample_count - w0;
            sum1 = gray_sum - sum0;
            mean0 = (uint16)(sum0 / w0);
            mean1 = (uint16)(sum1 / w1);
            diff = (mean0 > mean1) ? (mean0 - mean1) : (mean1 - mean0);
            score = (((uint32)w0 * (uint32)w1) >> 8) * (uint32)diff * (uint32)diff;

            if (score >= best_score)
            {
                best_score = score;
                threshold = (uint8)i;
                small_count = 0;
            }
            else
            {
                small_count++;
                if (small_count >= 5)
                {
                    break;
                }
            }
        }
    }

    if (threshold < BINARY_OTSU_MIN_THRESHOLD)
    {
        threshold = BINARY_OTSU_MIN_THRESHOLD;
    }

    if (threshold > last_threshold)
    {
        if ((uint8)(threshold - last_threshold) > 5)
        {
            threshold = (uint8)(last_threshold + 5);
        }
    }
    else if ((uint8)(last_threshold - threshold) > 5)
    {
        threshold = (uint8)(last_threshold - 5);
    }

    if (max_gray > 5 && threshold >= max_gray)
    {
        threshold = (uint8)(max_gray - 2);
    }

    last_threshold = threshold;
    return threshold;
}

uint8 Binarize_Mt9v03x_Image(void)
{
    int row;
    int col;
    uint8 threshold;

    if (image_gamma_enable && image_gamma_value > 0)
    {
        applyGammaCorrection_C(mt9v03x_image, image_gamma_value);
    }

    threshold = calculate_binary_threshold();
    for (row = 0; row < MT9V03X_H; row++)
    {
        for (col = 0; col < MT9V03X_W; col++)
        {
            mt9v03x_image[row][col] = (mt9v03x_image[row][col] > threshold) ? 1 : 0;
        }
    }

    return threshold;
}

uint8 search_left_edge(uint8 index[][MT9V03X_W], uint8 row, uint8 start)
{
    uint8 col;
    uint8 *p_row = &index[row][0];
    uint8 cur, left2;

    for (col = start; col > 3; col--)
    {
        cur   = p_row[col];
        left2 = p_row[col - 2];

        // 优先级1：参考点（左边点）是绝对白 → 还在赛道内，跳过
        if (left2 > white_max_point)
        {
            continue;
        }

        // 优先级2：参考点是绝对黑 → 这就是边界，返回参考点位置
        if (left2 < white_min_point)
        {
					if(cur<white_min_point)
					{
						return 1;
					}
					else
            return col - 2;
        }

        // 优先级3：中间灰度，计算差比和（需要亮→暗）
        if (cur > left2)
        {
            uint16 sum = (uint16)cur + left2;
            uint16 diff = cur - left2;

            if (sum > 0)
            {
                int16 ratio = (int16)(diff * 200 / sum);
                if (ratio > DIFF_RATIO_THRESHOLD)
                {
                    return col - 2;
                }
            }
        }
    }
    return 1;
}

uint8 search_right_edge(uint8 index[][MT9V03X_W], uint8 row, uint8 start)
{
    uint8 col;
    uint8 *p_row = &index[row][0];
    uint8 cur, right2;

    for (col = start; col < MT9V03X_W - 4; col++)
    {
        cur    = p_row[col];
        right2 = p_row[col + 2];

        // 优先级1：参考点（右边点）是绝对白 → 还在赛道内，跳过
        if (right2 > white_max_point)
        {
            continue;
        }

        // 优先级2：参考点是绝对黑 → 这就是边界，返回参考点位置
        if (right2 < white_min_point)
        {
					if(cur<white_min_point)
					{
						return MT9V03X_W-1;
					}
					else
            return col + 2;
        }

        // 优先级3：中间灰度，计算差比和（需要亮→暗）
        if (cur > right2)
        {
            uint16 sum = (uint16)cur + right2;
            uint16 diff = cur - right2;

            if (sum > 0)
            {
                int16 ratio = (int16)(diff * 200 / sum);
                if (ratio > DIFF_RATIO_THRESHOLD)
                {
                    return col + 2;
                }
            }
        }
    }
    return MT9V03X_W - 1;
}

void Search_edge(void)
{
    uint8 row;
    uint8 start_l, start_r;
    uint8 left_edge, right_edge;
    
    /* 每帧重置 */
    search_line_end = 10;
    
    /* 第0步：获取参考点 */
    get_reference_point((const uint8 *)mt9v03x_image);
    
    /* 初始化丢线计数 */
    l_lose_value = 0;
    r_lose_value = 0;
    
    /* ----- 调用 Mid_Col 计算最长白列，更新 search_line_end 和 start_col_for_search ----- */
    Mid_Col(mt9v03x_image);
    
    /* ----- 正式逐行搜索 ----- */
    for (row = MT9V03X_H - 1; row > search_line_end; row--)
    {
        /* 确定搜索起点 */
        if (row == MT9V03X_H - 1)
        {
            start_l = start_col_for_search;
            start_r = start_col_for_search;
        }
        else
        {
            /* 左起点 */
            if (l_search_flag[row + 1])
            {
                start_l = l_line_x[row + 1] + SEARCH_OFFSET;
                if (start_l > MT9V03X_W - 5) start_l = MT9V03X_W - 5;
            }
            else
            {
                start_l = start_col_for_search;
            }
            
            /* 右起点 */
            if (r_search_flag[row + 1])
            {
                start_r = r_line_x[row + 1] - SEARCH_OFFSET;
                if (start_r < 5) start_r = 5;
            }
            else
            {
                start_r = start_col_for_search;
            }
            
            if (start_l >= start_r)
            {
                start_l = start_col_for_search;
                start_r = start_col_for_search;
            }
        }
        
        /* 搜索左边界 */
        left_edge = search_left_edge(mt9v03x_image, row, start_l);
        if (left_edge > 1)
        {
            l_line_x[row] = left_edge;
            l_search_flag[row] = 1;
        }
        else
        {
            l_line_x[row] = 0;
            l_search_flag[row] = 0;
            l_lose_value++;
        }
        
        /* 搜索右边界 */
				right_edge = search_right_edge(mt9v03x_image, row, start_r);
        if (right_edge < MT9V03X_W - 1)
        {
            r_line_x[row] = right_edge;
            r_search_flag[row] = 1;
        }
        else
        {
            r_line_x[row] = MT9V03X_W - 1;
            r_search_flag[row] = 0;
            r_lose_value++;
        }
    }
}

void Blacking()
{
    for (height = (MT9V03X_H - 1); height > search_line_end; height--)
    {
        mt9v03x_image[height][(uint8)m_line_x[height]] = 0x00;
        mt9v03x_image[height][l_line_x[height]] = 0x00;
        mt9v03x_image[height][r_line_x[height]] = 0x00;
    }
    ips200_show_gray_image(0, 0, mt9v03x_image[0], MT9V03X_W, MT9V03X_H, MT9V03X_W, MT9V03X_H, 0);
}

void Calculate_Offset_1(void) 
{ 
    uint8 i; 
    uint16 weight_midline_sum = 0; 
    uint16 weight_sum = 0; 
    uint8 mid_line = MID_W;
    uint16 temp_mid;

    /* 只算一次 */
    for (i = turn_proline; i > turn_proline - 5; i--)  // 注意边界
    { 
        weight_midline_sum += m_line_x[i];
        weight_sum++;
    }
    if (weight_sum > 0) 
    { 
        mid_line = (uint8)(weight_midline_sum / weight_sum); 
    } 
    
    temp_mid = (last_mid_line * 2 + mid_line * 8) / 10;
    final_mid_line = (float)temp_mid;
    last_mid_line = temp_mid;
}

uint8 My_Sqrt(int16 x)
{
    uint8 ans = 0, p = 0x80;
    while (p != 0)
    {
        ans += p;
        if (ans * ans > x)
        {
            ans -= p;
        }
        p = (uint8)(p / 2);
    }
    return ans;
}

void La_zhixian(uint8 x1, uint8 y1, uint8 x2, uint8 y2, uint8 *array)
{
    int16 i;
    uint8 y_start, y_end, x_start, x_end;
    int16 diff_y;
    
    // 确保 y_start 是下方（大行号），y_end 是上方（小行号）
    if (y1 > y2)
    {
        y_start = y1; y_end = y2;
        x_start = x1; x_end = x2;
    }
    else
    {
        y_start = y2; y_end = y1;
        x_start = x2; x_end = x1;
    }
    
    diff_y = y_start - y_end;
    if (diff_y == 0) return;  // 防止除零
    
    for (i = y_start; i >= y_end; i--)
    {
        array[i] = x_start + (y_start - i) * (x_end - x_start) / diff_y;
    }
}

float parameterA = 0, parameterB = 0;

void Regression(uint8 type, uint8 startline, uint8 endline)//最小二乘法拟合
{
    uint8 i = 0;
    uint8 sumlines = endline - startline;
    int16 sumX = 0;
    int16 sumY = 0;
    float averageX = 0;
    float averageY = 0;
    float sumUp = 0;
    float sumDown = 0;

    if (type == 0)
    {
        for (i = startline; i < endline; i++)
        {
            sumX += i;
            sumY += m_line_x[i];
        }
        if (sumlines != 0)
        {
            averageX = sumX * 1.0 / sumlines;
            averageY = sumY * 1.0 / sumlines;
        }
        for (i = startline; i < endline; i++)
        {
            sumUp += (m_line_x[i] - averageY) * (i - averageX);
            sumDown += (i - averageX) * (i - averageX);
        }
        parameterB = sumUp / sumDown;
        parameterA = averageY - parameterB * averageX;
    }
    else if (type == 1)
    {
        for (i = startline; i < endline; i++)
        {
            sumX += i;
            sumY += l_line_x[i];
        }
        averageX = sumX * 1.0 / sumlines;
        averageY = sumY * 1.0 / sumlines;
        for (i = startline; i < endline; i++)
        {
            sumUp += (l_line_x[i] - averageY) * (i - averageX);
            sumDown += (i - averageX) * (i - averageX);
        }
        parameterB = sumUp / sumDown;
        parameterA = averageY - parameterB * averageX;
    }
    else if (type == 2)
    {
        for (i = startline; i < endline; i++)
        {
            sumX += i;
            sumY += r_line_x[i];
        }
        averageX = sumX * 1.0 / sumlines;
        averageY = sumY * 1.0 / sumlines;
        for (i = startline; i < endline; i++)
        {
            sumUp += (r_line_x[i] - averageY) * (i - averageX);
            sumDown += (i - averageX) * (i - averageX);
        }
        parameterB = sumUp / sumDown;
        parameterA = averageY - parameterB * averageX;
    }
}

void Hua_Xian(uint8 type, uint8 startline, uint8 endline, float parameterB, float parameterA)
{
    uint8 i;
    int i_int;

    if (type == 1)
    {
        for (i = startline; i < endline; i++)
        {
            l_line_x[i] = (uint8)(parameterB * i + parameterA);
            if (l_line_x[i] < 0)
            {
                l_line_x[i] = 0;
            }
        }
    }
    else if (type == 2)
    {
        for (i_int = startline; i_int < endline; i_int++)
        {
            r_line_x[i_int] = (uint8)(parameterB * i_int + parameterA);
            if (r_line_x[i_int] > 187)
            {
                r_line_x[i_int] = 187;
            }
        }
    }
    else if (type == 0)
    {
        for (i_int = startline; i_int < endline; i_int++)
        {
            m_line_x[i_int] = (uint8)((l_line_x[i_int] / 2 + r_line_x[i_int] / 2));
            if (m_line_x[i_int] < 0)
            {
                m_line_x[i_int] = 0;
            }
            if (m_line_x[i_int] > 185)
            {
                m_line_x[i_int] = 185;
            }
        }
    }
}

uint8 line_valid_start = 0;


//测试丢线位置的函数1左，2右
void Diuxian_weizhi_test(uint8 type, uint8 startline, uint8 endline)
{
    uint8 y;
    line_valid_start = 0;
    diuxian_sum = 0;

    if (type == 1)
    {
        for (y = startline; y >= endline; y--)
        {
            if (l_search_flag[y] == 1 && l_search_flag[y - 5] == 1 && line_valid_start == 0)
            {
                line_valid_start = y;
            }
            else if (l_search_flag[y] == 0 && l_search_flag[y - 5] == 0)
            {
                diuxian_sum++;
            }
        }
    }
    else if (type == 2)
    {
        for (y = startline; y >= endline; y--)
        {
            if (r_search_flag[y] == 1 && r_search_flag[y - 5] == 1 && line_valid_start == 0)
            {
                line_valid_start = y;
            }
            else if (r_search_flag[y] == 0 && r_search_flag[y - 5] == 0)
            {
                diuxian_sum++;
            }
        }
    }
}

uint8 l_guaidain_x1 = 0, l_guaidain_y1 = 0;
uint8 r_guaidain_x1 = 0, r_guaidain_y1 = 0;
uint8 l_guaidain_x2 = 0, l_guaidain_y2 = 0;
uint8 r_guaidain_x2 = 0, r_guaidain_y2 = 0;

uint8 Bianxian_guaidian_num(uint8 type, uint8 startline, uint8 endline)
{
    uint8 bianxian_guaidian_num = 0;
    uint8 y;

    l_guaidain_x1 = 0;
    l_guaidain_y1 = 0;
    r_guaidain_x1 = 0;
    r_guaidain_y1 = 0;
    l_guaidain_x2 = 0;
    l_guaidain_y2 = 0;
    r_guaidain_x2 = 0;
    r_guaidain_y2 = 0;

    if (type == 1)
    {
        if (l_line_x[MT9V03X_H - 1] != 0)
        {
            for (y = startline; y > endline; y--)
            {
                if (fabs(l_line_x[y] - l_line_x[y + 1]) < 4 && (l_line_x[y] - l_line_x[y - 3] > 8))
                {
                    l_guaidain_x1 = l_line_x[y];
                    l_guaidain_y1 = y;
                    bianxian_guaidian_num++;
                    break;
                }
            }
            if (bianxian_guaidian_num == 1)
            {
                for (y = l_guaidain_y1 - 20; y > endline; y--)
                {
                    if (fabs(l_line_x[y] - l_line_x[y - 1]) < 4 && (l_line_x[y] - l_line_x[y + 3] > 8))
                    {
                        l_guaidain_x2 = l_line_x[y];
                        l_guaidain_y2 = y;
                        bianxian_guaidian_num++;
                        break;
                    }
                }
            }
        }
    }
    if (type == 2)
    {
        if (r_line_x[MT9V03X_H - 1] != MT9V03X_W - 1)
        {
            for (y = startline; y > endline; y--)
            {
                if (fabs(r_line_x[y] - r_line_x[y + 1]) < 4 && (r_line_x[y] - r_line_x[y - 3] < -8))
                {
                    r_guaidain_x1 = r_line_x[y];
                    r_guaidain_y1 = y;
                    bianxian_guaidian_num++;
                    break;
                }
            }
            if (bianxian_guaidian_num == 1)
            {
                for (y = endline; y > r_guaidain_y1 - 20; y++)
                {
                    if (fabs(r_line_x[y] - r_line_x[y - 1]) < 4 && (r_line_x[y] - r_line_x[y + 3] < -8))
                    {
                        r_guaidain_x2 = r_line_x[y];
                        r_guaidain_y2 = y;
                        bianxian_guaidian_num++;
                        break;
                    }
                }
            }
        }
    }
    return bianxian_guaidian_num;
}

uint8 l_guaidain_x = 0, l_guaidain_y = 0;
uint8 r_guaidain_x = 0, r_guaidain_y = 0;

void Bianxian_guaidian(uint8 type, uint8 startline, uint8 endline)
{
    uint8 y;

    l_guaidain_x = 0;
    l_guaidain_y = 0;
    r_guaidain_x = 0;
    r_guaidain_y = 0;

    if (type == 1)
    {
        if (l_line_x[MT9V03X_H - 1] != 0)
        {
            for (y = startline; y > endline; y--)
            {
                if (fabs(l_line_x[y] - l_line_x[y + 1]) < 4 && (l_line_x[y] - l_line_x[y - 3] > 8))
                {
                    l_guaidain_y = y;
                    l_guaidain_x = l_line_x[y];
                    break;
                }
            }
        }
    }
    if (type == 2)
    {
        if (r_line_x[MT9V03X_H - 1] != MT9V03X_W - 1)
        {
            for (y = startline; y > endline; y--)
            {
                if (fabs(r_line_x[y] - r_line_x[y + 1]) < 4 && (r_line_x[y] - r_line_x[y - 3] < -8))
                {
                    r_guaidain_y = y;
                    r_guaidain_x = r_line_x[y];
                    break;
                }
            }
        }
    }
}



uint8 Num = 0;
float l_jifen=65;
//*****************************************环岛**************************************************//
uint8 huandao_stage = 1;

//*************************************************************************
void Handle_Left_Cirque()
{
    uint8 y;
    switch (huandao_stage)
    {
        case 1:
            /*if(!subed)
            {
                g_base_speed -=25;
                subed = 1;
            }*/
            Diuxian_weizhi_test(1, MT9V03X_H - 5, MT9V03X_H - 35);
            if (diuxian_sum >= 28)
                huandao_stage = 2;
            break;

        case 2:
            Diuxian_weizhi_test(1, MT9V03X_H - 5, MT9V03X_H - 36);
            if (line_valid_start >= 100 && diuxian_sum < 5)
            {
                huandao_stage = 3;
            }
            break;

        case 3:
            find_circle_point(1, 30, 85);
						get_distance=1;
				
            if (annulus_s1 > 340)
            {
                Diuxian_weizhi_test(1, MT9V03X_H - 1, MT9V03X_H - 30);
                if (diuxian_sum >= 28&&!circle_point_flag)
                {
                    huandao_stage = 4;
                    get_distance = 0;
                }
            }
            break;

        case 4:
            get_distance = 1;//启动距离积分
            find_circle_point2(2, 65, 20);
            if (circle_point_flag2 == 1 && annulus_s1 > 60)
            {
                huandao_stage = 5;
                get_distance = 0;
            }
            break;

        case 5:
            find_circle_point2(2, 105, 30);
            Diuxian_weizhi_test(2, MT9V03X_H - 15, MT9V03X_H - 40);
            if (diuxian_sum >= 23)
                huandao_stage = 6;
            break;

        case 6:
            get_distance = 1;
            Diuxian_weizhi_test(1, MT9V03X_H - 65, MT9V03X_H - 85);
            if (diuxian_sum < 5 || annulus_s1 > l_jifen)
            {
                huandao_stage = 7;
                get_distance = 0;
            }
            break;
        case 7:
            get_distance = 1;
            if (annulus_s1 > 300)
            {
								type2=0;//避免越界的操作
                road_type.LeftCirque = 0;
                huandao_stage = 1;
                get_distance = 0;
            }
            break;
    }

    // 补线操作
    if (huandao_stage == 1)
    {
        for (y = MT9V03X_H - 10; y >= search_line_end; y--)
        {
            l_line_x[y] = r_line_x[y] - kuandu_saidao[MT9V03X_H - 1 - y];
        }
    }
    else if (huandao_stage == 2)
    {
        for (y = MT9V03X_H - 10; y >= search_line_end; y--)
        {
            l_line_x[y] = r_line_x[y] - kuandu_saidao[MT9V03X_H - 1 - y];
        }
    }
    else if (huandao_stage == 3)
    {
        if (circle_point_flag)
        {
            La_zhixian(MT9V03X_W - 3, 118, circle_x, circle_y, r_line_x);
        }
    }
    else if (huandao_stage == 4)
    {
        // 等待，不补线
    }
    else if (huandao_stage == 5)
    {
        if (circle_point_flag2 == 1)
        {
            La_zhixian(circle_x2, circle_y2, 7, 50, r_line_x);
        }
    }
    else if (huandao_stage == 6)
    {
        uint8 start_row = search_line_end - 10;
        uint8 start_col = l_line_x[search_line_end - 10];
        La_zhixian(start_col, start_row, MT9V03X_W - 3, 118, r_line_x);
    }
    else if (huandao_stage == 7)
    {
				type2=1;
    }
}
float r_jifen=60;
//*************************************************************************
void Handle_Right_Cirque()
{
    uint8 y;
    switch (huandao_stage)
    {
        case 1:
					/*if(!subed)
					{
						g_base_speed-=15;
						subed=1;
					}*/
            Diuxian_weizhi_test(2, MT9V03X_H - 5, MT9V03X_H - 35);
            if (diuxian_sum >= 28)
                huandao_stage = 2;
            break;
        case 2:
					/*
						if(	you_pian(2,MT9V03X_H-5,MT9V03X_H-21)	)//右偏,范围为偶数
						{
							huandao_stage = 3;
						}
						break;
				*/
					
            Diuxian_weizhi_test(2, MT9V03X_H - 5, MT9V03X_H - 36);
            if (line_valid_start >= 100 && diuxian_sum < 5)
            {
                huandao_stage = 3;
							
            }
            break;
				
	    case 3:
		    find_circle_point(2, 30, 80);
        get_distance = 1;
		    if(annulus_s1>340)//积分距离如何把握
		    {
				    Diuxian_weizhi_test(2, MT9V03X_H - 1, MT9V03X_H - 30);//右下完全丢线
				    {
					    if (diuxian_sum>= 28)
					    {
						    huandao_stage = 4;
                get_distance = 0;
					    }
				    }
		    }
		    break;
        case 4:
            get_distance = 1;
            find_circle_point2(1, 65, 20);//想先积分再找，避免提前找
            if (circle_point_flag2 == 1&&annulus_s1>60)
            {
                huandao_stage = 5;
                get_distance = 0;
            }
            break;
        case 5:
			find_circle_point2(1, 105, 30);//find_circle_point2(1, 70, 2);
            Diuxian_weizhi_test(1, MT9V03X_H - 15, MT9V03X_H - 40);
            if (diuxian_sum >= 23)
                huandao_stage = 6;
            break;
        case 6:
        get_distance = 1;
		    Diuxian_weizhi_test(2, MT9V03X_H - 65, MT9V03X_H - 85);
			if(diuxian_sum<5||annulus_s1>r_jifen)
			{
				huandao_stage=7;
        get_distance = 0;
			}
            break;
        case 7:
            get_distance = 1;
            if (annulus_s1 > 300)
            {
                road_type.RightCirque = 0;
                huandao_stage = 1;
                get_distance = 0;
            }
            break;
    }

    if (huandao_stage == 1)
    {
        for (y = MT9V03X_H - 10; y >= search_line_end; y--)
        {
            r_line_x[y] = l_line_x[y] + kuandu_saidao[MT9V03X_H - 1 - y];
        }
    }
    else if (huandao_stage == 2)
    {
        for (y = MT9V03X_H - 10; y >= search_line_end; y--)
        {
            r_line_x[y] = l_line_x[y] + kuandu_saidao[MT9V03X_H - 1 - y];
        }
    }
    else if (huandao_stage == 3)
    {
        if (circle_point_flag)
        {
            La_zhixian(2, 118, circle_x, circle_y, l_line_x);
        }
    }	
    else if (huandao_stage == 4)
    {
			
    }
    else if (huandao_stage == 5)
    {
        if (circle_point_flag2 == 1)
        {
            La_zhixian(circle_x2, circle_y2, 180, 50, l_line_x);
        }
    }
		else if (huandao_stage == 6)
		{
				uint8 start_row = search_line_end-10;
				uint8 start_col = r_line_x[search_line_end-10];
				/* 拉线补左边界：从右上角点拉到左下(2, 118) */
				La_zhixian(start_col, start_row, 2, 118, l_line_x);
		}
    else if (huandao_stage == 7)
    {
        for (y = MT9V03X_H - 10; y >= search_line_end; y--)
        {
            r_line_x[y] = l_line_x[y] + kuandu_saidao[MT9V03X_H - 1 - y]+11;
        }
    }

}
//************************************************end*************************************//


//**********************************************十字***************************************//
uint16 cross_s = 0;

void Handle_Cross(void)
{
    get_distance = 1;
    if (annulus_s1 > 180)
    {
        get_distance = 0;
        cross_s = 0;
        road_type.Cross = 0;
    }
		
    Check_Cross_Guaidian(1);//检测左边
    if (found_left_corner[0] && found_left_corner[1])
    {
        La_zhixian(l_line_x[cross_left[0]], cross_left[0], l_line_x[cross_left[1]], cross_left[1], l_line_x);
    }
    else if (!found_left_corner[0] && found_left_corner[1])
    {
        Regression(1, cross_left[1] - 10, cross_left[1]);
        Hua_Xian(1, cross_left[1], MT9V03X_H - 3, parameterB, parameterA);
    }
    Check_Cross_Guaidian(2);//检测右边
    if (found_right_corner[0] && found_right_corner[1])
    {
        La_zhixian(r_line_x[cross_right[0]], cross_right[0], r_line_x[cross_right[1]], cross_right[1], r_line_x);
    }
    else if (!found_right_corner[0] && found_right_corner[1])
    {
        Regression(2, cross_right[1] - 10, cross_right[1]);
        Hua_Xian(2, cross_right[1], MT9V03X_H - 3, parameterB, parameterA);
    }
}

uint8 cross_left[2] = {0, 0};
uint8 cross_right[2] = {0, 0};
uint8 found_left_corner[2] = {0, 0};
uint8 found_right_corner[2] = {0, 0};

void Check_Cross_Guaidian(uint8 type)
{
    uint8 y1;
    uint8 y2;
    uint8 y;
    uint8 start_s = MT9V03X_H - 10;  // 从底部向上10行开始搜索
	
    cross_left[0] = cross_left[1] = cross_right[0] = cross_right[1] = 0;
    found_left_corner[0] = found_left_corner[1] = found_right_corner[0] = found_right_corner[1] = 0;

    if (type == 1)
    {
        for (y1 = start_s; y1 >= MT9V03X_H / 4 * 3; y1--)
        {
            if (fabs(l_line_x[y1] - l_line_x[y1 - 1]) < 4 && (l_line_x[y1] - l_line_x[y1 - 10]) > 16 )//&& l_line_x[y1 - 10] == 0
            {
                cross_left[0] = y1;
                found_left_corner[0] = 1;
                break;
            }
        }
        if (found_left_corner[0])
        {
            for (y2 = cross_left[0] - 1; y2 >= 25; y2--)
            {
                if (fabs(l_line_x[y2] - l_line_x[y2 - 1]) < 4 && l_line_x[y2] - l_line_x[y2 - 5] < -16 )//&& l_line_x[y2] == 0
                {
                    cross_left[1] = y2 - 5 - 5;
                    found_left_corner[1] = 1;
                    break;
                }
            }
        }
        else
        {
            for (y2 = MT9V03X_H / 4 * 3; y2 < start_s; y2++)
            {
                if (fabs(l_line_x[y2] - l_line_x[y2 - 1]) < 4 && l_line_x[y2] - l_line_x[y2 - 5] < -16 )//&& l_line_x[y2] == 0
                {
                    cross_left[1] = y2 - 5 - 5;
                    found_left_corner[1] = 1;
                    break;
                }
            }
        }
    }
    else if (type == 2)
    {
        for (y1 = start_s; y1 >= MT9V03X_H / 4 * 3; y1--)
        {
            if (fabs(r_line_x[y1] - r_line_x[y1 - 1]) < 4 && (r_line_x[y1] - r_line_x[y1 - 10]) < -16 && r_line_x[y1 - 10] == MT9V03X_W - 1)
            {
                cross_right[0] = y1;
                found_right_corner[0] = 1;
                break;
            }
        }
        if (found_right_corner[0])
        {
            for (y2 = cross_right[0] - 1; y2 >= 25; y2--)
            {
                if (fabs(r_line_x[y2] - r_line_x[y2 - 1]) < 4 && (r_line_x[y2] - r_line_x[y2 - 5]) > 16 && r_line_x[y2] == MT9V03X_W - 1)
                {
                    cross_right[1] = y2 - 5 - 5;
                    found_right_corner[1] = 1;
                    break;
                }
            }
        }
        else
        {
            for (y = MT9V03X_H / 4 * 3; y < start_s; y++)
            {
                if (fabs(r_line_x[y] - r_line_x[y - 1]) < 4 && r_line_x[y] - r_line_x[y - 10] > 16 && r_line_x[y] == MT9V03X_W - 1)
                {
                    cross_right[1] = y - 5 - 5;
                    found_right_corner[1] = 1;
                    break;
                }
            }
        }
    }
}
//*********************************************end**********************************************************//
uint8 check_zhibian(uint8 type)//距离下方50到90行
{
    int straight_count = 0;
    uint8 h;

    if (type == 1)
    {
        for (h = MT9V03X_H - 50; h > MT9V03X_H  - 90; h -= 4)
        {
            if (l_line_x[h] - l_line_x[h - 4] > -6 && l_line_x[h] - l_line_x[h - 4] <= -2)
            {
                straight_count++;
            }
        }
        if (straight_count >= 7)
            return 1;
        return 0;
    }
    if (type == 2)
    {
        straight_count = 0;
        for (h = MT9V03X_H - 50; h > MT9V03X_H  - 90; h -= 4)//50~90
        {
            if (r_line_x[h] - r_line_x[h - 4] < 6 && r_line_x[h] - r_line_x[h - 4] >= 2)
            {
                straight_count++;
            }
        }
        if (straight_count >= 7)
            return 1;
        return 0;
    }
    return 0;
}

uint8 check_circle_jump_point(uint8 type)//从距离底部10行到一半（60行）
{
    uint8 h;
    if(type==1)
    {
        for(h=MT9V03X_H-10;h>MT9V03X_H/2;h--)
        {
            if(l_line_x[h]-l_line_x[h-4]>10 && l_line_x[h-4]<5)
            {
                return 1;
            }
        }
        return 0;
    }
    if(type==2)
    {
        for(h=MT9V03X_H-10;h>MT9V03X_H/2;h--)
        {
            if(r_line_x[h]-r_line_x[h-4]<-10 && r_line_x[h-4]>188-5)
            {
                return 1;
            }
        }
        return 0;
    }
    return 0;
}

uint8 circle_x=0;
uint8 circle_y=0;
uint8 circle_point_flag=0;

void find_circle_point(uint8 type, uint8 kaishi_y, uint8 jiesu_y)
{
    uint8 h;
    circle_x = 0;
    circle_y = 0;
    circle_point_flag = 0;
    
    if (type == 1)  // 检测左边界
    {
        for (h = kaishi_y; h < jiesu_y; h++)
        {
            // 左边界角点特征：局部向左凸出（列坐标突然变小，然后恢复）
            if (l_line_x[h] - l_line_x[h+2] > 20 && 
                l_line_x[h] - l_line_x[h-2] > -4 
                )
            {
                circle_x = l_line_x[h];
                circle_y = h;
                circle_point_flag = 1;
                break;
            }
        }
    }
    else if (type == 2)  // 检测右边界
    {
        for (h = kaishi_y; h < jiesu_y; h++)
        {
            // 右边界角点特征：局部向右凸出（列坐标突然变大，然后恢复）
            if (r_line_x[h] - r_line_x[h+2] < -20 && 
                r_line_x[h] - r_line_x[h-2] < 4 
                )
            {
                circle_x = r_line_x[h];
                circle_y = h;
                circle_point_flag = 1;
                break;
            }
        }
    }
}
uint8 circle_x2=0;
uint8 circle_y2=0;
uint8 circle_point_flag2=0;

void find_circle_point2(uint8 type,uint8 down_y,uint8 up_y)
{
    uint8 h;
    circle_x2=0;
    circle_y2=0;
    circle_point_flag2=0;
    if(type==1)
    {
        for(h=down_y;h>=up_y;h=h-1)
        {
            if(l_line_x[h]>l_line_x[h-3]&&l_line_x[h]>l_line_x[h+3])
            {
                circle_x2=l_line_x[h+2];
                circle_y2=h+2;
                circle_point_flag2=1;
                break;
            }
        }
    }
    if(type==2)
    {
        for(h=down_y;h>=up_y;h=h-1)
        {
            if(r_line_x[h] < r_line_x[h-3] && r_line_x[h] < r_line_x[h+3] )
            {
                circle_x2=r_line_x[h+2];
                circle_y2=h+2;
                circle_point_flag2=1;
                break;
            }
        }
    }
}

/**
 * @brief 冲出赛道检测
 * 条件1：最长白列长度 < 10
 * 条件2：距离底部10行处，中间区域90%以上为黑色
 * 同时满足则触发停车
 */
void Check_Out(void)
{
    uint8 col;
    uint16 black_count = 0;
    uint16 total_count = 0;
    uint8 row = MT9V03X_H - 15;  /* 固定距离底部10行的那一行 */
    
    /* 条件2：该行中间区域(40~149列)统计黑色比例 */
    for (col = 10; col < 170; col++)
    {
        total_count++;
        if (mt9v03x_image[row][col] < 130)//大了灵敏130
        {
            black_count++;
        }
    }
    
    /* 两个条件同时满足 */
    if (total_count > 0 && (black_count * 100 / total_count) > 90)
    {
        flag.stop = 1;  /* 触发停车 */
    }
}


/* ==================== 斑马线检测宏定义 ==================== */
#define ZEBRA_SCAN_START_ROW  (MT9V03X_H - 55)  /* 扫描起始行，靠近底部 */
#define ZEBRA_SCAN_END_ROW    (MT9V03X_H - 52)   /* 扫描结束行 */
#define ZEBRA_BLACK_MIN_WIDTH 1                 /* 黑段最小宽度，过滤噪点 */
#define ZEBRA_BLACK_MAX_WIDTH 10                /* 黑段最大宽度，防止把长边界算进去 */
#define ZEBRA_MIN_SEGMENTS    8                 /* 一行最少黑段数（正常赛道1-2，斑马线5+） */
#define ZEBRA_CONFIRM_ROWS    3                 /* 最少确认行数 */
#define ZEBRA_EXIT_DISTANCE   130.0f            /* 编码器积分退出距离，根据实际调整 */
void zebra_test(void)
{
    int16 row;
    uint8 col;
    uint8 in_black;
    uint8 seg_len;
    uint8 seg_count;
    uint8 valid_zebra_rows = 0;
    uint16 sum_center = 0;
    uint8 seg_start = 0;
    uint8 first_center = 0;
    uint8 last_center = 0;
    uint8 has_first = 0;
    
    /* 已在斑马线、环岛或十字期间不重复检测 */
    if (road_type.banmaxian || road_type.LeftCirque || road_type.RightCirque || road_type.Cross)
        return;
    
    for (row = ZEBRA_SCAN_END_ROW; row >= ZEBRA_SCAN_START_ROW; row--)
    {
        in_black = 0;
        seg_count = 0;
        seg_len = 0;
        has_first = 0;
        
        /* 从0到187全宽扫描，统计黑段个数并记录首尾黑段中心 */
        for (col = 0; col < MT9V03X_W; col++)
        {
            if (mt9v03x_image[row][col] < white_min_point)
            {
                if (!in_black)
                {
                    in_black = 1;
                    seg_start = col;
                    seg_len = 1;
                }
                else
                {
                    seg_len++;
                }
            }
            else
            {
                if (in_black)
                {
                    in_black = 0;

                    if (seg_len >= ZEBRA_BLACK_MIN_WIDTH && seg_len <= ZEBRA_BLACK_MAX_WIDTH)
                    {
                        seg_count++;
                        if (!has_first)
                        {
                            first_center = (seg_start + (col - 1)) / 2;
                            has_first = 1;
                        }
                        last_center = (seg_start + (col - 1)) / 2;
                    }
                    seg_len = 0;
                }
            }
        }
        
        /* 行尾处理：如果最后一段是黑段 */
        if (in_black && seg_len >= ZEBRA_BLACK_MIN_WIDTH && seg_len <= ZEBRA_BLACK_MAX_WIDTH)
        {
            seg_count++;
            if (!has_first)
            {
                first_center = (seg_start + (MT9V03X_W - 1)) / 2;
                has_first = 1;
            }
            last_center = (seg_start + (MT9V03X_W - 1)) / 2;
        }
        
        if (seg_count >= ZEBRA_MIN_SEGMENTS && has_first)
        {
            valid_zebra_rows++;
            /* 该行首尾黑段中点作为该行斑马线中心 */
            sum_center += (first_center + last_center) / 2;
        }
    }
    
    /* 多行确认，触发斑马线状态 */	
    if (valid_zebra_rows >= ZEBRA_CONFIRM_ROWS)
    {
        road_type.banmaxian = 1;
        road_type.straight = 0;
        road_type.bend = 0;
        road_type.ruwan = 0;
        get_distance = 1;      /* 启动编码器积分 */
        annulus_s1 = 0;        /* 清零，重新计数 */
        
        /* 计算斑马线中心横坐标（多行平均） */
        if (valid_zebra_rows > 0)
        {
            zebra_center_x = (uint8)(sum_center / valid_zebra_rows);
            zebra_center_valid = 1;
        }
    }
}
uint8 ban_count=0;
void Handle_Zebra(void)
{
    if(road_type.banmaxian){
        /* printf("banmaxian!!!\n"); */
    }
    if (!road_type.banmaxian)
        return;
    if (annulus_s1 > ZEBRA_EXIT_DISTANCE)
    {
        road_type.banmaxian = 0;
        get_distance = 0;
        annulus_s1 = 0;
        zebra_center_valid = 0;  /* 退出斑马线后清零 */
		ban_count++;
    }
}
