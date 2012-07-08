/* 
Copyright (C) 2011,2012 Robert DeSantis
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


#include "stdafx.h"
#include "Form.h"

static const char * LINE_CLEAR = "\r                                                                                                \r";

// ----------------------------------------------------------------------------
//
bool Form::play() 
{
	if ( m_title.GetLength() > 0 )
		m_text_io->printf( "%s\n\n", (LPCSTR)m_title );

	// Find field length (this will not be 100% correct)
	if ( size() > 1 ) {
		m_field_width = 20;
		for ( size_t i=0; i < size(); i++ ) {					// Check hidden and displayed
			LPCSTR field_label = m_fields[i]->getLabel();
			size_t width = strlen(field_label);
			m_field_width = std::max<size_t>( m_field_width, width );
		}
		m_field_width += 5;										// Add some extra width
	}
	else
		m_field_width = 0;

	// Play all fields
	m_playing = true;
	m_current_field = first_field();

	while ( m_playing ) {
		Field* field = m_fields[m_current_field];

		try {
			field->isReady();
		}
		catch ( FieldException& e ) {
			m_text_io->printf( "\n>> %s\n", (LPCSTR)e.what() );
			return false;
		}

		CString label;
		formatFieldLabel( label, field );
		m_text_io->printf( "%s: ", (LPCSTR)label );

		CString input;
        bool token = m_text_io->haveTokens();
		int result = m_text_io->getString( input, field->isPassword() );

        try {
		    switch ( result ) {
			    case INPUT_HOME:
				    if ( m_current_field != first_field() ) {
					    m_text_io->printf( "\n" );
					    fieldLeaveNotify( m_current_field );
					    m_current_field = first_field();
				    }
				    break;

			    case INPUT_END:
                    while ( m_current_field < last_field() ) {
                        m_current_field++;
				        fieldLeaveNotify( m_current_field );
                        validateField( m_current_field );
                    }

					m_text_io->printf( "\n" );
				    break;

			    case INPUT_UP:
				    m_text_io->printf( LINE_CLEAR );
				    fieldLeaveNotify( m_current_field );

				    if ( m_current_field != first_field() ) {
					    m_text_io->printf( "%s:\n", (LPCSTR)label );
					    m_current_field = previous_field( m_current_field );
				    }
				    break;

			    case INPUT_EXIT:
				    m_text_io->printf( "\n>> Form cancelled\n" );
				    return false;

			    case INPUT_DOWN:
				    m_text_io->printf( LINE_CLEAR );
                    validateField( m_current_field );
				    fieldLeaveNotify( m_current_field );

				    if ( m_current_field != last_field() ) {
					    m_text_io->printf( "%s:\n", (LPCSTR)label );
					    m_current_field = next_field( m_current_field );
				    }
				    break;

			    case INPUT_CTL_RIGHT: {
				    bool changed = false;
				    for ( int count=0; count < 10 && field->nextValue(); count++ )
					    changed = true;
				    if ( changed )
					    fieldChangeNotify( m_current_field );
				    m_text_io->printf( LINE_CLEAR );
				    break;
			    }

			    case INPUT_CTL_LEFT: {
				    bool changed = false;
				    for ( int count=0; count < 10 && field->previousValue(); count++ )
					    changed = true;
				    if ( changed )
					    fieldChangeNotify( m_current_field );
				    m_text_io->printf( LINE_CLEAR );
				    break;
			    }

			    case INPUT_RIGHT:
				    if ( field->nextValue() )
					    fieldChangeNotify( m_current_field );
				    m_text_io->printf( LINE_CLEAR );
				    break;

			    case INPUT_LEFT:
				    if ( field->previousValue() )
					    fieldChangeNotify( m_current_field );
				    m_text_io->printf( LINE_CLEAR );
				    break;

                case INPUT_SUCCESS_AND_EXIT:
			    case INPUT_SUCCESS: {
				    bool move = true;

				    if ( input.GetLength() != 0 ) {
					    if ( field->isHelp( input ) ) {
						    CString help;
						    field->helpText( help );
						    m_text_io->printf( "\n\n%s\n", (LPCSTR)help );
						    break;
					    }

					    CString current_value = field->getValue();

						if ( !field->setValue( input ) ) {
							m_text_io->printf( LINE_CLEAR );
							break;
						}
					
					    fieldChangeNotify( m_current_field );

					    if ( !token && current_value != field->getValue() && 
					         ((m_current_field == last_field() && isStopOnLastField()) || !isMoveAfterChange()) )
						    move = false;
				    }
                    else
                        validateField( m_current_field );

				    if ( move ) {
					    fieldLeaveNotify( m_current_field );
					    m_text_io->printf( LINE_CLEAR );
					    formatFieldLabel( label, field );
					    m_text_io->printf( "%s: ", (LPCSTR)label );

					    if ( m_current_field == last_field() )
						    m_playing = false;
					    else
						    m_current_field = next_field( m_current_field );
				    }

                    if ( result == INPUT_SUCCESS_AND_EXIT ) {
                        while ( m_current_field < last_field() ) {
                            m_current_field++;
                            validateField( m_current_field );
					        fieldLeaveNotify( m_current_field );
                        }

                        m_playing = false;
                        break;
                    }

                    if ( move )
					    m_text_io->printf( "\n" );
                    else
				        m_text_io->printf( LINE_CLEAR );

				    break;
			    }
			    case INPUT_ERROR:
			    default:
				    m_text_io->printf( LINE_CLEAR );
				    break;
		    }
        }
		catch ( FieldException& e ) {
			m_text_io->printf( "\n>> %s\n", e.what() );
		}
	}

	m_text_io->printf( "\n" );

	formCompleteNotify();

	return true;
}

// ----------------------------------------------------------------------------
//
size_t Form::first_field() const {
	if ( !m_fields[0]->isHidden() )
		return 0;
	return next_field( 0 );
}

// ----------------------------------------------------------------------------
//
size_t Form::last_field() const {
	size_t last_field = m_fields.size()-1;
	if ( !m_fields[last_field]->isHidden() )
		return last_field;
	return previous_field( last_field );
}

// ----------------------------------------------------------------------------
//
size_t Form::next_field( size_t current_field ) const {
	for ( size_t field_num=current_field; ++field_num < m_fields.size(); )
		if ( !m_fields[field_num]->isHidden() )
		return field_num;
	return current_field;
}

// ----------------------------------------------------------------------------
//
size_t Form:: previous_field( size_t current_field ) const {
	for ( size_t field_num=current_field; field_num-- > 0; )
		if ( !m_fields[field_num]->isHidden() )
		return field_num;
	return current_field;
}
