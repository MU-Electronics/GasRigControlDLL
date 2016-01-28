#include "DigitalExtControl.h"
#include <string>
#include <map>


/**
 * Runs on init of the class
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
DigitalExtControl::DigitalExtControl(void)
{
}

/**
 * Runs when class is destroyed
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
DigitalExtControl::~DigitalExtControl(void)
{
}







/**
 * Handles errors and converts then into an error number
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
long DigitalExtControl::ErrorHandler(LJ_ERROR lngErrorcode, long lngLineNumber, long lngIteration)
{
	char err[255];

	if (lngErrorcode != LJE_NOERROR)
	{
		ErrorToString(lngErrorcode,err);
		return lngErrorcode;
		// err = string    lngLineNumber    lngIteration
	}
	 return 0;
}


/**
 * Execute a cached set of commands
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
bool DigitalExtControl::Execute(void)
{
	while (true)
	{
		// Execute the requests
		lngErrorcode = GoOne (lngHandle);
		ErrorHandler(lngErrorcode, __LINE__, 0);

		// Set some vars
		long lngIOType=0, lngChannel=0;
		double dblValue=0; lngGetNextIteration = 0;

		//Get all the results
		lngErrorcode = GetFirstResult(lngHandle, &lngIOType, &lngChannel, &dblValue, 0, 0);
		ErrorHandler(lngErrorcode, __LINE__, 0);

		// Loop though results
		while(lngErrorcode < LJE_MIN_GROUP_ERROR)
		{
			// Set data into data array
			data[lngGetNextIteration] = dblValue;

			// Check if there is any more data to be read
			lngErrorcode = GetNextResult(lngHandle, &lngIOType, &lngChannel, &dblValue, 0, 0);
			if(lngErrorcode != LJE_NO_MORE_DATA_AVAILABLE)
			{
				ErrorHandler(lngErrorcode, __LINE__, lngGetNextIteration);
			}

			// Increments to loop iteration
			lngGetNextIteration++;
		}

		// Reset caches for the next set of requests
		lngErrorcode = 0; lngHandle = 0; lngGetNextIteration = 0;

		return true;
	}
	
}







/**
 * Open a connection to the slave hardware to be controlled
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
bool DigitalExtControl::OpenConnection(bool defaultPins)
{
	// Define vars
	lngHandle = 0;

	// Open the first found LabJack U3.
	lngErrorcode = OpenLabJack(LJ_dtU3, LJ_ctUSB, "1", 1, &lngHandle);
	if (ErrorHandler(lngErrorcode, __LINE__, 0) == 0)
	{
		// Reset pins back to there default
		if(defaultPins)
			this->ResetPins();

		return true;
	}

	return false;
}

/**
 * Closes a connection to the slave hardware
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
bool DigitalExtControl::CloseConnection()
{
	// Define vars
	data.clear();

	return false;
}


/**
 * Reset all the pins back to factory
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
bool DigitalExtControl::ResetPins()
{
	lngErrorcode = ePut(lngHandle, LJ_ioPIN_CONFIGURATION_RESET, 0, 0, 0);
	if(this->ErrorHandler(lngErrorcode, __LINE__, 0) == 0){
		return true;
	}

	return false;
}

/**
 * Set a digital output
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
double DigitalExtControl::DigitalOut(int pin, int logic)
{
	// Open a connection
	this->OpenConnection(false);

	// Set pin to digital
	// @todo

	// Set value of port
	lngErrorcode = AddRequest(lngHandle, LJ_ioPUT_DIGITAL_BIT, (long) pin, (double) logic, 0, 0);
	if(this->ErrorHandler(lngErrorcode, __LINE__, 0) == 0)
	{
		// Execute set commands
		if(this->Execute() == true){
			double toReturn = this->data[0];
			this->CloseConnection();
			return toReturn;
		}
			 	
	}
	
	return false;
}

/**
 * Read a digital input
 *
 * @author Sam Mottley <sam.mottley@manchester.ac.uk>
 */
double DigitalExtControl::DigitalRead(int pin)
{
	// Open a connection
	this->OpenConnection(false);

	// Set pin to digital input
	// @todo

	// Set value of port
	lngErrorcode = AddRequest (lngHandle, LJ_ioGET_DIGITAL_BIT, (long) pin, 0, 0, 0);
	if(this->ErrorHandler(lngErrorcode, __LINE__, 0) == 0)
	{
		// Execute set commands
		if(this->Execute() == true){
			double toReturn = this->data[0];
			this->CloseConnection();
			return toReturn;
		}	
	}
	
	return false;
}


