#pragma once

#include <string>
#include <windows.h>


class TC110Communicator
{
private:
	// Serial comm handler
	HANDLE hSerial;

	// Connection status
	bool connected;

	// Get various information about the connection
	COMSTAT status;

	// Keep track of last error
	DWORD errors;

	// ID of pumping station
	int id;

	// Write a package to the TC110
	bool WritePackage(std::string package);
	
	// Read a package from TC110
	std::string ReadPackage(unsigned int byteCount);

	//Calculate the check sum for a package being sent
	std::string CalculateCheckSum(std::string string);

	// Check a check sum for data provided
	bool CheckSumValidation(std::string package);

	// Create the package
	std::string CreatePackage(std::string action, std::string parameterValue, std::string data);

public:
	// Start the communication
	TC110Communicator(char *portName, int id);

	// Close class and communication link
	~TC110Communicator(void);

	// Close the com port
	bool Close();

	// Is the PC still connected to TC110?
	bool IsConnected();

	// Send the package
	std::string send(std::string action, std::string parameterValue, std::string data, unsigned int bytesRead);

	// Get temperature 
	double GetTemperature(int location);

	// Get turbo speed
	double GetTurboSpeed(int type);

	// get error message
	std::string GetError(int id);

	// Get gas mode
	int GetGasMode();

	// Get backing pump mode
	int GetBackingPumpMode();

	// Get turbo pump mode
	int GetTurboPumpState();

	// Is unit pumping?
	int GetPumpingState();



	// Set turbo speed
	bool SetTurboSpeed(int speed);

	// Set gas mode
	bool SetGasMode(int mode);

	// Set backing pump mode
	bool SetBackingPumpMode(int mode);

	// Set turbo pump state
	bool SetTurboPumpState(int state);

	// Should the station be pumping?
	bool SetPumpingState(int state);

};

