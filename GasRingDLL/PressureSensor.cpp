#include "PressureSensor.h"

/**
 * Initisise communication with the COM port
 */ 
void PressureSensor::InitCommunication()
{	
	// Set handle of open port to default value
	m_hPort=INVALID_HANDLE_VALUE;

	m_bRepeatIfError= false;
}

/**
 * Open and setup the COM port
 */ 
int PressureSensor::OpenCommPort(LPTSTR lpszPort, _u32 baud)
{

	DWORD dwBaud;

  if ((baud != 1200) && (baud != 2400) && (baud != 4800) &&
	 (baud != 9600) && (baud != 19200) && (baud != 38400) &&
	 (baud != 57600) && (baud != 115200))
	 return 0;	

	switch(baud) {
		case 1200:	dwBaud= CBR_1200;
					break;		
		case 2400:	dwBaud= CBR_2400;	
					break;
		case 4800:	dwBaud= CBR_4800;	
					break;
		case 9600:	dwBaud= CBR_9600;	
					break;
		case 19200:	dwBaud= CBR_19200;
					break;
		case 38400:	dwBaud= CBR_38400;
					break;
		case 57600:	dwBaud= CBR_57600;
					break;
		case 115200:	dwBaud= CBR_115200;
					break;
	}

	if(m_hPort!=INVALID_HANDLE_VALUE) {
		CloseHandle(m_hPort);
		m_hPort=INVALID_HANDLE_VALUE;
	}
	Sleep(200);

	// 1. Open port
	m_hPort= CreateFile(
		lpszPort,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		0,
		0
	);

	if(m_hPort==INVALID_HANDLE_VALUE) 
		return nResult= COMM_NOT_CREATEFILE;
	
	// 2. Settings
	_DCB DCB;

	DCB.DCBlength= sizeof(DCB);
	// Userdefined baudrate
	DCB.BaudRate= dwBaud;
	DCB.fBinary= TRUE;
	DCB.fParity= FALSE;
	DCB.fOutxCtsFlow= FALSE;
	DCB.fOutxDsrFlow= FALSE;
	DCB.fDtrControl= DTR_CONTROL_ENABLE;
	DCB.fDsrSensitivity= FALSE;
	DCB.fTXContinueOnXoff= FALSE; 
	DCB.fOutX= FALSE;
	DCB.fInX= FALSE;
	DCB.fErrorChar= FALSE;
	DCB.fNull= FALSE;
	DCB.fRtsControl= RTS_CONTROL_DISABLE;
	DCB.fAbortOnError= FALSE;
	DCB.XonLim= 0;
	DCB.XoffLim= 0;
	DCB.ByteSize= 8;
	DCB.Parity= NOPARITY;
	DCB.StopBits= ONESTOPBIT;
	DCB.XonChar= 0;
	DCB.XoffChar= 0;
	DCB.ErrorChar= 0;
	DCB.EofChar= 0;
	DCB.EvtChar= 0;

	if(!SetCommState(m_hPort,&DCB)) 
		return nResult= COMM_NOT_SETCOMMSTATE;

	// 3. Timeouts
	_COMMTIMEOUTS timeouts;

	timeouts.ReadIntervalTimeout= 400;
	timeouts.ReadTotalTimeoutMultiplier= 0;
	timeouts.ReadTotalTimeoutConstant= 500;
	timeouts.WriteTotalTimeoutMultiplier= 0;
	timeouts.WriteTotalTimeoutConstant= 200;

	if(!SetCommTimeouts(m_hPort,&timeouts)) 
		return nResult= COMM_NOT_SETCOMMTIMEOUTS;

	// 4. OK
	Sleep(500);

	return nResult=COMM_OK;
}

/**
 * Close the COM port
 */ 
void PressureSensor::CloseCommPort()
{
	Sleep(500);
	if(m_hPort!=INVALID_HANDLE_VALUE) {
		CloseHandle(m_hPort);
		m_hPort=INVALID_HANDLE_VALUE;
	}
}

/**
 * CRC16 calculation
 */ 
_u16 PressureSensor::CalcCrc16(_u8* Data,_u16 nCnt)
{
	BOOL	b;
	_u16	crc= 0xFFFF;

	for(int i=0;i<nCnt;i++)	{
		crc^= *Data++;
		for(int n=0;n<8;n++) {
			if(crc%2==1)
				b= TRUE;
			else
				b= FALSE;
			crc/= 2;
			if(b)
				crc^= 0xA001;
		}
	}
	return crc;
}

/**
 * Send and receive
 */ 
int PressureSensor::TransferData(_u16 nTX,_u16 nRX)
{
	BOOL	bRepeat;

	do {
		bRepeat= FALSE;

		if(::WaitForSingleObject(m_eventCancel,0) == WAIT_OBJECT_0) 
			nResult= COMM_CANCEL;
		else
			nResult= TransferDataDirect(nTX,nRX);

		if(::WaitForSingleObject(m_eventCancel,0) == WAIT_OBJECT_0) 
			nResult= COMM_CANCEL;
		
		switch(nResult) {
		case COMM_NO_RESPONSE:
		case COMM_BAD_RESPONSE:
		case COMM_BAD_LENGTH:
		case COMM_BAD_CRC:
		case COMM_BAD_ADDR: 
		case COMM_BAD_EXCEPTION:
		case COMM_RX_ERROR:
			if(m_bRepeatIfError) {
				Sleep(400);			
				bRepeat= TRUE;
			}
			break;
		}
	} while(bRepeat);

	return nResult;
}

/**
 * Transfer data
 */ 
int PressureSensor::TransferDataDirect(_u16 nTX,_u16 nRX)
{
	DWORD		dw;
	_u16		n,nCrc;

	// 1. Reset port
	if(!ClearCommError(m_hPort,&dw,NULL)) 
		return COMM_NOT_CLEARCOMMERROR;
	
	// 2. Flash buffer
	if(!PurgeComm(m_hPort,PURGE_TXCLEAR | PURGE_RXCLEAR)) 
		return COMM_NOT_PURGECOMM;

	// 3. Add CRC16-checksum
	nCrc= CalcCrc16(&m_TxBuffer[0],nTX);
	m_TxBuffer[nTX++]= HIBYTE(nCrc);
	m_TxBuffer[nTX++]= LOBYTE(nCrc);

	// 4. Send
	if(!WriteFile(m_hPort,m_TxBuffer,DWORD(nTX),&dw,NULL)) 
		return COMM_NOT_WRITEFILE;
	n= _u16(dw);

	if(n!=nTX) 
		return COMM_NOT_NTX;
																				
	if(_bEcho) {
		// with echo receiving
		if(!ReadFile(m_hPort,m_RxBuffer,DWORD(nTX+nRX),&dw,NULL)) 
			return COMM_NOT_READFILE;
		n= _u16(dw);	
	}
	else {
		// without receiving Echo
		if(!ReadFile(m_hPort,&m_RxBuffer[nTX],DWORD(nRX),&dw,NULL)) 
			return COMM_NOT_READFILE;
		n= _u16(dw+nTX);
	}
	
	// Echo received ?
	if(_bEcho) {
		if(n < nTX+nRX)
			return COMM_NO_RESPONSE;
	}
	else{
		if(n < nRX)
			return COMM_NO_RESPONSE;
	}

	// check echo
	if(_bEcho) {	
		for(int i=0;i<nTX;i++) 
			if(m_TxBuffer[i] != m_RxBuffer[i]) 
				return COMM_BAD_RESPONSE;
	}

	// 8. Check length of answer
	if(n!=nTX+nRX)	
		return COMM_BAD_LENGTH;

	// 9. Check CRC16
	nCrc= CalcCrc16(&m_RxBuffer[nTX],n-nTX-2);

	if((HIBYTE(nCrc)!=m_RxBuffer[n-2]) || (LOBYTE(nCrc)!=m_RxBuffer[n-1])) 
		return COMM_BAD_CRC;

	// 10. Check device address
	if(m_TxBuffer[0]!=m_RxBuffer[nTX])	
		return COMM_BAD_ADDR;

	// 11. Function ok?
	if(m_TxBuffer[1]==m_RxBuffer[nTX+1]) 
		return COMM_OK;

	// 12. Exception ?
	if(m_TxBuffer[1]==m_RxBuffer[nTX+1]+128) 
		switch(m_RxBuffer[nTX+2]) {
		case 1:
			return COMM_EXCEPTION_1;
		case 2:
			return COMM_EXCEPTION_2;
		case 3:
			return COMM_EXCEPTION_3;
		case 32:
			return COMM_EXCEPTION_32;
		default:
			return COMM_BAD_EXCEPTION;
		}

	// 13. General error
	return COMM_RX_ERROR;
}

 /**
  * Initialise the device
  */ 
int PressureSensor::F48(_u8* Class,_u8* Group,_u8* Year,_u8* Week,_u8* Buffer,_u8* State)
{
	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 48;
	
	if(TransferData(2,10) == COMM_OK) {
		*Class= m_RxBuffer[6];
		*Group= m_RxBuffer[7];
		*Year= m_RxBuffer[8];
		*Week= m_RxBuffer[9];
		*Buffer= m_RxBuffer[10];
		*State= m_RxBuffer[11];
	}

	return nResult;
}

/**
 * Read coefficients
 */
int PressureSensor::F30(_u8 Koeff, float* fValue)
{
	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 30;
	m_TxBuffer[2]= Koeff;
		
	if (TransferData(3,8) == COMM_OK)  {

		_u8	bteArr[4];

		bteArr[0] = m_RxBuffer[10];
		bteArr[1] = m_RxBuffer[9];
		bteArr[2] = m_RxBuffer[8];
		bteArr[3] = m_RxBuffer[7];

		*fValue= *(float*)(&bteArr[0]);
	}	
	return nResult;
}

/**
 * Write coefficients
 */
int PressureSensor::F31(_u8 Koeff, float fValue)
{
	FloatByte fb;

	fb.f= 0;
	for(int i=0;i<4;i++)
		fb.b[i]= 0;

	fb.f= fValue;

	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 31;
	m_TxBuffer[2]= Koeff;
	m_TxBuffer[3]= fb.b[3];
	m_TxBuffer[4]= fb.b[2];
	m_TxBuffer[5]= fb.b[1];
	m_TxBuffer[6]= fb.b[0];
		
	nResult = TransferData(7,5);
		
	return nResult;
}

/**
 * Change device-address
 */
int PressureSensor::F66(_u8 Addr, _u8* actAddr)
{

	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 66;
	m_TxBuffer[2]= Addr;
	

	if(TransferData(3,5) == COMM_OK) {
		*actAddr = m_RxBuffer[7];
	}	

	return nResult;
}

/**
 *Read the serial number
 */
int PressureSensor::F69(_u32* SN) 
{
	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 69;
	
	if(TransferData(2,8) == COMM_OK) {
		*SN= 256*65536*(_u32)m_RxBuffer[6]+65536*(_u32)m_RxBuffer[7]+256*(_u32)m_RxBuffer[8]+(_u32)m_RxBuffer[9];
	}

	return nResult;
}

/**
 * Read the  actual value of "Channel"
 */
int PressureSensor::F73(_u8 Channel, float* fValue)
{
	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 73;
	m_TxBuffer[2]= Channel;
	

	if (TransferData(3,9) == COMM_OK) {

		_u8	bteArr[4];

		bteArr[0] = m_RxBuffer[10];
		bteArr[1] = m_RxBuffer[9];
		bteArr[2] = m_RxBuffer[8];
		bteArr[3] = m_RxBuffer[7];
		
		*fValue= *(float*)(&bteArr[0]);
	}	
	return nResult;
}

/**
 * Set zero
 */
int PressureSensor::F95(_u8 Command)
{

	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 95;
	m_TxBuffer[2]= Command;
	
	TransferData(3,5);
	
	return nResult;
}

/**
 * Set to value
 */
int PressureSensor::F95_Val(_u8 Command, float fValue)
{

	FloatByte fb;

	fb.f= 0;
	for(int i=0;i<4;i++)
		fb.b[i]= 0;

	fb.f= fValue;

	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 95;
	m_TxBuffer[2]= Command;
	m_TxBuffer[3]= fb.b[3];
	m_TxBuffer[4]= fb.b[2];
	m_TxBuffer[5]= fb.b[1];
	m_TxBuffer[6]= fb.b[0];
	
	TransferData(7,5);
	
	return nResult;
}

/**
 * Read configuration
 */
int PressureSensor::F100( _u8 Index, _u8* Val1, _u8* Val2,
			  _u8* Val3, _u8* Val4, _u8* Val5)
{
	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 100;
	m_TxBuffer[2]= Index;

	if (TransferData(3,9) == COMM_OK) {

		*Val1 = m_RxBuffer[7];
		*Val2 = m_RxBuffer[8];
		*Val3 = m_RxBuffer[9];
		*Val4 = m_RxBuffer[10];
		*Val5 = m_RxBuffer[11];
	}	
	return nResult;
}
