#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
/* 7 segment LED PORTC.0 (low), PORTA.0-7(A-G,DP)
**            A
*          F   B
*            G
*          E   C
*            D       DP
*/
#define _BIT(x) (1 << (x))
#define LED_A  _BIT(0)
#define LED_B  _BIT(1)
#define LED_C  _BIT(2)
#define LED_D  _BIT(3)
#define LED_E  _BIT(4)
#define LED_F  _BIT(5)
#define LED_G  _BIT(6)
#define LED_DP _BIT(7)
#define GAME_MODE_START	        (1)
#define GAME_MODE_JUDGEMENT  	(2)
#define GAME_MODE_SETCHANGE      (3)
#define GAME_MODE_RESULT          (4)
#define SEGMENT_1ST			(0)
#define SEGMENT_2ND			(1)
#define SEGMENT_3RD			(2)
#define SEGMENT_4TH			(3)
 uint8_t Segment_Num[10] = 
{	LED_A | LED_B | LED_C | LED_D | LED_E | LED_F, 	/* 0 */ 
	LED_B | LED_C ,					/* 1 */ 			
	LED_A | LED_B | LED_G | LED_E | LED_D,		/* 2 */ 
	LED_A | LED_B | LED_G | LED_C | LED_D ,		/* 3 */ 
	LED_F | LED_G | LED_B | LED_C,			/* 4 */ 	
	LED_A | LED_F | LED_G | LED_C | LED_D,		/* 5 */ 
	LED_A | LED_F | LED_E | LED_D | LED_C | LED_G,		/* 6 */ 
	LED_A | LED_B | LED_C,				/* 7 */
	LED_A | LED_B | LED_C | LED_D | LED_E | LED_F | LED_G,	/* 8 */ 
	LED_A | LED_B | LED_C | LED_D | LED_F | LED_G		/* 9 */ 
};
uint8_t Segment_Win[4] = 
{	LED_A | LED_B | LED_C | LED_E | LED_F,
	LED_E | LED_F,
	LED_B | LED_C | LED_D,
	LED_B | LED_C | LED_D | LED_E | LED_F | LED_DP
};
uint8_t Segment_Lose[4] = 
{	LED_A | LED_D | LED_E | LED_F | LED_G,
	LED_A | LED_F | LED_G | LED_C | LED_D,
	LED_A | LED_B | LED_C | LED_D | LED_E | LED_F,
	LED_D | LED_E | LED_F
};
uint8_t Segment_Set[2] =
{	LED_A | LED_B | LED_C | LED_D | LED_E | LED_F | LED_G | LED_DP,	/* B. */	
	LED_A | LED_F | LED_G | LED_C | LED_D | LED_DP			/* S. */ 
};
uint8_t Rand_flag[10] = {0,0,0,0,0,0,0,0,0,0};
uint8_t Rand_Num[4];
uint8_t count,idxSegment,idxDisplay;
uint16_t rand_num;
uint8_t rand_n1,rand_n10,rand_n100,rand_n1000;
uint8_t target_n1,target_n10,target_n100,target_n1000;
uint16_t timer_4ms,timer_100ms,timer_1000ms;
uint16_t blink_timer;
uint8_t flag_4ms,flag_100ms;
uint8_t stSwitch1,stSwitch1Old;
uint8_t stSwitch2,stSwitch2Old;
uint8_t seg_num1,seg_num2,seg_num3,seg_num4;
uint8_t randT_n1,randT_n10,randT_n100,randT_n1000;
uint16_t cnt_Buzzer,buzzer_timeout;
uint8_t Game_Mode,Game_Strike,Game_Ball;
uint16_t Game_Mode_Timer;
uint8_t Game_life = 8;
uint8_t Game_Result;
static void Game_mode(void);
static void Switch_read(void);
static void Segment_display(void);
static void LED_display(void);
static void Buzzer(void);
int main(void)
{	DDRG = 0x0F;
	DDRB = 0xFF;
	DDRC = 0xFF;
	DDRE = 0x00;
	DDRA = 0xFF;
	PORTA = 0xFF;
		TIMSK = 0x04; // Timer 1 Overflow interrupt enable
	TCCR1A = 0x00;
	TCCR1B = 0x04; //일반모드, 256 분주
	TCNT1 =  65286; //1/16*256*(65536-65224) : 4[ms]
	SREG = 0x80; // Global Interrupt Enable
	  
    /* Replace with your application code */
    while (1) 
    {
		if(1 == flag_4ms)
		{
			Game_mode();
			Segment_display();
			LED_display();
			Buzzer();
			flag_4ms = 0;
		}
		if(1 == flag_100ms)
		{
			Switch_read();
			flag_100ms = 0;
		}
    }
}static void Switch_read(void)
{				
	stSwitch1 = (PINE&0x10)>>4;
	stSwitch2 = (PINE&0x20)>>5;	
		// switch 2번만 눌린 상태일때, Seven segment 위치 이동
	if((1 == stSwitch1)&&(1 == stSwitch1Old)&&(0 == stSwitch2)&&(1 == stSwitch2Old))
	{
		idxSegment++;
	}
	// switch 1번만 눌린 상태일때, Seven segment 숫자 증가
	if((0 == stSwitch1)&&(1 == stSwitch1Old)&&(1 == stSwitch2)&&(1 == stSwitch2Old))
	{
		switch(idxSegment)
		{
			case SEGMENT_1ST :
				target_n1++;
				if(target_n1>=10)
				{
					target_n1 = 0;
				}
			break;
			case SEGMENT_2ND :
				target_n10++;
				if(target_n10>=10)
				{
					target_n10 = 0;
				}
			break;
			case SEGMENT_3RD :
				target_n100++;			
				if(target_n100>=10)
				{
					target_n100 = 0;
				}
			break;
			case SEGMENT_4TH :
				target_n1000++;			
				if(target_n1000>=10)
				{
					target_n1000 = 0;
				}
			break;
		}
	}
	// switch 1번,2번 동시에 눌릴 경우, 숫자 설정 확정 및 Random값 capture
	if((0 == stSwitch1)&&(1 == stSwitch1Old)&&(0 == stSwitch2)&&(1 == stSwitch2Old))
	{
		for(uint8_t i = 0; i<4;)
		{
			uint8_t temp = (rand()%10);
			if(0 == Rand_flag[temp])
			{
				Rand_flag[temp] = 1;
				Rand_Num[i] = temp;
				i++;
			}
		}
		rand_n1 = Rand_Num[SEGMENT_1ST];
		rand_n10 = Rand_Num[SEGMENT_2ND];
		rand_n100 = Rand_Num[SEGMENT_3RD];
		rand_n1000 = Rand_Num[SEGMENT_4TH];
		
		Game_Mode = GAME_MODE_START;
	}
	stSwitch1Old = stSwitch1;
	stSwitch2Old = stSwitch2;
}static void Game_mode(void)
{	switch(Game_Mode)
	{
		case GAME_MODE_START : 
			if(idxSegment>SEGMENT_4TH)
			{
				idxSegment = 0;
				Game_Mode = GAME_MODE_JUDGEMENT;
			}
		break;
		case GAME_MODE_JUDGEMENT :
			if(target_n1 == rand_n1)
			{
				Game_Strike++;
			}
			else
			{
				if(target_n1 == rand_n10)
				{
					Game_Ball++;
				}
				else
				{
					if(target_n1 == rand_n100)
					{
						Game_Ball++;
					}
					else
					{
						if(target_n1 == rand_n1000)
						{
							Game_Ball++;
						}						
					}
				}
			}
			if(target_n10 == rand_n10)
			{
				Game_Strike++;	
			}
			else
			{
				if(target_n10 == rand_n1)
				{
					Game_Ball++;
				}
				else
				{
					if(target_n10 == rand_n100)
					{
						Game_Ball++;
					}
					else
					{
						if(target_n10 == rand_n1000)
						{
							Game_Ball++;
						}
					}
				}
			}
			if(target_n100 == rand_n100)
			{
				Game_Strike++;	
			}
			else
			{
				if(target_n100 == rand_n1)
				{
					Game_Ball++;
				}
				else
				{
					if(target_n100 == rand_n10)
					{
						Game_Ball++;
					}
					else
					{
						if(target_n100 == rand_n1000)
						{
							Game_Ball++;
						}
					}
				}
			}
			
			if(target_n1000 == rand_n1000)
			{
				Game_Strike++;	
			}
			else
			{
				if(target_n1000 == rand_n1)
				{
					Game_Ball++;
				}
				else
				{
					if(target_n1000 == rand_n10)
					{
						Game_Ball++;
					}
					else
					{
						if(target_n1000 == rand_n100)
						{
							Game_Ball++;
						}
					}
				}
			}
			Game_life--;
			Game_Mode = GAME_MODE_SETCHANGE;
		break;
		case GAME_MODE_SETCHANGE :
			Game_Mode_Timer++;
			if((Game_Mode_Timer >= 1250))
			{
				if((0 == Game_life)||(4 == Game_Strike))
				{
					Game_Mode = GAME_MODE_RESULT;
				}
				else
				{
					Game_Mode = GAME_MODE_START;
					Game_Strike = 0; // 1셋트 종료시, 스트라이크 횟수 초기화
					Game_Ball = 0; // 1셋트 종료시, 볼 횟수 초기화
				}
				Game_Mode_Timer = 0;
			}
		break;
		case GAME_MODE_RESULT :
			Game_Mode_Timer++;
			if(4 == Game_Strike)
			{
				Game_Result = 1;  //Win
			}		
			if(0 == Game_life)
			{
				Game_Result = 2; //Lose
			}
			
			if((Game_Mode_Timer >= 1250))
			{
				Game_Mode_Timer = 0;
				target_n1 = 0;
				target_n10 = 0;
				target_n100 = 0;
				target_n1000 = 0;
				Game_life = 8;
				Game_Strike = 0; // 스트라이크 횟수 초기화
				Game_Ball = 0; //  볼 횟수 초기화
				idxSegment = 0;
				blink_timer = 0;
				for(uint8_t i=0;i<10;i++)
				{
					Rand_flag[i] = 0;// 랜덤 함수 중복 방지 flag 초기화
				}				
				for(uint8_t i=0;i<4;i++)
				{
					Rand_Num[i] = 0;// 랜덤 함수 저장 값 초기화
				}
				Game_Mode = GAME_MODE_START;
			}
		break;
	}
}static void Segment_display(void)
{	
	switch(Game_Mode)
	{
		case GAME_MODE_START : 
			switch(idxDisplay)
			{
				case SEGMENT_1ST :
					if(SEGMENT_1ST != idxSegment)
					{
						PORTG = 0x01;
						PORTC = Segment_Num[target_n1];
					}
					else
					{
						if(blink_timer<=25)
						{
							PORTG = 0x01;
							PORTC = Segment_Num[target_n1];							
						}
						else
						{
							PORTG = 0x01;
							PORTC = 0x00;		
						}
					}
				break;
				case SEGMENT_2ND :
					if(SEGMENT_2ND != idxSegment)
					{
						PORTG = 0x02;
						PORTC = Segment_Num[target_n10];
					}
					else
					{
						if(blink_timer<=25)
						{
							PORTG = 0x02;
							PORTC = Segment_Num[target_n10];							
						}
						else
						{
							PORTG = 0x02;
							PORTC = 0x00;		
						}
					}
				break;
				case SEGMENT_3RD :
					if(SEGMENT_3RD != idxSegment)
					{
						PORTG = 0x04;
						PORTC = Segment_Num[target_n100];
					}
					else
					{
						if(blink_timer<=25)
						{
							PORTG = 0x04;
							PORTC = Segment_Num[target_n100];							
						}
						else
						{
							PORTG = 0x04;
							PORTC = 0x00;		
						}
					}
				break;
				case SEGMENT_4TH :					
					if(SEGMENT_4TH != idxSegment)
					{
						PORTG = 0x08;
						PORTC = Segment_Num[target_n1000];
					}
					else
					{
						if(blink_timer<=25)
						{
							PORTG = 0x08;
							PORTC = Segment_Num[target_n1000];							
						}
						else
						{
							PORTG = 0x08;
							PORTC = 0x00;		
						}
					}
				break;
			}	
		break;
		case GAME_MODE_SETCHANGE : 
			switch(idxDisplay)
			{
				case SEGMENT_1ST :
					PORTG = 0x01;
					PORTC = Segment_Set[0];
				break;
				case SEGMENT_2ND :
					PORTG = 0x02;
					PORTC = Segment_Num[Game_Ball];
				break;
				case SEGMENT_3RD :
					PORTG = 0x04;
					PORTC = Segment_Set[1];
				break;
				case SEGMENT_4TH :
					PORTG = 0x08;
					PORTC = Segment_Num[Game_Strike];
				break;
			}	
		break;
		case GAME_MODE_RESULT :
			switch(idxDisplay)
			{
				case SEGMENT_1ST :
					PORTG = 0x01;
				break;
				case SEGMENT_2ND :
					PORTG = 0x02;
				break;
				case SEGMENT_3RD :
					PORTG = 0x04;
				break;
				case SEGMENT_4TH :
					PORTG = 0x08;
				break;
			}			
			if(1 == Game_Result)
			{
				PORTC = Segment_Win[idxDisplay];
			}
			if(2 == Game_Result)
			{
				PORTC = Segment_Lose[idxDisplay];				
			}
		break;
	}
	idxDisplay++;
	if(idxDisplay>3)
	{
		idxDisplay = 0;
	}
	blink_timer++;
	if(blink_timer>=50)
	{
		blink_timer = 0;
	}
}static void Buzzer(void)
{	if(GAME_MODE_RESULT == Game_Mode)
	{
		if(cnt_Buzzer < 4)
		{
			PORTB = 0x10;
		}
		else
		{
			PORTB = 0x00;
		}
	}
	else if(GAME_MODE_SETCHANGE == Game_Mode)
	{	
		if(cnt_Buzzer < 12)
		{
			PORTB = 0x10;
		}
		else
		{
			PORTB = 0x00;
		}
	}
	else
	{
		PORTB = 0x00;
		cnt_Buzzer = 0;
	}
		cnt_Buzzer++;
	if(cnt_Buzzer>=25)
	{
		cnt_Buzzer = 0;
	}
}static void LED_display(void)
{	switch(Game_life)
	{
		case 0:
			PORTA = 0x00;
		break;
		case 1:
			PORTA = 0x01;
		break;
		case 2:
			PORTA = 0x03;
		break;
		case 3:
			PORTA = 0x07;
		break;
		case 4:
			PORTA = 0x0F;
		break;
		case 5:
			PORTA = 0x1F;
		break;
		case 6:
			PORTA = 0x3F;
		break;
		case 7:
			PORTA = 0x7F;
		break;
		case 8:
			PORTA = 0xFF;
		break;
	}
}ISR(TIMER1_OVF_vect)
{	TCNT1 =  65286; //1/16*256*(65536-65224) : 4[ms]
	timer_4ms++;
	flag_4ms = 1;
	if(timer_4ms>=25)
	{
		timer_100ms++;
		flag_100ms = 1;
		timer_4ms=0;
	}
}