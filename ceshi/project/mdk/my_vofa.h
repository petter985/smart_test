#ifndef __MY_VOFA_H__
#define __MY_VOFA_H__

#include "zf_common_headfile.h"

//==================================================================
//                          用户配置区
//==================================================================

// WiFi 配置（连接路由器）
//#define VOFA_WIFI_SSID              "readme"            // WiFi 名称
//#define VOFA_WIFI_PASSWORD          "34567821"          // WiFi 密码

#define VOFA_WIFI_SSID          "Runner"
#define VOFA_WIFI_PASSWORD      "060920tr"

// VOFA+ 服务器配置（VOFA+ 作为 TCP Server）
//#define VOFA_TARGET_IP              "10.110.4.107"   // 运行 VOFA+ 的电脑 IP
#define VOFA_TARGET_IP              "192.168.43.245"
#define VOFA_TARGET_PORT            8089                // VOFA+ 监听端口
#define VOFA_LOCAL_PORT             8089                // 本地端口

// 通道与参数配置
#define VOFA_MAX_CHANNELS           10                  // 最大波形通道数
#define VOFA_MAX_PARAMS             16                  // 最大参数个数

// 协议选择（1=JustFloat二进制, 0=FireWater文本）
#define VOFA_USE_JUSTFLOAT          1

// 参数定义上报开关（1=开机自动上报参数列表，方便VOFA+显示）
#define VOFA_REPORT_PARAM_DEF       1

// 接收缓冲区大小
#define VOFA_RX_BUF_SIZE            128

//==================================================================
//                          数据结构
//==================================================================

// 参数结构体
typedef struct {
    char     nam[16];     // 参数名（显示用）
    float    value;        // 当前值
    float    min;          // 最小值
    float    max;          // 最大值
    float    step;         // 步进（显示用）
    uint8    updated;      // 更新标志（1=有新值，读取后自动清零）
    uint8    used;         // 是否已注册
} vofa_param_t;

//==================================================================
//                          API 接口
//==================================================================

/**
 * @brief 初始化 WiFi 并连接 VOFA+ 服务器
 * @note  阻塞直到连接成功
 */
void my_vofa_init(void);

/**
 * @brief VOFA+ 主处理函数（接收命令、解析参数）
 * @note  需在主循环中周期调用（建议 5~20ms 一次）
 */
void my_vofa_process(void);

/**
 * @brief 发送波形数据到 VOFA+
 * @param  data        浮点数组指针
 * @param  channel_num 通道数（不超过 VOFA_MAX_CHANNELS）
 * @note   根据 VOFA_USE_JUSTFLOAT 选择协议
 */
void vofa_send_wave(float *dat, uint8 channel_num);

/**
 * @brief 注册参数
 * @param  index 参数索引（0 ~ VOFA_MAX_PARAMS-1）
 * @param  name  参数名（最多15字符）
 * @param  value 初始值
 * @param  min   最小值
 * @param  max   最大值
 * @param  step  步进
 */
void vofa_register_param(uint8 index, const char *nam, float value, float min, float max, float step);

/**
 * @brief 获取参数当前值
 * @param  index 参数索引
 * @return 参数值
 */
float vofa_get_param(uint8 index);

/**
 * @brief 直接设置参数值（代码侧修改，不触发updated标志）
 * @param  index 参数索引
 * @param  value 新值
 */
void vofa_set_param(uint8 index, float value);

/**
 * @brief 检查参数是否被上位机更新
 * @param  index 参数索引
 * @return 1=已更新（读取后自动清零），0=未更新
 * @note   典型用法：if(vofa_param_updated(0)) { Kp = vofa_get_param(0); }
 */
uint8 vofa_param_updated(uint8 index);

/**
 * @brief 上报所有已注册参数的定义（名称、范围等）
 * @note  开机自动调用一次；也可手动调用重新上报
 */
void vofa_report_params(void);

/**
 * @brief 上报所有参数的当前值
 * @note  可周期调用，让 VOFA+ 显示当前参数状态
 */
void vofa_report_values(void);

#endif // __MY_VOFA_H__
