#include "stm32f10x.h"

uint32_t LED_pin = 13;

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
 * TIM4_Init: ������� ��� ��������� ������� ������ ���������� TIM4
 *----------------------------------------------------------------*/
void TIM4_Init () {
		// �������� ������
		RCC -> APB1ENR |= RCC_APB1ENR_TIM4EN;
		// �������� ������� �������
		TIM4 -> CR1 = TIM_CR1_CEN;
		
		// ����������� �������� � �������� ������ ����� ����������� �� 0 � �������� ����������
		// ��� ����� ������������� �� ����������� �������, ����� ���������� ����������� ������ �������
		TIM4->PSC = (SystemCoreClock / 1000) / 1000 - 1;
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
		// �������� ������ �� CRL, ������ ��  CRH
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
		
		// ���������� ��� PA13 ��� ��������
		enable_pin(LED_pin);
}

/*----------------------------------------------------------------
 * switch_pin: �������, ������ ������ �������� ������ �� ���
			   (���� 1, ������ 0 � ��������)
 *----------------------------------------------------------------*/
void switch_pin (uint32_t pin_number) {
		if(GPIOA->ODR & (1ul << pin_number)){
			GPIOA->BSRR = 1ul << (pin_number + 16ul);
		}
		else{
			GPIOA->BSRR = 1ul << pin_number;
		}
}

/*----------------------------------------------------------------
 * TIM4_IRQHandler: ���������� ���������� ������� 
 *----------------------------------------------------------------*/
void TIM4_IRQHandler () {
		TIM4->SR &= ~TIM_SR_UIF;
		switch_pin (LED_pin);
}




int main (void) {
		SystemCoreClockConfigure();
		SystemCoreClockUpdate();

		GPIO_Init();
		TIM4_Init ();

		while (1) {}
}