#ifndef TYPES_H
#define TYPES_H

	#include <string>
	#include <vector>
	#include "wininclude.h"

	enum Multivar_type{VT_NUMBER, VT_STRING, VT_NIL};
	enum BatchJob_type{MEM_BYTE, MEM_UBYTE, MEM_SHORT, MEM_USHORT,
			MEM_INT, MEM_UINT, MEM_FLOAT, MEM_DOUBLE, MEM_STRING, MEM_SKIP};

	class CMultivar;
	typedef CMultivar Multivar;
	typedef unsigned int ALuint;

	class CMultivar
	{
		protected:
			Multivar_type type;
			double fValue;
			std::string szValue;

		public:
			CMultivar() : type(VT_NIL), fValue(0.0), szValue("") { }

			void setNumber(double);
			void setString(std::string);
			void setNil();

			double getNumber();
			std::string getString();
			Multivar_type getType();
	};

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

	/* Describes a job (type and length) */
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
#endif
