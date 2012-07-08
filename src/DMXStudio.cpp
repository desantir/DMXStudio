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

#include "DMXStudio.h"
#include "DMXFixtureIDs.h"
#include "FixtureDefinition.h"
#include "DMXTextUI.h"
#include "DMXHttpServer.h"
#include "VenueWriter.h"
#include "VenueReader.h"
#include "IniFile.h"
#include "MusicPlayer.h"

DMXStudio studio;

static CString getUserDocumentDirectory();
extern void readFixtureDefinitions( void );

// ----------------------------------------------------------------------------
//
int main( int argc, char* argv[] ) {
    studio.runStudio();
}

// ----------------------------------------------------------------------------
//
 DMXStudio::DMXStudio() :
    m_hLog( NULL ),
    m_debug( false ),
    m_venue( NULL ),
    m_http_port( 80 ),
    m_whiteout_strobe_slow( 300, 200 ),
    m_whiteout_strobe_fast( 100, 50 ),
    m_enable_mobile( true ),
    m_music_player( NULL )
{
}

// ----------------------------------------------------------------------------
//
 DMXStudio::~DMXStudio()
{
    if ( m_music_player )
        free( m_music_player );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::runStudio()
{
	CoInitializeEx( NULL, COINIT_MULTITHREADED );

	try {
        openStudioLogFile();

		log_status( "DMX Studio v0.0.11" );

        readIniFile();

        // Read fixture definitions
        FixtureDefinition::readFixtureDefinitions();

        // Load all available audio capture devices
		AudioInputStream::collectAudioCaptureDevices();

        // Enumerate the IP addresses
        showIpAddress();

        // Start the request server
		DMXHttpServer server;
        if ( m_enable_mobile )
		    server.start();

        // Connect to the music player if available
        if ( hasMusicPlayer() ) {
            getMusicPlayer()->initialize( );
            getMusicPlayer()->connect();
        }

        // Load the default venue
	    if ( !DMXStudio::loadVenue( getDefaultVenueFilename() ) )
            m_venue = new Venue();

        // Start the console UI
		DMXTextUI ui;
		ui.run();

        if ( m_enable_mobile )
            server.stop();
	}
	catch ( StudioException& ex ) {
		log( ex );
		getchar();
	}
	catch ( std::exception& ex ) {
		log( ex );
		getchar();
	}

    if ( m_venue )
		delete m_venue;

    if ( hasMusicPlayer() && getMusicPlayer()->isLoaded() )
        getMusicPlayer()->disconnect( );

    closeStudioLogFile();

    //writeIniFile();

	CoUninitialize();
}
// ----------------------------------------------------------------------------
//
void DMXStudio::createMusicPlayer( LPCSTR username, LPCSTR player_dll_path )
{
    m_music_player = new MusicPlayer( player_dll_path, username );
}

// ----------------------------------------------------------------------------
//
bool DMXStudio::loadVenue( LPCSTR venue_filename )
{
	CSingleLock lock( &m_venue_mutex, TRUE );

	Venue* new_venue = DMXStudio::readVenue( venue_filename );
	if ( !new_venue )
		return false;

	if ( m_venue )
		delete m_venue;

	m_venue = new_venue;
	m_venue_filename = venue_filename;

	m_venue->open( );

	return true;
}

// ----------------------------------------------------------------------------
//
bool DMXStudio::saveVenue( LPCSTR venue_filename )
{
	CSingleLock lock( &m_venue_mutex, TRUE );

    writeVenue( venue_filename );

    m_venue_filename = venue_filename;

    return true;
}

// ----------------------------------------------------------------------------
//
CString getUserDocumentDirectory()
{
	char input_file[MAX_PATH]; 
	HRESULT result = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, input_file); 
	if ( result != S_OK )
		throw StudioException( "Error %d finding document directory", result );
    return CString( input_file );
}

// ----------------------------------------------------------------------------
//
CString DMXStudio::getDefaultVenueFilename( )
{
    if ( m_venue_filename.GetLength() )
        return m_venue_filename;
	
	CString filename;
	filename.Format( "%s\\DMXStudio\\DefaultVenue.xml", getUserDocumentDirectory() );
	return filename;
}

// ----------------------------------------------------------------------------
//
void DMXStudio::openStudioLogFile( )
{
	CString filename;
	filename.Format( "%s\\DMXStudio\\DMXStudio.log", getUserDocumentDirectory() );

    m_hLog = _fsopen( filename, "at", _SH_DENYWR );
    fputs( "\n", m_hLog );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::closeStudioLogFile( )
{
    if ( m_hLog != NULL ) {
        fflush( m_hLog );
        fclose( m_hLog );
        m_hLog = NULL;
    }
}

// ----------------------------------------------------------------------------
//
Venue* DMXStudio::readVenue( LPCSTR input_file )
{
	if ( GetFileAttributes( input_file ) == INVALID_FILE_ATTRIBUTES )
		return NULL;

    VenueReader reader;

	CSingleLock lock( &m_venue_mutex, TRUE );

    return reader.read( input_file );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::writeVenue( LPCSTR output_file )
{
	// Create the path if it does not exist
	CString path( output_file ); 
    int pos = path.ReverseFind( '\\' ); 
 
    if ( pos != -1) {
		path = path.Left( pos+1 );      
		if ( !PathFileExists( path ) )
			if ( !CreateDirectory( path, NULL ) )	// TODO - need to created entire path
				throw StudioException( "Cannot create directory '%s' (ERROR %lu)", 
									   path, GetLastError() );
	}

	CSingleLock lock( &m_venue_mutex, TRUE );

    VenueWriter writer;
    writer.write( m_venue, output_file );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::log( std::exception& ex ) {
	CString output;
	output.Format( "EXCEPTION: %s", ex.what() );

	printf( "%s\n", (LPCSTR)output );
	log( output );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::log( StudioException& ex ) {
	CString output;

	if ( strlen( ex.getFile() ) > 0 )
		output.Format( "EXCEPTION: %s (%s:%ld)", ex.what(), ex.getFile(), ex.getLine() );
	else
		output.Format( "EXCEPTION: %s", ex.what() );

	printf( "%s\n", (LPCSTR)output );
	log( output );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::log_status( const char *fmt, ... ) {
	va_list list;
	va_start( list, fmt );

	CString output( "STATUS: " );
	output.AppendFormatV( fmt, list );

	printf( "%s\n", (LPCSTR)output );
	log( output );

	va_end( list );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::log( const char *fmt, ... ) {
	va_list list;
	va_start( list, fmt );

    if ( studio.m_hLog ) {
        time_t rawtime;
        struct tm timeinfo;
        char buffer[80];

        time ( &rawtime );
        localtime_s( &timeinfo, &rawtime );

        strftime( buffer, sizeof(buffer), "[%x %X] ", &timeinfo );

	    CString output;
        output = buffer;
	    output.AppendFormatV( fmt, list );
	    output.Append( "\n" );
        
        // fputs should be a sigle atomic operation (i.e. no log collisions)
        fputs( output, studio.m_hLog );
        fflush( studio.m_hLog );
    }

	va_end( list );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::showIpAddress()
{
    char        cpuName [128] ;
    DWORD       dwSize = sizeof(cpuName);
    WSADATA     wsaData;
    int         iResult;

    if ( GetComputerName( cpuName, &dwSize) == 0 )
        return;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult == 0) {
        PADDRINFOA  pAddrInfo;
        iResult = getaddrinfo( cpuName, "http", NULL, &pAddrInfo );

        if ( iResult == 0 ) {
            for ( PADDRINFOA pAddr=pAddrInfo; pAddr != NULL; pAddr=pAddr->ai_next ) {
                if ( AF_INET == pAddr->ai_family ) {
                    struct sockaddr_in  *sockaddr_ipv4;
                    sockaddr_ipv4 = (struct sockaddr_in *) pAddr->ai_addr;
                    log_status( "%s IP address %s", cpuName, inet_ntoa(sockaddr_ipv4->sin_addr) );
                }
            }

            freeaddrinfo( pAddrInfo );
        }
    }

    WSACleanup();
}

// ----------------------------------------------------------------------------
//
void DMXStudio::readIniFile()
{
	CString filename;
	filename.Format( "%s\\DMXStudio\\DMXStudio.ini", getUserDocumentDirectory() );

    IniFile iniFile;
    iniFile.read( filename, this );
}

// ----------------------------------------------------------------------------
//
void DMXStudio::writeIniFile( )
{
	CString filename;
	filename.Format( "%s\\DMXStudio\\DMXStudio.ini", getUserDocumentDirectory() );

    IniFile iniFile;
    iniFile.write( filename, this );
}