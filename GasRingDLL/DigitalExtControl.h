
#ifndef DIGITALEXTCONTROL_H
#define DIGITALEXTCONTROL_H

#pragma once

#include "LabJackUD.h"
#include <string>

class DigitalExtControl
{
private:
	bool OpenConnection(void);
	long ErrorHandler(LJ_ERROR lngErrorcode, long lngLineNumber, long lngIteration);
public:
	DigitalExtControl(void);
	~DigitalExtControl(void);
	bool DigitalOut(int pin);
	bool DigitalRead(int pin);
	
};


#endif
