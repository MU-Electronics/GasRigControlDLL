#include <Afxtempl.h>
#include <afxmt.h>
#include <extcode.h>
#include "PressureSensor.h"

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
	return 0;
}

/**
 * This function closes a valve via the labjack digital outout pin
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) CloseValve(int32_t id)
{
	return 0;
}

/**
 * This function turns on the turbo pump vacuum station 
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) TurboPumpOn()
{
	return 0;
}

/**
 * This function turns on the turbo pump vacuum station 
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
int32_t __declspec(dllexport) TurboPumpOff()
{
	return 0;
}

/**
 * This function returns a pressure reading from the keller pressure sensor
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
double __declspec(dllexport) ReadPressure()
{
	PressureSensor PressureSensor;
	
	// Init Communication
	PressureSensor.m_nDevice= 250;
	PressureSensor.InitCommunication();

	// Set the port
	CString S = "COM2"; LPTSTR lpsz = new TCHAR[S.GetLength()+1]; _tcscpy(lpsz, S);	
	_u32 baud = 9600;
	PressureSensor._bEcho= false;

	// Open Com Port
	PressureSensor.OpenCommPort(lpsz, baud);

	// Set default values
	_u8 nClass;
	_u8 nGroup;
	_u8 nYear; 
	_u8 nWeek;
	_u8 nBuffer;
	_u8 nState;
	_u32 nSN;

	// Wake up the device
	PressureSensor.F48(&nClass, &nGroup, &nYear, &nWeek, &nBuffer, &nState);
	Sleep(200);

	// Initialise the device
	int nRes= PressureSensor.F48(&nClass, &nGroup, &nYear, &nWeek, &nBuffer, &nState);

	// Check the answer
	if(nRes!= COMM_OK) {
		return 0;
	}

	// Read the acutal value
	float fVal;
	int channel = 1;
	int nRess= PressureSensor.F73(channel, &fVal);

	// Check the answer
	if(nRess != COMM_OK) {
		return 0;
	}

	// Close the com port
	PressureSensor.CloseCommPort();

	return fVal;
}