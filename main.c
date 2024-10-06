#include "stm32f10x.h"

// ����� ���� ����������
uint32_t LED_pin = 13;

// ��������� ��� �������� �������
struct time{
		uint32_t hours;
		uint32_t minutes;
		uint32_t seconds;
};
struct time current_time; // ������� �����
struct time alarm_time; 	// ����� ����������

uint32_t start_alarm_flag; // ����� ������ ����� �������� ������

/*----------------------------------------------------------------
 * SystemCoreClockConfigure: ������� ��� ��������� �������
 *----------------------------------------------------------------*/
void SystemCoreClockConfigure(void) {
		// �������� HSI
		RCC->CR |= ((uint32_t)RCC_CR_HSION);
		// ������� ��������� HSI
		while ((RCC->CR & RCC_CR_HSIRDY) == 0);

		// ������������� HSI � �������� ��������� �������������
		RCC->CFGR = RCC_CFGR_SW_HSI;
		// ������� ����������
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

		// ��� ��� ���������� (������ ��������� ������������ ������ ���������� � ������)
		RCC->CFGR |= RCC_CFGR_HPRE_DIV1; // HCLK = SYSCLK
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV1; // APB1 = HCLK/4
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV1; // APB2 = HCLK

		// ��������� PLL ����� ��������� ���������
		RCC->CR &= ~RCC_CR_PLLON;

		// �������� �������� HSI/2, ��������� 
		// (����� ���� �������� ������� HSI � SW ��� ������������� PLL, �� � ������ ���)
		RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
		RCC->CFGR |= (RCC_CFGR_PLLSRC_HSI_Div2 | RCC_CFGR_PLLMULL2);

		// ������� �������� PLL
		RCC->CR |= RCC_CR_PLLON;
		// ������� ���������
		while((RCC->CR & RCC_CR_PLLRDY) == 0) __NOP();
		
		// ������������� PLL � �������� ��������� 
		RCC->CFGR &= ~RCC_CFGR_SW;
		RCC->CFGR |= RCC_CFGR_SW_PLL;
		// ������� ����� �������
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/*----------------------------------------------------------------
 * TIM3_Init: ������� ��� ��������� ������� ������ ���������� TIM3
 * (��� ������� �������)
 *----------------------------------------------------------------*/
void TIM3_Init () {
		// �������� ������
		RCC -> APB1ENR |= RCC_APB1ENR_TIM3EN;
		// �������� ������� �������
		TIM3 -> CR1 = TIM_CR1_CEN;
		
		// ����������� �������� � �������� ������ ����� ����������� �� 0 � �������� ����������
		// ��� ����� ������������� �� ����������� �������, ����� ���������� ����������� ������ �������
		TIM3->PSC = (SystemCoreClock) / 1000 - 1;
		// ����� ��� ������
		TIM3->ARR = 1000 - 1;

		// ��������� ����������
		TIM3->DIER |= TIM_DIER_UIE;
		NVIC_EnableIRQ (TIM3_IRQn);
}

/*----------------------------------------------------------------
 * TIM4_Init: ������� ��� ��������� ������� ������ ���������� TIM4
 * (��� ������� ����������)
 *----------------------------------------------------------------*/
void TIM4_Init () {
		// �������� ������
		RCC -> APB1ENR |= RCC_APB1ENR_TIM4EN;
		// �������� ������� �������
		TIM4 -> CR1 = TIM_CR1_CEN;
		
		// ����������� �������� � �������� ������ ����� ����������� �� 0 � �������� ����������
		// ��� ����� ������������� �� ����������� �������, ����� ���������� ����������� ������ �������
		TIM4->PSC = (SystemCoreClock / 2) / 1000 - 1;
		// ����� ��� ������
		TIM4->ARR = 1000 - 1;

		// ��������� ����������
		TIM4->DIER |= TIM_DIER_UIE;
		NVIC_EnableIRQ (TIM4_IRQn);
}

/*----------------------------------------------------------------
 * enable_pin: ������� ��� ��������� ���� �� ��� ������ (��� ����� A) 
 *----------------------------------------------------------------*/
void enable_pin(uint32_t pin_number){
		if(pin_number < 8ul){
				GPIOA->CRL &= ~((15ul << 4*pin_number));
				GPIOA->CRL |= (( 1ul << 4*pin_number));
		}
		else{
				pin_number -= 8ul;
				GPIOA->CRH &= ~((15ul << 4*pin_number));
				GPIOA->CRH |= (( 1ul << 4*pin_number));
		}
}

/*----------------------------------------------------------------
 * GPIO_Init: ������� ��� ��������� ������ �����-������
 *----------------------------------------------------------------*/
void GPIO_Init (void) {
		// ��������� ������������ ����� A
		RCC->APB2ENR |= (1UL << 2);
		
		// ���������� ��� PA5 ��� �������� (��� ����������)
		// PA5 ������ ��� �� ��������� ��������� ����� ���� ��������� �� ������ �����
		enable_pin(LED_pin);
}

void switch_pin (uint32_t pin_number) {
		if(GPIOA->ODR & (1ul << pin_number)){
			GPIOA->BSRR = 1ul << (pin_number + 16ul);
		}
		else{
			GPIOA->BSRR = 1ul << pin_number;
		}
}

/*----------------------------------------------------------------
 * TIM3_IRQHandler: ���������� ���������� ������� TIM3
 *----------------------------------------------------------------*/
void TIM3_IRQHandler () {
		TIM3->SR &= ~TIM_SR_UIF;
		
		// ������ ������� ������� ������� �����
		current_time.seconds++;
		if(current_time.seconds > 60){
				current_time.seconds = 0;
				current_time.minutes++;
				if(current_time.minutes > 60){
						current_time.minutes = 0;
						current_time.hours++;
						if(current_time.hours > 24){
								current_time.hours = 0;
							}
				}
		}
		// ��������� ����� �� ��� ������� ����������
		if(current_time.hours == alarm_time.hours && current_time.minutes == alarm_time.minutes){
				start_alarm_flag = 1;
		}
}

/*----------------------------------------------------------------
 * TIM4_IRQHandler: ���������� ���������� ������� TIM4
 *----------------------------------------------------------------*/
void TIM4_IRQHandler () {
		TIM4->SR &= ~TIM_SR_UIF;
		
		// ���� ��� ��������� ����� ����� ������ ������ (������������� �� ��������� �������? ����� ����� ������ ������?)
		if(start_alarm_flag){
				switch_pin(LED_pin);
		}
}




int main (void) {
		// ����������� ��������� (���� ���)
		alarm_time.hours = 0;
		alarm_time.minutes = 1;
		
		// ������������� �������
		SystemCoreClockConfigure();
		SystemCoreClockUpdate();

		// ����������� ����� ����� ������
		GPIO_Init();
	
		// ����������� �������
		TIM3_Init();
		TIM4_Init();
	
		while (1) {
			
		}
}