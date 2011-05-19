#define WIN32_LEAN_AND_MEAN

// Main Include
#include "BISTio.h"

// Other Includes
#include <Windows.h>
#include "dpcdecl.h"

bool BSio::Connect(enum BSFPGA family)
{
	if (BISTio::Connect())
		return false;

	_family = family;
	GotoBSState(TEST_LOGIC_RESET);
	return true;
}
bool BSio::GotoBSState(enum BSState bstate)
{
	if (bstate == TEST_LOGIC_RESET)	// the following ensures me make it to the reset state, even if were already in it
	{								// this ensures a first call to reset is executed so the machine is put in a known state
		BYTE tms = 0xFF;
		this->SendTMSBits(&tms,7,0);	// we set tms to a 1 and clock 7 times
		
		this->_state = bstate;
		return true;
	}
	else if (_state == bstate)		// otherwise if were in the correct state, were done
		return true;
	
	// At this point we will map how to move to the next state
	// and recursively call this function until we arrive at our destination
	// essentially dumbing down this function to a move to the next state until we get to
	// where we need to be
	switch (_state)
	{
	case TEST_LOGIC_RESET:	// we must go to the run idle
		if (!this->ClockTck(1,false,false))	// clock once with tms = 0 and tdi = 0
			return false;
		_state = RUN_IDLE;
		break;
	case RUN_IDLE:			// We must goto DR_SCAN
		if (!this->ClockTck(1,false,true))  // clock once with tms = 1 and tdi = 0
			return false;
		_state = DR_SELECT;
		break;
	case DR_SELECT:
		{		
			switch (bstate)
			{
				// Any of the IR States
				case IR_SELECT:
				case IR_CAPTURE:
				case IR_SHIFT:
				case IR_EXIT1:
				case IR_EXIT2:
				case IR_PAUSE:
				case IR_UPDATE:
					if (!this->ClockTck(1,false,true))	// clock once with tms = 1 to move to the IR side of the TAP controller
						return false;
					break;
				// Any Other State
				default:
					if (!this->ClockTck(1,false,false))	// clock once with tms = 0 to move to the DR side of the TAP controller
						return false;
					break;
			}
		}
		break;
	case DR_CAPTURE:
		if (bstate != DR_SHIFT)
		{
			if (!this->ClockTck(1,false,true))	// clock once with tms = 1 to move to exit1 state
				return false;
			_state = DR_EXIT1;
		}
		else
		{
			if (!this->ClockTck(1,false,false))	// clock once with tms = 0 to move into shift state
			_state = DR_SHIFT;
		}
		break;
	case DR_SHIFT:
		if (!this->ClockTck(1,false,true))		// set tms = 1 to move out of shift state. NOTE: this will shift a 0 into the shift register upon exit!
			return false;
		_state = DR_EXIT1;
		break;					// there is no real easy way around it, thus its not suggested to call this to move out of the shift register
	case DR_EXIT1:
		switch (bstate)
		{
		case DR_PAUSE:
		case DR_EXIT2:
			if (!this->ClockTck(1,false,false))	// move to pause state
				return false;
			_state = DR_PAUSE;
			break;
		default:
			if (!this->ClockTck(1,false,true))	// set tms = 1 and clock to move into Update state
				return false;
			_state = DR_UPDATE;
			break;
		}
		break;
	case DR_PAUSE:
		if (!this->ClockTck(1,false,true))		// set tms = 1 to move into the EXIT2 state
			return false;
		_state = DR_EXIT2;
		break;
	case DR_EXIT2:
		switch (bstate)
		{
		case DR_SHIFT:
		case DR_EXIT1:
		case DR_PAUSE:
			if (!this->ClockTck(1,false,false))	// return to shift state so we can get into shift exit or pause (note, if you went to exit, it will shif tan additional 0 into the shift register!)
				return false;
			_state = DR_SHIFT;
			break;
		default:
			if (!this->ClockTck(1,false,true))	 // clock with tck=1 so we can move to UPDATE
				return false;
			_state = DR_UPDATE;
			break;
		}
		break;
	case DR_UPDATE:
		if (bstate == RUN_IDLE)
		{
			if (!this->ClockTck(1,false,false))	// set tms=0 and move to run_idle
				return false;
			_state = RUN_IDLE;
		}
		else
		{
			if (!this->ClockTck(1,false,true))	// set tms=1 and move to DR_SELECT
				return false;
			_state = DR_SELECT;
		}
		break;
	case IR_SELECT:
		if (bstate == TEST_LOGIC_RESET)
		{
			if (!this->ClockTck(1,false,true))		// move into reset mode with tms = 1
				return false;
			_state = TEST_LOGIC_RESET;
		}
		else
		{
			if (!this->ClockTck(1,false,false))		// move into capture with tms = 0
				return false;
			_state = IR_CAPTURE;
		}
		break;
	case IR_CAPTURE:
		if (bstate == IR_SHIFT)
		{
			if (!this->ClockTck(1,false,false))	// move into shift with tms = 0;
				return false;
			_state = IR_SHIFT;
		}
		else
		{
			if (!this->ClockTck(1,false,true))	// just passing through, set tms =1 and bypass shift mode
				return false;
			_state = IR_EXIT1;
		}
		break;
	case IR_SHIFT:
		if (!this->ClockTck(1,false,true))		// this will shift an extra 0 into the IR register so be careful
			return false;
		_state = IR_EXIT1;
		break;					
	case IR_EXIT1:
		switch (bstate)
		{
		case IR_PAUSE:
		case IR_EXIT2:
			if (!this->ClockTck(1,false,true))	// set tms=1 and move to pause
				return false;
			_state = IR_PAUSE;
			break;
		default:
			if (!this->ClockTck(1,false,false))	// set tms=0 and move to update
				return false;
			_state = IR_UPDATE;
			break;
		}
		break;
	case IR_PAUSE:
		if (!this->ClockTck(1,false,true))		// set tms=1 and move to exit2
			return false;
		_state = IR_EXIT2;
		break;
	case IR_EXIT2:
		switch (bstate)
		{
		case IR_SHIFT:
		case IR_EXIT1:
		case IR_PAUSE:
			if (!this->ClockTck(1,false,false))		// set tms = 0 and move back into shift, will add a 0 to the shift register when you leave it
				return false;
			_state = IR_SHIFT;
			break;
		default:
			if (!this->ClockTck(1,false,true))		// set tms = 1 and move to update
				return false;
			_state = IR_UPDATE;
			break;
		}
		break;
	case IR_UPDATE:
		if (bstate == RUN_IDLE)
		{
			if (!this->ClockTck(1,false,false))		// set tms = 0 and move to idle
				return false;
			_state = RUN_IDLE;
		}
		else
		{
			if (!this->ClockTck(1,false,true))		// set tms = 1 and move to dr select
				return false;
			_state = DR_SELECT;
		}
		break;
	}
	return GotoBSState(bstate); // will call until the _state == bstate
}
enum BSState BSio::GetBSState() { return _state; }
bool BSio::SetBSInstruction(enum BSInstruction instruction)
{
	GotoBSState(IR_SHIFT);			// goto IR_SHIFT

	// Bypass the Flash Device (write 8 1's)
	for (int i = 0; i < 8; i++)
		this->ClockTck(8,1,0);

	// Send the VERTEX header if required
	// NOTE: the instruction gets reversed so we send in the MSBs first
	if (this->_family == VERTEX)
		if (!this->ClockTck(4,1,0))
			return false;

	// Send the remaining bits MSB of instruction first (bit 5 is the MSB)
	for (int mask = 32; mask > 1; mask = mask >> 2)
		if (!this->ClockTck(1,(instruction & mask) == mask,0))
			return false;
	if (!this->ClockTck(1,(instruction & 1) == 1, true))		// TMS has to be set to 1 so we finish our shift operation
		return false;

	_state = IR_EXIT1;
	return true;
}
enum BSDevice BSio::GetDevice()
{
	// Set the ID Code Instruction
	GotoBSState(RUN_IDLE);	
	SetBSInstruction(IDCODE);

	// Goto shift and shift out the ID code
	GotoBSState(DR_SHIFT);

	// Retreive the ID bits
	BYTE bID[4];
	this->GetTDOBits(0,0,bID,32);
	
	// Get out of shift mode (we don't care about shifting in an extra 0 since we're using the IDCODE instruction anyway)
	GotoBSState(RUN_IDLE);

	// Reverse the data sent to us
	unsigned long ID=0;
	for (int i = 0; i < 4; i++)
		ID |= this->ReverseByte(bID[i]) << (3-i)*8;
	
	switch (ID)
	{
	case dvcS1000:
		return dvcS1000;
	case dvcS200:
		return dvcS200;
	default:
		return dvcUnknown;
	}
}
bool BSio::ShiftBit(bool tdi, bool *tdo, bool last)
{
	BYTE out = 0;
	if (!this->GetTDOBits(tdi,last,&out,1))
		return false;
	*tdo = out != 0;

	if (last)
		_state = DR_EXIT1;
	return true;
}
bool BSio::ShiftBinary(char *tdi, BYTE *tdo, bool last)
{
	// Calculate required byte array length
	int bits = strlen(tdi);
	int length = bits/8;
	if (bits%8 > 0)
		length++;

	// Create array and zero it out
	BYTE *send = new BYTE[length];
	memset(send,0,sizeof(BYTE)*length);

	// Loop through binary string
	for (int i = 0, b = 0; i < bits; i++, b = i/8)
	{
		if (tdi[i] != '0')
			send[b] |= 1 << i%8;
	}

	// Send and Retreive Results
	bool result = this->SendTDIBits(send, last ? bits-1 : bits,false,tdo);
	delete [] send;
	if (!result)
		return false;

	// If last shift handle the last bit when we shift out
	if (last)
	{
		bool out = false;
		if (!this->ShiftBit(tdi[bits-1] != '0',&out,true))
			return false;
		else if (tdo != NULL && out)		// if TDO isn't null and the bs interface returned a 1, lets set that in the tdo array
			tdo[length-1] |= 1 << bits%8;
	}

	return true;
}
bool BSio::ShiftByte(BYTE tdi, BYTE *tdo, bool last)
{
	if (!this->SendTDIBits(&tdi,last ? 7 : 8,false,tdo))
		return false;

	if (last)
	{
		bool output = false;
		if (!ShiftBit((tdi & 0x80) == 0x80, &output, true))
			return false;
		else if (tdo != NULL && output)
			tdo[0] |= 0x80;
	}
	return true;
}
bool BSio::ShiftBytes(BYTE *tdi, int length, BYTE *tdo, bool last)
{
	int bits = length*8;
	if (!this->SendTDIBits(tdi,last ? bits -1 : bits, false, tdo))
		return false;

	if (last)
	{
		bool output = false;
		if (!ShiftBit((tdi[length-1] & 0x80) == 0x80, &output, true))
			return false;
		else if (tdo != NULL && output)
			tdo[length-1] |= 0x80;
	}
	return true;
}
bool BSio::ShiftUShort(unsigned short tdi, BYTE *tdo, bool last)
{
	BYTE send[2];
	send[0] = tdi & 0x00FF;
	send[1] = (tdi & 0xFF00) >> 8;
	if (!this->SendTDIBits(send,last ? 15 : 16,false,tdo))
		return false;

	if (last)
	{
		bool output = false;
		if (!ShiftBit((tdi &0x8000) == 0x8000, &output, true))
			return false;
		else if (tdo != NULL && output)
			tdo[1] |= 0x80;
	}
	return true;
}
bool BSio::ShiftShort(short tdi, BYTE *tdo, bool last)
{
	BYTE send[2];
	send[0] = tdi & 0x00FF;
	send[1] = (tdi & 0xFF00) >> 8;
	if (!this->SendTDIBits(send,last ? 15 : 16,false,tdo))
		return false;

	if (last)
	{
		bool output = false;
		if (!ShiftBit((tdi &0x8000) == 0x8000, &output, true))
			return false;
		else if (tdo != NULL && output)
			tdo[1] |= 0x80;
	}
	return true;
}
bool BSio::ShiftUInt(unsigned int tdi, BYTE *tdo, bool last)
{
	BYTE send[4];
	send[0] = tdi & 0x000000FF;
	send[1] = (tdi & 0x0000FF00) >> 8;
	send[2] = (tdi & 0x00FF0000) >> 16;
	send[3] = (tdi & 0xFF000000) >> 24;
	if (!this->SendTDIBits(send,last ? 31 : 32,false,tdo))
		return false;

	if (last)
	{
		bool output = false;
		if (!ShiftBit((tdi &0x80000000) == 0x80000000, &output, true))
			return false;
		else if (tdo != NULL && output)
			tdo[3] |= 0x80;
	}
	return true;
}
bool BSio::ShiftInt(int tdi, BYTE *tdo, bool last)
{
	BYTE send[4];
	send[0] = tdi & 0x000000FF;
	send[1] = (tdi & 0x0000FF00) >> 8;
	send[2] = (tdi & 0x00FF0000) >> 16;
	send[3] = (tdi & 0xFF000000) >> 24;
	if (!this->SendTDIBits(send,last ? 31 : 32,false,tdo))
		return false;

	if (last)
	{
		bool output = false;
		if (!ShiftBit((tdi &0x80000000) == 0x80000000, &output, true))
			return false;
		else if (tdo != NULL && output)
			tdo[3] |= 0x80;
	}
	return true;
}