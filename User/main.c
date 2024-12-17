/**
 ****************************************************************************************************
 * @file        main.c
 * @author      maxuhui
 * @version     V1.0
 * @date        2024-12-12
 * @brief       FATFS 实验
 * @license     
 ****************************************************************************************************
 * @attention
 *
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./USMART/usmart.h"
#include "./MALLOC/malloc.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SDIO/sdio_sdcard.h"
#include "./BSP/NORFLASH/norflash.h"
#include "./FATFS/exfuns/exfuns.h"

//FIL fil;
//FRESULT res;
//UINT bww, bww2, bww3;


extern UART_HandleTypeDef huart2;// 串口2句柄
extern UART_HandleTypeDef huart3;// 串口3句柄
// 文件名缓冲区
char file_name[20];
FIL fil;           // 文件对象
FRESULT res;       // 文件操作结果
UINT bww;           // 写入字节数
char buf[50];

// 文件计数
static int file_count = 1;

// 记录标志
int recording = 0;

// TDS数据缓冲区
char tds_data[100];

// 更新TDS数值显示的函数
void update_tds_display() {
    // 获取最新的TDS数值
    int tds1 = S2data.tds;
    int tds2 = S3data.tds;
    // 更新LCD显示
    lcd_show_string(30, 190, 200, 16, 16, "tds1:", BLUE);
    lcd_show_string(30, 210, 200, 16, 16, "tds2:", BLUE);
    lcd_show_num(30 + 8 * 5, 190, tds1, 5, 16, BLUE);  
    lcd_show_num(30 + 8 * 5, 210, tds2, 5, 16, BLUE);  
}

// 将file_count变量写入SD卡的函数
void write_file_count_to_sd() {
    res = f_open(&fil, "0:/file_count.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK) {
        sprintf(buf, "%d", file_count);
        f_write(&fil, buf, strlen(buf), &bww);
        f_close(&fil);
    }
}

// 假设这是处理KEY1按键按下的函数
void handle_KEY1_press() {
    // 执行文件关闭操作
    f_close(&fil);
	lcd_show_string(30, 230, 200, 12, 16, "Stop recording:", BLUE);
    printf("Stop recording\r\n");
    recording = 0;
    // 文件计数递增
    file_count++;
	// 将更新后的file_count写入SD卡
    write_file_count_to_sd();
    // 确保文件关闭操作完成后，更新显示
    update_tds_display();
}

// 从SD卡读取file_count变量的函数
void read_file_count_from_sd() {
    res = f_open(&fil, "0:/file_count.txt", FA_READ);
    if (res == FR_OK) {
        f_read(&fil, buf, sizeof(buf), &bww);
        buf[bww] = '\0';
        file_count = atoi(buf);
        f_close(&fil);
    } else {
        // 如果读取失败，设置为默认值1
        file_count = 0;
    }
}


int main(void)
{
    uint32_t total, free;
    uint8_t t = 0;
	uint8_t key;
    uint8_t res = 0;

    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(RCC_PLL_MUL9);     /* 设置时钟, 72Mhz */
    delay_init(72);                         /* 延时初始化 */
    usart_init(115200);                     /* 串口初始化为115200 */
	uart2_init();
	uart3_init();
    usmart_dev.init(72);                    /* 初始化USMART */
    led_init();                             /* 初始化LED */
    lcd_init();                             /* 初始化LCD */
    key_init();                             /* 初始化按键 */
    my_mem_init(SRAMIN);                    /* 初始化内部SRAM内存池 */

    lcd_show_string(30,  50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30,  70, 200, 16, 16, "HAOSHUI TDS60  TEST", RED);
    lcd_show_string(30,  90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "Use USMART for test", RED);

    while (sd_init()) /* 检测不到SD卡 */
    {
        lcd_show_string(30, 130, 200, 16, 16, "SD Card Error!", RED);
        delay_ms(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check! ", RED);
        delay_ms(500);
        LED0_TOGGLE(); /* LED0闪烁 */
    }

    exfuns_init();                 /* 为fatfs相关变量申请内存 */
    f_mount(fs[0], "0:", 1);       /* 挂载SD卡 */
    res = f_mount(fs[1], "1:", 1); /* 挂载FLASH. */

    if (res == 0X0D) /* FLASH磁盘,FAT文件系统错误,重新格式化FLASH */
    {
        lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Formatting...", RED); /* 格式化FLASH */
        res = f_mkfs("1:", 0, 0, FF_MAX_SS);                                    /* 格式化FLASH,1:,盘符;0,使用默认格式化参数 */

        if (res == 0)
        {
            f_setlabel((const TCHAR *)"1:ALIENTEK");                                /* 设置Flash磁盘的名字为：ALIENTEK */
            lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Format Finish", RED); /* 格式化完成 */
        }
        else
        {
            lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Format Error ", RED); /* 格式化失败 */
        }

        delay_ms(1000);
    }

    lcd_fill(30, 130, 240, 150 + 16, WHITE);    /* 清除显示 */

    while (exfuns_get_free((uint8_t*)"0", &total, &free)) /* 得到SD卡的总容量和剩余容量 */
    {
        lcd_show_string(30, 130, 200, 16, 16, "SD Card Fatfs Error!", RED);
        delay_ms(200);
        lcd_fill(30, 130, 240, 150 + 16, WHITE); /* 清除显示 */
        delay_ms(200);
        LED0_TOGGLE(); /* LED0闪烁 */
    }

    lcd_show_string(30, 130, 200, 16, 16, "FATFS OK!", BLUE);
    lcd_show_string(30, 150, 200, 16, 16, "SD Total Size:     MB", BLUE);
    lcd_show_string(30, 170, 200, 16, 16, "SD  Free Size:     MB", BLUE);
    lcd_show_num(30 + 8 * 14, 150, total >> 10, 5, 16, BLUE);               /* 显示SD卡总容量 MB */
    lcd_show_num(30 + 8 * 14, 170, free >> 10, 5, 16, BLUE);                /* 显示SD卡剩余容量 MB */

	// 先从SD卡读取file_count的值
    read_file_count_from_sd();
    while (1)
    {
		// 定义要发送的十六进制数据
		uint8_t cmd[] = {0x55, 0x07, 0x05, 0x01, 0x00, 0x00, 0x00, 0x62};
		UART2_SendData(cmd, sizeof(cmd));
	//	delay_ms(1000);
	//	HAL_UART_Transmit(&huart3, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
		// 读取TDS数据（假设使用串口或其他方式获取TDS数据）
		UART3_SendData(cmd, sizeof(cmd));
		delay_ms(50);
		S2data = Serial2_ParseData();
		S3data = Serial3_ParseData();
        int tds1 = S2data.tds;
        int tds2 = S3data.tds;
		lcd_show_string(30, 190, 200, 16, 16, "tds1:", BLUE);
		lcd_show_string(30, 210, 200, 16, 16, "tds2:", BLUE);
		lcd_show_num(30 + 8 * 5, 190, tds1, 5, 16, BLUE);  
		lcd_show_num(30 + 8 * 5, 210, tds2, 5, 16, BLUE);  
		printf("tds1:%d;   tds2:%d\r\n",tds1,tds2);
		key = key_scan(0);

        if (key == KEY0_PRES && recording == 0)   /* KEY0按下了 */
        {
			// 创建新文件名
            sprintf(file_name, "0:/test%d_tds.txt", file_count);
			// 打开文件以创建并写入
            res = f_open(&fil, file_name, FA_CREATE_ALWAYS | FA_WRITE);
            if (res == FR_OK) {
                printf("Start recording to %s\r\n", file_name);
				lcd_show_string(30, 230, 200, 12, 16, "Start recording:   file", BLUE);
				lcd_show_num(30 + 8 * 15, 230, file_count, 3, 16, BLUE);  
                recording = 1;
            } else {
                printf("Failed to create file %s, error: %d\r\n", file_name, res);
				lcd_show_string(30, 230, 200, 12, 16, "Failed to create:", BLUE);
				lcd_show_num(30 + 8 * 15, 230, file_count, 3, 16, BLUE); 
            }
              /* sd卡中创建一个新文件，并写入数值。文件名是test1_tds,test2_tds,test3_tds，逐一递增；注意当写入完成后，才有权限创建新的文件 */
        }
		if (key == KEY1_PRES && recording == 1)   /* KEY1按下了 */
        {
//              // 关闭当前文件
//            f_close(&fil);
//            printf("Stop recording\r\n");
//			lcd_show_string(30, 230, 200, 12, 16, "Stop recording:", BLUE);
//            recording = 0;

//            // 文件计数递增
//            file_count++;
			handle_KEY1_press();
        }
		// 如果正在记录，每1秒读取一次TDS数据并写入文件
        if (recording)
        {
            // 格式化TDS数据字符串
            sprintf(tds_data, "test_tds1:%d tds2:%d\r\n", tds1, tds2);

            // 写入数据到文件
            res = f_write(&fil, tds_data, strlen(tds_data), &bww);
            if (res != FR_OK) {
                printf("Write error: %d\r\n", res);
                // 停止记录
                f_close(&fil);
                recording = 0;
                file_count++;
            }
        }
        t++;
        delay_ms(500);
        LED0_TOGGLE(); /* LED0闪烁 */
    }
}









