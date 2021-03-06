#ifndef PRESSURESENSOR_H
#define PRESSURESENSOR_H

//#pragma once

#include <Afxtempl.h>
#include <afxmt.h>

/** 
 * Cancel Code
 */
#define COMM_CANCEL					-16

/**
 * Error codes that always cause a feedback
 */
#define COMM_NOT_CREATEFILE			-15
#define COMM_NOT_SETCOMMSTATE		-14
#define COMM_NOT_SETCOMMTIMEOUTS	-13
#define	COMM_NOT_CLEARCOMMERROR		-12
#define COMM_NOT_PURGECOMM			-11
#define COMM_NOT_WRITEFILE			-10
#define COMM_NOT_NTX				-9
#define COMM_NOT_READFILE			-8

/**
 * Error-Codes
 */
#define COMM_NO_RESPONSE			-7
#define COMM_BAD_RESPONSE			-6
#define COMM_BAD_LENGTH				-5
#define COMM_BAD_CRC				-4
#define COMM_BAD_ADDR				-3
#define COMM_BAD_EXCEPTION			-2
#define COMM_RX_ERROR				-1

/**
 * OK-Code
 */
#define COMM_OK						0

/**
 * Exception Codes
 */
#define COMM_EXCEPTION_1			1
#define COMM_EXCEPTION_2			2
#define COMM_EXCEPTION_3			3
#define COMM_EXCEPTION_32			32

/**
 * Definitions
 */
#define	COMM_TX_MAX		20		// Length of send-buffer
#define	COMM_RX_MAX		260		// Length of receive-buffer= TX_MAX+ RX_MAX

#define	MAX_CHANNELS	32
#define	MAX_UNITS		15

#define _u8 unsigned char
#define	_u16 unsigned short
#define _u32 unsigned long

union FloatByte
{
	float	f;
	_u8		b[4];
};

class PressureSensor
{
	private:

		HANDLE	m_hPort;			// Handle of port
		BOOL	m_bRepeatIfError;

		// Serielle Schnittstelle		
		_u8		m_TxBuffer[COMM_TX_MAX];				// Send-buffer
		_u8		m_RxBuffer[COMM_TX_MAX+ COMM_RX_MAX];	// Receive-buffer

		_u16 CalcCrc16(_u8* Data, _u16 nCnt);			// CRC16-calculation
		int TransferDataDirect(_u16 nTX, _u16 nRX);		// send and receive
		int TransferData(_u16 nTX, _u16 nRX);			

	public:

		// Result
		int		nResult;
		_u8		m_nDevice;			// Device address variable
		BOOL	_bEcho;

		// 
		CEvent	m_eventCancel;		// Object to cancel the communication

		// KELLER busfunctions		
		BOOL OpenCommPort(LPTSTR lpszPort, _u32 baud);	 												
		int F48(_u8* Class, _u8* Group, _u8* Year, _u8* Week, _u8* Buffer, _u8* State);
		int F30(_u8 Koeff, float* fValue);
		int F31(_u8 Koeff, float fValue);
		int F66(_u8 Addr, _u8* actAddr);
		int F69(_u32* SN); 
		int F73(_u8 Channel, float* fValue);
		int F95(_u8 Command);
		int F95_Val(_u8 Command, float fValue);
		int F100( _u8 Index, _u8* Val1, _u8* Val2,
				  _u8* Val3, _u8* Val4, _u8* Val5);
		void CloseCommPort();// Close port

		void InitCommunication();
};

#endif