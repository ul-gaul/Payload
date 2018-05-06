#include <mbed.h>

Timer timer;
AnalogIn therm(PA_1);
AnalogIn photo(PB_0);
DigitalOut g_LED(LED2);
Serial pc(USBTX, USBRX);

// TEC current to DAC PIN PA4 12 bits 4096 to 3.3 V **** réduire à 750 mV (Marc)

uint16_t samples[5];
int begin, end;

int main() 
{	
	timer.start();
	begin = timer.read_ms();
	
	for (;;)
	{
		
		uint8_t i;
		float average;
		
		for (i = 0; i < 5; i++) 
		{
			samples[i] = therm.read() * 1023;
			wait_ms(10);
		}
		
		average = 0;
		for (i = 0; i < 5; i++) {
			average += samples[i];
		}
		average /= 5;
		
		average = 1023 / average - 1;
		average = 10000 / average;
		
		float steinhart;
		steinhart = average / 10000;      // (R/Ro)
		steinhart = log(steinhart);                   // ln(R/Ro)
		steinhart /= 3900;                    // 1/B * ln(R/Ro)
		steinhart += 1.0 / (25 + 273.15);  // + (1/To)
		steinhart = 1.0 / steinhart;                  // Invert
		steinhart -= 273.15;                          // convert to C
		
		end = timer.read_ms();
		pc.printf("%d ", end - begin);
		pc.printf("%f ", steinhart);
		//pc.printf("%f\n", steinhart);
		//pc.printf("%f\n", photo.read());
		// set tec alim current ::: wait heating ::: start log ::: start TEC ::: wait for temp cte. ::: stop TEC ::: wait for temp cte. (30) ::: Save log
		// verify closed TEC time
		wait_ms(100);
	}
	
/*
	for (;;)
	{
		pc.printf("Temp: %f\n", therm.read());
		g_LED = 1;
		wait_ms(100);
		g_LED = 0;
		wait_ms(100);
	}
*/
	
}