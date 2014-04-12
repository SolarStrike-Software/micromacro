#ifndef TABLE_ADDON_H
#define TABLE_ADDON_H

	#define FLOAT_EPSILON			1.19209290e-07F 			// float
	#define DOUBLE_EPSILON 			2.2204460492503131e-16 		// double
	#define TABLE_PRINT_MAXDEPTH	10

	#define TABLE_MODULE_NAME		"table"
	typedef struct lua_State lua_State;

	class Table_addon
	{
		protected:
			static int copy(lua_State *);
			static int find(lua_State *);
			static int print(lua_State *);
			// TODO: table.save() and table.load()

		public:
			static int regmod(lua_State *);
	};
#endif
