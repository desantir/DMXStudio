/* 
Copyright (C) 2017 Robert DeSantis
hopluvr at gmail dot com

This file is part of DMX Studio.
 
DMX Studio is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.
 
DMX Studio is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.
 
You should have received a copy of the GNU General Public License
along with DMX Studio; see the file _COPYING.txt.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.
*/

#pragma once

#include "stdafx.h"

#ifdef COLOR_SUPPORT
#include "RGBWA.h"
#endif

// ----------------------------------------------------------------------------
//
class JsonObject
{
    bool        m_isArray;
    CString     m_name;
    bool        m_first;

public:
    JsonObject( bool array, LPCSTR name ) :
        m_isArray( array ),
        m_name( name ),
        m_first( true )
    {}

    inline bool needSeparator() {
        bool need = !m_first;
        m_first = false;
        return need;
    }

    inline bool isArray() const {
        return m_isArray;
    }

    inline bool isName( LPCSTR name ) const {
        return m_name.Compare( name ) == 0;
    }
};

// ----------------------------------------------------------------------------
//
class JsonWriter
{
	bool	m_closed;

public:
	JsonWriter() :
		m_closed( false )
	{}

    virtual void Append( LPCSTR value ) = 0;
    virtual void AppendFormat( LPCSTR format, ... ) = 0;
	virtual void AppendChar( char c ) = 0;
	virtual void Append( int value ) = 0;
	virtual void Append( ULONG value ) = 0;
	virtual void Append( bool value ) = 0;
	virtual void Append( unsigned value ) = 0;
	virtual void Append( double value ) = 0;
    virtual void Append( UINT64 value ) = 0;

	inline bool isClosed() const {
		return m_closed;
	}

	virtual void close() {
		m_closed = true;
	}
};

// ----------------------------------------------------------------------------
//
class JsonStringWriter : public JsonWriter
{
#define BUFFER_CHUNK 2048

    CString*        m_buffer;
	size_t			m_length;
	size_t			m_capacity;
	char*			m_ptr;

public:
    JsonStringWriter() : 
        m_buffer( NULL ),
		m_length( 0 ),
		m_capacity( 0 ),
		m_ptr( NULL )
    {}

    JsonStringWriter( CString& buffer ) : 
        m_buffer( &buffer ),
		m_length( 0 ),
		m_capacity( BUFFER_CHUNK )
    {
		m_ptr = m_buffer->GetBufferSetLength( m_capacity );
	}

	inline void close() {
		m_buffer->ReleaseBufferSetLength( m_length );
		JsonWriter::close();
	}

	inline void AppendChar( char c ) {
		ensureCapacity( 1 );
		m_ptr[m_length++] = c;
	}

    void AppendFormat( LPCSTR format, ... ) {
		int estimated_size = 0;
		int len = 0;

		va_list args;
		va_start( args, format );

		do { 
			estimated_size += 256;

			ensureCapacity( estimated_size );

			len = vsnprintf( m_ptr+m_length, estimated_size, format, args );
		}
		while ( len > estimated_size );

		m_length += len;

		va_end( args );
    }

	inline void Append( LPCSTR value ) {
		size_t len = strlen( value );
		ensureCapacity( len );
		memcpy( m_ptr+m_length, value, len );
		m_length += len;
	}

	inline void Append( int value ) {
		ensureCapacity( 32 );
		int len = snprintf( m_ptr+m_length, 32, "%d", value );
		m_length += len;
	}

	inline void Append( ULONG value ) {
		ensureCapacity( 64 );
		int len = snprintf( m_ptr+m_length, 64, "%lu", value );
		m_length += len;
	}

    inline void Append( UINT64 value ) {
        ensureCapacity( 64 );
        int len = snprintf( m_ptr+m_length, 64, "%llu", value );
        m_length += len;
    }

	inline void Append( unsigned value ) {
		ensureCapacity( 32 );
		int len = snprintf( m_ptr+m_length, 32, "%u", value );
		m_length += len;
	}

	inline void Append( double value ) {
		ensureCapacity( 64 );
		int len = snprintf( m_ptr+m_length, 64, "%lf", value );
		m_length += len;
	}

	inline void Append( bool value ) {
		Append( (value) ? "true" : "false" );
	}

private:
	void ensureCapacity( size_t size ) {
		while ( m_length + size + 1 > m_capacity ) {
			m_capacity += BUFFER_CHUNK;
			m_ptr = m_buffer->GetBufferSetLength( m_capacity );
		}
	}
};

// ----------------------------------------------------------------------------
//
class JsonFileWriter : public JsonWriter
{
    FILE*       m_fp;

public:
    JsonFileWriter( LPCSTR file_name ) {
        errno_t err = fopen_s( &m_fp, file_name, "w" );

        if ( err || !m_fp ) {
            CString error;
            error.Format( "Unable to open file %s for write", file_name );
            throw std::exception( error );
        }
    }

    ~JsonFileWriter() {
        if ( !isClosed() )
            close();
    }

    inline void close() {
        fflush( m_fp );
        fclose( m_fp );
        JsonWriter::close();
    }

    inline void Append( LPCSTR value ) {
        fwrite( value, 1, strlen( value ), m_fp );
    }

	inline void AppendChar( char c ) {
		fputc( c, m_fp );
	}

    void AppendFormat( LPCSTR format, ... ) {
        va_list _ArgList;
        __crt_va_start(_ArgList, format);
        vfprintf_s( m_fp, format, _ArgList );
        __crt_va_end(_ArgList);    
    }

	inline void Append( int value ) {
		fprintf( m_fp, "%d", value );
	}

	inline void Append( ULONG value ) {
		fprintf( m_fp, "%lu", value );
	}

    inline void Append( UINT64 value ) {
        fprintf( m_fp, "%llu", value );
    }

	inline void Append( bool value ) {
		Append( (value) ? "true" : "false" );
	}

	inline void Append( unsigned value ) {
		fprintf( m_fp, "%u", value );
	}

	inline void Append( double value ) {
		fprintf( m_fp, "%lf", value );
	}
};

// ----------------------------------------------------------------------------
//
class JsonBuilder
{
    JsonStringWriter    m_stringWriter;
    JsonWriter&         m_buffer;
    bool                m_pretty;
	CString				m_json_string;
	bool				m_complete;
	JsonObject*			m_back;

    std::vector<JsonObject> m_stack;

public:
    JsonBuilder( CString& buffer, bool pretty = false ) : 
        m_stringWriter( buffer ),
        m_pretty( pretty ),
        m_buffer( m_stringWriter ),
		m_complete( false ),
		m_back( NULL )
    {}

    JsonBuilder( JsonWriter& writer, bool pretty = false ) : 
        m_pretty( pretty ),
        m_buffer( writer ),
		m_complete( false ),
		m_back( NULL )
    {}

    ~JsonBuilder() {
        if ( m_stack.size() > 0 )
            printf( "%d UNRESOLVED JSON OBJECTS", m_stack.size() );
    }

    void startObject( LPCSTR name = NULL ) {
		if ( m_buffer.isClosed() )
			throw std::exception( "Object added after JSON stream closed" );

		addSeparator();

        if ( name != NULL ) {
            m_buffer.AppendFormat( "\"%s\": {", name );
            m_stack.emplace_back( false /* isArray */, name );
        }
        else {
			m_buffer.AppendChar( '{' );
			m_stack.emplace_back( false /* isArray */, "" );
		}

		m_back = &m_stack.back();
    }

	void endObject( LPCSTR name = NULL ) {
		if ( m_stack.size() == 0 ) 
			throw std::exception( "END OBJECT ON EMPTY STACK" );
		else {
			JsonObject jobj = m_stack.back();
			if ( jobj.isArray() ) 
				throw std::exception( "EXPECTING OBJECT" );
			if ( name != NULL && !jobj.isName( name ) ) {
				CString error;
				error.Format( "EXPECTING OBJECT NAME %s", name );
				throw std::exception( error );
			}

			m_stack.pop_back();
			indent();
			m_buffer.AppendChar( '}' );

			if ( m_stack.size() == 0 ) {
				m_back = NULL;
				m_buffer.close();
			}
			else
				m_back = &m_stack.back();
		}
	}

    void startArray( LPCSTR name = NULL ) {
		if ( m_buffer.isClosed() )
			throw std::exception( "Object added after JSON stream closed" );

		addSeparator();

		if ( name != NULL ) {
			m_buffer.AppendFormat( "\"%s\": [", name );
			m_stack.emplace_back( true /* isArray */, name );
		}
		else {
			m_buffer.AppendChar( '[' );
			m_stack.emplace_back( true /* isArray */, "" );
		}

		m_back = &m_stack.back();
    }

    void endArray( LPCSTR name = NULL ) {
        if ( m_stack.size() == 0 )
            throw std::exception( "END ARRAY ON EMPTY STACK" );
        else {
            JsonObject jobj = m_stack.back();
            if ( !jobj.isArray() ) 
                throw std::exception( "EXPECTING ARRAY" );
            if ( name != NULL && !jobj.isName( name ) ) {
                CString error;
                error.Format( "EXPECTING ARRAY NAME %s", name );
                throw std::exception( error );
            }

            m_stack.pop_back();
            indent();
            m_buffer.AppendChar( ']' );

			if ( m_stack.size() == 0 ) {
				m_back = NULL;
				m_buffer.close();
			}
			else
				m_back = &m_stack.back();
        }
    }

	inline void addNull( LPCSTR name) {
		addName( name );
		m_buffer.Append( "null" );
    }

#ifdef COLOR_SUPPORT
    inline void add( LPCSTR name, RGBWA value ) {
        addName( name );
        m_buffer.AppendFormat( "\"%06lX\"", (unsigned long)value );
    } 

    inline void add( RGBWA value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%06lX\"", (long)value );
    } 
#endif

	inline void add( LPCSTR name, int value ) {
		addName( name );
		m_buffer.Append( value );
    }

	inline void add( LPCSTR name, unsigned value ) {
		addName( name );
		m_buffer.Append( value );
    }

	inline void add( LPCSTR name, double value ) {
		addName( name );
		m_buffer.Append( value );
    }
    
	inline void add( LPCSTR name, LPCSTR value ) {
		addName( name );
		m_buffer.Append( makeJsonString(value) );
    } 

	inline void add( LPCSTR name, ULONG value ) {
		addName( name );
		m_buffer.Append( value );
    } 

    inline void add( LPCSTR name, UINT64 value ) {
        addName( name );
        m_buffer.Append( value );
    } 

	inline void add( LPCSTR name, bool value ) {
		addName( name );
		m_buffer.Append( value );
    }

    inline void addNull() {
        addSeparator();
        m_buffer.Append( "null" );
    }
    
	inline void add( int value ) {
        addSeparator();
		m_buffer.Append( value );
    }
    
	inline void add( LPCSTR value ) {
        addSeparator();
		m_buffer.Append( makeJsonString(value) );
    }   
    
	inline void add( ULONG value ) {
        addSeparator();
		m_buffer.Append( value );
    }
    
	inline void add( bool value ) {
        addSeparator();
		m_buffer.Append( value );
    }

	inline void add( unsigned value ) {
        addSeparator();
		m_buffer.Append( value );
    }

	inline void add( double value ) {
        addSeparator();
		m_buffer.Append( value );
    }

    inline void add( UINT64 value ) {
        addSeparator();
        m_buffer.Append( value );
    }
    
    template <class T>
    void addArray( LPSTR name, T& values ) {
        startArray( name );
        for ( T::iterator it=values.begin(); it != values.end(); it++ )
            add( (*it) );
        endArray();
    }

private:
	inline void addName( LPCSTR name ) {
		addSeparator();

		m_buffer.AppendChar( '"' );
		m_buffer.Append( name );
		m_buffer.Append( "\": " );
	}

    inline void addSeparator() {
        if ( m_back != NULL && m_back->needSeparator() ) {
            m_buffer.AppendChar( ',' );
        }

        indent();
    }

    inline void indent() {
        if ( m_pretty && m_back != NULL ) {
            m_buffer.AppendChar( '\n' );

            for ( int i=m_stack.size(); i--; )
                m_buffer.Append( "    " );
        }
    }

	LPCSTR makeJsonString( LPCSTR string ) {
		LPSTR target = m_json_string.GetBufferSetLength( strlen(string) * 2 + 3 );
		LPSTR fence = target;

		*target++ = '"';

		while ( char c = *string++ ) {
			switch ( c ) {
			case '\r':
				*target++ = '\\';
				*target++ = 'r';
				break;

			case '\n':
				*target++ = '\\';
				*target++ = 'n';
				break;

			case '"':
				*target++ = '\\';
				*target++ = '"';
				break;

			case '\\':
				*target++ = '\\';
				*target++ = '\\';
				break;

			default:
				*target++ = c;
				break;
			}
		}

		*target++ = '"';

		m_json_string.ReleaseBufferSetLength( static_cast<int>(target-fence) );

		return (LPCSTR)m_json_string;
	}
};
