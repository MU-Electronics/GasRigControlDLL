#include "TC110Communicator.h"
#include <string>
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

/**
 * PUBLIC: Open communication link between PC and TC110 turbo pump controller 
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param char $portName The name of the windows port
 */
TC110Communicator::TC110Communicator(const char *portName, int id)
{
	// Not connected yet
	this->connected = false;

	// Set turbo station id
	this->id = id;

	// format convert
	wchar_t portBuffer[20];
	std::mbstowcs(portBuffer, portName, strlen(portName)+1);//Plus null
	LPWSTR port = portBuffer;

	// Try to connect to the given port throuh CreateFile
	this->hSerial = CreateFile(port,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

	//Check if the connection was successfull
	if(this->hSerial==INVALID_HANDLE_VALUE)
	{
		//If not success full display an Error
		if(GetLastError()==ERROR_FILE_NOT_FOUND){
			//Print Error if neccessary
			printf("ERROR: Handle was not attached. Reason: %s not available.\n", portName);
		}else{
			printf("ERROR: Check error."); std::cout<<GetLastError()<<std::endl;
		}
	}
	else
	{
		// If connected we try to set the comm parameters
		DCB dcbSerialParams = {0};

		//Try to get the current
		if (!GetCommState(this->hSerial, &dcbSerialParams))
		{
			//If impossible, show an error
			printf("failed to get current serial parameters!");
		}
		else
		{
			//Define serial connection parameters for the arduino board
			dcbSerialParams.BaudRate=CBR_9600;
			dcbSerialParams.ByteSize=8;
			dcbSerialParams.StopBits=ONESTOPBIT;
			dcbSerialParams.Parity=NOPARITY;

			//Setting the DTR to Control_Enable ensures that the Arduino is properly
			//reset upon establishing a connection
			dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

			 //Set the parameters and check for their proper application
			 if(!SetCommState(hSerial, &dcbSerialParams))
			 {
				printf("ALERT: Could not set Serial Port parameters");
			 }
			 else
			 {
				//If everything went fine we're connected
				this->connected = true;

				//Flush any remaining characters in the buffers 
				PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

				//We wait 2s for resetting
				Sleep(2000);

				// Check connection with id
				std::string response = this->send("00", "349", "=?", 19);
				if(response == "false"){
					this->connected = false;
				}
			 }
		}
	}
}

/**
 * PUBLIC: Close the communication link between PC and TC110 turbo pump controller + class
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
TC110Communicator::~TC110Communicator(void)
{
	//Check if we are connected before trying to disconnect
	if(this->connected)
	{
		//We're no longer connected
		this->connected = false;

		//Close the serial handler
		CloseHandle(this->hSerial);
	}
}




/**
 * PRIVATE: Write a package to the TC110 turbo pump controller 
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param string package The package of data to be sent
 * @return bool
 */
bool TC110Communicator::WritePackage(std::string package)
{
	DWORD bytesSend;

	//Try to write the buffer on the Serial port
	if(!WriteFile(this->hSerial, package.c_str(), package.length(), &bytesSend, 0))
	{
		//In case it don't work get comm error and return false
		ClearCommError(this->hSerial, &this->errors, &this->status);

		return false;
	}else{
		return true;
	}
}

/**
 * PRIVATE: Read an imcomming package for the TC110 turbo pump controller
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param unsigned int btyeCount Byte count for incomming bytes
 * @return bool
 */
std::string TC110Communicator::ReadPackage(unsigned int byteCount)
{
	//Number of bytes we'll have read
	DWORD bytesRead;

	// Create buffer
	//char package[size] = "";
	std::vector<char> package;
	for (int i = 0; i < byteCount+1; ++i) {
		package.push_back(0);
	}
	const char *package_char = reinterpret_cast<char *>(package.data());

	//Number of bytes we'll really ask to read
	unsigned int toRead;

	//Use the ClearCommError function to get status info on the Serial port
	ClearCommError(this->hSerial, &this->errors, &this->status);

	//Check if there is something to read
	if(this->status.cbInQue>0)
	{
		//If there is we check if there is enough data to read the required number
		//of characters, if not we'll read only the available characters to prevent
		//locking of the application.
		if(this->status.cbInQue > byteCount)
		{
			toRead = byteCount;
		}
		else
		{
			toRead = this->status.cbInQue;
		}

		//Try to read the require number of chars, and return the number of read bytes on success
		if(ReadFile(this->hSerial, (LPVOID) package_char, toRead, &bytesRead, NULL) )
		{
			//return std::to_string(bytesRead);
			std::string buffer_string = std::string(package_char);
			return buffer_string;
		}else{ 	return "false to read"; }

	}

	//If nothing has been read, or that an error was detected return 0
	return "false";
}

/**
 * PRIVATE: Calcuate a check sum-8 for given data
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param string string The string to create the check sum for
 * @return string
 */
std::string TC110Communicator::CalculateCheckSum(std::string string)
{
	// Find check sum 8
	int sum = 0;
	for (unsigned int i = 0; i < string.size(); i++) {
		if(string[i] == ' '){
			sum += 32;
		}else{
			sum += string[i];
		}
	}
	int modulus_int = sum % 256;

	// Ensure 
	std::string modulus = std::to_string(modulus_int);
	if(modulus_int < 100){
		std::stringstream holder;
		holder << std::setw(3) << std::setfill('0') << modulus << std::endl;
		modulus = holder.str();
	}

	return modulus;
}


/**
 * PRIVATE: Check the check sum provided against the data provided
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param string package The string with check sum that needs validating
 * @return bool
 */
bool TC110Communicator::CheckSumValidation(std::string package)
{
	// Get check sum
	std::string checkSum = package.substr( package.length() - 4 );
	checkSum = checkSum.erase(checkSum.size()-1);

	// Get data that is for the check sum
	std::string data = package.erase(package.size()-4);

	// Calcuate check sum for data
	std::string newSum = this->CalculateCheckSum(data);

	// Check sums are the same
	if (checkSum.compare(newSum) != 0)
		return true;

	return false;
}

/**
 * PRIVATE: Create package for sending, the check sum is generated automatically
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param std::string action The action eg request, command etc (See Pfeffier Vacuum Protocol for "RS485" p.25 chap. 7.2 in TC110 OPerating Instructions)
 * @param std::string parameterValue The id for the information / action required
 * @param std::string data The control data to be sent
 * @return bool
 */
std::string TC110Communicator::CreatePackage(std::string action, std::string parameterValue, std::string data)
{
	// Format data legnth 2 bytes
	int size = data.length();
	std::string data_length = std::to_string(size);
	if(size < 10){
		data_length = "0" + std::to_string(size);
	}

	// Compile string
	std::string package = std::to_string(this->id) + action + parameterValue + data_length + data;
	
	// Append the check sum  and carriage return
	package = package + this->CalculateCheckSum(package) + "\r";

	// Return string
	return package;
}




/**
 * PUBLIC: Close the communication link between PC and TC110 turbo pump controller 
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @return bool
 */
bool TC110Communicator::Close()
{
	//Check if we are connected before trying to disconnect
	if(this->connected)
	{
		//We're no longer connected
		this->connected = false;

		//We wait 2s for resetting
		Sleep(200);

		//Close the serial handler
		if(this->hSerial != INVALID_HANDLE_VALUE) {
			CloseHandle(this->hSerial);
			this->hSerial = INVALID_HANDLE_VALUE;
		}

		//We wait 2s for resetting
		Sleep(200);
	}

	return this->connected;
}


/**
 * PUBLIC: Check if the PC and TC110 turbo pump controller are still connected
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @return bool
 */
bool TC110Communicator::IsConnected()
{
	return this->connected;
}

/**
 * PUBLIC: Creates a package and send it to the bus, then wait for a responce and validates the responce. 
 *
 * @param std::string action The action eg request, command etc (See Pfeffier Vacuum Protocol for "RS485" p.25 chap. 7.2 in TC110 OPerating Instructions)
 * @param std::string parameterValue The id for the information / action required
 * @param std::string data The control data to be sent
 * @param unsigned int bytesRead The number of bytes for the returned response
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @return string
 */
std::string TC110Communicator::send(std::string action, std::string parameterValue, std::string data, unsigned int bytesRead)
{
	// Create package to send
	std::string package = this->CreatePackage(action, parameterValue, data);

	// Write the package to the bus
	this->WritePackage(package);

	// Wait for buffer
	Sleep(125);

	// Read the responce from the sent package only accepting id of 961
	std::string read = this->ReadPackage(bytesRead);

	if(read != "" && read != "\r"){
		// Check for valid message via check sum
		if(this->CheckSumValidation(read))
			return read;
	}
	
	return "false";
}

/**
 * PUBLIC: Get temperature of selected location
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int location	Select the area for the temperature reading
 *							1 = Pump Bottom
 *							2 = Electronics
 *							3 = Bearing
 *							4 = Motor
 * @return double
 */
double TC110Communicator::GetTemperature(int location)
{
   // Get the correct param type
	std::string param = "342";
	switch(location)
	{
		case 1: // Pump Bottom
			param = "330"; break;
		case 2: // Electronics
			param = "326"; break;
		case 3: // Bearing
			param = "342"; break;
		case 4: // Motor
			param = "346"; break;
	}

	// Send request, receive it and check it's valid
	std::string response = this->send("00", param, "=?", 20);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		// Return as double to match Pfeiffer RS485 spec
		return atof(response.c_str());
	}

	return 0;
}

/**
 * PUBLIC: Get trubo speed
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int type	Select the type of speed reading
 *							1 = Accel-Decel
 *							2 = Actual Rotation Speed
 *							3 = Nominal Speed
 *							4 = Motor
 * @return double
 */
double TC110Communicator::GetTurboSpeed(int type)
{
	// Get the correct param type
	std::string param = "398";
	switch(type)
	{
		case 1: // Actual Rotation Speed could us 309 for Hz
			param = "398"; break;
		case 2: // Accel-Decel
			param = "336"; break;
		case 3: // Nominal Speed
			param = "399"; break;
		case 4: // Set rotation speed
			param = "397"; break;
	}

	// Send request, receive it and check it's valid
	std::string response = this->send("00", param, "=?", 20);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		// Return as double to match Pfeiffer RS485 spec
		return atof(response.c_str());
	}

	return 0;
}

/**
 * PUBLIC: Get error message histroy
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int id 1-10 error history list	
 * @return double
 */
std::string TC110Communicator::GetError(int id)
{
	// Get the correct param type
	std::string param = "360";
	switch(id)
	{
		case 1: 
			param = "360"; break;
		case 2: 
			param = "361"; break;
		case 3: 
			param = "362"; break;
		case 4: 
			param = "364"; break;
		case 5: 
			param = "364"; break;
		case 6: 
			param = "365"; break;
		case 7: 
			param = "366"; break;
		case 8: 
			param = "367"; break;
		case 9: 
			param = "368"; break;
		case 10: 
			param = "369"; break;
	}

	// Send request, receive it and check it's valid
	std::string response = this->send("00", param, "=?", 20);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		return response;
	}
 
	return "ERROR: Communication DLL failed!";
}


/**
 * PUBLIC: Get the gas mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @return int How heavy is the gas? 
 *				0 = Heavy (39 < X < 80)
 *				1 = Light(=<39)
 *				2 = Helium
 */
int TC110Communicator::GetGasMode()
{
	// Get the correct param type
	std::string param = "027";

	// Send request, receive it and check it's valid
	std::string response = this->send("00", param, "=?", 17);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 13 );

		// Return as int to match Pfeiffer RS485 spec
		if(response == "000"){
			return 0;
		}else if(response == "001"){
			return 1;
		}else if(response == "002"){
			return 2;
		}
	}

   return 999;
}

/**
 * PUBLIC: Get backing pump mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @return int How heavy is the gas? 
 *				0 = Continuous
 *				1 = Intermittent
 *				2 = Delayed
 *				3 = Delayed + Intermittent
 */
int TC110Communicator::GetBackingPumpMode()
{
   // Get the correct param type
	std::string param = "025";

	// Send request, receive it and check it's valid
	std::string response = this->send("00", param, "=?", 17);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 13 );

		// Return as int to match Pfeiffer RS485 spec
		if(response == "000"){
			return 0;
		}else if(response == "001"){
			return 1;
		}else if(response == "002"){
			return 2;
		}else if(response == "003"){
			return 3;
		}
	}

   return 999;
}

/**
 * PUBLIC: Get turbo pump state on or off
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int state Should the turbo pump turn on when station on or just use backing pump? 
 *					0 = off
 *					1 = on
 * @return bool
 */
int TC110Communicator::GetTurboPumpState()
{
	// Get the correct param type
	std::string param = "023";

	// Send request, receive it and check it's valid
	std::string response = this->send("00", param, "=?", 20);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		// Return as int to match Pfeiffer RS485 spec
		if(response == "000000"){
			return 0;
		}else if(response == "111111"){
			return 1;
		}
	}

   return 999;
}


/**
 * PUBLIC: Is the unit pumping?
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int state Is the unit pumping? 
 *					0 = off
 *					1 = on
 * @return bool
 */
int TC110Communicator::GetPumpingState()
{
   // Get the correct param type
	std::string param = "010";

	// Send request, receive it and check it's valid
	std::string response = this->send("00", param, "=?", 20);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		// Return as int to match Pfeiffer RS485 spec
		if(response == "000000"){
			return 0;
		}else if(response == "111111"){
			return 1;
		}
	}

   return 999;
}



/**
 * PUBLIC: Set the rotational speed of the turbo
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int speed	Set the speed 
 * @return bool
 */
bool TC110Communicator::SetTurboSpeed(int speed)
{
   return true;
}

/**
 * PUBLIC: Set the gas mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int mode	How heavy is the gas? 
 *						0 = Heavy (39 < X < 80)
 *                      1 = Light(=<39)
 *						2 = Helium
 * @return bool
 */
bool TC110Communicator::SetGasMode(int mode)
{
	// Get the correct param type
	std::string param = "027";
	std::string modeChange;
	if(mode == 0){
		modeChange = "000";
	}else if(mode == 1){
		modeChange = "001";
	}else if(mode == 2){
		modeChange = "002";
	}

	// Send request, receive it and check it's valid
	std::string response = this->send("10", param, modeChange, 17);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		// Return as int to match Pfeiffer RS485 spec
		if(response == modeChange){
			return true;
		}
	}

   return false;
}


/**
 * PUBLIC: Set backing pump mode
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int mode How heavy is the gas? 
 *					0 = Continuous
 *					1 = Intermittent
 *					2 = Delayed
 *					3 = Delayed + Intermittent
 * @return bool
 */
bool TC110Communicator::SetBackingPumpMode(int mode)
{
   // Get the correct param type
	std::string param = "025";
	std::string modeChange;
	if(mode == 0){
		modeChange = "000";
	}else if(mode == 1){
		modeChange = "001";
	}else if(mode == 2){
		modeChange = "002";
	}else if(mode == 3){
		modeChange = "003";
	}

	// Send request, receive it and check it's valid
	std::string response = this->send("10", param, modeChange, 17);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		// Return as int to match Pfeiffer RS485 spec
		if(response == modeChange){
			return true;
		}
	}

   return false;
}


/**
 * PUBLIC: Set turbo pump on or off
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int state Should the turbo pump turn on when station on or just use backing pump? 
 *					0 = off
 *					1 = on
 * @return bool
 */
bool TC110Communicator::SetTurboPumpState(int state)
{
	// Get the correct param type
	std::string param = "023";
	std::string modeChange;
	std::string returnedModeChange;
	if(state == 0){
		modeChange = "000";
		returnedModeChange = "000000";
	}else if(state == 1){
		modeChange = "111";
		returnedModeChange = "111111";
	}

	// Send request, receive it and check it's valid
	std::string response = this->send("10", param, modeChange, 20);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		// Return as int to match Pfeiffer RS485 spec
		if(response == returnedModeChange){
			return true;
		}
	}

	return false;
}


/**
 * PUBLIC: Set the unit pumping or off?
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 * @param int state Should the unit be pumping? 
 *					0 = off
 *					1 = on
 * @return bool
 */
bool TC110Communicator::SetPumpingState(int state)
{
   // Get the correct param type
	std::string param = "010";
	std::string modeChange;
	std::string returnedModeChange;
	if(state == 0){
		modeChange = "000";
		returnedModeChange = "000000";
	}else if(state == 1){
		modeChange = "111";
		returnedModeChange = "111111";
	}

	// Send request, receive it and check it's valid
	std::string response = this->send("10", param, modeChange, 20);

	if(response != "false"){
		// Take the data we want
		response = response.erase( response.length()-3 );
		response = response.substr( 11, 15 );

		// Return as int to match Pfeiffer RS485 spec
		if(response == returnedModeChange){
			return true;
		}
	}

	return false;
}

