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
	// So this is sort of complicated because we have to reverse the bits not the byte array
	// This means if the bits don't fall on a byte boundary it can get hairy
	int length = bits / 8 + bits % 8;
	int shift = bits % 8;

	// Handle Simple Case
	if (data == NULL)
		return data;
	else if (length == 1)
	{
		ReverseByte(data[0]);
		data[0] = data[0] >> shift;
		return data;
	}

	// Handle Complex Case
	// NOTE: There is a complex way to make this exactly n element operations, but for easiness we'll do two passes, 
	// one to reverse the array, then one to repack the bits if necessary
	// This will make it worst case something like 3/2n
	unsigned short swapBuffer = 0;
	for (int i = 0, far = length-i-1; i <= length/2; i++, far = length-i-1)
	{
		// Reverse Both Bytes
		ReverseByte(data[i]);
		ReverseByte(data[far]);
		
		// Swap them (borrowing the swapBuffer)
		swapBuffer = data[far];
		data[far] = data[i];
		data[i] = (BYTE)swapBuffer;
	}
	if (length%2 == 1)						// if it is odd, we need to reverse the bits of the middle byte
		ReverseByte(data[length/2+1]);		// even though it doesn't change position

	// Repack the bits if necessary
	// If the bits don't end on a word boundary we have to repack them together after reversing them
	if (shift == 0)							// if we landed on a word boundary, no repacking necessary
		return data;
	swapBuffer = data[0] >> shift;			// grab the first byte and shift it back into its place
	for (int i = 1; i < length; i++)
	{
		swapBuffer |= data[i] << (8-shift);	// or in the new byte shifted to account of the misalignment
		data[i-1] = swapBuffer & 0xFF;		// set the last 8 bits as the previous byte
		swapBuffer = swapBuffer >> 8;		// shift the swapBufer 8 bits so it can continue this trend
	}
	data[length-1] = swapBuffer & 0xFF;

	return data;
}
BYTE& BISTio::ReverseByte(BYTE &data)
{
	data = (data & 0x0F) << 4 | (data & 0xF0) >> 4;
	data = (data & 0x33) << 2 | (data & 0xCC) >> 2;
	data = (data & 0x55) << 1 | (data & 0xAA) >> 1;
	return data;
}