/**
 * @file    my_vofa.c
 * @brief   VOFA+ WiFi 调参模块实现（适配逐飞 WiFi2 模块 + STC32G144K）
 * @version V1.0
 */
#include "my_vofa.h"

//==================================================================
//                          内部定义
//==================================================================

// JustFloat 协议帧尾（float 类型的 NaN：0x7f800000）
#define VOFA_JUSTFLOAT_TAIL0    0x00
#define VOFA_JUSTFLOAT_TAIL1    0x00
#define VOFA_JUSTFLOAT_TAIL2    0x80
#define VOFA_JUSTFLOAT_TAIL3    0x7f

// 命令起始符
#define VOFA_CMD_SET            '$'     // 设置参数：$<index>,<value>\n
#define VOFA_CMD_REPORT_DEF     '#'     // 请求上报参数定义：#\n
#define VOFA_CMD_REPORT_VAL     '?'     // 请求上报参数值：?\n

//==================================================================
//                          内部变量
//==================================================================

// 参数表
static vofa_param_t vofa_params[VOFA_MAX_PARAMS];

// 接收缓冲区（命令解析用）
static uint8  vofa_rx_buf[VOFA_RX_BUF_SIZE];
static uint16 vofa_rx_len = 0;

// 发送缓冲区（波形发送用，JustFloat: 通道数*4 + 4字节帧尾）
static uint8  vofa_tx_buf[VOFA_MAX_CHANNELS * 4 + 4];

// 文本发送缓冲区（参数上报用）
static uint8  vofa_text_buf[96];

//==================================================================
//                     底层 WiFi 收发封装
//==================================================================

/**
 * @brief 通过 WiFi 发送数据
 */
static void vofa_wifi_send(uint8 *dat, uint16 len)
{
    if(len > 0)
    {
        wifi_spi_send_buffer(dat, (uint32)len);
    }
}

/**
 * @brief 通过 WiFi 接收数据
 * @return 实际接收的字节数
 * @note   逐飞库中函数名拼写为 "reveive"（非 "receive"），
 *         如编译报错未定义，请将下方函数名改为 wifi_spi_receive_buffer
 */
static uint16 vofa_wifi_receive(uint8 *dat, uint16 max_len)
{
    return (uint16)wifi_spi_read_buffer(dat, (uint32)max_len);
}

//==================================================================
//                     JustFloat 波形发送
//==================================================================

/**
 * @brief JustFloat 协议发送波形（二进制）
 * @note  STC32G 为小端序，float 内存布局与协议一致，可直接拷贝
 */
static void vofa_send_justfloat(float *dat, uint8 channel_num)
{
    uint8  i;
    uint8 *ptr;
    uint16 total_len;

    if(channel_num > VOFA_MAX_CHANNELS)
    {
        channel_num = VOFA_MAX_CHANNELS;
    }
    if(channel_num == 0)
    {
        return;
    }

    ptr = vofa_tx_buf;

    // 拷贝 float 数据（小端序直接拷贝，STC32G 天然兼容）
    for(i = 0; i < channel_num; i++)
    {
        // 逐字节拷贝，避免对齐问题
        *ptr++ = ((uint8 *)&dat[i])[0];
        *ptr++ = ((uint8 *)&dat[i])[1];
        *ptr++ = ((uint8 *)&dat[i])[2];
        *ptr++ = ((uint8 *)&dat[i])[3];
    }

    // 帧尾
    *ptr++ = VOFA_JUSTFLOAT_TAIL0;
    *ptr++ = VOFA_JUSTFLOAT_TAIL1;
    *ptr++ = VOFA_JUSTFLOAT_TAIL2;
    *ptr++ = VOFA_JUSTFLOAT_TAIL3;

    total_len = (uint16)(channel_num * 4 + 4);
    vofa_wifi_send(vofa_tx_buf, total_len);
}

/**
 * @brief FireWater 协议发送波形（文本）
 */
static void vofa_send_firewater(float *dat, uint8 channel_num)
{
    uint16 offset = 0;
    uint8  i;

    if(channel_num > VOFA_MAX_CHANNELS)
    {
        channel_num = VOFA_MAX_CHANNELS;
    }
    if(channel_num == 0)
    {
        return;
    }

    for(i = 0; i < channel_num; i++)
    {
        if(i == 0)
        {
            offset += (uint16)sprintf((char *)&vofa_tx_buf[offset], "%.4f", dat[i]);
        }
        else
        {
            offset += (uint16)sprintf((char *)&vofa_tx_buf[offset], ",%.4f", dat[i]);
        }

        // 防止缓冲区溢出
        if(offset >= sizeof(vofa_tx_buf) - 8)
        {
            break;
        }
    }

    vofa_tx_buf[offset++] = '\n';
    vofa_wifi_send(vofa_tx_buf, offset);
}

//==================================================================
//                     简易浮点数解析（替代 atof）
//==================================================================

/**
 * @brief 解析字符串为浮点数（支持负数、小数）
 * @param  str 字符串指针
 * @param  end 解析结束位置（输出参数，可为 NULL）
 * @return 解析得到的浮点数
 */
static float vofa_atof(const uint8 *str, const uint8 **end)
{
    float  result = 0.0f;
    float  sign = 1.0f;
    float  decimal_div = 10.0f;
    uint8  has_decimal = 0;
    const uint8 *p = str;

    // 跳过空格
    while(*p == ' ' || *p == '\t')
    {
        p++;
    }

    // 符号
    if(*p == '-')
    {
        sign = -1.0f;
        p++;
    }
    else if(*p == '+')
    {
        p++;
    }

    // 整数部分和小数部分
    while(*p != '\0' && *p != '\n' && *p != '\r' && *p != ',' && *p != '}')
    {
        if(*p >= '0' && *p <= '9')
        {
            if(has_decimal)
            {
                result += (float)(*p - '0') / decimal_div;
                decimal_div *= 10.0f;
            }
            else
            {
                result = result * 10.0f + (float)(*p - '0');
            }
        }
        else if(*p == '.')
        {
            has_decimal = 1;
        }
        else
        {
            break;
        }
        p++;
    }

    if(end != (const uint8 **)0)
    {
        *end = p;
    }

    return sign * result;
}

//==================================================================
//                     命令解析与处理
//==================================================================

/**
 * @brief 解析一行命令并执行
 * @param  line 命令行数据（不含末尾的\n）
 * @param  len  命令行长度
 */
static void vofa_parse_line(uint8 *line, uint16 len)
{
    uint8  cmd;
    uint16 i;
    int    param_index;
    float  param_value;
    const uint8 *end_ptr;

    if(len == 0)
    {
        return;
    }

    cmd = line[0];

    switch(cmd)
    {
        // 设置参数：$<index>,<value>
        case VOFA_CMD_SET:
        {
            i = 1;

            // 跳过空格
            while(i < len && (line[i] == ' ' || line[i] == '\t'))
            {
                i++;
            }

            // 解析参数索引
            param_index = 0;
            while(i < len && line[i] >= '0' && line[i] <= '9')
            {
                param_index = param_index * 10 + (line[i] - '0');
                i++;
            }

            // 跳过空格和逗号
            while(i < len && (line[i] == ' ' || line[i] == ',' || line[i] == '\t'))
            {
                i++;
            }

            // 解析参数值
            param_value = vofa_atof(&line[i], &end_ptr);

            // 设置参数
            if(param_index >= 0 && param_index < VOFA_MAX_PARAMS && vofa_params[param_index].used)
            {
                // 限幅
                if(param_value < vofa_params[param_index].min)
                {
                    param_value = vofa_params[param_index].min;
                }
                if(param_value > vofa_params[param_index].max)
                {
                    param_value = vofa_params[param_index].max;
                }

                vofa_params[param_index].value = param_value;
                vofa_params[param_index].updated = 1;

                // 调试输出（可选，调试通过后可注释）
                printf("[VOFA] SET P%d = %.4f\r\n", param_index, param_value);
            }
            else
            {
                printf("[VOFA] ERR: param %d not registered\r\n", param_index);
            }
            break;
        }

        // 请求上报参数定义：#
        case VOFA_CMD_REPORT_DEF:
        {
            vofa_report_params();
            break;
        }

        // 请求上报参数值：?
        case VOFA_CMD_REPORT_VAL:
        {
            vofa_report_values();
            break;
        }

        default:
            // 未知命令，忽略
            break;
    }
}

/**
 * @brief 处理接收缓冲区，提取完整命令行
 */
static void vofa_process_rx(void)
{
    uint16 i;
    uint16 line_start;
    uint16 line_len;

    // 在缓冲区中查找 '\n'，提取每一行命令
    line_start = 0;
    for(i = 0; i < vofa_rx_len; i++)
    {
        if(vofa_rx_buf[i] == '\n')
        {
            line_len = i - line_start;

            // 去掉行尾的 '\r'
            if(line_len > 0 && vofa_rx_buf[line_start + line_len - 1] == '\r')
            {
                line_len--;
            }

            // 在行尾添加 '\0' 方便处理
            if(line_len > 0)
            {
                vofa_rx_buf[line_start + line_len] = '\0';
                vofa_parse_line(&vofa_rx_buf[line_start], line_len);
            }

            line_start = i + 1;
        }
    }

    // 将未处理的数据移到缓冲区开头
    if(line_start > 0)
    {
        if(line_start < vofa_rx_len)
        {
            // 还有未处理的数据（没有找到\n的不完整命令）
            for(i = 0; i < vofa_rx_len - line_start; i++)
            {
                vofa_rx_buf[i] = vofa_rx_buf[line_start + i];
            }
            vofa_rx_len = vofa_rx_len - line_start;
        }
        else
        {
            // 所有数据都已处理
            vofa_rx_len = 0;
        }
    }
}

//==================================================================
//                          公开 API
//==================================================================

void my_vofa_init(void)
{
    uint8 i;
    // 新增：端口字符串缓存，逐飞socket接口必须传char*字符串
    char target_port_str[8];
    char local_port_str[8];

    // 初始化参数表
    for(i = 0; i < VOFA_MAX_PARAMS; i++)
    {
        vofa_params[i].nam[0] = '\0';
        vofa_params[i].value = 0.0f;
        vofa_params[i].min = 0.0f;
        vofa_params[i].max = 0.0f;
        vofa_params[i].step = 0.0f;
        vofa_params[i].updated = 0;
        vofa_params[i].used = 0;
    }

    vofa_rx_len = 0;

    // 连接 WiFi
    printf("\r\n[VOFA] Connecting WiFi [%s]...\r\n", VOFA_WIFI_SSID);
    while(wifi_spi_init((uint8 *)VOFA_WIFI_SSID, (uint8 *)VOFA_WIFI_PASSWORD))
    {
        printf("[VOFA] WiFi connect failed, retry...\r\n");
        system_delay_ms(100);
    }

    printf("[VOFA] WiFi connected!\r\n");
    printf("[VOFA] module version: %s\r\n", wifi_spi_version);
    printf("[VOFA] module mac    : %s\r\n", wifi_spi_mac_addr);
    printf("[VOFA] module ip     : %s\r\n", wifi_spi_ip_addr_port);

    // 数字端口转为字符串
    sprintf(target_port_str, "%d", VOFA_TARGET_PORT);
    sprintf(local_port_str, "%d", VOFA_LOCAL_PORT);

    // 连接 VOFA+ TCP 服务器
    printf("[VOFA] Connecting VOFA+ server %s:%d...\r\n", VOFA_TARGET_IP, VOFA_TARGET_PORT);
    while(wifi_spi_socket_connect(
        "TCP", 
        VOFA_TARGET_IP, 
        target_port_str, 
        local_port_str))
    {
        printf("[VOFA] Connect server failed, retry...\r\n");
        system_delay_ms(100);
    }

    printf("[VOFA] VOFA+ connected! Ready to go.\r\n");
    
    // 强制上报参数定义，移除条件编译
    system_delay_ms(500);  // 增加延时确保连接稳定
    vofa_report_params();
}

void my_vofa_process(void)
{
    uint16 recv_len;
    uint16 free_len;

    // 接收数据
    free_len = VOFA_RX_BUF_SIZE - vofa_rx_len;
    if(free_len > 0)
    {
        recv_len = vofa_wifi_receive(&vofa_rx_buf[vofa_rx_len], free_len);

        if(recv_len > 0)
        {
            vofa_rx_len += recv_len;

            // 如果缓冲区满了还没找到\n，清空缓冲区防止死锁
            if(vofa_rx_len >= VOFA_RX_BUF_SIZE)
            {
                vofa_rx_len = 0;
            }
            else
            {
                // 解析命令
                vofa_process_rx();
            }
        }
    }
    else
    {
        // 缓冲区满，清空
        vofa_rx_len = 0;
    }
}

void vofa_send_wave(float *dat, uint8 channel_num)
{
#if VOFA_USE_JUSTFLOAT
    vofa_send_justfloat(dat, channel_num);
#else
    vofa_send_firewater(dat, channel_num);
#endif
}

void vofa_register_param(uint8 index, const char *nam, float value, float min, float max, float step)
{
    uint8 i;

    if(index >= VOFA_MAX_PARAMS)
    {
        return;
    }

    // 拷贝名称
    for(i = 0; i < 15 && nam[i] != '\0'; i++)
    {
        vofa_params[index].nam[i] = nam[i];
    }
    vofa_params[index].nam[i] = '\0';

    vofa_params[index].value = value;
    vofa_params[index].min = min;
    vofa_params[index].max = max;
    vofa_params[index].step = step;
    vofa_params[index].updated = 0;
    vofa_params[index].used = 1;

    printf("[VOFA] Registered P%d: %s = %.4f [%.4f, %.4f] step=%.4f\r\n",
           index, vofa_params[index].nam, value, min, max, step);
}

float vofa_get_param(uint8 index)
{
    if(index >= VOFA_MAX_PARAMS || !vofa_params[index].used)
    {
        return 0.0f;
    }
    return vofa_params[index].value;
}

void vofa_set_param(uint8 index, float value)
{
    if(index >= VOFA_MAX_PARAMS || !vofa_params[index].used)
    {
        return;
    }

    // 限幅
    if(value < vofa_params[index].min)
    {
        value = vofa_params[index].min;
    }
    if(value > vofa_params[index].max)
    {
        value = vofa_params[index].max;
    }

    vofa_params[index].value = value;
}

uint8 vofa_param_updated(uint8 index)
{
    uint8 flag;

    if(index >= VOFA_MAX_PARAMS || !vofa_params[index].used)
    {
        return 0;
    }

    flag = vofa_params[index].updated;
    vofa_params[index].updated = 0;
    return flag;
}

void vofa_report_params(void)
{
    uint8 i;
    uint16 offset;

    // 发送分隔标识，方便 VOFA+ 文本面板识别
    offset = (uint16)sprintf((char *)vofa_text_buf, "=== VOFA+ Param Definition ===\n");
    vofa_wifi_send(vofa_text_buf, offset);

    for(i = 0; i < VOFA_MAX_PARAMS; i++)
    {
        if(vofa_params[i].used)
        {
            // 格式：#<index>,<name>,<value>,<min>,<max>,<step>\n
            offset = (uint16)sprintf((char *)vofa_text_buf,
                "#%d,%s,%.4f,%.4f,%.4f,%.4f\n",
                i, vofa_params[i].nam, vofa_params[i].value,
                vofa_params[i].min, vofa_params[i].max, vofa_params[i].step);
            vofa_wifi_send(vofa_text_buf, offset);
        }
    }

    offset = (uint16)sprintf((char *)vofa_text_buf,
        "Usage: send $<index>,<value>\\n to set param\n");
    vofa_wifi_send(vofa_text_buf, offset);
}

void vofa_report_values(void)
{
    uint8 i;
    uint16 offset;

    offset = (uint16)sprintf((char *)vofa_text_buf, "=== VOFA+ Param Values ===\n");
    vofa_wifi_send(vofa_text_buf, offset);

    for(i = 0; i < VOFA_MAX_PARAMS; i++)
    {
        if(vofa_params[i].used)
        {
            offset = (uint16)sprintf((char *)vofa_text_buf,
                "P%d %s = %.4f\n", i, vofa_params[i].nam, vofa_params[i].value);
            vofa_wifi_send(vofa_text_buf, offset);
        }
    }
}
