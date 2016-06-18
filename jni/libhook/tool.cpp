/*
 * tool.cpp
 *
 *  Created on: 2015年7月7日
 *      Author: Administrator
 */

#include "tool.h"
#include <sys/time.h>
#include <math.h>
#include <unistd.h>

unsigned long long GetCurSysTime()
{
	struct timeval tv;
	if(gettimeofday(&tv,0) < 0)
	{
		return -1;
	}

	return (((unsigned long long)tv.tv_sec) * 1000 + ((unsigned long long)tv.tv_usec) / 1000);
}

void mysleep(unsigned int milliseconds)
{
	unsigned int micro_seconds = milliseconds*1000;
	usleep(micro_seconds);
}




