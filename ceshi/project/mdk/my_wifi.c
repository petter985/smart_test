#include "my_wifi.h"
#include "zf_common_headfile.h" 
#include "seekfree_assistant.h"
extern uint8 time_test;
#define WIFI_SSID_TEST          "readme"
#define WIFI_PASSWORD_TEST      "34567821"

//#define WIFI_SSID_TEST          "Runner"
//#define WIFI_PASSWORD_TEST      "060920tr"
int temp8=0;
//-------------------------------------------------------------------------------------------------------------------
// ???????     ????????????????? + ???
//-------------------------------------------------------------------------------------------------------------------
static void my_wifi_parameter_handler(void)
{
    uint8 i;
    static uint32 call_count = 0;
    
    call_count++;
    
    // ? 50 ????????
    if(call_count % 50 == 0)
    {
        printf("[WiFi] handler running... %ld\r\n", call_count);
    }
    
    seekfree_assistant_data_analysis();
    
    for(i = 0; i < SEEKFREE_ASSISTANT_SET_PARAMETR_COUNT; i++)
    {
        if(seekfree_assistant_parameter_update_flag[i])
        {
            printf(">>> GOT DATA! CH%d = %f <<<\r\n", i, seekfree_assistant_parameter[i]);
            seekfree_assistant_parameter_update_flag[i] = 0;
            
            switch(i)
            {
                case 0:  temp8 = seekfree_assistant_parameter[0];  break;
                case 1:  servo_pid.Kd = seekfree_assistant_parameter[1];  break;
                case 2:  motor_pid_l.Kp = seekfree_assistant_parameter[2];  break;
                case 3:  motor_pid_r.Ki = seekfree_assistant_parameter[3];  break;
                default: break;
            }
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
// ???????     WiFi ?????????????????????
//-------------------------------------------------------------------------------------------------------------------
void my_wifi_init()
{
    while(wifi_spi_init(WIFI_SSID_TEST, WIFI_PASSWORD_TEST))
    {
        printf("\r\n connect wifi failed. \r\n");
        system_delay_ms(100);
    }

    printf("\r\n module version:%s", wifi_spi_version);
    printf("\r\n module mac    :%s", wifi_spi_mac_addr);
    printf("\r\n module ip     :%s", wifi_spi_ip_addr_port);

    if(1 != WIFI_SPI_AUTO_CONNECT)
    {
        while(wifi_spi_socket_connect(
            "TCP",
            WIFI_SPI_TARGET_IP,
            WIFI_SPI_TARGET_PORT,
            WIFI_SPI_LOCAL_PORT))
        {
            printf("\r\n Connect TCP Servers error, try again.");
            system_delay_ms(100);
        }
    }
    
    seekfree_assistant_interface_init(SEEKFREE_ASSISTANT_WIFI_SPI);//mt9v03x_ima ge,image_01
    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, mt9v03x_image, MT9V03X_W, MT9V03X_H);
		seekfree_assistant_camera_boundary_config(X_BOUNDARY, MT9V03X_H, l_line_x, r_line_x, m_line_x, NULL, NULL ,NULL);
}
void boxing()
{
        seekfree_assistant_oscilloscope_data.dat[0] =motor_l.encoder_speed;
        seekfree_assistant_oscilloscope_data.dat[1] =motor_r.encoder_speed;
        seekfree_assistant_oscilloscope_data.dat[2] =left_target;
				seekfree_assistant_oscilloscope_data.dat[3] =right_target;
				seekfree_assistant_oscilloscope_data.dat[4] =time_test;
        // ??????????????????????????
        seekfree_assistant_oscilloscope_data.channel_num = 5;

        // ???????????4??????????????????8???
        seekfree_assistant_oscilloscope_send(&seekfree_assistant_oscilloscope_data);

        // ????????????????????????????G??????????????????????WIFI???????????????

        // ???????????????????????????????????????seekfree_assistant_oscilloscope_data???????????????????????????????
        // ??????????????????????????????????????????????????????????????????????????????????????????20ms
        seekfree_assistant_data_analysis();
}
//-------------------------------------------------------------------------------------------------------------------
// ???????     WiFi ?????????
//-------------------------------------------------------------------------------------------------------------------
void my_wifi_process()
{
    // ????????
    //my_wifi_parameter_handler();
    // ???????
    seekfree_assistant_camera_send();
        //??????
    //boxing();
}