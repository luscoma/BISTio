/*
	BISTio Header File

	This file defines the interface into the BISTio Library
	Exports/Imports the classes and functions to interface with the BIST
*/

// Export Macro
// Ensure BISTIO_EXPORTS is not defined when using this in an application
#ifdef BISTIO_EXPORTS
	#define BISTIO_API __declspec(dllexport)
#else
	#define BISTIO_API __declspec(dllimport)
#endif

/* Typedefs That Are Required */
typedef unsigned short WORD;	// required in dpcdefs.h and we're not including windows.h by default
typedef unsigned long HIF;		// used so we don't have to make the client include dpcdecl.h
typedef unsigned char BYTE;		// another -_-

// Includes
#include "dpcdefs.h"			// defines several error message

/* Enumeration Declarations */
// In a second -_-

/**
	This class is an abstract class which defines a basic interface for other classes to implement 
	It wraps a few of the basic functions of the Adept SDK
*/
class BISTIO_API BISTio
{
private:
	/*
		The handle to the open device if any
	*/
	HIF 				_hif;
public:
	/**
		Creates a new BISTio object
	*/
	BISTio();
	/**
		Sets up the USB for the first use or
		Reconfigures it for talking with the programmer

		@returns false if mode is not set to USB
	*/
	bool ConfigureUSB();
	/**
		Disconnects from the USB device if connected
		Ignored in parallel mode

		@returns false if already disconnected
	*/
	bool Disconnect();
	/**
		Connects to the USB device
		Ignored in parallel mode

		@returns false if unable to connect (will return true if already connected)
	*/
	bool Connect();
	/**
		Checks to see if this cable is already connected

		@returns true if connected
	*/
	bool IsConnected();
	/**
		Terminates any DPC call open
	*/
	void Shutdown();
	/**
		Returns the name of the connected device

		@returns The current name
	*/
	char *GetDeviceName();

	/**
		Set the pins of the JTAG cable to a value

		@param tms	TMS pin
		@param tdi	TDI pin
		@param tck	TCK pin

		@returns true if successful
	*/
	bool SetPins(bool tms, bool tdi, bool tck);
	/**
		Returns the current values of tms, tdi, tdo, tck.
		The result is packed into the 4 LSBs of a BYTE. 

		@returns tms<<3|tdi<<2|tdo<<1|tck or 0xFF on failure
	*/
	BYTE GetPins();

	/**
		Clocks TCK for a specified number of clock cycles

		@param cycles	Number of clock cycles to clock tck for
		@param tms		Value to set tms to during clocks
		@param tdi		Value to set tdi to during clocks

		@returns true if successful
	*/
	bool ClockTck(unsigned long cycles, bool tms, bool tdi);
	/**
		Sends a given number of tdi bits into the the circuit
		Starting at the lsb of tdi[0] and going for the specified number of bits.

		The number of bytes in the tdi array (and tdo array if not NULL) 
		should be ceil(bits / 8)

		@param tdi			Byte array with the bits to set to tdi
		@param bits			number of bits in tdi
		@param tms			value to set tms to during shift operation
		@param tdo			Byte array to store tdo values (NULL if unused)
	*/
	bool SendTDIBits(BYTE *tdi, int bits, bool tms, BYTE *tdo = 0);
	/**
		Sends a given number of tms bits into the the circuit
		Starting at the lsb of tms[0] and going for the specified number of bits.

		The number of bytes in the tms array (and tdo array if not NULL) 
		should be ceil(bits / 8)

		@param tms			Byte array with the bits to set to tms
		@param bits			number of bits in tdi
		@param tdi			value to set tdi to during shift operation
		@param tdo			Byte array to store tdo values (NULL if unused)
	*/
	bool SendTMSBits(BYTE *tms, int bits, bool tdi, BYTE *tdo = 0);
	/**
		Sets tdi and tms then retreives bits from tdo storing them in an array
		The bits retreived are stored in the lsb of tdo[0] first

		@param tdi			value of tdi
		@param tms			value of tms
		@param tdo			array to store tdo bits in 
		@param bits			number of bits to retreive

		@returns true if successful
	*/
	bool GetTDOBits(bool tdi, bool tms, BYTE *tdo, int bits);

	/**
		Returns the last error if any that occurred

		@returns Error Code
	*/
	static ERC GetLastError();
	/**
		Converts an ERC code into the symbolic name
		and description

		@param erc			Error Code
		@param szErrorName	Name of Error
		@param iNameLen		Length of szErrorName in characters
		@param szErrorDesc	Description of Error
		@param iDescLeng	Length of szErrorDesc in characters
	*/
	static void StringFromERC(ERC erc, char *szErrorName, int iNameLength, char *szErrorDesc, int iDescLen);
};

/**
	This class defines a boundary scan interface.  It is only guarenteed to work with Xilinx FPGAs
	though it is fairly generic and should work with other boundary scan interfaces with minor
	modifications
*/
class BISTIO_API BSio
{
private:
public:
};

/**
	This class defines a simple spi interface
	It uses tdi as the input to the system and reads back data via tdo
*/
class BISTIO_API SPIio
{
private:
public:
};


