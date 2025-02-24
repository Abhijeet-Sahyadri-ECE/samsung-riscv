#include <ch32v00x.h>
#include <debug.h>

#define TRIG_PORT GPIOA
#define TRIG_PIN GPIO_Pin_1
#define ECHO_PORT GPIOA
#define ECHO_PIN GPIO_Pin_2

#define IR_PORT GPIOD
#define IR_PIN GPIO_Pin_3

#define CLOCK_ENABLE RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOD | RCC_APB1Periph_TIM2, ENABLE)

volatile int passenger_count = 0;

void init_sensors(void);
uint32_t measure_distance(void);
int detect_ir(void);
void init_timer(void);

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    init_sensors();
    init_timer();

    while (1)
    {
        uint32_t distance = measure_distance();
        int ir_detected = detect_ir();

        if (ir_detected)
        {
            if (distance < 18)
            {
                passenger_count++;
                Delay_Ms(400);
            }
            else if (distance > 28)
            {
                if (passenger_count > 0)
                {
                    passenger_count--;
                    Delay_Ms(400);
                }
            }
        }
        Delay_Ms(100);
    }
}

void init_sensors(void)
{
    CLOCK_ENABLE;

    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.GPIO_Pin = TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TRIG_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(ECHO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = IR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(IR_PORT, &GPIO_InitStructure);
}

void init_timer(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    TIM_TimeBaseStructure.TIM_Period = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock / 1000000 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    TIM_Cmd(TIM2, ENABLE);
}

uint32_t measure_distance(void)
{
    uint32_t time = 0, timeout = 30000;

    GPIO_WriteBit(TRIG_PORT, TRIG_PIN, Bit_SET);
    Delay_Us(10);
    GPIO_WriteBit(TRIG_PORT, TRIG_PIN, Bit_RESET);

    while (!GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) && timeout--)
        Delay_Us(1);

    if (timeout == 0) return 999;

    timeout = 30000;

    TIM_SetCounter(TIM2, 0);

    while (GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) && timeout--)
        time = TIM_GetCount