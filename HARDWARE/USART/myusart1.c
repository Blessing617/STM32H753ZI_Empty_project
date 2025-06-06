#include "myusart1.h"

uint8_t MyUSART1_buffer[MYUSART1_MAX_LEN];
uint32_t MyUSART1_bufferPos = 0;
uint8_t MyUSART1_bufferOverflowFlag = 0;
UART_HandleTypeDef *MyUSART1_dummyHandle;

struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       

//�ض���fputc���� 
int fputc(int ch, FILE *f)
{
  while (!__HAL_UART_GET_FLAG(MyUSART1_dummyHandle, UART_FLAG_TC))
    ;
#if defined(STM32H750xx) || defined(STM32L431xx)|| defined(STM32G4)
  USART1->TDR = (uint8_t)ch;
#endif
#if defined(STM32F407xx) || defined(STM32F103xB) 
  USART1->DR = (uint8_t)ch;
#endif
  return ch;
}

void MyUSART1_Init(UART_HandleTypeDef *huart)
{
  MyUSART1_dummyHandle = huart;
  __HAL_UART_ENABLE_IT(MyUSART1_dummyHandle, UART_IT_RXNE);
  ;
}

void MyUSART1_WriteChar(uint8_t ch)
{
  while (!__HAL_UART_GET_FLAG(MyUSART1_dummyHandle, UART_FLAG_TC))
    ;
#if defined(STM32H750xx) || defined(STM32L431xx)|| defined(STM32G4)
  USART1->TDR = ch;
#endif
#if defined(STM32F407xx) || defined(STM32F103xB)
  USART1->DR = ch;
#endif
}

uint8_t MyUSART1_Write(uint8_t *str, uint16_t len)
{
  if (len > MYUSART1_MAX_LEN)
    return 0;
  for (uint32_t i = 0; i < len; i++)
  {
    MyUSART1_WriteChar(*(str + i));
  }
  return len;
}
uint8_t MyUSART1_WriteStr(uint8_t *str) // the \0 will BE sent;
{
  return MyUSART1_WriteUntil(str, '\0');
}

uint8_t MyUSART1_WriteLine(uint8_t *str) // the \0 of the string will not be sent, and the transimitted data ends with \r\n(no \0 at the end)
{
  uint32_t i;
  for (i = 0; i < MYUSART1_MAX_LEN; i++)
  {
    if (*(str + i) == '\0')
      break;
    MyUSART1_WriteChar(*(str + i));
  }
  MyUSART1_WriteChar('\r');
  MyUSART1_WriteChar('\n');
  return i + 2 < MYUSART1_MAX_LEN;
}

uint8_t MyUSART1_WriteUntil(uint8_t *str, uint8_t endChar) // similar to the MyUSART1_WriteStr() with the customized EndChar
{
  uint32_t i;
  for (i = 0; i < MYUSART1_MAX_LEN; i++)
  {
    MyUSART1_WriteChar(*(str + i));
    if (*(str + i) == endChar)
      break;
  }
  return i < MYUSART1_MAX_LEN;
}

uint8_t MyUSART1_ReadChar(void)
{
  if (MyUSART1_bufferPos != 0)
    MyUSART1_bufferPos--;
  else
    MyUSART1_buffer[0] = 0;
  return MyUSART1_buffer[MyUSART1_bufferPos];
}

uint8_t MyUSART1_PeekChar(void)
{
  if (MyUSART1_bufferPos == 0)
    return MyUSART1_buffer[MYUSART1_MAX_LEN - 1];
  return MyUSART1_buffer[MyUSART1_bufferPos - 1];
}

uint8_t MyUSART1_CanReadLine(void)
{
  uint32_t i;
  for (i = 1; i < MyUSART1_bufferPos; i++)
  {
    if (MyUSART1_buffer[i - 1] == '\r' && MyUSART1_buffer[i] == '\n')
      return 1;
  }
  return 0;
}

uint8_t MyUSART1_CanReadUntil(uint16_t endChar)
{
  uint32_t i;
  for (i = 0; i < MyUSART1_bufferPos; i++)
  {
    if (MyUSART1_buffer[i] == endChar)
      return 1;
  }
  return 0;
}

uint8_t MyUSART1_CanReadStr()
{
  return MyUSART1_CanReadUntil('\0');
}

uint32_t MyUSART1_Read(uint8_t *str, uint16_t maxLen)
{
  maxLen = maxLen < MyUSART1_bufferPos ? maxLen : MyUSART1_bufferPos;
  uint32_t i;
  for (i = 0; i < maxLen; i++)
  {
    *(str + i) = MyUSART1_buffer[i];
  }
  __MyUSART1_Shift(maxLen);
  return maxLen;
}

uint32_t MyUSART1_ReadStr(uint8_t *str)
{
  return MyUSART1_ReadUntil(str, '\0');
}

uint32_t MyUSART1_ReadLine(uint8_t *str)
{
  if (!MyUSART1_CanReadLine())
    return 0;
  uint32_t i;
  for (i = 0; i < MyUSART1_bufferPos; i++)
  {
    *(str + i) = MyUSART1_buffer[i];
    if (i > 0 && *(str + i - 1) == '\r' && *(str + i) == '\n')
    {
      i++;
      break;
    }
  }
  __MyUSART1_Shift(i);
  return i;
}

uint32_t MyUSART1_ReadUntil(uint8_t *str, uint16_t endChar)
{
  if (!MyUSART1_CanReadUntil(endChar))
    return 0;
  uint32_t i;
  for (i = 0; i < MyUSART1_bufferPos; i++)
  {
    *(str + i) = MyUSART1_buffer[i];
    if (*(str + i) == endChar)
    {
      i++;
      break;
    }
  }
  __MyUSART1_Shift(i);
  return i;
}

uint32_t MyUSART1_ReadAll(uint8_t *str)
{
  uint32_t i;
  for (i = 0; i < MyUSART1_bufferPos; i++)
  {
    *(str + i) = MyUSART1_buffer[i];
  }
  __MyUSART1_Shift(i);
  return i;
}

void __MyUSART1_Shift(uint32_t len)
{
  for (uint32_t i = 0; i < MYUSART1_MAX_LEN - len; i++)
  {
    MyUSART1_buffer[i] = MyUSART1_buffer[i + len];
  }
  MyUSART1_bufferPos -= len;
}

void __MyUSART1_DumpAll(void)
{
  if (MYUSART1_MAX_LEN > 100)
    return;
  MyUSART1_WriteChar(MyUSART1_bufferPos);
  for (uint32_t i = 0; i < MYUSART1_MAX_LEN; i++)
  {
    MyUSART1_WriteChar(MyUSART1_buffer[i]);
  }
}

void MyUSART1_ClearBuffer(void)
{
  MyUSART1_bufferPos = 0;
}

void MyUSART1_IRQHandler(USART_TypeDef *source)
{

  if (!__HAL_UART_GET_FLAG(MyUSART1_dummyHandle, UART_FLAG_RXNE))
    return;
#if defined(STM32H753xx) || defined(STM32L431xx)|| defined(STM32G4)
  MyUSART1_buffer[MyUSART1_bufferPos++] = USART1->RDR;
#endif
#if defined(STM32F407xx) || defined(STM32F103xB)
  MyUSART1_buffer[MyUSART1_bufferPos++] = USART1->DR;
#endif
  if (MyUSART1_bufferPos == MYUSART1_MAX_LEN)
  {
    MyUSART1_bufferPos = 0;
    MyUSART1_bufferOverflowFlag = 1;
  }
}
