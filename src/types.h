/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef TYPES_H
#define TYPES_H

	#include <string>
	#include <vector>
	#include "wininclude.h"

	enum Multivar_type{VT_NUMBER, VT_STRING, VT_NIL};
	enum BatchJob_type{MEM_BYTE, MEM_UBYTE, MEM_SHORT, MEM_USHORT,
			MEM_INT, MEM_UINT, MEM_FLOAT, MEM_DOUBLE, MEM_STRING, MEM_SKIP};

	//class CMultivar;
	//typedef CMultivar Multivar;
	typedef unsigned int ALuint;

	/* Used in window.find() */
	struct EnumWindowPair
	{
		HWND hwnd;
		char *windowname;
		char *classname;
	};

	/* Used in window.findList() */
	struct EnumWindowListPair
	{
		std::vector<HWND> hwndList;
		char *windowname;
		char *classname;
	};

	/* Used in some window operations */
	struct WindowDCPair
	{
		HWND hwnd;
		HDC hdc;
	};

	/* Describes a memory read job (type and length) */
	struct BatchJob
	{
		unsigned int count;
		BatchJob_type type;

		BatchJob &operator=(const BatchJob &);
	};

	/* Used to load and play sound */
	struct AudioResource
	{
		ALuint buffer;
		ALuint source;
	};

	/* Holds data read from a process */
	struct MemoryChunk
	{
		unsigned int address;
		unsigned int size;
		char *data;
	};

	template <class T>
	T getChunkVariable(MemoryChunk *pChunk, unsigned int offset, int &err)
	{
		err = 0;
		if( (offset+sizeof(T)) > pChunk->size )
		{
			err = -1;
			return (T)0;
		}

		return *(T*)(pChunk->data+offset);
	}

	std::string getChunkString(MemoryChunk *pChunk, unsigned int offset, unsigned int length, int &err);
#endif
