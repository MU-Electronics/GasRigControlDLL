#include <Afxtempl.h>
#include <afxmt.h>
#include <extcode.h>
#include "PressureSensor.h"
#include "DigitalExtControl.h"
#include "TC110Communicator.h"
#include "string.h"
extern "C" { int _afxForceUSRDLL; } 

TC110Communicator* TC110;
PressureSensor* Pressure;

/**
 * This function adds two numbers together and is used for debugging to ensure the DLL is working (will be removed)
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) plus(double a, double b, double *out)
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
int32_t __declspec(dllexport) ReadFlowRate(int32_t COM, int32_t Baud, uint8_t IDH, uint8_t IDM, uint8_t IDL, uint8_t IDS, float *Flow, uint8_t *FlowUnit, uint8_t *S1, uint8_t *S2)
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
int32_t __declspec(dllexport) SetFlowRate(int32_t COM, int32_t Baud, uint8_t IDH, uint8_t IDM, uint8_t IDL, uint8_t IDS, uint8_t SetPointCode, float SetPoint, uint8_t *S1, uint8_t *S2)
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
int32_t __declspec(dllexport) OpenValve(int32_t id)
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
int32_t __declspec(dllexport) CloseValve(int32_t id)
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
int32_t __declspec(dllexport) ReadVacPressure()
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
int32_t __declspec(dllexport) PressureConnection(char *com, int32_t id)
{
	Pressure = new PressureSensor();

	// Init Communication
	Pressure->m_nDevice= 250;
	Pressure->InitCommunication();

	// Set the port
	CString S = "COM2"; LPTSTR lpsz = new TCHAR[S.GetLength()+1]; _tcscpy(lpsz, S);	
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
int32_t __declspec(dllexport) PressureConnectionClose()
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
double __declspec(dllexport) ReadPressure()
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
int32_t __declspec(dllexport) PumpConnection(char *com, int32_t id)
{
	TC110 = new TC110Communicator(com, (int) id); //    "\\\\.\\COM12"

	if(TC110->IsConnected())
		return 1;

	return 0;
}

/**
 * This function retrieves temperatures from the vac station
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) VacStationTemperature(int32_t type)
{
	return 0;
}

/**
 * This function retrieves currently set gas mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) GasMode()
{
	return 0;
}

/**
 * This function sets the state of the turbo 
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) TurboState()
{
	return 0;
}

/**
 * This function retrieves the turbo speed
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) TurboSpeed(int32_t type)
{
	return 0;
}

/**
 * This function retrieves error log
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
void __declspec(dllexport) Error(int32_t id, char* buffer)
{
	char bufferContent[] = "Err006";

	// Put into buffer
	for(int i = 0; i < 6; ++i)
		buffer[i] = bufferContent[i];
}

/**
 * This function retrieves the backing pump mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) BackingPumpMode(int32_t type)
{
	return 0;
}

/**
 * This function retrieves the pumping state
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) PumpingState()
{
	return 0;
}


/**
 * This function sets the set gas mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) SetGasMode(int32_t mode)
{
	return 0;
}

/**
 * This function sets the backing pump mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) SetBackingPumpMode(int32_t mode)
{
	return 0;
}

/**
 * This function sets whether the turbo pump is enbaled or not
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) SetTurboPumpState(int32_t mode)
{
	return 0;
}

/**
 * This function turns on the vac station on and off
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) SetPumping(int32_t onOff)
{
	return 0;
}

/**
 * This function sets the turbo speed
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) SetTurboSpeed(int32_t speed)
{
	return 0;
}

