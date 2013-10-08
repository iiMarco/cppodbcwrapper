/*
  Name: table.h
  Copyright: Zammitron
  Author: Mark Zammit
  Date: 05/09/12
  Description: Designs a manipulable table built off a vector of fields
               Fields are formattable output-values and containers
               Rows are containers for fields and deal with high level info
*/

// ** = incomplete functions

#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <iostream>
#include <map>
#include <vector>


/** UNICODE SUPPORT **/
#if !defined(TCHAR)
    #if defined(UNICODE) || defined(_UNICODE_)
        #define TCHAR   wchar_t
    #else
        #define TCHAR   char
    #endif
#endif

#if defined(UNICODE) || defined(_UNICODE_)
    #define SZ_TCHAR L'\0'
#else
    #define SZ_TCHAR '\0'
#endif

#if !defined(TOSTREAM)
    #if defined(UNICODE) || defined(_UNICODE_)
        #define TOSTREAM    std::wostream
    #else
        #define TOSTREAM    std::ostream
    #endif
#endif

#if !defined(TSTR)
    #if defined(UNICODE) || defined(_UNICODE_)
        #define TSTR    std::wstring
    #else
        #define TSTR    std::string
    #endif
#endif


#define FIELDPAIR std::pair<TSTR,field>
#define FIELDVEC std::vector<FIELDPAIR >

// Prototypes
class unordered_row;
class row;
class field;

enum buffer_position
{
    lpos,
    rpos
};

// Fields are the base data containers, they store a field ID and
// value, the value can then be formatted and delivered as that
// without editting the initialized value
class field
{
    public:
        // default constructor, initializes defaults
        field() { _width = 0; _id_locked = false; _value_locked = false; _buffer = SZ_TCHAR; _buffered = false; }
        // fixed-width assignement, initializes rest to defaults
        field(unsigned int width)
        {
            _width = width;
            _buffer = SZ_TCHAR; _buffered = false; _pos = lpos;
            _id_locked = false; _value_locked = false;
        }
        // fixed-width & buffer TCHAR assignement, initializes rest to defaults
        field(unsigned int width, TCHAR buffer)
        {
            (width <= _value.size()) ? _width = _value.size() : _width = width;
            _buffer = buffer; _buffered = buffer!=SZ_TCHAR; _pos = lpos;
            _id_locked = false; _value_locked = false;
        }
        // initializes name and value only, width is set to size of value and buffer is null
        field(TSTR key, TSTR value)
        {
            _id = key; _value = value; _width = _value.size();
            _buffer = SZ_TCHAR; _buffered = false; _pos = lpos;
            _id_locked = true; _value_locked = true;
        }
        // initializes name, value and width to a width > value
        field(TSTR key, TSTR value, unsigned int width)
        {
            _id = key; _value = value;
            (width <= value.size()) ? _width = value.size() : _width = width;
            _buffer = SZ_TCHAR; _buffered = false; _pos = lpos;
            _id_locked = true; _value_locked = true;
        }
        // initializes a fixed-width field with name and buffered value
        field(TSTR key, TSTR value, unsigned int width, TCHAR buffer)
        {
            _id = key; _value = value;
            (width <= value.size()) ? _width = value.size() : _width = width;
            _buffer = buffer; _buffered = buffer!='\0'; _pos = lpos;
            _id_locked = true; _value_locked = true;
        }
        // default destructor
        ~field() {}

        // returns a TCHAR position in the field value
		// will return \0 if position is empty
		TCHAR operator[](unsigned int pos) { if(pos < _value.size()) { return _value[pos]; } return SZ_TCHAR; }

		// tests the values against another field is true
		bool operator==(const field &rhs) { field tmp = rhs; if(_value==tmp.value()) return true; else return false; }

		// tests the value of this field's value with a TCHAR* is true
		bool operator==(const TCHAR* fld) { if(_value==fld) return true; else return false; }

		// tests the value against another field is false
		bool operator!=(const field &rhs) { field tmp = rhs; if(_value!=tmp.value()) return true; else return false; }

		// tests the value of this field's value with a TCHAR* is false
		bool operator!=(const TCHAR* fld) { if(_value!=fld) return true; else return false; }

        // returns field name
        TSTR name() { return _id; }

        // returns buffered value if set or returns
        // initialized value
        TSTR value()
        {
            TSTR ret = _value;

            if(_buffered)
            {
                // keeps buffering until fixed-width reached
                while(ret.size() != _width)
                {
                    // buffers return value
                    (_pos == rpos) ? ret += _buffer : ret = _buffer + ret;
                }
            }
            return ret;
        }

        // returns only the initialized value, no formatting
        TSTR init_value() { return _value; }

        // returns the length of the value
        // note that this is not the same as TSTR::length
        size_t length() { return _value.size(); }

        // returns the fixed-width setting
        unsigned int fixed_width() { return _width; }
        // sets a fixed-width value
		void change_width(unsigned int width) { (width <= _value.size()) ? _width = _value.size() : _width = width; }
		// sets a buffer TCHAR to use, needs to be in conjunction with
		// a fixed-width value or it will not be applied
		void change_buffer(TCHAR buffer) { _buffer = buffer; _buffered = true;}
		// sets the buffer position, if 'left' it will buffer from the left
		// if 'right' it will buffer from the end of the init value
		void change_buffer_position(buffer_position pos) { _pos = pos; }
		// returns whether or not a buffering has been set
        bool is_buffered() { return _buffered; }
        // returns the buffer value
        TCHAR buffer() { return _buffer; }

        // sets the field name of a default initialized field
        // this can ONLY be applied on default init, otherwise it is locked
        void set_name(TSTR name) { if(!_id_locked) { _id = name; _id_locked = true; } }
        // sets the field value of a default initialized field
        // this can ONLY be applied on default init, otherwise it is locked
        void set_value(TSTR value)
        {
            if(!_value_locked)
            {
                _value = value;
                // sets the width if it is less than the size of the new value
                (_width <= value.size()) ? _width = value.size() : _width = _width;
                _value_locked = true;
            }
        }

        // output stream override, prints field value
        friend TOSTREAM &operator<< (TOSTREAM &out, field _field)
		{
			out << _field.value();
			return out;
		}

    protected:
        // Key
        TSTR _id;
        // Value
        TSTR _value;

        // Locks prevent double init of fields
        // fields should only ever retain an init key=>value
        bool _id_locked;
        bool _value_locked;

        // Formatting params
        unsigned int _width;
        bool _buffered;
        TCHAR _buffer;
        buffer_position _pos;
};


// Rows are ordered based on HASH assigned to them upon entering
// the map, this gives much faster speed when dealing with high
// volumes of data or when ordering is preferable
// Rows can only hold unique field identifiers
// Inherits from std::map
class row : public std::map<TSTR,field>
{
    public:
        // default constructor, initializes defaults
		row() { _id = 0; _locked = false; _id_locked = false; _reset = false; _itr = begin(); }
		// initializes with a row ID#
        row(unsigned long id) { _id = id; _locked = false; _id_locked = true; _reset = false; _itr = begin();}
        // initializes with a row ID# and an array of pre-defined fields
        // useful for field imports
        row(unsigned long id, std::vector<field> fields)
        {
            std::vector<field>::iterator it;

            // adds each pre-built field
            for(it=fields.begin(); it!= fields.end(); ++it)
            {
                add_field(*it);
            }

            _id = id;
            _locked = true;
            _reset = false;
            _itr = begin();
        }
        // default destructor
        ~row() {}

        // returns the row ID#
        unsigned long row_id() { return _id;}

        // sets the row ID# if none has been set yet,
        // otherwise this is locked to prevent overrides
		void set_row_id(unsigned long id) { if(!_id_locked) { _id = id; _id_locked = true; } }

		// adds a field class to the row
		// this is automatically ordered
        bool add_field(field fld)
        {
            // checks that it wasn't initialized
            // with a pre-defined field list
            if(!_locked)
            {
                insert(std::pair<TSTR,field>(fld.name(),fld));
                _itr = begin();
                return true;
            }
            else
            {
                _reset = false;
                return false;
            }
        }

        // returns the current number of fields
        size_t num_fields() { return size(); }

        // returns whether the internal pointer isn't
        // yet pointing to the end, return field gets assigned
        // a copy of the field in the row
        bool fetch_field(field &f)
        {
            // resets the iterator if necessary
            if(!_reset) reset_iterator();

            if(_itr!=end())
            {
                f = _itr->second;
                ++_itr;
            }
            else
            {
                _itr = begin();
                _reset = false;
                return false;
            }

            return true;
        }

        // same as fetch_field(field &f) except that
        // this passes back a pointer to the memory address
        // of an internally stored field, this is allows
        // post-assignment formatting adjustments
		bool fetch_field(field *&f)
        {
            if(!_reset) reset_iterator();

            if(_itr!=end())
            {
                f = &(_itr->second);
                ++_itr;
            }
            else
            {
                _itr = begin();
                _reset = false;
                return false;
            }

            return true;
        }

        // Returns a copy of a single valid field
        field get_field(TSTR id)
        {
            std::map<TSTR,field>::iterator itr;
            // null return
            field f(id,TSTR());

            itr = find(id);
            if(itr!=end()) { return itr->second; } else { return f; }
        }

        // Returns an encoded row output in the format of:
        // [row_id, {field_1: value_1}, {field_2: value_2}, etc.]
        friend TOSTREAM &operator<< (TOSTREAM &out, row _row)
		{
		    std::map<TSTR,field>::iterator it;
            out << _T("[id=") << _row.row_id();

            for(it=_row.begin(); it!=_row.end(); ++it)
            {
                out << _T(", {") << it->first << _T(": ") << it->second.value() << _T("}");
            }

            out << _T("]");
			return out;
		}

    protected:
        unsigned long _id;

        // Locks prevent double init of fields
        // fields should only ever retain an init key=>value
        bool _id_locked;
		bool _locked;

		// tells the row to reset its internal pointer incase it has been moved
		// this usually occurs if field has been editted by ref
		bool _reset;

		// field internal pointer, key:= field name, value:= field class
        std::map<TSTR,field>::iterator _itr;

        // resets the internal pointer back to the start
        void reset_iterator() { if(!(&_itr==&begin())) _itr = begin(); _reset = true; }
};

// Unordered rows takes fields based on how they are inserted
// this will not hash the result into an ordered map
// 'unordered_rows' are slower than 'rows'
// Unordered rows can only hold unique field identifiers
// Inherits from std::vector
class unordered_row : public FIELDVEC
{
    public:
        // default constructor, initializes defaults
		unordered_row() { _id = 0; _locked = false; _id_locked = false; _reset = false; _itr = begin(); }
		// initializes with a row ID#
        unordered_row(unsigned long id) { _id = id; _locked = false; _id_locked = true; _reset = false; _itr = begin();}
        // initializes with a row ID# and an array of pre-defined fields
        unordered_row(unsigned long id, std::vector<field> fields)
        {
            std::vector<field>::iterator it;

            // adds each pre-built field
            for(it=fields.begin(); it!=fields.end(); ++it)
            {
                add_field(*it);
            }

            _id = id;
            _locked = true;
            _reset = false;
            _itr = begin();
        }
        // default destructor
        ~unordered_row() {}

        // returns the row ID#
        unsigned long row_id() { return _id;}

        // sets the row ID# if none has been set yet,
        // otherwise this is locked to prevent overrides
		void set_row_id(unsigned long id) { if(!_id_locked) { _id = id; _id_locked = true; } }

		// adds a field class to the row
		// this is automatically ordered
        bool add_field(field fld)
        {
            // checks that it wasn't initialized
            // with a pre-defined field list and that
            // the field doesn't currently exist in the container
            if(!_locked && (find(fld.name())==end()))
            {
                push_back(FIELDPAIR(fld.name(),fld));
                _itr = begin();
                return true;
            }
            else
            {
                _reset = false;
                return false;
            }
        }

        // returns the current number of fields
        size_t num_fields() { return size(); }

        // returns whether the internal pointer isn't
        // yet pointing to the end, return field gets assigned
        // a copy of the field in the row
        bool fetch_field(field &f)
        {
            if(!_reset) reset_iterator();

            if(_itr!=end())
            {
                f = _itr->second;
                ++_itr;
            }
            else
            {
                _itr = begin();
                _reset = false;
                return false;
            }

            return true;
        }

        // same as fetch_field(field &f) except that
        // this passes back a pointer to the memory address
        // of an internally stored field, this is allows
        // post-assignment formatting adjustments
		bool fetch_field(field *&f)
        {
            if(!_reset) reset_iterator();

            if(_itr!=end())
            {
                f = &(_itr->second);
                ++_itr;
            }
            else
            {
                _itr = begin();
                _reset = false;
                return false;
            }

            return true;
        }

        // Returns a copy of a single valid field
        field get_field(TSTR id)
        {
            FIELDVEC::iterator itr;
            // null return
            field f(id,TSTR());

            itr = find(id);
            if(itr!=end()) { return itr->second; } else { return f; }
        }

        // Returns an encoded row output in the format of:
        // [row_id, {field_1: value_1}, {field_2: value_2}, etc.]
        friend TOSTREAM &operator<< (TOSTREAM &out, unordered_row _row)
		{
		    FIELDVEC::iterator it;
            out << _T("[id=") << _row.row_id();

            for(it=_row.begin(); it!=_row.end(); ++it)
                out << _T(", {") << it->first << _T(": ") << it->second.value() << _T("}");

            out << _T("]");
			return out;
		}

    protected:
        unsigned long _id;

        // Locks prevent double init of fields
        // fields should only ever retain an init key=>value
        bool _id_locked;
		bool _locked;

		// tells the row to reset its internal pointer incase it has been moved
		// this usually occurs if field has been editted by ref
		bool _reset;

		// field internal pointer, points to
		// a std::pair in the format of key:= field name, value:= field class
        FIELDVEC::iterator _itr;

        // resets the internal pointer back to the start
        void reset_iterator() { if(!(&_itr==&begin())) _itr = begin(); _reset = true; }

        // returns an iterator to an existing field through a sequential search
        // as the vector is unordered and uses a string key
        // this is to make sure that fields are unique
        FIELDVEC::iterator find(TSTR id)
        {
            FIELDVEC::iterator it;

            for(it=begin(); it!=end(); ++it)
            {
                if(it->first==id) break;
            }

            return it;
        }
};


#endif
