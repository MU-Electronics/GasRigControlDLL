
#ifndef DIGITALEXTCONTROL_H
#define DIGITALEXTCONTROL_H

#pragma once

#include "LabJackUD.h"
#include <string>
#include <map>

class DigitalExtControl
{
private:
	LJ_HANDLE lngHandle;
	LJ_ERROR lngErrorcode;
	bool connected;
	long lngGetNextIteration;
	bool OpenConnection(bool defaultPins);
	long ErrorHandler(LJ_ERROR lngErrorcode, long lngLineNumber, long lngIteration);
	bool Execute(void);
public:
	std::map<int, double> data;
	DigitalExtControl(void);
	~DigitalExtControl(void);
	bool isConnected();
	double DigitalOut(int pin, int logic);
	double DigitalRead(int pin);
	bool ResetPins(void);
	double AnalougRead(int pin);
	bool CloseConnection();
};


#endif
