#include "headers\odbc.h"

/*****************
* PUBLIC METHODS *
******************/

odbc::odbc()
{
	_dsn.clear();
	_uid.clear();
	_pwd.clear();

	init();
}

odbc::odbc(TSTR dsn)
{
	_dsn = dsn;
	_uid.clear();
	_pwd.clear();

	init();
}

odbc::odbc(TSTR dsn, TSTR uid, TSTR pwd)
{
	_dsn = dsn;
	_uid = uid;
	_pwd = pwd;

	init();
}

//**
odbc::~odbc()
{
	disconnect();
}

void odbc::set_connector(TSTR dsn)
{
	free_link();

	_dsn = dsn;
	_uid.clear();
	_pwd.clear();

	init();
}

void odbc::set_connector(TSTR dsn, TSTR uid, TSTR pwd)
{
	free_link();

	_dsn = dsn;
	_uid = uid;
	_pwd = pwd;

	init();
}


// Allocate environment handle and connection handle, connect to data source, and allocate statement handle.
bool odbc::connect()
{
	try
	{
		if(_init && _henv && _hdbc)
		{
		    if(_uid!=TSTR())
		        _rc = SQLConnect(_hdbc, (SQLTCHAR*)_dsn.c_str(), SQL_NTS,
                                        (SQLTCHAR*)_uid.c_str(), SQL_NTS,
                                        (SQLTCHAR*)_pwd.c_str(), SQL_NTS);
		    else
		        _rc = SQLConnect(_hdbc, (SQLTCHAR*)_dsn.c_str(), SQL_NTS, NULL, 0, NULL, 0);

			if(!SQL_SUCCEEDED(_rc))
			{
				extract_error(_T("connect()"),_hdbc, SQL_HANDLE_DBC);
				SQLFreeHandle(SQL_HANDLE_ENV, _henv);
				SQLFreeHandle(SQL_HANDLE_DBC,_hdbc);
				return false;
			}

            _rc = SQLAllocStmt(_hdbc, &_hstmt);
			_connected = SQL_SUCCEEDED(_rc);
		}
		else
			_rc = SQL_ERROR;
	}
	catch(_com_error &e)
	{
		_err = _T("_com_error: ") + e.Error();
	}

	return _connected;
}

// Free the statement handle, disconnect, free the connection handle, and free the environment handle.
bool odbc::disconnect()
{

	try
	{
		if(_connected)
		{
			_rc = SQLFreeStmt(_hstmt, SQL_DROP);
			_rc = SQLDisconnect(_hdbc);
			_rc = SQLFreeHandle(SQL_HANDLE_DBC,_hdbc);
			_rc = SQLFreeHandle(SQL_HANDLE_ENV,_henv);

			_hstmt = NULL;
			_hdbc = NULL;
			_henv = NULL;

			_connected = false;

		}
		else
			_rc = SQL_ERROR;
	}
	catch(_com_error &e)
	{
		_err = _T("_com_error: ") + e.Error();
	}

	return !_connected;
}


bool odbc::connection_status()
{
	return _connected;
}

SQLRETURN odbc::last_status()
{
	return _rc;
}

TSTR odbc::last_error()
{
	return _err;
}

// returns odbc formatted connection string on successful connection
TSTR odbc::connection_string()
{
    if(_connected)
    {
        SQLTCHAR outstr[1024];
        SQLSMALLINT outstrlen;
        TSTR str;

        if(_uid!=TSTR())
            str = TSTR(_T("DSN=")) + _dsn + _T(";UID=") + _uid + _T(";PWD=") + _pwd;
        else
            str = TSTR(_T("DSN=")) + _dsn;

        free_session();
        _rc = SQLDriverConnect(_hdbc, NULL, (SQLTCHAR*)str.c_str(),
                               SQL_NTS, outstr, sizeof(outstr), &outstrlen,
                               SQL_DRIVER_COMPLETE);

        if(SQL_SUCCEEDED(_rc))
            return TSTR((TCHAR*)outstr);
    }

    return TSTR();
}


// prepares the statement and then loops through a vector of params
// and binds each param in the vector, important to note that the
// number of params defined must all be bound
bool odbc::prepare_and_bind(TSTR sql_stmt,std::vector<param> params)
{
	bool ret;
	std::vector<param>::iterator it;

	try
	{
		if(prepare(sql_stmt))
		{
			it=params.begin();

			while(it!=params.end())
			{
				ret = bind_param(it->col, it->val, it->type, it->size, it->decimals);
				++it;
			}

			return ret;
		}
		else
            return false;
	}
	catch(_com_error &e)
	{
		_err = _T("_com_error: ") + e.Error();
	}

	return false;
}

// Prepares the statement and if successful stores the SQL statement for re-use and for binding
bool odbc::prepare(TSTR sql_stmt)
{
	try
	{
		if(_connected)
		{
			_rc = SQLPrepare(_hstmt, (SQLTCHAR*)sql_stmt.c_str(), sql_stmt.size());

			if(!SQL_SUCCEEDED(_rc))
			{
				_err = _T("Failed to prepare statement with SQL error");
				extract_error(_T("prepare()"),_hstmt, SQL_HANDLE_STMT);
				return false;
			}
			else
			{
				_sql_stmt = sql_stmt;
				return true;
			}
		}
		else
		{
			_err = _T("Failed to prepare statement, connection hasn't been established yet");
			return false;
		}
	}
	catch(_com_error &e)
	{
		_err = _T("_com_error: ") + e.Error();
	}

	return false;
}

// Binds a parameter to a prepared SQL statement
bool odbc::bind_param(short col, TSTR val, short sql_field_type, SQLULEN col_size ,short decimal_pts)
{
	try
	{
		if(_connected && _sql_stmt.size())
		{
			TCHAR* cval = new TCHAR[val.size()+1];
			strcpy(cval,val.c_str());

			_rc = SQLBindParameter(_hstmt,col,SQL_PARAM_INPUT,SQL_C_CHAR,sql_field_type,
								   col_size,decimal_pts, cval, val.length(), NULL);
			if(!SQL_SUCCEEDED(_rc))
			{
				extract_error(_T("bind_param()"),_hstmt, SQL_HANDLE_STMT);
				_err = _T("Unable to bind PARAM: ") + val;
				_bound = false;
				return false;
			}
			else
			{
				_bound = true;
				return true;
			}
		}

	}
	catch(_com_error &e)
	{
		_err = _T("_com_error: ") + e.Error();
	}

	return false;
}

void odbc::free_session()
{
	free_link();
	init();
}

void odbc::free_statement()
{
    _fields = 0;
    _rows = 0;

    field_info.erase(field_info.begin(),field_info.end());
    field_names.erase(field_names.begin(),field_names.end());
	_table.erase(_table.begin(),_table.end());

	if(_hstmt) _rc = SQLFreeStmt(_hstmt, SQL_DROP);

	_rc = SQLAllocStmt(_hdbc, &_hstmt);

	if(SQL_SUCCEEDED(_rc))
	{
        _sql_stmt.clear();
        _bound = false;
        _built = false;
        _executed = false;
	}
	else
	{
	    free_session();
	    connect();
	}
}

// executes a prepared and bound statement
bool odbc::execute()
{
	try
	{
		if(_connected)
		{
			_rc = SQLExecute(_hstmt);

			if(!SQL_SUCCEEDED(_rc))
			{
				extract_error(_T("execute()"),_hstmt, SQL_HANDLE_STMT);
				return false;
			}

			_built = false;
			_executed = true;
			return true;
		}
	}
	catch(_com_error &e)
	{
		_err = _T("_com_error: ") + e.Error();
	}

	return false;
}

bool odbc::execute_direct(TSTR sql_stmt)
{
	try
	{
		if(_connected)
		{
			_rc = SQLExecDirect(_hstmt,(SQLTCHAR*)sql_stmt.c_str(), SQL_NTS);

			if(!SQL_SUCCEEDED(_rc))
			{
				extract_error(_T("execute_direct()"),_hstmt, SQL_HANDLE_STMT);
				return false;
			}

			_built = false;
			_executed = true;
			return true;
		}
	}
	catch(_com_error &e)
	{
		_err = _T("_com_error: ") + e.Error();
	}

	return false;
}

// keeps returning each row of a built result set table until
// the internal pointer reaches the end
bool odbc::fetch(unordered_row &r)
{
	if(!_built) {build_result_set();} else if(!_fetching) {reset_iterator();}

	if(_itr!=_table.end())
    {
        _fetching = true;
        r = _itr->second;
        ++_itr;
    }
    else
    {
        _itr = _table.begin();
        _fetching = false;
        return false;
    }

    return true;
}


bool odbc::fetch(unordered_row *&r)
{
	if(!_built) {build_result_set();} else if(!_fetching) {reset_iterator();}

	if(_itr!=_table.end())
    {
        _fetching = true;
        r = &(_itr->second);
        ++_itr;
    }
    else
    {
        _itr = _table.begin();
        _fetching = false;
        return false;
    }

    return true;
}


// returns only a single row from the result set
unordered_row odbc::fetch_row(unsigned long row_id)
{
	if(!_built) build_result_set();
	unordered_row r(0);

	if(_table.size() && row_id >= 1 && row_id <= _table.size())
	    return _table.find(row_id)->second;

	return r;
}

bool odbc::fetch_direct(unordered_row &r)
{
    if(_executed && _connected)
    {
        SQLUSMALLINT col;

        try
        {
			SQLNumResultCols(_hstmt, (SQLSMALLINT*)&_fields);
			set_field_descriptors();

			if(SQL_SUCCEEDED(SQLFetch(_hstmt)))
			{
				++_row_ptr;
				unordered_row next_row(_row_ptr);

				for(col=1;col<=_fields;++col)
				{
					SQLINTEGER indicator;
					SQLTCHAR buf[255] = {0};

					_rc = SQLGetData(_hstmt, col, SQL_C_TCHAR, buf, sizeof(buf), &indicator);

					if(SQL_SUCCEEDED(_rc))
					{
						if(indicator == SQL_NULL_DATA)
						{
							field f(field_names[col-1],_T("NULL"));
							next_row.add_field(f);
						}
						else
						{
							field f(field_names[col-1],(TCHAR*)buf);
							next_row.add_field(f);
						}
					}
					else
					{
						field f(field_names[col-1],_T("NULL"));
						next_row.add_field(f);
					}
				}

				r = next_row;
				return true;
			}
			else
			{
			    _row_ptr = 0;
			    return false;
			}
        }
        catch(_com_error &e)
		{
			_err = _T("_com_error: ") + e.Error();
		}
    }

    _row_ptr = 0;
    return false;
}

// fetches each DSN & DSN Description from the DSN table
// initialized on ODBC init, will return blank if ODBC failed to connect
bool odbc::fetch_dsn(TSTR &dsn, TSTR &dsn_desc)
{
	if(_dsn_itr!=_dsntable.end())
    {
        dsn = _dsn_itr->first;
        dsn_desc = _dsn_itr->second;
        ++_dsn_itr;
    }
    else
    {
        _dsn_itr = _dsntable.begin();
        return false;
    }

    return true;
}


TSTR odbc::get_field_name(unsigned long col)
{
	TSTR ret;

	if(col >= 1 && col <= _fields)
	{
		ret = (TCHAR*)field_info.at(col-1).colName;
	}

	return ret;
}

unsigned long odbc::fields()
{
	return _fields;
}

unsigned long odbc::rows()
{
	return _rows;
}

bool odbc::move_to_result_set(unsigned long set_pos)
{
    if(_executed && _connected)
    {
        //_rc = SQL_SUCCEEDED(SQLFetchScroll(_hstmt,SQL_FETCH_FIRST,set_pos));
        while(set_pos)
        {
            _rc = (SQLMoreResults(_hstmt)!=SQL_NO_DATA);
            set_pos--;
        }
        return _rc;
    }

    return false;
}

unsigned long odbc::affected_rows()
{
    SQLLEN i;
    if(_executed && _connected)
    {
        _rc = SQL_SUCCEEDED(SQLRowCount(_hstmt,&i));
        _affected_rows = i;
    }

	return _affected_rows;
}

/******************
* PRIVATE METHODS *
*******************/

// initializes handlers
void odbc::init()
{
	_fields = 0;
	_rows = 0;
	_row_ptr = 0;
	_affected_rows = 0;
	_connected = false;
	_init = false;
	_built = false;
	_executed = false;
	_bound = false;
	_fetching = false;
	_rc = SQL_SUCCESS;

	try
	{
		_rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_henv);
		_rc = SQLSetEnvAttr(_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
		set_dsn_list();
		_rc = SQLAllocHandle(SQL_HANDLE_DBC, _henv, &_hdbc);

		_init = SQL_SUCCEEDED(_rc);
	}
	catch(_com_error &e)
	{
		_err = _T("_com_error: ") + e.Error();
	}
}

// sets up the field_description vector
void odbc::set_field_descriptors()
{
	field_description c;
	c.colNumber = 1;
	field_info.clear();

	if(_hstmt)
	{
		while(Describe(c) == SQL_SUCCESS)
		{
			field_info.push_back(c);
			field_names.push_back((TCHAR*)c.colName);
			++c.colNumber;
		}
	}
}

void odbc::set_dsn_list()
{
    TCHAR dsn[256];
    TCHAR desc[256];
    SQLSMALLINT dsn_ret;
    SQLSMALLINT desc_ret;
    SQLUSMALLINT direction;

    if(!_henv) return;

    direction = SQL_FETCH_FIRST;

    while(SQL_SUCCEEDED(_rc = SQLDataSources(_henv, direction,
                                             (SQLTCHAR*)dsn, sizeof(dsn), &dsn_ret,
                                             (SQLTCHAR*)desc, sizeof(desc), &desc_ret)))
    {
        direction = SQL_FETCH_NEXT;

        _dsntable.insert(std::pair<TSTR,TSTR>(TSTR(dsn),
                         TSTR(desc)));
    }

    _dsn_itr = _dsntable.begin();
}

// returns a field_descriptor containing field data
// queried from the DB
SQLRETURN odbc::Describe(field_description& c)
{
	return SQLDescribeCol(_hstmt,c.colNumber,
		(SQLTCHAR*)c.colName, sizeof(c.colName), &c.nameLen,
		&c.dataType, &c.colSize, &c.decimalDigits, &c.nullable);
}


void odbc::build_result_set()
{
    if(_executed && !_built && _connected)
    {
        SQLUSMALLINT col;
		unsigned long row_id = 0;
        _fields = 0;
        _rows = 0;

        field_info.erase(field_info.begin(),field_info.end());
        field_names.erase(field_names.begin(),field_names.end());
		_table.erase(_table.begin(),_table.end());

        try
        {
			SQLNumResultCols(_hstmt, (SQLSMALLINT*)&_fields);
			set_field_descriptors();

			while(SQL_SUCCEEDED(SQLFetch(_hstmt)))
			{
				++row_id;
				unordered_row r(row_id);

				for(col=1;col<=_fields;++col)
				{
					SQLINTEGER indicator;
					SQLTCHAR buf[255] = {0};

					_rc = SQLGetData(_hstmt, col, SQL_C_TCHAR, buf, sizeof(buf), &indicator);

					if(SQL_SUCCEEDED(_rc))
					{
						if(indicator == SQL_NULL_DATA)
						{
							field f(field_names[col-1],_T("NULL"));
							r.add_field(f);
						}
						else
						{
							field f(field_names[col-1],(TCHAR*)buf);
							r.add_field(f);
						}
					}
					else
					{
						field f(field_names[col-1],_T("NULL"));
						r.add_field(f);
					}
				}
				_table.insert(std::pair<unsigned long, unordered_row>(r.row_id(),r));
			}

			if(row_id>0)
			{
				_rows=row_id;
			}
        }
        catch(_com_error &e)
		{
			_err = _T("_com_error: ") + e.Error();
		}
    }

    _built = true;
    _fetching = false;
	_itr = _table.begin();
}

void odbc::extract_error(TCHAR *fn, SQLHANDLE handle, SQLSMALLINT type)
{
	SQLINTEGER i = 0;
	SQLINTEGER native;
	SQLTCHAR state[ 7 ];
	SQLTCHAR text[256];
	SQLSMALLINT len;
	SQLRETURN ret;
	FTPRINTF(stderr,
			 _T("The driver reported the following diagnostics whilst running ")
			 _T("%s\n"),
			 fn);

	do
	{
        ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len );

		if (SQL_SUCCEEDED(ret))
            _tprintf(_T("%s:%ld:%ld:%s\n"), state, i, native, text);
	}
	while( ret == SQL_SUCCESS );
}

void odbc::free_link()
{
	disconnect();

	_fields = 0;
    _rows = 0;
    _row_ptr = 0;

    field_info.erase(field_info.begin(),field_info.end());
    field_names.erase(field_names.begin(),field_names.end());
	_table.erase(_table.begin(),_table.end());

    if(_connected)
    {
        if(_hstmt) _rc = SQLFreeStmt(_hstmt, SQL_DROP);
        if(_hdbc) _rc = SQLFreeHandle(SQL_HANDLE_DBC,_hdbc);
        if(_henv) _rc = SQLFreeHandle(SQL_HANDLE_ENV,_henv);
    }

	_init = false;

	_sql_stmt.clear();
	_bound = false;
	_built = false;
	_executed = false;
}

void odbc::reset_iterator()
{
    if(!(&_itr==&_table.begin())) _itr = _table.begin();
}
