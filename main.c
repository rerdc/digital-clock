#define F_CPU 16000000UL
#define FOSC 16000000UL
#define MYUBRR F_CPU/16/BAUD-1
#define BAUD 9600

#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "AT.h"
#include "Data_handler.h"
#include "System.h"
#include "I2CTWI.h"
#include "LCD.h"
#include "rtc.h"
#include "lists_records.h"



volatile int flag = 0;
volatile int timer = 0;


int main(void)
{
	_delay_ms(2000);
	sei();
	init_RTC();
	lcd_init();
	config_addresses();
	setup_USART(MYUBRR);
	setup_timer1(31249);
	
	struct macs mac_list[16];
	struct records current_record;
	
	uint8_t records_index = 0;
	uint16_t Address = 11;
	
	char received_data[256]; //to save data received from ESP
	char rssi[8]; //to save power level
	char mac[32]; //to save mac address
	uint16_t arr_len = sizeof(mac_list)/sizeof(mac_list[0]); //size of the array of structures containing a predefined list of mac addresses
	
	_delay_ms(1000);
	
	int sys_ready = ESP_config(); //Configure start up settings(remove echo, list by power) of ESP and confirm if it's set
	
	fill_macs(mac_list); // fill the array of structures with a location and corresponding mac address for comparison
	
	_delay_ms(3000);
	
    while (1 && sys_ready == 1) //loop if the configuration of ESP was set and while there's space to store more data // && sys_ready == 1
	{
		i2c_ler_relogio();
		lcd_print(vect);
		USART_Flush(); //Clear unwanted data on USART buffer
		
		if (timer % 6 == 0 && records_index < 20 && flag == 0 && timer != 0)
		{
			send_AT("AT+CWLAP=\"ISEPWLAN\"\0"); //send AT command requesting list of available AP with id ISEPWLAN //
			_delay_ms(1000);
			get_AT(received_data); //receive the first AP with best power and store it on received_data variable
			get_rssi(received_data, rssi); //from the received data filter the power level
			get_mac(received_data, mac); //from the received data filter the mac address
			_delay_ms(100);
			
			if (atoi(rssi) > -60) //check if the power level is bigger than a predefine level and if there is space to store
			{
				for (int i = 0; i < arr_len; i++)
				{
					if (strcmp(mac, mac_list[i].mac) == 0) //check if the filtered mac corresponds to any of the macs on the predefined list
					{
						strcpy(current_record.room, mac_list[i].room); //save the mac
						save_date(&current_record);
						for (int j = 0; j < sizeof(current_record.room); j++)
						{
							vect[j+28] = current_record.room[j];
						}
						vect[10] = current_record.room[1];
						writeStruct(Address, current_record, records_index); //save structure on eeprom
						eeprom_busy_wait();
						records_index++; //increments the stored data index
						eeprom_write_byte((uint8_t*)10, records_index);
						eeprom_busy_wait();
						timer = 0;
						break;
					};
				};
			};
		}
		
		if (flag == 1)
		{
			int index = eeprom_read_byte((uint8_t *)10);
			struct records list[index];
			readStruct(Address, list, index);
			send_records(list, index);
			timer = 0;
			flag = 0;
		};
		
	};
};     

ISR(INT1_vect)
{
	flag = 1;
}   

ISR(TIMER1_COMPA_vect)
{
	timer++;
}                                                                                                                                                                                                                                                                                                                                                        