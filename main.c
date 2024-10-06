#include "stm32f10x.h"

// Номер пина светодиода
uint32_t LED_pin = 13;

// Структура для хранения времени
struct time{
		uint32_t hours;
		uint32_t minutes;
		uint32_t seconds;
};
struct time current_time; // Текущее время
struct time alarm_time; 	// Время будильника

uint32_t start_alarm_flag; // Чтобы понять когда подавать сигнал

/*----------------------------------------------------------------
 * SystemCoreClockConfigure: Функция для настройки частоты
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
 * TIM3_Init: функция для настройки таймера общего назначения TIM3
 * (для отсчета времени)
 *----------------------------------------------------------------*/
void TIM3_Init () {
		// Включаем таймер
		RCC -> APB1ENR |= RCC_APB1ENR_TIM3EN;
		// Включаем счетчик таймера
		TIM3 -> CR1 = TIM_CR1_CEN;
		
		// Настраиваем значение с которого таймер будет отсчитывать до 0 и вызывать прерывание
		// Нам нужно отталкиваться от настроенной частоты, чтобы прерывание срабатывало каждую секунду
		TIM3->PSC = (SystemCoreClock) / 1000 - 1;
		// Зачем так делать
		TIM3->ARR = 1000 - 1;

		// Разрешаем прерывания
		TIM3->DIER |= TIM_DIER_UIE;
		NVIC_EnableIRQ (TIM3_IRQn);
}

/*----------------------------------------------------------------
 * TIM4_Init: функция для настройки таймера общего назначения TIM4
 * (для мигания светодиода)
 *----------------------------------------------------------------*/
void TIM4_Init () {
		// Включаем таймер
		RCC -> APB1ENR |= RCC_APB1ENR_TIM4EN;
		// Включаем счетчик таймера
		TIM4 -> CR1 = TIM_CR1_CEN;
		
		// Настраиваем значение с которого таймер будет отсчитывать до 0 и вызывать прерывание
		// Нам нужно отталкиваться от настроенной частоты, чтобы прерывание срабатывало каждую секунду
		TIM4->PSC = (SystemCoreClock / 2) / 1000 - 1;
		// Зачем так делать
		TIM4->ARR = 1000 - 1;

		// Разрешаем прерывания
		TIM4->DIER |= TIM_DIER_UIE;
		NVIC_EnableIRQ (TIM4_IRQn);
}

/*----------------------------------------------------------------
 * enable_pin: Функция для включения пина по его номеру (для порта A) 
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
 * GPIO_Init: Функция для настройки портов ввода-вывода
 *----------------------------------------------------------------*/
void GPIO_Init (void) {
		// Разрешаем тактирование порта A
		RCC->APB2ENR |= (1UL << 2);
		
		// Определяем пин PA5 как выходной (для светодиода)
		// PA5 потому что на имеющейся обучающей плате есть светодиод на данном порту
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
 * TIM3_IRQHandler: Обработчик прерываний таймера TIM3
 *----------------------------------------------------------------*/
void TIM3_IRQHandler () {
		TIM3->SR &= ~TIM_SR_UIF;
		
		// Каждую секунду считаем текущее время
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
		// Проверяем равно ли оно времени будильника
		if(current_time.hours == alarm_time.hours && current_time.minutes == alarm_time.minutes){
				start_alarm_flag = 1;
		}
}

/*----------------------------------------------------------------
 * TIM4_IRQHandler: обработчик прерываний таймера TIM4
 *----------------------------------------------------------------*/
void TIM4_IRQHandler () {
		TIM4->SR &= ~TIM_SR_UIF;
		
		// пока что непонятно когда нужно убрать сигнал (автоматически по истечению времени? когда будет нажата кнопка?)
		if(start_alarm_flag){
				switch_pin(LED_pin);
		}
}




int main (void) {
		// Настраиваем будильник (пока так)
		alarm_time.hours = 0;
		alarm_time.minutes = 1;
		
		// Устанавливаем частоту
		SystemCoreClockConfigure();
		SystemCoreClockUpdate();

		// Настраиваем порты ввода вывода
		GPIO_Init();
	
		// Настраиваем таймеры
		TIM3_Init();
		TIM4_Init();
	
		while (1) {
			
		}
}