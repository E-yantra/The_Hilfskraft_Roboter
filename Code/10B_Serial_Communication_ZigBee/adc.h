#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


unsigned char ADC_Value;
unsigned char lline = 0;
unsigned char cline = 0;
unsigned char rline = 0;
unsigned char fir = 0;

//for PID

float kp=1.2;
float kd=1.22;
float ki=0;
int prop=0;
int der=0;
int integral=0;
int last_prop=0;
int pow_diff;
int max=80; //speed

int thresh = 170;
//int speed=50;
//int delta=0; // positive if right is faster



//This Function accepts the Channel Number and returns the corresponding Analog Value 
unsigned char ADC_Conversion(unsigned char Ch)
{
 unsigned char a;
 Ch = Ch & 0x07;  			
 ADMUX= 0x20| Ch;	   		
 ADCSRA = ADCSRA | 0x40;	//Set start conversion bit
 while((ADCSRA&0x10)==0);	//Wait for ADC conversion to complete
 a=ADCH;
 ADCSRA = ADCSRA|0x10;      //clear ADIF (ADC Interrupt Flag) by writing 1 to it
 return a;
}

void init_sensor_values(void)
{
	lline = ADC_Conversion(3);	//Getting data of Left WL Sensor
	cline = ADC_Conversion(4);	//Getting data of Center WL Sensor
	rline = ADC_Conversion(5);	//Getting data of Right WL Sensor
	print_sensor(2,1,3);		//Prints value of White Line Sensor Left
	print_sensor(2,5,4);		//Prints value of White Line Sensor Center
	print_sensor(2,9,5);		//Prints Value of White Line Sensor Right
	if(lline < thresh )
	{
		if(cline < thresh) prop =max/2; // white white black
		else if(rline>thresh) prop=max/4; //white black black 
	}
	else if(rline < thresh)
	{
		if(lline<thresh) prop=0;  // white black white
		else if(cline<thresh) prop=-max/2; // black white white
		else prop=-max/4; // black black white
	}
	der = prop-last_prop; //derivative
	integral += prop; // integral
	last_prop = prop; 
	pow_diff = kp*prop + ki*integral + kd*der;
	if(pow_diff > max) pow_diff = max;
	if(pow_diff < -max) pow_diff = -max;
	
	fir = ADC_Conversion(1); // front IR
}

// This Function prints the Analog Value Of Corresponding Channel No. at required Row
// and Coloumn Location. 
void print_sensor(char row, char coloumn,unsigned char channel)
{
 ADC_Value = ADC_Conversion(channel);
 lcd_print(row, coloumn, ADC_Value, 3);
}


void take_turn(int d)
{
	int left1;
	int right1;
	int flag=0;
	while(1)
	{
		if(d==1) left();
		else if(d==2) right();
		init_sensor_values();	
		if(pow_diff < 0) velocity((max+pow_diff), max);
		else velocity(max, (max-pow_diff));	
		//left1=speed*0.5;
		//right1=(speed-delta)*0.5;
		//velocity(left1,right1);
		//lcd_print(2,13,left1,2);
		//lcd_print(2,15,right1,2);
		
		
		if(cline<thresh && lline<thresh && rline<thresh)  //all on white
		{
			_delay_ms(130);
			stop();
			_delay_ms(50);
			break;
		}
	}
	while(1)
	{
		if(d==1) soft_left();
		else if(d==2) soft_right();
		init_sensor_values();
		if(pow_diff < 0) velocity((max+pow_diff)/2, max/2);
		else velocity(max/2, (max-pow_diff)/2);
		/*if(d==1)
		{
			left1=0;
			right1=(speed-delta)*0.5;
		}
		else if(d==2) 
		{
			left1=speed*0.5;
			right1=0;
		}
		velocity(left1,right1);
		lcd_print(2,13,left1,2);
		lcd_print(2,15,right1,2);		*/
		if(cline>thresh && lline>thresh*0.6 && d==1) flag=1;
		if(cline>thresh && rline>thresh*0.6 && d==2) flag=1;
		if(flag==1)
		{
			stop();
			_delay_ms(40);
			break;
		}
	}
}
void turn_left()
{
	lcd_cursor(1,1);		
	lcd_string("Left");
	take_turn(1);
}
void turn_right()
{
	lcd_cursor(1,1);		
	lcd_string("Right");
	take_turn(2);
}

int checkobstacle() //returns 0 if there is an obstacle
{
	init_sensor_values();
	if(fir<0x28)
	{
		
		stop();
		lcd_cursor(1,1);		
		lcd_string("Obstacle");		
		_delay_ms(100);		
		return  0;
	}	
	return 1;
}



int checkintersection() 	//returns 1 if there is an intersection
{
	init_sensor_values();
	if( cline>thresh && lline>thresh && rline >thresh ) //all on black
	{
		if(pow_diff < 0) velocity(max+pow_diff, max);
		else velocity(max, max-pow_diff);
		_delay_ms(2150);
		lcd_cursor(1,1);		
		lcd_string("Intersection");
		stop();
		_delay_ms(3000);
		return 1;
	}
	return 0;
}

void follow()
{
	lcd_cursor(1,1);		
	lcd_string("Go Straight");
	init_sensor_values();
	forward();
	int left,right;
	if(pow_diff < 0) velocity(max+pow_diff, max);
	else velocity(max, max-pow_diff);
	/*if(cline>thresh)
	{
		if(rline>thresh)
		{
			left=speed; 
			right=0.8*(speed-delta);
		}
		else if(lline>thresh)
		{
			left=0.8*speed;
			right=(speed-delta);
		}
		else
		{
			left=speed;
			right=(speed-delta);
		 }
	}
	else 
	{
		if(rline>thresh)
		{
			left=speed;
			right = 0.6*(speed-delta);
		}
		else if(lline>thresh)
		{
			left=speed*0.6;
			right=(speed-delta);
		}
	}
	*/
	if((cline<thresh) && (lline<thresh) && (rline<thresh) ) 
	{
		forward();
		left=0;
		right=0;
		stop();
		//buzzer_on();
		lcd_cursor(1,1);		
		lcd_string("Stop");
		
	}
	//velocity(left,right);
	//lcd_print(2,13,left,2);
	//lcd_print(2,15,right,2);
}

