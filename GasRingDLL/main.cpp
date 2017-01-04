#include <Afxtempl.h>
#include <afxmt.h>
#include <extcode.h>
#include "PressureSensor.h"
#include "DigitalExtControl.h"
#include "TC110Communicator.h"
#include "string.h"
#include <sstream>
#include <iostream>
extern "C" { int _afxForceUSRDLL; } 

TC110Communicator* TC110;
PressureSensor* Pressure;

/**
 * This function adds two numbers together and is used for debugging to ensure the DLL is working (will be removed)
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) plus(double a, double b, double *out)
{
	*out = a + b;

	return 0;
}

/**
 * This function returns a flow rate reading from the Brooks mass flow device
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
typedef int32_t(__cdecl* ReadFlowDef)(int32_t COM, int32_t Baud, uint8_t IDH, uint8_t IDM, uint8_t IDL, uint8_t IDS, float *Flow, uint8_t *FlowUnit, uint8_t *S1, uint8_t *S2);
extern "C" int32_t __declspec(dllexport) ReadFlowRate(int32_t COM, int32_t Baud, uint8_t IDH, uint8_t IDM, uint8_t IDL, uint8_t IDS, float *Flow, uint8_t *FlowUnit, uint8_t *S1, uint8_t *S2)
{
	HMODULE BrooksDLL = LoadLibrary(L"BrooksDLL.dll");

	if(BrooksDLL != NULL)
	{
		ReadFlowDef ReadFlow = (ReadFlowDef)GetProcAddress(BrooksDLL, "ReadFlow");

		if(ReadFlow != NULL)
		{
			int32_t FlowRate = ReadFlow(COM, Baud, IDH, IDM, IDL, IDS, Flow, FlowUnit, S1, S2);

			return FlowRate;
		}

		return 1;
	}

	return 0;
}

/**
 * This function sets a flow rate of gas for the Brooks mass flow device
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
typedef int32_t(__cdecl* SetFlowRateDef)(int32_t COM, int32_t Baud, uint8_t IDH, uint8_t IDM, uint8_t IDL, uint8_t IDS, uint8_t SetPointCode, float SetPoint, uint8_t *S1, uint8_t *S2);
extern "C" int32_t __declspec(dllexport) SetFlowRate(int32_t COM, int32_t Baud, uint8_t IDH, uint8_t IDM, uint8_t IDL, uint8_t IDS, uint8_t SetPointCode, float SetPoint, uint8_t *S1, uint8_t *S2)
{
	HMODULE BrooksDLL = LoadLibrary(L"BrooksDLL.dll");

	if(BrooksDLL != NULL)
	{
		SetFlowRateDef SetFlow = (SetFlowRateDef)GetProcAddress(BrooksDLL, "WriteSetPoint");

		if(SetFlow != NULL)
		{
			int32_t Set = SetFlow(COM, Baud, IDH, IDM, IDL, IDS, SetPointCode, SetPoint, S1, S2);

			return Set;
		}

		return 1;
	}

	return 0;
}





/**
 * This function opens a valve via the labjack digital outout pin
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) OpenValve(int32_t id)
{
	DigitalExtControl* hardware = new DigitalExtControl(); 
	
	double valve = hardware->DigitalOut(id, 1);

	return 1;
}

/**
 * This function closes a valve via the labjack digital outout pin
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) CloseValve(int32_t id)
{
	DigitalExtControl* hardware = new DigitalExtControl(); 
	
	double valve = hardware->DigitalOut(id, 0);

	return (int) valve;
}

/**
 * This function reads the vacuum pressure guage
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) ReadVacPressure()
{
	DigitalExtControl* hardware = new DigitalExtControl(); 
	
	double valve = hardware->AnalougRead(0);

	return (int) valve;
}





/**
 * Set up pressure sensor connection
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) PressureConnection(int32_t com, int32_t id)
{
	std::stringstream ss;
	if(com > 10){
		std::string comport = "\\\\.\\COM";
		ss << comport << com;
	}else{
		std::string comport = "COM";
		ss << comport << com;
	}
	std::string fullcomPort = ss.str();


	Pressure = new PressureSensor();

	// Init Communication
	Pressure->m_nDevice= 250;
	Pressure->InitCommunication();

	// Set the port
	CString S = fullcomPort.c_str(); LPTSTR lpsz = new TCHAR[S.GetLength()+1]; _tcscpy(lpsz, S);	
	_u32 baud = 9600;
	Pressure->_bEcho= false;

	// Open Com Port
	Pressure->OpenCommPort(lpsz, baud);

	// Set default values
	_u8 nClass;
	_u8 nGroup;
	_u8 nYear; 
	_u8 nWeek;
	_u8 nBuffer;
	_u8 nState;
	_u32 nSN;

	// Wake up the device
	Pressure->F48(&nClass, &nGroup, &nYear, &nWeek, &nBuffer, &nState);
	Sleep(200);

	return 1;
}

/**
 * Set up pressure sensor connection
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) PressureConnectionClose()
{
	// Close the com port
	Pressure->CloseCommPort();

	return 1;
}

/**
 * This function returns a pressure reading from the keller pressure sensor
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" double __declspec(dllexport) ReadPressure()
{

	// Set default values
	_u8 nClass;
	_u8 nGroup;
	_u8 nYear; 
	_u8 nWeek;
	_u8 nBuffer;
	_u8 nState;
	_u32 nSN;

	// Wake up the device
	Pressure->F48(&nClass, &nGroup, &nYear, &nWeek, &nBuffer, &nState);
	Sleep(200);

	// Initialise the device
	int nRes= Pressure->F48(&nClass, &nGroup, &nYear, &nWeek, &nBuffer, &nState);

	// Check the answer
	if(nRes!= COMM_OK) {
		return 0;
	}

	// Read the acutal value
	float fVal;
	int channel = 1;
	int nRess= Pressure->F73(channel, &fVal);

	// Check the answer
	if(nRess != COMM_OK) {
		return 0;
	}

	return fVal;
}





/**
 * Set up turbo pump connection
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) PumpConnection(int32_t com, int32_t id)
{
	std::stringstream ss;
	if(com > 10){
		std::string comport = "\\\\.\\COM";
		ss << comport << com;
	}else{
		std::string comport = "COM";
		ss << comport << com;
	}
	std::string fullcomPort = ss.str();

	TC110 = new TC110Communicator(fullcomPort.c_str() , (int) id); //    "\\\\.\\COM12"

	if(TC110->IsConnected())
		return 1;

	return 0;
}

/**
 * Close the pump connection
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) PumpConnectionClose(void)
{
	if(!TC110->Close())
		return 1;

	return 0;
}

/**
 * This function retrieves temperatures from the vac station
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) VacStationTemperature(int32_t type)
{
	if(type >= 1 && type <= 4){
		int temperature = TC110->GetTemperature((int) type);
		return temperature;
	}
		
	return 999;
}

/**
 * This function retrieves currently set gas mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) GasMode()
{
	int gasMode = TC110->GetGasMode();
	return gasMode;
}

/**
 * This function gets the state of the turbo 
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) TurboState()
{
	int turboState = TC110->GetTurboPumpState();
	return turboState;
}

/**
 * This function retrieves the turbo speed
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) TurboSpeed(int32_t type)
{
	if(type >= 1 && type <= 4){
		int turboSpeed = TC110->GetTurboSpeed((int) type);
		return turboSpeed;
	}

	return 999;
}

/**
 * This function retrieves error log
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" void __declspec(dllexport) Error(int32_t id, char* buffer)
{

	if(id >= 1 && id <= 10){
		std::string bufferContent = TC110->GetError((int) id);
		for(int i = 0; i < 6; ++i)
			buffer[i] = bufferContent[i];
	}

	char bufferContent[] = "Err999";
	for(int i = 0; i < 6; ++i)
		buffer[i] = bufferContent[i];
	
}

/**
 * This function retrieves the backing pump mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) BackingPumpMode()
{
	int backingPumpMode = TC110->GetBackingPumpMode();
	return backingPumpMode;
}

/**
 * This function retrieves the pumping state
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) PumpingState()
{
	int pumpingState = TC110->GetPumpingState();
	return pumpingState;
}


/**
 * This function sets the set gas mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) SetGasMode(int32_t mode)
{
	if(mode >= 0 && mode <= 2){
		if(TC110->SetGasMode((int) mode)){
			return 1;
		}
	}

	return 0;
}

/**
 * This function sets the backing pump mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) SetBackingPumpMode(int32_t mode)
{
	if(mode >= 0 && mode <= 3){
		if(TC110->SetBackingPumpMode((int) mode)){
			return 1;
		}
	}

	return 0;
}

/**
 * This function sets whether the turbo pump is enbaled or not
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) SetTurboPumpState(int32_t mode)
{
	if(mode >= 0 && mode <= 1){
		if(TC110->SetTurboPumpState((int) mode)){
			return 1;
		}
	}

	return 0;
}

/**
 * This function turns on the vac station on and off
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) SetPumping(int32_t onOff)
{
	if(onOff >= 0 && onOff <= 1){
		if(TC110->SetPumpingState((int) onOff)){
			return 1;
		}
	}

	return 0;
}

/**
 * This function sets the turbo speed
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
extern "C" int32_t __declspec(dllexport) SetTurboSpeed(int32_t speed)
{
	return 0;
}

