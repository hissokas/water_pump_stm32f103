#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

// Изображения цифр

const uint8_t chargen[] = { ~0x3F, ~0x06, ~0x5B, ~0x4F, ~0x66, ~0x6D, ~0x7D, ~0x07, ~0x7F, ~0x6F, ~0x3F };

// Кнопки

#define BUTTON0 ((GPIOA->IDR & (1<<9 )) == 0)
#define BUTTON1 ((GPIOA->IDR & (1<<10)) == 0)
#define BUTTON2 ((GPIOA->IDR & (1<<8 )) == 0)
#define BUTTON3 ((GPIOA->IDR & (1<<11)) == 0)

// ШИМ

volatile uint16_t aa = 3, bb = 39, nn = 0;

// Остальные переменные

volatile uint8_t  led_step = 0;
volatile uint8_t  buttons_delay = 0;
volatile uint16_t tick = 0;

// Лог шкала значений

const uint16_t val[] = {
    0,1,2,3,4,6,7,10,13,18,24,32,42,56,75,100,133,177,236,315,420,561,747,997,1329,1772,2362,
    3150,4199,5599,7466,9954,13272,17696,23595,31460,41946,55928,65535,
};

#define MAX_VAL (sizeof(val)/sizeof(val[0]))

// Таймер

void TIM4_IRQHandler()
{    
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) == RESET) return;
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

    tick++;

    // Подтяжка кнопок
    uint16_t a = 0x0F00;

    // Рабочий цикл
    uint16_t aaa = val[aa];
    uint16_t bbb = val[bb];
    if(nn >= aaa + bbb) nn = 0; else nn++;
    if(nn > bbb || BUTTON0) a |= 0x1000;

    // Светодиоды
    switch(led_step)
    {
        case 0: a |= chargen[aa / 10]; GPIOB->ODR = 0x000F; led_step=1; break;
        case 1: a |= chargen[aa % 10]; GPIOB->ODR = 0x00F0; led_step=2; if(tick & 0x100) a &= ~0x80; break;
        case 2: a |= chargen[bb / 10]; GPIOB->ODR = 0x0F00; led_step=3; break;
        case 3: a |= chargen[bb % 10]; GPIOB->ODR = 0xF000; led_step=0; if(a & 0x1000) a &= ~0x80; break;
    }

    // Вывод в порт
    GPIOA->ODR = a;

    // Кнопки
    if(buttons_delay)
    {
        buttons_delay--;
    }
    else
    {
	    if(BUTTON2) { if(BUTTON1) { if(aa>0      ) aa--; } else { if(bb>0      ) bb--; } buttons_delay = 64; }
    	if(BUTTON3) { if(BUTTON1) { if(aa<MAX_VAL) aa++; } else { if(bb<MAX_VAL) bb++; } buttons_delay = 64; }
    }
}

void init_timer(void)
{
}

int main(void)
{
    // A12, A7-A0 на выход
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = 0x10FF;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // A8-A11 на вход c подтяжкой. Возможно это не нужно.
    GPIO_InitStructure.GPIO_Pin = 0xF00;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Порт B на выход
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = 0xFFFF;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Запуск таймера
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    TIM_TimeBaseInitTypeDef Timer;
    TIM_TimeBaseStructInit(&Timer);
    Timer.TIM_Prescaler = SystemCoreClock / 1000 - 1;
    Timer.TIM_Period = 1;
    TIM_TimeBaseInit(TIM4, &Timer);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
    NVIC_EnableIRQ(TIM4_IRQn);

    // Запущено
    for(;;);
}
