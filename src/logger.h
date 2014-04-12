#ifndef LOGGER_H
#define LOGGER_H

	#include <fstream>
	#include <string>
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>

	class CLogger;
	typedef CLogger Logger;

	class CLogger
	{
		private:
			static CLogger *pinstance;
			std::ofstream outfile;
			std::string openedFilename;

		protected:
			CLogger();
			~CLogger();
			CLogger(const CLogger &);
			CLogger &operator=(const CLogger &);

		public:
			static CLogger *instance();
			int open(const char *);
			void close();
			bool is_open();
			void add(const char *, ...);
			void add_raw(const char *);
			std::string get_filename();
	};

#endif
