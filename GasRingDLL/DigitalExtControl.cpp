#include "DigitalExtControl.h"
#include <string>

DigitalExtControl::DigitalExtControl(void)
{
}


DigitalExtControl::~DigitalExtControl(void)
{
}


long DigitalExtControl::ErrorHandler(LJ_ERROR lngErrorcode, long lngLineNumber, long lngIteration)
{
	char err[255];

	if (lngErrorcode != LJE_NOERROR)
	{
		ErrorToString(lngErrorcode,err);
		return lngErrorcode;
		// err = string    lngLineNumber    lngIteration
	}
	
}


bool DigitalExtControl::DigitalOut(int pin)
{
	return false;
}


bool DigitalExtControl::DigitalRead(int pin)
{
	return false;
}


bool DigitalExtControl::OpenConnection(void)
{
	return false;
}
