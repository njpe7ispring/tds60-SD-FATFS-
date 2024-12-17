/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      maxuhui
 * @version     V1.1
 * @date        2023-06-05
 * @brief       ���ڳ�ʼ������(һ���Ǵ���1)��֧��printf
 * @license     
 ****************************************************************************************************
 * @attention

 *
 * �޸�˵��
 * V1.0 20241217
 * ��һ�η���
 * V1.1 20241217
 * �޸�HAL_UART_RxCpltCallback()  �޸��ж�bug
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"


/* ���ʹ��os,����������ͷ�ļ�����. */
#if SYS_SUPPORT_OS
#include "os.h" /* os ʹ�� */
#endif

/******************************************************************************************/
/* �������´���, ֧��printf����, ������Ҫѡ��use MicroLIB */

#if 1

#if (__ARMCC_VERSION >= 6010050)            /* ʹ��AC6������ʱ */
__asm(".global __use_no_semihosting\n\t");  /* ������ʹ�ð�����ģʽ */
__asm(".global __ARM_use_no_argv \n\t");    /* AC6����Ҫ����main����Ϊ�޲�����ʽ�����򲿷����̿��ܳ��ְ�����ģʽ */

#else
/* ʹ��AC5������ʱ, Ҫ�����ﶨ��__FILE �� ��ʹ�ð�����ģʽ */
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
    /* Whatever you require here. If the only file you are using is */
    /* standard output using printf() for debugging, no file handling */
    /* is required. */
};

#endif

/* ��ʹ�ð�����ģʽ��������Ҫ�ض���_ttywrch\_sys_exit\_sys_command_string����,��ͬʱ����AC6��AC5ģʽ */
int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

/* ����_sys_exit()�Ա���ʹ�ð�����ģʽ */
void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}


/* FILE �� stdio.h���涨��. */
FILE __stdout;

/* MDK����Ҫ�ض���fputc����, printf�������ջ�ͨ������fputc����ַ��������� */
int fputc(int ch, FILE *f)
{
    while ((USART_UX->SR & 0X40) == 0);     /* �ȴ���һ���ַ�������� */

    USART_UX->DR = (uint8_t)ch;             /* ��Ҫ���͵��ַ� ch д�뵽DR�Ĵ��� */
    return ch;
}
#endif
/******************************************************************************************/

#if USART_EN_RX /*���ʹ���˽���*/

/* ���ջ���, ���USART_REC_LEN���ֽ�. */
uint8_t g_usart_rx_buf[USART_REC_LEN];

/*  ����״̬
 *  bit15��      ������ɱ�־
 *  bit14��      ���յ�0x0d
 *  bit13~0��    ���յ�����Ч�ֽ���Ŀ
*/
uint16_t g_usart_rx_sta = 0;      
uint8_t g_rx_buffer[RXBUFFERSIZE];  /* HAL��ʹ�õĴ��ڽ��ջ��� */
uint8_t rx2_data = 0;
uint8_t rx3_data = 0;

//// ���崮��2�������ݽṹ��
//typedef struct {
//    uint16_t RxPacket[11];
//    uint8_t RxFlag;
//    SensorData data;
//} Serial2Data;

//// ���崮��3�������ݽṹ��
//typedef struct {
//    uint16_t RxPacket[11];
//    uint8_t RxFlag;
//    SensorData data;
//} Serial3Data;

UART_HandleTypeDef g_uart1_handle;  /* UART���  ѡ�ô���1*/
// ����2���
UART_HandleTypeDef huart2;
// ����3���
UART_HandleTypeDef huart3;

// ȫ�ֱ��������ڴ洢����2���յ�����
uint16_t Serial2_RxPacket[11];
uint8_t Serial2_RxFlag;
SensorData S2data = {0, 0, 0.0};
// ȫ�ֱ��������ڴ洢����3���յ�����
uint16_t Serial3_RxPacket[11];
uint8_t Serial3_RxFlag;
SensorData S3data = {0, 0, 0.0};

/**
 * @brief       ����X��ʼ������ ѡ�ô���1
 * @param       baudrate: ������, �����Լ���Ҫ���ò�����ֵ
 * @note        ע��: ����������ȷ��ʱ��Դ, ���򴮿ڲ����ʾͻ������쳣.
 *              �����USART��ʱ��Դ��sys_stm32_clock_init()�������Ѿ����ù���.
 * @retval      ��
 */
void usart_init(uint32_t baudrate)
{
    /*UART ��ʼ������*/
    g_uart1_handle.Instance = USART_UX;                                       /* USART_UX */
    g_uart1_handle.Init.BaudRate = baudrate;                                  /* ������ */
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;                      /* �ֳ�Ϊ8λ���ݸ�ʽ */
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;                           /* һ��ֹͣλ */
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;                            /* ����żУ��λ */
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;                      /* ��Ӳ������ */
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;                               /* �շ�ģʽ */
    HAL_UART_Init(&g_uart1_handle);                                           /* HAL_UART_Init()��ʹ��UART1 */
	
	if (HAL_UART_Init(&g_uart1_handle)!= HAL_OK) {
        // ������1��ʼ��ʧ�ܵ�����������ӡ������Ϣ
        printf("����1��ʼ��ʧ��\n");
    }

    /* �ú����Ὺ�������жϣ���־λUART_IT_RXNE���������ý��ջ����Լ����ջ��������������� */
    HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE); 
	
}


/**
 * @brief       ����2��ʼ������
 * @param       ��
 * @note        
 * @retval      ��
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
        // ������2��ʼ��ʧ�ܵ�����������ӡ������Ϣ
        printf("����2��ʼ��ʧ��\n");
    }
   /* �ú����Ὺ�������жϣ���־λUART_IT_RXNE���������ý��ջ����Լ����ջ��������������� */
    HAL_UART_Receive_IT(&huart2, (uint8_t *)g_rx_buffer, RXBUFFERSIZE); 
	
}


// ��ʼ������3
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
        // ������3��ʼ��ʧ�ܵ�����������ӡ������Ϣ
        printf("����3��ʼ��ʧ��\n");
    }
	
   /* �ú����Ὺ�������жϣ���־λUART_IT_RXNE���������ý��ջ����Լ����ջ��������������� */
    HAL_UART_Receive_IT(&huart3, (uint8_t *)g_rx_buffer, RXBUFFERSIZE); 
}


/**
 * @brief       UART�ײ��ʼ������
 * @param       huart: UART�������ָ��
 * @note        �˺����ᱻHAL_UART_Init()����
 *              ���ʱ��ʹ�ܣ��������ã��ж�����
 * @retval      ��
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (huart->Instance == USART_UX)                            /* ����Ǵ���1�����д���1 MSP��ʼ�� */
    {
        USART_TX_GPIO_CLK_ENABLE();                             /* ʹ�ܴ���TX��ʱ�� */
        USART_RX_GPIO_CLK_ENABLE();                             /* ʹ�ܴ���RX��ʱ�� */
        USART_UX_CLK_ENABLE();                                  /* ʹ�ܴ���ʱ�� */

        gpio_init_struct.Pin = USART_TX_GPIO_PIN;               /* ���ڷ������ź� */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* IO�ٶ�����Ϊ���� */
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);
                
        gpio_init_struct.Pin = USART_RX_GPIO_PIN;               /* ����RX�� ģʽ���� */
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;    
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);   /* ����RX�� �������ó�����ģʽ */
        
#if USART_EN_RX
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);                      /* ʹ��USART1�ж�ͨ�� */
        HAL_NVIC_SetPriority(USART_UX_IRQn, 3, 3);              /* ��2��������ȼ�:��ռ���ȼ�3�������ȼ�3 */
#endif
    }
	if (huart->Instance == USART2)                            /* ����Ǵ���2�����д���2 MSP��ʼ�� */
    {
        USART2_TX_GPIO_CLK_ENABLE();                             /* ʹ�ܴ���2 TX��ʱ�� */
        USART2_RX_GPIO_CLK_ENABLE();                             /* ʹ�ܴ���2 RX��ʱ�� */
        USART2_UX_CLK_ENABLE();                                  /* ʹ�ܴ���2ʱ�� */

        gpio_init_struct.Pin = USART2_TX_GPIO_PIN;               /* ����2�������ź� */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* IO�ٶ�����Ϊ���� */
        HAL_GPIO_Init(USART2_TX_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = USART2_RX_GPIO_PIN;               /* ����2 RX�� ģʽ���� */
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;
        HAL_GPIO_Init(USART2_RX_GPIO_PORT, &gpio_init_struct);   /* ����2 RX�� �������ó�����ģʽ */

#if USART2_EN_RX
        HAL_NVIC_EnableIRQ(USART2_IRQn);                      /* ʹ��USART2�ж�ͨ�� */
        HAL_NVIC_SetPriority(USART2_IRQn, 3, 3);              /* ��2��������ȼ�:��ռ���ȼ�3�������ȼ�3 */
#endif
    }
	if (huart->Instance == USART3)                            /* ����Ǵ���3�����д���3 MSP��ʼ�� */
    {
        USART3_TX_GPIO_CLK_ENABLE();                             /* ʹ�ܴ���3 TX��ʱ�� */
        USART3_RX_GPIO_CLK_ENABLE();                             /* ʹ�ܴ���3 RX��ʱ�� */
        USART3_UX_CLK_ENABLE();                                  /* ʹ�ܴ���3ʱ�� */

        gpio_init_struct.Pin = USART3_TX_GPIO_PIN;               /* ����3�������ź� */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* IO�ٶ�����Ϊ���� */
        HAL_GPIO_Init(USART3_TX_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = USART3_RX_GPIO_PIN;               /* ����3 RX�� ģʽ���� */
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;
        HAL_GPIO_Init(USART3_RX_GPIO_PORT, &gpio_init_struct);   /* ����3 RX�� �������ó�����ģʽ */

#if USART3_EN_RX
        HAL_NVIC_EnableIRQ(USART3_IRQn);                      /* ʹ��USART3�ж�ͨ�� */
        HAL_NVIC_SetPriority(USART3_IRQn, 3, 3);              /* ��2��������ȼ�:��ռ���ȼ�3�������ȼ�3 */
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
//        // ��������У�飨�����У��͵ȣ�
//        // ����Э���������
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
 * @brief       �������ݽ��ջص�����
                ���ݴ������������
 * @param       huart:���ھ��
 * @retval      ��
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART_UX)                    /* ����Ǵ���1 */
    {
        if ((g_usart_rx_sta & 0x8000) == 0)             /* ����δ��� */
        {
            if (g_usart_rx_sta & 0x4000)                /* ���յ���0x0d�����س����� */
            {
                if (g_rx_buffer[0] != 0x0a)             /* ���յ��Ĳ���0x0a�������ǻ��м��� */
                {
                    g_usart_rx_sta = 0;                 /* ���մ���,���¿�ʼ */
                }
                else                                    /* ���յ�����0x0a�������м��� */
                {
                    g_usart_rx_sta |= 0x8000;           /* ��������� */
                }
            }
            else                                        /* ��û�յ�0X0d�����س����� */
            {
                if (g_rx_buffer[0] == 0x0d)
                    g_usart_rx_sta |= 0x4000;
                else
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = g_rx_buffer[0];
                    g_usart_rx_sta++;

                    if (g_usart_rx_sta > (USART_REC_LEN - 1))
                    {
                        g_usart_rx_sta = 0;             /* �������ݴ���,���¿�ʼ���� */
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
        // ��������У�飨�����У��͵ȣ�
        // ����Э���������
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
			// ��������У�飨�����У��͵ȣ�
			// ����Э���������
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
 * @brief       ����1�жϷ�����  �˺������Զ����ã��������ж�������
 * @param       ��
 * @retval      ��
 */
void USART_UX_IRQHandler(void)
{
#if SYS_SUPPORT_OS                          /* ʹ��OS */
    OSIntEnter();    
#endif

    HAL_UART_IRQHandler(&g_uart1_handle);   /* ����HAL���жϴ����ú��� */

#if SYS_SUPPORT_OS                          /* ʹ��OS */
    OSIntExit();
#endif

}

// ����2�жϷ�������
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);   /* ����HAL���жϴ����ú��� */
}

// ����3�жϷ�������
void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3);   /* ����HAL���жϴ����ú��� */
}


// ���ݴ�����������2������Э���������
SensorData Serial2_ParseData(void)
{
//	printf("����Serial2_ParseData \r\n");
	if(Serial2_RxFlag == 1)
	{
//		printf(" Serial2_RxFlag == 1 ����Serial2_ParseData \r\n");
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

// ���ݴ�����������3������Э���������
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

