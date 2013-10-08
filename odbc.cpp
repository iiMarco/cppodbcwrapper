/*
  Name: odbc.h
  Copyright: Mark Zammit
  Author: Mark Zammit
  Date: 01/02/12
  Description: ODBC class wrapper for windows odbc.lib API
*/

// If not running a windows compiler you will need to
// add into your search directories the windows ODBC library paths:
// -	.\windows\system32
// -	.\Program Files\Microsoft SDKs\Windows\v7.0A\Include
// -	.\Program Files\Microsoft SDKs\Windows\v7.0A\Lib
// If you have Visual Studio installed then the above microsoft SDKs can be
// depreciated to the following:
// -	.\Program Files\Microsoft Visual Studio x.x\VC\include
// -	.\Program Files\Microsoft Visual Studio x.x\VC\lib
// -	.\Program Files\Microsoft Visual Studio x.x\VC\bin



#ifndef ODBC_CON_H
#define ODBC_CON_H

#define SQL_SUCCEEDED(rc) (((rc)&(~1))==0)
#define RMAP std::map<unsigned long,unordered_row>
#define DSNMAP std::map<TSTR,TSTR>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <windows.h>
#include <tchar.h>
#include <sql.h>
#include <sqltypes.h>
#include <sqlucode.h>
#include <sqlext.h>
#include <conio.h>
#include <comdef.h>
#include <mbstring.h>
#include "table.h"
#include <map>
#include <unordered_map>
#pragma comment( lib, "odbc32.lib" )
#pragma warning(disable: 4996)3


/** UNICODE SUPPORT **/
#if !defined(TSTR)
    #if defined(UNICODE) || defined(_UNICODE_)
        #define TSTR    std::wstring
    #else
        #define TSTR    std::string
    #endif
#endif

#if defined(UNICODE) || defined(_UNICODE_)
    #define FTPRINTF(x,y,...)    fwprintf(x,y,__VA_ARGS__)
#else
    #define FTPRINTF(x,y,...)    fprintf(x,y,__VA_ARGS__)
#endif

// [SQL DATA TYPES]
//---------------------------------------------------------------------------
// SQL ODBC DATA TYPE				| SQL DATA TYPE							|
//---------------------------------------------------------------------------
// SQL_CHAR							| CHAR(n)								|
// SQL_VARHCHAR						| VARCHAR(n)							|
// SQL_LONGVARCHAR					| LONG VARCHAR							|
// SQL_WCHAR						| WCHAR(n)								|
// SQL_WLONGVARCHAR					| LONGWVARCHAR							|
// SQL_DECIMAL						| DECIMAL(p,s)							|
// SQL_NUMERIC						| NUMERIC(p,s)							|
// SQL_SMALLINT						| SMALLINT								|
// SQL_INTEGER						| INTEGER								|
// SQL_REAL							| REAL									|
// SQL_FLOAT						| FLOAT(p)								|
// SQL_DOUBLE						| DOUBLE PRECISION						|
// SQL_BIT							| BIT									|
// SQL_TINYINT						| TINYINT								|
// SQL_BIGINT						| BIGINT								|
// SQL_BINARY						| BINARY(n)								|
// SQL_VARBINARY					| VARBINARY(n)							|
// SQL_LONGVARBINARY				| LONG VARBINARY						|
// SQL_TYPE_DATE					| DATE									|
// SQL_TYPE_TIME					| TIME(p)								|
// SQL_TYPE_TIMESTAMP				| TIMESTAMP(p)							|
// SQL_TYPE_UTCDATETIME				| UTCDATETIME							|
// SQL_TYPE_UTCTIME					| UTCTIME								|
// SQL_INTERVAL_MONTH				| INTERVAL MONTH(p)						|
// SQL_INTERVAL_YEAR				| INTERVAL YEAR(p)						|
// SQL_INTERVAL_YEAR_TO_MONTH		| INTERVAL YEAR(p) TO MONTH				|
// SQL_INTERVAL_DAY					| INTERVAL DAY(p)						|
// SQL_INTERVAL_HOUR				| INTERVAL HOUR(p)						|
// SQL_INTERVAL_MINUTE				| INTERVAL MINUTE(p)					|
// SQL_INTERVAL_SECOND				| INTERVAL SECOND(p,q)					|
// SQL_INTERVAL_DAY_TO_HOUR			| INTERVAL DAY(p) TO HOUR				|
// SQL_INTERVAL_DAY_TO_MINUTE		| INTERVAL DAY(p) TO MINUTE				|
// SQL_INTERVAL_DAY_TO_SECOND		| INTERVAL DAY(p) TO SECOND(q)			|
// SQL_INTERVAL_HOUR_TO_MINUTE		| INTERVAL HOUR(p) TO MINUTE			|
// SQL_INTERVAL_HOUR_TO_SECOND		| INTERVAL HOUR(p) TO SECOND(q)			|
// SQL_INTERVAL_MINUTE_TO_SECOND	| INTERVAL MINUTE(p) TO SECOND(q)		|
// SQL_GUID							| GUID									|
//---------------------------------------------------------------------------


// [SQL RETURN VALUES]
// SQL_SUCCESS = 0
// SQL_SUCCESS_WITH_INFO = 1
// SQL_ERROR = -1
// SQL_INVALID_HANDLE = -2
// SQL_NO_DATA = 99

class odbc;
struct param;

struct param
{
	SQLSMALLINT col;
	TSTR val;
	SQLSMALLINT type;
	SQLULEN size;
	SQLSMALLINT decimals;
};


class odbc
{
	public:
		// default constructor
		odbc();
		// initializes with a DSN
		odbc(TSTR dsn);
		// initializes with a DSN, username and password, only necessary
		// if ODBC connection requires login each time
		odbc(TSTR dsn, TSTR uid, TSTR pwd);
		// disconnects on destruct
		~odbc();

		// changes the DSN, this also calls free_link() which
		// which frees up all the handles and DSN specific data
		// to stop memory leaks or result double ups
		void set_connector(TSTR dsn);
		void set_connector(TSTR dsn, TSTR uid, TSTR pwd);

		// connects to the database
		bool connect();
		// disconnects from the database
		bool disconnect();

		// returns the current connection status
		bool connection_status();

		// returns the last return code for the last SQL operation
		SQLRETURN last_status();
		// returns a custom error message if any
		TSTR last_error();

		// returns odbc formatted connection string on successful connection
        TSTR connection_string();

		// prepares a SQL statement and then binds a list of parameters
		// requires the list to be param structs to build the binding
		bool prepare_and_bind(TSTR sql_stmt,std::vector<param> params);
		// prepares a SQL statement
		bool prepare(TSTR sql_stmt);
		// binds parameters to the prepared statement
		bool bind_param(short col, TSTR val, short sql_field_type, SQLULEN col_size ,short decimal_pts);

		// clears the last prepared statement without closing the connection
		void free_session();

		// clears the memory allocated to the statement handle to allow
		// multiple statements to be executed during the single connection
		void free_statement();

		// executes a prepared statement
		bool execute();
		// executes a non-bindable statement
		bool execute_direct(TSTR sql_stmt);

		// fetches a unordered_row at a time
		bool fetch(unordered_row &r);

		// fetches a unordered_row at a time by reference
		// this allows unordered_row data to have settings changed
		bool fetch(unordered_row *&r);

		// fetches a specific unordered_row from result set
		unordered_row fetch_row(unsigned long row_id);

        // fetches each row directly from the database
        // slower but will handle very large data set sizes since
        // it doesnt load the data into memory first and eliminates memory errors
		bool fetch_direct(unordered_row &r);

		// fetches each DSN & DSN Description from the DSN table
		// initialized on ODBC init, will return blank if ODBC failed to connect
		bool fetch_dsn(TSTR &dsn, TSTR &dsn_desc);

		// returns a field name of a particular column
		TSTR get_field_name(unsigned long col);

        // moves the result set cursor to a specific result set
        // once the result sets have been passed they will be lost!
		bool move_to_result_set(unsigned long set_pos);

		// returns the number of fields in the result set
		unsigned long fields();
		// returns the number of rows in the result set
		unsigned long rows();

		// returns the affected rows from statements like INSERT/DELETE/UPDATE
		unsigned long affected_rows();

	private:
		// err/info value
        TSTR _err;

		// stores field data for active table
        struct field_description
        {
            SQLSMALLINT colNumber;
            SQLTCHAR colName[80];
            SQLSMALLINT nameLen;
            SQLSMALLINT dataType;
            SQLULEN colSize;
            SQLSMALLINT decimalDigits;
            SQLSMALLINT nullable;
        };

		// ODBC handlers
		// Environment handler
		// must be initialized before connection
        SQLHANDLE _henv;
		// Connection handler necessary before statement handler
        SQLHANDLE _hdbc;
		// Statement handler wipes when connection handler
		// disconnects, needs to be re-established each time
        SQLHANDLE _hstmt;

		// connection string for driver access
        TSTR _dsn;
		// user id for DB
        TSTR _uid;
		// user password for DB
        TSTR _pwd;

		TSTR _sql_stmt;

		unsigned long _fields;
        unsigned long _rows;
		unsigned long _affected_rows;
		unsigned long _row_ptr;

		// return code from ODBC based on last operation
        SQLRETURN _rc;

		// current connection status
        bool _connected;
		bool _init;
		bool _bound;
		bool _built;
		bool _executed;
		bool _fetching;

		// stores specific field info
        std::vector<field_description> field_info;
		// stores vector of field names
        std::vector<TSTR> field_names;
        RMAP _table;
        RMAP::iterator _itr;
        RMAP::iterator __itr;

        // holds DSN list
        DSNMAP _dsntable;
        DSNMAP::iterator _dsn_itr;

		//std::tr1::unordered_map<unsigned int, row> _table;
		//std::tr1::unordered_map<unsigned int, row>::iterator _itr;

		// initializes handlers
        void init();
        // sets up the DSN listing from connected ODBC
        void set_dsn_list();
		// sets up the field_description vector
        void set_field_descriptors();
		// returns a field_descriptor containing field data
		// queried from the DB
        SQLRETURN Describe(field_description& c);
		void error_out();
		void build_result_set();
		void extract_error(TCHAR *fn,SQLHANDLE handle,SQLSMALLINT type);
		void free_link();
		void reset_iterator();
};


#endif
