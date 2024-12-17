/**
 ****************************************************************************************************
 * @file        usart.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.1
 * @date        2023-06-05
 * @brief       ���ڳ�ʼ������(һ���Ǵ���1)��֧��printf
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� STM32F103������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20211103
 * ��һ�η���
 * V1.1 20230605
 * ɾ��USART_UX_IRQHandler()�����ĳ�ʱ������޸�HAL_UART_RxCpltCallback()
 *
 ****************************************************************************************************
 */

#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* ���� �� ���� ���� 
 * Ĭ�������USART1��.
 * ע��: ͨ���޸��⼸���궨��,����֧��USART1~UART5����һ������.
 */
 
 //����1
#define USART_TX_GPIO_PORT                  GPIOA
#define USART_TX_GPIO_PIN                   GPIO_PIN_9
#define USART_TX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA��ʱ��ʹ�� */

#define USART_RX_GPIO_PORT                  GPIOA
#define USART_RX_GPIO_PIN                   GPIO_PIN_10
#define USART_RX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA��ʱ��ʹ�� */

#define USART_UX                            USART1
#define USART_UX_IRQn                       USART1_IRQn
#define USART_UX_IRQHandler                 USART1_IRQHandler
#define USART_UX_CLK_ENABLE()               do{ __HAL_RCC_USART1_CLK_ENABLE(); }while(0)  /* USART1 ʱ��ʹ�� */

// ���� �� ���� ���� - USART2
#define USART2_TX_GPIO_PORT                 GPIOA
#define USART2_TX_GPIO_PIN                  GPIO_PIN_2
#define USART2_TX_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA��ʱ��ʹ�� */

#define USART2_RX_GPIO_PORT 				GPIOA
#define USART2_RX_GPIO_PIN 					GPIO_PIN_3
#define USART2_RX_GPIO_CLK_ENABLE() 		do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA��ʱ��ʹ�� */

#define USART2_UX 							USART2
#define USART2_UX_IRQn 						USART2_IRQn
#define USART2_UX_IRQHandler 				USART2_IRQHandler
#define USART2_UX_CLK_ENABLE() 				do{ __HAL_RCC_USART2_CLK_ENABLE(); }while(0)  /* USART2 ʱ��ʹ�� */

// ���� �� ���� ���� - USART3
#define USART3_TX_GPIO_PORT 				GPIOB
#define USART3_TX_GPIO_PIN 					GPIO_PIN_10
#define USART3_TX_GPIO_CLK_ENABLE() 		do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* PB��ʱ��ʹ�� */

#define USART3_RX_GPIO_PORT 				GPIOB
#define USART3_RX_GPIO_PIN 					GPIO_PIN_11
#define USART3_RX_GPIO_CLK_ENABLE() 		do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* PB��ʱ��ʹ�� */

#define USART3_UX 							USART3
#define USART3_UX_IRQn 						USART3_IRQn
#define USART3_UX_IRQHandler 				USART3_IRQHandler
#define USART3_UX_CLK_ENABLE() 				do{ __HAL_RCC_USART3_CLK_ENABLE(); }while(0)  /* USART3 ʱ��ʹ�� */

/******************************************************************************************/

#define USART_REC_LEN               200         /* �����������ֽ��� 200 */
#define USART_EN_RX                 1          /* ʹ�ܣ�1��/��ֹ��0������1���� */
#define USART2_EN_RX                1
#define USART3_EN_RX                1
#define RXBUFFERSIZE   1                        /* �����С */

typedef struct {
    uint16_t tds;
	uint16_t cond;
    float temp;
} SensorData;
extern SensorData S2data;
extern SensorData S3data;

//UART_HandleTypeDef huart2;// ����2���
//UART_HandleTypeDef huart3;// ����3���
void uart2_init(void);
void uart3_init(void);
extern SensorData Serial2_ParseData(void);
extern SensorData Serial3_ParseData(void);
extern UART_HandleTypeDef g_uart1_handle;       /* HAL UART��� */

extern uint8_t  g_usart_rx_buf[USART_REC_LEN];  /* ���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� */
extern uint16_t g_usart_rx_sta;                 /* ����״̬��� */
extern uint8_t g_rx_buffer[RXBUFFERSIZE];       /* HAL��USART����Buffer */


void usart_init(uint32_t bound);                /* ���ڳ�ʼ������ */
void UART2_SendData(uint8_t *data, uint16_t size);
void UART3_SendData(uint8_t *data, uint16_t size);
#endif


