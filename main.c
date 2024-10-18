#include "stm32f10x.h"

uint32_t LED_pin = 13;

/*----------------------------------------------------------------
 * SystemCoreClockConfigure: функция для настройки частоты
 *----------------------------------------------------------------*/
void SystemCoreClockConfigure(void) {
		// Включаем HSI
		RCC->CR |= ((uint32_t)RCC_CR_HSION);
		// Ожидаем включения HSI
		while ((RCC->CR & RCC_CR_HSIRDY) == 0);

		// Устанавилваем HSI в качестве источника такстирования
		RCC->CFGR = RCC_CFGR_SW_HSI;
		// Ожидаем завершения
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

		// Что тут происходит (видимо настройка тактирования самого процессора и портов)
		RCC->CFGR |= RCC_CFGR_HPRE_DIV1; // HCLK = SYSCLK
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV1; // APB1 = HCLK/4
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV1; // APB2 = HCLK

		// Выключаем PLL чтобы настроить множители
		RCC->CR &= ~RCC_CR_PLLON;

		// Выбираем источник HSI/2, множитель 
		// (можно было напрямую выбрать HSI в SW без использования PLL, но я сделал так)
		RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
		RCC->CFGR |= (RCC_CFGR_PLLSRC_HSI_Div2 | RCC_CFGR_PLLMULL2);

		// Обратно включаем PLL
		RCC->CR |= RCC_CR_PLLON;
		// Ожидаем включения
		while((RCC->CR & RCC_CR_PLLRDY) == 0) __NOP();
		
		// Устанавливаем PLL в качестве источника 
		RCC->CFGR &= ~RCC_CFGR_SW;
		RCC->CFGR |= RCC_CFGR_SW_PLL;
		// Ожидаем конца запуска
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/*----------------------------------------------------------------
 * TIM4_Init: функция для настройки таймера общего назначения TIM4
 *----------------------------------------------------------------*/
void TIM4_Init () {
		// Включаем таймер
		RCC -> APB1ENR |= RCC_APB1ENR_TIM4EN;
		// Включаем счетчик таймера
		TIM4 -> CR1 = TIM_CR1_CEN;
		
		// Настраиваем значение с которого таймер будет отсчитывать до 0 и вызывать прерывание
		// Нам нужно отталкиваться от настроенной частоты, чтобы прерывание срабатывало каждую секунду
		TIM4->PSC = (SystemCoreClock / 1000) / 1000 - 1;
		// Зачем так делать
		TIM4->ARR = 1000 - 1;

		// Разрешаем прерывания
		TIM4->DIER |= TIM_DIER_UIE;
		NVIC_EnableIRQ (TIM4_IRQn);
}

/*----------------------------------------------------------------
 * enable_pin: функция для включения пина по его номеру (для порта A) 
 *----------------------------------------------------------------*/
void enable_pin(uint32_t pin_number){
		// Половина портов на CRL, другая на  CRH
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
 * GPIO_Init: функция для настройки портов ввода-вывода
 *----------------------------------------------------------------*/
void GPIO_Init (void) {
		// Разрешаем тактирование порта A
		RCC->APB2ENR |= (1UL << 2);
		
		// Определяем пин PA13 как выходной
		enable_pin(LED_pin);
}

/*----------------------------------------------------------------
 * switch_pin: функция, котора подает обратный сигнал на пин
			   (было 1, станет 0 и наоборот)
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
 * TIM4_IRQHandler: обработчик прерываний таймера 
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