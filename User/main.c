/**
 ****************************************************************************************************
 * @file        main.c
 * @author      maxuhui
 * @version     V1.0
 * @date        2024-12-12
 * @brief       FATFS ʵ��
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


extern UART_HandleTypeDef huart2;// ����2���
extern UART_HandleTypeDef huart3;// ����3���
// �ļ���������
char file_name[20];
FIL fil;           // �ļ�����
FRESULT res;       // �ļ��������
UINT bww;           // д���ֽ���
char buf[50];

// �ļ�����
static int file_count = 1;

// ��¼��־
int recording = 0;

// TDS���ݻ�����
char tds_data[100];

// ����TDS��ֵ��ʾ�ĺ���
void update_tds_display() {
    // ��ȡ���µ�TDS��ֵ
    int tds1 = S2data.tds;
    int tds2 = S3data.tds;
    // ����LCD��ʾ
    lcd_show_string(30, 190, 200, 16, 16, "tds1:", BLUE);
    lcd_show_string(30, 210, 200, 16, 16, "tds2:", BLUE);
    lcd_show_num(30 + 8 * 5, 190, tds1, 5, 16, BLUE);  
    lcd_show_num(30 + 8 * 5, 210, tds2, 5, 16, BLUE);  
}

// ��file_count����д��SD���ĺ���
void write_file_count_to_sd() {
    res = f_open(&fil, "0:/file_count.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK) {
        sprintf(buf, "%d", file_count);
        f_write(&fil, buf, strlen(buf), &bww);
        f_close(&fil);
    }
}

// �������Ǵ���KEY1�������µĺ���
void handle_KEY1_press() {
    // ִ���ļ��رղ���
    f_close(&fil);
	lcd_show_string(30, 230, 200, 12, 16, "Stop recording:", BLUE);
    printf("Stop recording\r\n");
    recording = 0;
    // �ļ���������
    file_count++;
	// �����º��file_countд��SD��
    write_file_count_to_sd();
    // ȷ���ļ��رղ�����ɺ󣬸�����ʾ
    update_tds_display();
}

// ��SD����ȡfile_count�����ĺ���
void read_file_count_from_sd() {
    res = f_open(&fil, "0:/file_count.txt", FA_READ);
    if (res == FR_OK) {
        f_read(&fil, buf, sizeof(buf), &bww);
        buf[bww] = '\0';
        file_count = atoi(buf);
        f_close(&fil);
    } else {
        // �����ȡʧ�ܣ�����ΪĬ��ֵ1
        file_count = 0;
    }
}


int main(void)
{
    uint32_t total, free;
    uint8_t t = 0;
	uint8_t key;
    uint8_t res = 0;

    HAL_Init();                             /* ��ʼ��HAL�� */
    sys_stm32_clock_init(RCC_PLL_MUL9);     /* ����ʱ��, 72Mhz */
    delay_init(72);                         /* ��ʱ��ʼ�� */
    usart_init(115200);                     /* ���ڳ�ʼ��Ϊ115200 */
	uart2_init();
	uart3_init();
    usmart_dev.init(72);                    /* ��ʼ��USMART */
    led_init();                             /* ��ʼ��LED */
    lcd_init();                             /* ��ʼ��LCD */
    key_init();                             /* ��ʼ������ */
    my_mem_init(SRAMIN);                    /* ��ʼ���ڲ�SRAM�ڴ�� */

    lcd_show_string(30,  50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30,  70, 200, 16, 16, "HAOSHUI TDS60  TEST", RED);
    lcd_show_string(30,  90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "Use USMART for test", RED);

    while (sd_init()) /* ��ⲻ��SD�� */
    {
        lcd_show_string(30, 130, 200, 16, 16, "SD Card Error!", RED);
        delay_ms(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check! ", RED);
        delay_ms(500);
        LED0_TOGGLE(); /* LED0��˸ */
    }

    exfuns_init();                 /* Ϊfatfs��ر��������ڴ� */
    f_mount(fs[0], "0:", 1);       /* ����SD�� */
    res = f_mount(fs[1], "1:", 1); /* ����FLASH. */

    if (res == 0X0D) /* FLASH����,FAT�ļ�ϵͳ����,���¸�ʽ��FLASH */
    {
        lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Formatting...", RED); /* ��ʽ��FLASH */
        res = f_mkfs("1:", 0, 0, FF_MAX_SS);                                    /* ��ʽ��FLASH,1:,�̷�;0,ʹ��Ĭ�ϸ�ʽ������ */

        if (res == 0)
        {
            f_setlabel((const TCHAR *)"1:ALIENTEK");                                /* ����Flash���̵�����Ϊ��ALIENTEK */
            lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Format Finish", RED); /* ��ʽ����� */
        }
        else
        {
            lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Format Error ", RED); /* ��ʽ��ʧ�� */
        }

        delay_ms(1000);
    }

    lcd_fill(30, 130, 240, 150 + 16, WHITE);    /* �����ʾ */

    while (exfuns_get_free((uint8_t*)"0", &total, &free)) /* �õ�SD������������ʣ������ */
    {
        lcd_show_string(30, 130, 200, 16, 16, "SD Card Fatfs Error!", RED);
        delay_ms(200);
        lcd_fill(30, 130, 240, 150 + 16, WHITE); /* �����ʾ */
        delay_ms(200);
        LED0_TOGGLE(); /* LED0��˸ */
    }

    lcd_show_string(30, 130, 200, 16, 16, "FATFS OK!", BLUE);
    lcd_show_string(30, 150, 200, 16, 16, "SD Total Size:     MB", BLUE);
    lcd_show_string(30, 170, 200, 16, 16, "SD  Free Size:     MB", BLUE);
    lcd_show_num(30 + 8 * 14, 150, total >> 10, 5, 16, BLUE);               /* ��ʾSD�������� MB */
    lcd_show_num(30 + 8 * 14, 170, free >> 10, 5, 16, BLUE);                /* ��ʾSD��ʣ������ MB */

	// �ȴ�SD����ȡfile_count��ֵ
    read_file_count_from_sd();
    while (1)
    {
		// ����Ҫ���͵�ʮ����������
		uint8_t cmd[] = {0x55, 0x07, 0x05, 0x01, 0x00, 0x00, 0x00, 0x62};
		UART2_SendData(cmd, sizeof(cmd));
	//	delay_ms(1000);
	//	HAL_UART_Transmit(&huart3, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
		// ��ȡTDS���ݣ�����ʹ�ô��ڻ�������ʽ��ȡTDS���ݣ�
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

        if (key == KEY0_PRES && recording == 0)   /* KEY0������ */
        {
			// �������ļ���
            sprintf(file_name, "0:/test%d_tds.txt", file_count);
			// ���ļ��Դ�����д��
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
              /* sd���д���һ�����ļ�����д����ֵ���ļ�����test1_tds,test2_tds,test3_tds����һ������ע�⵱д����ɺ󣬲���Ȩ�޴����µ��ļ� */
        }
		if (key == KEY1_PRES && recording == 1)   /* KEY1������ */
        {
//              // �رյ�ǰ�ļ�
//            f_close(&fil);
//            printf("Stop recording\r\n");
//			lcd_show_string(30, 230, 200, 12, 16, "Stop recording:", BLUE);
//            recording = 0;

//            // �ļ���������
//            file_count++;
			handle_KEY1_press();
        }
		// ������ڼ�¼��ÿ1���ȡһ��TDS���ݲ�д���ļ�
        if (recording)
        {
            // ��ʽ��TDS�����ַ���
            sprintf(tds_data, "test_tds1:%d tds2:%d\r\n", tds1, tds2);

            // д�����ݵ��ļ�
            res = f_write(&fil, tds_data, strlen(tds_data), &bww);
            if (res != FR_OK) {
                printf("Write error: %d\r\n", res);
                // ֹͣ��¼
                f_close(&fil);
                recording = 0;
                file_count++;
            }
        }
        t++;
        delay_ms(500);
        LED0_TOGGLE(); /* LED0��˸ */
    }
}









