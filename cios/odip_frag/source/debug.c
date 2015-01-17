#include <debug.h>

#include <stdarg.h>    // for the s_printf function

#include <syscalls.h>

#include "usb.h"

#define os_puts usb_puts

#ifdef DEBUG

static char mem_cad[32];

void int_char(int num)
{
int sign=num<0;
int n,m;

	if(num==0)
		{
		mem_cad[0]='0';mem_cad[1]=0;
		return;
		}

	for(n=0;n<10;n++)
		{
		m=num % 10;num/=10;if(m<0) m=-m;
		mem_cad[25-n]=48+m;
		}

	mem_cad[26]=0;

	n=0;m=16;
	if(sign) {mem_cad[n]='-';n++;}

	while(mem_cad[m]=='0') m++;

	if(mem_cad[m]==0) m--;

	while(mem_cad[m]) 
	 {
	 mem_cad[n]=mem_cad[m];
	 n++;m++;
	 }
	mem_cad[n]=0;

}

void uint_char(unsigned int num)
{
int n,m;

	if(num==0)
		{
		mem_cad[0]='0';mem_cad[1]=0;
		return;
		}

	for(n=0;n<10;n++)
		{
		m=num % 10;num/=10;
		mem_cad[25-n]=48+m;
		}

	mem_cad[26]=0;

	n=0;m=16;

	while(mem_cad[m]=='0') m++;

	if(mem_cad[m]==0) m--;

	while(mem_cad[m]) 
	 {
	 mem_cad[n]=mem_cad[m];
	 n++;m++;
	 }
	mem_cad[n]=0;

}

void hex_char(u32 num)
{
int n,m;

	if(num==0)
		{
		mem_cad[0]='0';mem_cad[1]=0;
		return;
		}

	for(n=0;n<8;n++)
		{
		m=num & 15;num>>=4;
		if(m>=10) m+=7;
		mem_cad[23-n]=48+m;
		}

	mem_cad[24]=0;

	n=0;m=16;
    
	mem_cad[n]='0';n++;
	mem_cad[n]='x';n++;

	while(mem_cad[m]=='0') m++;

	if(mem_cad[m]==0) m--;

	while(mem_cad[m]) 
	 {
	 mem_cad[n]=mem_cad[m];
	 n++;m++;
	 }
	mem_cad[n]=0;

}

void s_printf(char *format,...)
{
 va_list	opt;

 char out[2]=" ";

 int val;

 char *s;

 va_start(opt, format);

 while(format[0])
	{
	if(format[0]!='%') {out[0]=*format++;os_puts(out);}
	else
		{
		format++;
		switch(format[0])
			{
			case 'd':
			case 'i':
				val=va_arg(opt,int);
				int_char(val);
				
				os_puts(mem_cad);
				
				break;

			case 'u':
				val=va_arg(opt, unsigned);
				uint_char(val);
				
				os_puts(mem_cad);
				
				break;

			case 'x':
				val=va_arg(opt,int);
				hex_char((u32) val);
				os_puts(mem_cad);
				
				break;

			case 's':
				s=va_arg(opt,char *);
				os_puts(s);
				break;

			}
		 format++;
		}
	
	}
   
	va_end(opt);

	
}

#endif
