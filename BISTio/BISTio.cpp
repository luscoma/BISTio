// BISTio.cpp : Defines the exported functions for the DLL application.
//
#define WIN32_LEAN_AND_MEAN

// Main Include
#include "BISTio.h"

// Other Includes
#include <Windows.h>
#include "dpcdecl.h"
#include "dmgr.h"
#include "djtg.h"

// BISTio Base Class
BISTio::BISTio() { this->_hif = hifInvalid; }
bool BISTio::ConfigureUSB()
{
	if (DmgrOpenDvcMg(NULL) == 0)
		return false;
	return true;
}
bool BISTio::Connect()
{
	// Check to see if we're already connected
	if (this->_hif != NULL)
		return false;

	// Retreive the first device in the table
	DVC dvc;
	memset(&dvc,0,sizeof(dvc));
	if (DmgrGetDvc(0,&dvc) == 0)			
		return false;	// no device exists

	// Open the device
	if (DmgrOpen(&this->_hif, dvc.szName) == 0)
		return false;

	// Enable JTAG for this device
	if (DjtgEnable(this->_hif) == 0)
		return false;

	return true;
}
bool BISTio::Disconnect()
{
	if (DjtgDisable(this->_hif) == 0)
		return false;
	if (DmgrClose(this->_hif) == 0)
		return false;
	this->_hif = hifInvalid;
	return true;
}
bool BISTio::SetPins(bool tdi, bool tms, bool tck) { return DjtgSetTmsTdiTck(this->_hif,tms,tdi,tck) != 0; }
BYTE BISTio::GetPins()
{
	BOOL tms = 0, tdi = 0, tdo = 0, tck = 0;
	if (DjtgGetTmsTdiTdoTck(this->_hif, &tms, &tdi, &tdo, &tck) == 0)
		return 0xFF;
	return tms << 3 | tdi << 2 | tdo << 1 | tck;
}
bool BISTio::ClockTck(unsigned long cycles, bool tdi, bool tms)
{
	if (DjtgClockTck(this->_hif, tms, tdi, cycles,FALSE) == 0)
		return false;
	return true;
}
bool BISTio::SendTDIBits(BYTE *tdi, int bits, bool tms, BYTE *tdo) 
{ 
	if (DjtgPutTdiBits(this->_hif, tms, tdi, tdo, bits, FALSE) == 0)
		return false;
	return true;
}
bool BISTio::SendTMSBits(BYTE *tms, int bits, bool tdi, BYTE *tdo)
{
	if (DjtgPutTmsBits(this->_hif, tdi,tms, tdo, bits, FALSE) == 0)
		return false;
	return true;
}
bool BISTio::SendTDITMSBits(BYTE *tdi, BYTE *tms, int bits, BYTE *tdo)
{
	// Calculate length and new array to pack
	int length = ((bits / 8) + 1) * 2;		// get the number of bytes and multiply the length by two since each word is 2 bits
	BYTE *words = new BYTE[length]; 
	for (int i = 0; i < bits; i++)
	{
		words[i/4] |= (tdi[i/8] << ((i%4)*2));
		words[i/4] |= (tdi[i/8] << ((i%4)*2+1));
	}

	if (DjtgPutTmsTdiBits(this->_hif, words, tdo, bits, FALSE) == 0)
		return false;
	return true;
}
bool BISTio::GetTDOBits(bool tdi, bool tms, BYTE *tdo, int bits)
{
	if (DjtgGetTdoBits(this->_hif, tdi,tms,tdo,bits,FALSE) == 0)
		return false;
	return true;
}
BYTE *BISTio::ReverseBits(BYTE *data, int bits)
{
	int length = bits/8+bits%8;
	if (bits % 8 == 0)			// simple case, we just need to switch the bytes since we're on a byte boundary
	{
		if (length == 1)
			ReverseByte(data[0]);
		else
		{
			for (int i = 0, length = bits/8; i < length; i++)
			{
				// Reverse Bits
				BYTE swap = data[length-i-1];			// store one of the bytes
				ReverseByte(swap);						// reverse the bits in this byte
				ReverseByte(data[i]);					// reverse the bits in the other byte

				// Swap bytes
				data[length-i-1] = data[i];
				data[i] = swap;
			}
		}
	}

	return data;
}
BYTE& BISTio::ReverseByte(BYTE &data)
{
	data = (data & 0x0F) << 4 | (data & 0xF0) >> 4;
	data = (data & 0x33) << 2 | (data & 0xCC) >> 2;
	data = (data & 0x55) << 1 | (data & 0xAA) >> 1;
	return data;
}