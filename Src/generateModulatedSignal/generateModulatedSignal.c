/*
 * generateModulatedSignal.c
 *
 *  Created on: Jan 29, 2019
 *      Author: uskov.o
 */

#include "math.h"

#include "generateModulatedSignal.h"
void Error_Handler(void);

#define true 1
#define false 0


#define Number_of_point 500
#define DAC_SHIFT 2047 /// zero of voltage

extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim1;

int GenerateSIN_to_DAC(void);
void Ratio_OfModulationCalc(uint16_t);
void Calculate_next_Ampl(void);


uint16_t Base_sin_Array[Number_of_point]; //Array of cosinus data
const uint16_t Ampl_base = 511;
uint16_t Ampl = 0;
uint16_t Ratio_OfModulation = 1;
const int minimalFrequancy = (500000 / Number_of_point)/2;
int delta = 1; // set frequancy
uint8_t bit_in_symbol = 4; // may be 1, 2, 4, 8
uint8_t periodWithBaseAmpl = 1;
uint8_t* dataToSend_ptr;
uint8_t mask = 0xFF;
int numberOfTransferedByte = 0;
int PacketLenght = 0;


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance==TIM1) //check if the interrupt comes from TIM1
	{
		if (GenerateSIN_to_DAC()){ // finish period
			if (periodWithBaseAmpl) {
				Calculate_next_Ampl();
				Ratio_OfModulationCalc(Ampl);

			}
			else {
				Ratio_OfModulationCalc(0);

			}

			periodWithBaseAmpl= !periodWithBaseAmpl;

		}
	}

}

int GenerateSIN_to_DAC()
{
  static int val = 0;
  static int phase = 0;
  *(uint32_t*)(DAC_BASE + 0x00000008U) = val;
  phase += delta;
  //phase = phase > Number_of_point * 2 - 1 ? phase - Number_of_point * 2 : phase;
  if(phase > Number_of_point * 2 - 1) {
	  phase = phase - Number_of_point * 2;
	  return true;
  	}


  if(phase < Number_of_point) {
	val = DAC_SHIFT + Ratio_OfModulation*Base_sin_Array[phase];
  }
  else if(phase < 2 * Number_of_point) {
	val = DAC_SHIFT - Ratio_OfModulation*Base_sin_Array[2 * Number_of_point - 1 - phase];

  }
  return false;

}

void Ratio_OfModulationCalc(uint16_t newAmpl) //Ampl in range 0 to 1523
{
	Ratio_OfModulation = (uint16_t)(round(newAmpl/Ampl_base + 1));
}


void HalfSinArrayCreating() //Ampl in range 0 to 1523
{

	//if (Ampl_base < -1524 || Ampl_base > 1524) return; ////???
    const float PI = 3.1415927;
    for(int i = 0; i < Number_of_point; i++) {
      Base_sin_Array[i] = (uint16_t)( round(Ampl_base * sin((i * PI) / (Number_of_point))));
    }
}




void Calculate_next_Ampl() //Ampl in range 0 to 1523
{
	static int partOfbyte = 0;
	Ampl = (*dataToSend_ptr) >> (bit_in_symbol *  partOfbyte); // (8 >> (bit_in_symbol-1)) ��� ������� �� ���
	Ampl &= mask; // clean unnecessary bit
	partOfbyte++;

	if (partOfbyte == (8/bit_in_symbol)) // finish to send byte
	{
		numberOfTransferedByte++;
		dataToSend_ptr++;
		partOfbyte = 0;
	}


	if (numberOfTransferedByte == PacketLenght+1)
	{
		numberOfTransferedByte = 0;
		if(HAL_TIM_Base_Stop_IT(&htim1) != HAL_OK)
		{
		/* Starting Error */
		Error_Handler();
		}
		if (HAL_DAC_Stop(&hdac, DAC_CHANNEL_1) != HAL_OK)
		{
		Error_Handler();
		}
	}
}

void InitArrayOfBaseSin(int Init_freq,int Init_bit_in_symbol) ///???500k/500 = 1k ������� ��� ������ =1
{
	HalfSinArrayCreating();
	delta = Init_freq/ minimalFrequancy; ///????
	bit_in_symbol = Init_bit_in_symbol;
	mask = (0xFF >> (8 - Init_bit_in_symbol));
}

void sendPacket(uint8_t* data_ptr, int Lenght) //Ampl in range 0 to 1523
{
	//extern DAC_HandleTypeDef hdac;
	//extern TIM_HandleTypeDef htim;
	//extern int  numberOfTransferedByte;
	numberOfTransferedByte = 1; //indicate about starting of transfer data
	PacketLenght = Lenght;
	dataToSend_ptr = data_ptr;
	//������ �������
	if(HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
	{
	/* Starting Error */
	Error_Handler();
	}
	if (HAL_DAC_Start(&hdac, DAC_CHANNEL_1) != HAL_OK)
	{
	Error_Handler();
	}
}
