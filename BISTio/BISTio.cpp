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
bool BISTio::ClockTck(unsigned long cycles, bool tms, bool tdi)
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
bool BISTio::GetTDOBits(bool tdi, bool tms, BYTE *tdo, int bits)
{
	if (DjtgGetTdoBits(this->_hif, tdi,tms,tdo,bits,FALSE) == 0)
		return false;
	return true;
}