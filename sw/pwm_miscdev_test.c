#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

// TODO: update these offsets if your address are different
#define Red_out_OFFSET 0x0
#define Green_out_OFFSET 0x4
#define Blue_out_OFFSET 0x08

int main () {
	FILE *adc;
	FILE *pwm;
	size_t ret;	
	size_t r;
	size_t g;
	size_t b;		
	uint32_t val;

	adc = fopen("/dev/adc" , "rb+" );
	if (adc == NULL) {
		printf("failed to open file\n");
		exit(1);
	}

	pwm = fopen("/dev/pwm" , "rb+" );
	if (pwm == NULL) {
		printf("failed to open file\n");
		exit(1);
	}

	// Test reading the registers sequentially
	printf("\n************************************\n*");
	printf("* reading from adc\n");
	printf("************************************\n\n");

	r = fread(&val, 4, 1, adc);
	printf("red_out = 0x%x\n", val);
	printf("fread ret = %d\n", ret);
	printf("errno =%s\n", strerror(errno));

	g = fread(&val, 4, 1, adc);
	printf("green_out = 0x%x\n", val);
	printf("fread ret = %d\n", ret);
	printf("errno =%s\n", strerror(errno));

	b = fread(&val, 4, 1, adc);
	printf("blue_out = 0x%x\n", val);
	printf("fread ret = %d\n", ret);
	printf("errno =%s\n", strerror(errno));
	
	// Reset file position to 0
	ret = fseek(adc, 0, SEEK_SET);
	printf("fseek ret = %d\n", ret);
	printf("errno =%s\n", strerror(errno));
	//r = ret;
	//g = ret;
	//b = ret;


	printf("\n************************************\n*");
	printf("* write values to the pwm\n");
	printf("************************************\n\n");
	// Turn on software-control mode
	val = r;
    ret = fseek(pwm, Red_out_OFFSET, SEEK_SET);
		printf("fseek ret = %d\n", ret);
	printf("errno =%s\n", strerror(errno));
	ret = fwrite(&val, 4, 1, pwm);
	// We need to "flush" so the OS finishes writing to the file before our code continues.
	fflush(pwm);

	// Write some values to each PWM for RGB led
	printf("writing patterns to LEDs....\n");
	val = g;
    ret = fseek(pwm, Blue_out_OFFSET, SEEK_SET);
		printf("fseek ret = %d\n", ret);
	printf("errno =%s\n", strerror(errno));
	ret = fwrite(&val, 4, 1, pwm);
	fflush(pwm);

	sleep(1);

	val = b;
    ret = fseek(pwm, Green_out_OFFSET, SEEK_SET);
		printf("fseek ret = %d\n", ret);
	printf("errno =%s\n", strerror(errno));
	ret = fwrite(&val, 4, 1, pwm);
	fflush(pwm);


	printf("\n************************************\n*");
	printf("* read new register values\n");
	printf("************************************\n\n");
	
	// Reset file position to 0
	ret = fseek(adc, 0, SEEK_SET);

	ret = fread(&val, 4, 1, adc);

	ret = fread(&val, 4, 1, adc);

	ret = fread(&val, 4, 1, adc);
	r = ret;
	g = ret;
	b = ret;

	fclose(adc);
	return 0;
}
