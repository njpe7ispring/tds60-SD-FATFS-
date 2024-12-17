/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      maxuhui
 * @version     V1.1
 * @date        2023-06-05
 * @brief       串口初始化代码(一般是串口1)，支持printf
 * @license     
 ****************************************************************************************************
 * @attention

 *
 * 修改说明
 * V1.0 20241217
 * 第一次发布
 * V1.1 20241217
 * 修改HAL_UART_RxCpltCallback()  修复中断bug
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"


/* 如果使用os,则包括下面的头文件即可. */
#if SYS_SUPPORT_OS
#include "os.h" /* os 使用 */
#endif

/******************************************************************************************/
/* 加入以下代码, 支持printf函数, 而不需要选择use MicroLIB */

#if 1

#if (__ARMCC_VERSION >= 6010050)            /* 使用AC6编译器时 */
__asm(".global __use_no_semihosting\n\t");  /* 声明不使用半主机模式 */
__asm(".global __ARM_use_no_argv \n\t");    /* AC6下需要声明main函数为无参数格式，否则部分例程可能出现半主机模式 */

#else
/* 使用AC5编译器时, 要在这里定义__FILE 和 不使用半主机模式 */
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
    /* Whatever you require here. If the only file you are using is */
    /* standard output using printf() for debugging, no file handling */
    /* is required. */
};

#endif

/* 不使用半主机模式，至少需要重定义_ttywrch\_sys_exit\_sys_command_string函数,以同时兼容AC6和AC5模式 */
int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

/* 定义_sys_exit()以避免使用半主机模式 */
void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}


/* FILE 在 stdio.h里面定义. */
FILE __stdout;

/* MDK下需要重定义fputc函数, printf函数最终会通过调用fputc输出字符串到串口 */
int fputc(int ch, FILE *f)
{
    while ((USART_UX->SR & 0X40) == 0);     /* 等待上一个字符发送完成 */

    USART_UX->DR = (uint8_t)ch;             /* 将要发送的字符 ch 写入到DR寄存器 */
    return ch;
}
#endif
/******************************************************************************************/

#if USART_EN_RX /*如果使能了接收*/

/* 接收缓冲, 最大USART_REC_LEN个字节. */
uint8_t g_usart_rx_buf[USART_REC_LEN];

/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart_rx_sta = 0;      
uint8_t g_rx_buffer[RXBUFFERSIZE];  /* HAL库使用的串口接收缓冲 */
uint8_t rx2_data = 0;
uint8_t rx3_data = 0;

//// 定义串口2接收数据结构体
//typedef struct {
//    uint16_t RxPacket[11];
//    uint8_t RxFlag;
//    SensorData data;
//} Serial2Data;

//// 定义串口3接收数据结构体
//typedef struct {
//    uint16_t RxPacket[11];
//    uint8_t RxFlag;
//    SensorData data;
//} Serial3Data;

UART_HandleTypeDef g_uart1_handle;  /* UART句柄  选用串口1*/
// 串口2句柄
UART_HandleTypeDef huart2;
// 串口3句柄
UART_HandleTypeDef huart3;

// 全局变量，用于存储串口2接收的数据
uint16_t Serial2_RxPacket[11];
uint8_t Serial2_RxFlag;
SensorData S2data = {0, 0, 0.0};
// 全局变量，用于存储串口3接收的数据
uint16_t Serial3_RxPacket[11];
uint8_t Serial3_RxFlag;
SensorData S3data = {0, 0, 0.0};

/**
 * @brief       串口X初始化函数 选用串口1
 * @param       baudrate: 波特率, 根据自己需要设置波特率值
 * @note        注意: 必须设置正确的时钟源, 否则串口波特率就会设置异常.
 *              这里的USART的时钟源在sys_stm32_clock_init()函数中已经设置过了.
 * @retval      无
 */
void usart_init(uint32_t baudrate)
{
    /*UART 初始化设置*/
    g_uart1_handle.Instance = USART_UX;                                       /* USART_UX */
    g_uart1_handle.Init.BaudRate = baudrate;                                  /* 波特率 */
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;                      /* 字长为8位数据格式 */
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;                           /* 一个停止位 */
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;                            /* 无奇偶校验位 */
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;                      /* 无硬件流控 */
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;                               /* 收发模式 */
    HAL_UART_Init(&g_uart1_handle);                                           /* HAL_UART_Init()会使能UART1 */
	
	if (HAL_UART_Init(&g_uart1_handle)!= HAL_OK) {
        // 处理串口1初始化失败的情况，例如打印错误信息
        printf("串口1初始化失败\n");
    }

    /* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
    HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE); 
	
}


/**
 * @brief       串口2初始化函数
 * @param       无
 * @note        
 * @retval      无
 */
void uart2_init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart2);
	
	 if (HAL_UART_Init(&huart2)!= HAL_OK) {
        // 处理串口2初始化失败的情况，例如打印错误信息
        printf("串口2初始化失败\n");
    }
   /* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
    HAL_UART_Receive_IT(&huart2, (uint8_t *)g_rx_buffer, RXBUFFERSIZE); 
	
}


// 初始化串口3
void uart3_init(void)
{
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 9600;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart3);
	
	 if (HAL_UART_Init(&huart3)!= HAL_OK) {
        // 处理串口3初始化失败的情况，例如打印错误信息
        printf("串口3初始化失败\n");
    }
	
   /* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
    HAL_UART_Receive_IT(&huart3, (uint8_t *)g_rx_buffer, RXBUFFERSIZE); 
}


/**
 * @brief       UART底层初始化函数
 * @param       huart: UART句柄类型指针
 * @note        此函数会被HAL_UART_Init()调用
 *              完成时钟使能，引脚配置，中断配置
 * @retval      无
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (huart->Instance == USART_UX)                            /* 如果是串口1，进行串口1 MSP初始化 */
    {
        USART_TX_GPIO_CLK_ENABLE();                             /* 使能串口TX脚时钟 */
        USART_RX_GPIO_CLK_ENABLE();                             /* 使能串口RX脚时钟 */
        USART_UX_CLK_ENABLE();                                  /* 使能串口时钟 */

        gpio_init_struct.Pin = USART_TX_GPIO_PIN;               /* 串口发送引脚号 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 复用推挽输出 */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* IO速度设置为高速 */
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);
                
        gpio_init_struct.Pin = USART_RX_GPIO_PIN;               /* 串口RX脚 模式设置 */
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;    
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);   /* 串口RX脚 必须设置成输入模式 */
        
#if USART_EN_RX
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);                      /* 使能USART1中断通道 */
        HAL_NVIC_SetPriority(USART_UX_IRQn, 3, 3);              /* 组2，最低优先级:抢占优先级3，子优先级3 */
#endif
    }
	if (huart->Instance == USART2)                            /* 如果是串口2，进行串口2 MSP初始化 */
    {
        USART2_TX_GPIO_CLK_ENABLE();                             /* 使能串口2 TX脚时钟 */
        USART2_RX_GPIO_CLK_ENABLE();                             /* 使能串口2 RX脚时钟 */
        USART2_UX_CLK_ENABLE();                                  /* 使能串口2时钟 */

        gpio_init_struct.Pin = USART2_TX_GPIO_PIN;               /* 串口2发送引脚号 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 复用推挽输出 */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* IO速度设置为高速 */
        HAL_GPIO_Init(USART2_TX_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = USART2_RX_GPIO_PIN;               /* 串口2 RX脚 模式设置 */
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;
        HAL_GPIO_Init(USART2_RX_GPIO_PORT, &gpio_init_struct);   /* 串口2 RX脚 必须设置成输入模式 */

#if USART2_EN_RX
        HAL_NVIC_EnableIRQ(USART2_IRQn);                      /* 使能USART2中断通道 */
        HAL_NVIC_SetPriority(USART2_IRQn, 3, 3);              /* 组2，最低优先级:抢占优先级3，子优先级3 */
#endif
    }
	if (huart->Instance == USART3)                            /* 如果是串口3，进行串口3 MSP初始化 */
    {
        USART3_TX_GPIO_CLK_ENABLE();                             /* 使能串口3 TX脚时钟 */
        USART3_RX_GPIO_CLK_ENABLE();                             /* 使能串口3 RX脚时钟 */
        USART3_UX_CLK_ENABLE();                                  /* 使能串口3时钟 */

        gpio_init_struct.Pin = USART3_TX_GPIO_PIN;               /* 串口3发送引脚号 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 复用推挽输出 */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* IO速度设置为高速 */
        HAL_GPIO_Init(USART3_TX_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = USART3_RX_GPIO_PIN;               /* 串口3 RX脚 模式设置 */
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;
        HAL_GPIO_Init(USART3_RX_GPIO_PORT, &gpio_init_struct);   /* 串口3 RX脚 必须设置成输入模式 */

#if USART3_EN_RX
        HAL_NVIC_EnableIRQ(USART3_IRQn);                      /* 使能USART3中断通道 */
        HAL_NVIC_SetPriority(USART3_IRQn, 3, 3);              /* 组2，最低优先级:抢占优先级3，子优先级3 */
#endif
    }
}


//static uint8_t Serial2_ErrorFlag = 0;
//void handleUartReceive(UART_HandleTypeDef *huart, uint8_t *rx_data, uint8_t *RxPacket, uint8_t pRxPacket, uint8_t RxFlag, uint8_t ErrorFlag)
//{
//    static enum {WAIT_START, RECEIVING, RECEIVED} state = WAIT_START;
//    uint8_t localErrorFlag = ErrorFlag;
//    uint8_t localRxFlag = RxFlag;
//    uint8_t localpRxPacket = pRxPacket;
//    switch (state)
//    {
//    case WAIT_START:
//        if (localErrorFlag == 1)
//        {
//            localErrorFlag = 0;
//        }
//        if (*rx_data == 0x55)
//        {
//            state = RECEIVING;
//            RxPacket= *rx_data;
//            localpRxPacket = 1;
//        }
//        break;
//    case RECEIVING:
//        RxPacket[localpRxPacket++]= *rx_data;
//        if (localpRxPacket == 11)
//        {
//            state = RECEIVED;
//        }
//        break;
//    case RECEIVED:
//        // 进行数据校验（如果有校验和等）
//        // 按照协议解析数据
//        if (RxPacket == 0x55 && RxPacket[1] == 0x0A && RxPacket[2] == 0x85 && RxPacket[3] == 0x01)
//        {
//            state = WAIT_START;
//            localpRxPacket = 0;
//            localRxFlag = 1;
//            localErrorFlag = 0;
//        }
//        else
//        {
//            localErrorFlag = 1;
//            state = WAIT_START;
//            localpRxPacket = 0;
//        }
//        break;
//    }
//    HAL_UART_Receive_IT(huart, rx_data, 1);
//}
/**
 * @brief       串口数据接收回调函数
                数据处理在这里进行
 * @param       huart:串口句柄
 * @retval      无
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART_UX)                    /* 如果是串口1 */
    {
        if ((g_usart_rx_sta & 0x8000) == 0)             /* 接收未完成 */
        {
            if (g_usart_rx_sta & 0x4000)                /* 接收到了0x0d（即回车键） */
            {
                if (g_rx_buffer[0] != 0x0a)             /* 接收到的不是0x0a（即不是换行键） */
                {
                    g_usart_rx_sta = 0;                 /* 接收错误,重新开始 */
                }
                else                                    /* 接收到的是0x0a（即换行键） */
                {
                    g_usart_rx_sta |= 0x8000;           /* 接收完成了 */
                }
            }
            else                                        /* 还没收到0X0d（即回车键） */
            {
                if (g_rx_buffer[0] == 0x0d)
                    g_usart_rx_sta |= 0x4000;
                else
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = g_rx_buffer[0];
                    g_usart_rx_sta++;

                    if (g_usart_rx_sta > (USART_REC_LEN - 1))
                    {
                        g_usart_rx_sta = 0;             /* 接收数据错误,重新开始接收 */
                    }
                }
            }
        }

        HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
    }
	static uint8_t Serial2_pRxPacket = 0;
    static uint8_t Serial3_pRxPacket = 0;

if (huart->Instance == USART2)
{
	static uint8_t Serial2_ErrorFlag = 0;
    static enum {WAIT_START, RECEIVING, RECEIVED} state = WAIT_START;
    static uint8_t Serial2_pRxPacket = 0;
    switch (state)
    {
    case WAIT_START:
		if (Serial2_ErrorFlag == 1)
		{
			Serial2_ErrorFlag = 0;
		}
        if (rx2_data == 0x55)
        {
            state = RECEIVING;
            Serial2_RxPacket[0]= rx2_data;
            Serial2_pRxPacket = 1;
        }
        break;
    case RECEIVING:
        Serial2_RxPacket[Serial2_pRxPacket++]= rx2_data;
        if (Serial2_pRxPacket == 11)
        {
            state = RECEIVED;
        }
        break;
    case RECEIVED:
        // 进行数据校验（如果有校验和等）
        // 按照协议解析数据
        if (Serial2_RxPacket[0] == 0x55 && Serial2_RxPacket[1] == 0x0A && Serial2_RxPacket[2] == 0x85 && Serial2_RxPacket[3] == 0x01)
        {
			state = WAIT_START;
			Serial2_pRxPacket = 0;
			Serial2_RxFlag = 1;
        }   
		 else
		{
			Serial2_ErrorFlag = 1;
			state = WAIT_START;
			Serial2_pRxPacket = 0;
		}
        break;
    }
        HAL_UART_Receive_IT(&huart2, &rx2_data, 1);

//	 handleUartReceive(&huart2, &rx2_data, Serial2_RxPacket, Serial2_pRxPacket, Serial2_RxFlag, Serial2_ErrorFlag);
}

	if (huart->Instance == USART3)
	{
		static uint8_t Serial3_ErrorFlag = 0;
		static enum {WAIT_START, RECEIVING, RECEIVED} state = WAIT_START;
		static uint8_t Serial3_pRxPacket = 0;
		switch (state)
		{
		case WAIT_START:
			if (Serial3_ErrorFlag == 1)
			{
				Serial3_ErrorFlag = 0;
			}
			if (rx3_data == 0x55)
			{
				state = RECEIVING;
				Serial3_RxPacket[0]= rx3_data;
				Serial3_pRxPacket = 1;
			}
			
			break;
		case RECEIVING:
			Serial3_RxPacket[Serial3_pRxPacket++]= rx3_data;
			if (Serial3_pRxPacket == 11)
			{
				state = RECEIVED;
			}
			break;
		case RECEIVED:
			// 进行数据校验（如果有校验和等）
			// 按照协议解析数据
			if (Serial3_RxPacket[0] == 0x55 && Serial3_RxPacket[1] == 0x0A && Serial3_RxPacket[2] == 0x85 && Serial3_RxPacket[3] == 0x01)
			{
				state = WAIT_START;
				Serial3_pRxPacket = 0;
				Serial3_RxFlag = 1;
			}	
			 else
			{
				Serial3_ErrorFlag = 1;
				state = WAIT_START;
				Serial3_pRxPacket = 0;
			}
			break;
		}
			HAL_UART_Receive_IT(&huart3, &rx3_data, 1);

//	 handleUartReceive(&huart3, &rx3_data, Serial3_RxPacket, Serial3_pRxPacket, Serial3_RxFlag, Serial3_ErrorFlag);

	}

}

/**
 * @brief       串口1中断服务函数  此函数会自动调用，在设置中断条件下
 * @param       无
 * @retval      无
 */
void USART_UX_IRQHandler(void)
{
#if SYS_SUPPORT_OS                          /* 使用OS */
    OSIntEnter();    
#endif

    HAL_UART_IRQHandler(&g_uart1_handle);   /* 调用HAL库中断处理公用函数 */

#if SYS_SUPPORT_OS                          /* 使用OS */
    OSIntExit();
#endif

}

// 串口2中断服务例程
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);   /* 调用HAL库中断处理公用函数 */
}

// 串口3中断服务例程
void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3);   /* 调用HAL库中断处理公用函数 */
}


// 数据处理函数，串口2，按照协议解析数据
SensorData Serial2_ParseData(void)
{
//	printf("进入Serial2_ParseData \r\n");
	if(Serial2_RxFlag == 1)
	{
//		printf(" Serial2_RxFlag == 1 进入Serial2_ParseData \r\n");
//		printf(" Serial2_RxPacket[]:%d Serial2_RxPacket[1]:%d Serial2_RxPacket[2]:%d Serial2_RxPacket[3]:%d Serial2_RxPacket[4]:%d Serial2_RxPacket[5]:%d\r\n",
//       Serial2_RxPacket[0], Serial2_RxPacket[1], Serial2_RxPacket[2], Serial2_RxPacket[3], Serial2_RxPacket[4], Serial2_RxPacket[5]);

        if (Serial2_RxPacket[0] == 0x55 && Serial2_RxPacket[1] == 0x0a && Serial2_RxPacket[2] == 0x85 && Serial2_RxPacket[3] == 0x01)
        {
            uint16_t tempAdc = (Serial2_RxPacket[6] << 8) | Serial2_RxPacket[7];
            uint16_t condAdc = (Serial2_RxPacket[4] << 8) | Serial2_RxPacket[5];
            uint16_t checksum = Serial2_RxPacket[10];
            uint16_t sum = 0;

            for (uint8_t i = 0; i < 10; i++)
            {
                sum += Serial2_RxPacket[i];
            }
            sum = sum & 0xFF;

            if (sum == checksum)
            {
                S2data.temp = tempAdc / 10.0;
                S2data.cond = condAdc / 10;
                S2data.tds = S2data.cond / 2;
            }
        }
	}
			return S2data;
}

// 数据处理函数，串口3，按照协议解析数据
SensorData Serial3_ParseData(void)
{
	if(Serial3_RxFlag == 1)
	{
        if (Serial3_RxPacket[0] == 0x55 && Serial3_RxPacket[1] == 0x0a && Serial3_RxPacket[2] == 0x85 && Serial3_RxPacket[3] == 0x01)
        {
            uint16_t tempAdc = (Serial3_RxPacket[6] << 8) | Serial3_RxPacket[7];
            uint16_t condAdc = (Serial3_RxPacket[4] << 8) | Serial3_RxPacket[5];
            uint16_t checksum = Serial3_RxPacket[10];
            uint16_t sum = 0;

            for (uint8_t i = 0; i < 10; i++)
            {
                sum += Serial3_RxPacket[i];
            }
            sum = sum & 0xFF;

            if (sum == checksum)
            {
               S3data.temp = tempAdc / 10.0;
               S3data.cond = condAdc / 10;
               S3data.tds =S3data.cond / 2;
            }
        }
	}
    return S3data;
}

void UART2_SendData(uint8_t *data, uint16_t size)
{
    HAL_UART_Transmit(&huart2, data, size, HAL_MAX_DELAY);
}

void UART3_SendData(uint8_t *data, uint16_t size)
{
    HAL_UART_Transmit(&huart3, data, size, HAL_MAX_DELAY);
}
#endif

