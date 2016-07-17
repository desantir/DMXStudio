/* 
Copyright (C) 2011-2016 Robert DeSantis
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

#include "AudioVolumeController.h"
#include "Functiondiscoverykeys_devpkey.h"

#define AUDIO_VOLUME_ASSERT( hr, ... ) \
    if ( !SUCCEEDED(hr) ) { \
        CString message( "Audio volume controller " ); \
        message.AppendFormat( __VA_ARGS__ ); \
        message.AppendFormat( " (0x%lx)", hr ); \
        throw StudioException( __FILE__, __LINE__, (LPCSTR)message ); \
    }

// ----------------------------------------------------------------------------
//
AudioVolumeController::AudioVolumeController( IAudioEndpointVolume* endpointVolume, LPCSTR endpoint_name ) :
    m_endpointVolume( endpointVolume ),
    m_render_name( endpoint_name ),
    m_reference_count( 0L )
{
    m_endpointVolume->RegisterControlChangeNotify( this );

    HRESULT hr;
    float currentVolume = 0;

    hr = m_endpointVolume->GetMasterVolumeLevelScalar( &currentVolume );
    AUDIO_VOLUME_ASSERT( hr, "Cannot get current volume" );
    m_volume = static_cast<UINT>( (currentVolume + .005) * 100 );

    hr = m_endpointVolume->GetMute( &m_mute );
    AUDIO_VOLUME_ASSERT( hr, "Cannot get mute state" );
}

// ----------------------------------------------------------------------------
//
AudioVolumeController::~AudioVolumeController(void)
{
    m_endpointVolume->UnregisterControlChangeNotify( this );

    SAFE_RELEASE( m_endpointVolume ); 
}

// ----------------------------------------------------------------------------
//
AudioVolumeController* AudioVolumeController::createVolumeController( )
{
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDefaultDevice = NULL;
    IAudioEndpointVolume *endpointVolume = NULL;
    LPWSTR pstrDefaultId = NULL;
    IPropertyStore *pProperties = NULL;

    try {
        hr = CoCreateInstance(
               CLSID_MMDeviceEnumerator, NULL,
               CLSCTX_ALL, IID_IMMDeviceEnumerator,
               (void**)&pEnumerator);
        AUDIO_VOLUME_ASSERT( hr, "Cannot create COM device enumerator instance" );

        // Get the default audio endpoint (if we don't get one its not an error)
        hr = pEnumerator->GetDefaultAudioEndpoint( eRender, eConsole, &pDefaultDevice );
        AUDIO_VOLUME_ASSERT( hr, "Cannot get default audio render device" );

        hr = pDefaultDevice->OpenPropertyStore( STGM_READ, &pProperties );
        AUDIO_VOLUME_ASSERT( hr, "Cannot open IMMDevice property store" );

        PROPVARIANT varName;
        // Initialize container for property value.
        PropVariantInit(&varName);

        // Get the endpoint's friendly-name property.
        hr = pProperties->GetValue( PKEY_Device_DeviceDesc , &varName);
        AUDIO_VOLUME_ASSERT( hr, "Cannot open IMMDevice name property" );

        CString render_name = CW2A( varName.pwszVal );

        DMXStudio::log_status( "Default audio render device '%s'", render_name );

        PropVariantClear(&varName);

        hr = pDefaultDevice->Activate( IID_IAudioEndpointVolume, CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume );
        AUDIO_VOLUME_ASSERT( hr, "Cannot activate default render device" );

        SAFE_RELEASE( pDefaultDevice );
        SAFE_RELEASE( pProperties );
        SAFE_RELEASE( pEnumerator );

        CoTaskMemFree( pstrDefaultId );

        return new AudioVolumeController( endpointVolume, render_name );
    }
    catch ( ... ) {
        CoTaskMemFree( pstrDefaultId );

        SAFE_RELEASE( pDefaultDevice );
        SAFE_RELEASE( pProperties );
        SAFE_RELEASE( pEnumerator );

        throw;
    }
}

// ----------------------------------------------------------------------------
//
void AudioVolumeController::releaseVolumeController( AudioVolumeController* volume_controller )
{
    if ( volume_controller ) {
        delete volume_controller;
    }
}

// ----------------------------------------------------------------------------
//
UINT AudioVolumeController::getMasterVolume( )
{
    return m_volume;
}

// ----------------------------------------------------------------------------
// Get master volume
//
void AudioVolumeController::setMasterVolume( UINT volume )
{
    STUDIO_ASSERT( volume >= 0 && volume <= 100, "Master volume value outside allowed range of 0-100" );

    HRESULT hr;

    hr = m_endpointVolume->SetMasterVolumeLevelScalar( static_cast<float>(volume) / 100.0F, NULL );
    AUDIO_VOLUME_ASSERT( hr, "Cannot set current volume" );
}

// ----------------------------------------------------------------------------
//
bool AudioVolumeController::isMute( )
{
    return m_mute ? true : false;
}

// ----------------------------------------------------------------------------
// Get master volume
//
void AudioVolumeController::setMute( bool mute )
{
    HRESULT hr;

    hr = m_endpointVolume->SetMute( mute, NULL );
    AUDIO_VOLUME_ASSERT( hr, "Cannot set mute" );
}

// ----------------------------------------------------------------------------
// Get master volume
//
HRESULT AudioVolumeController::OnNotify( PAUDIO_VOLUME_NOTIFICATION_DATA pNotify )
{
    UINT volume = static_cast<UINT>( (pNotify->fMasterVolume + .005) * 100 );
    BOOL mute = pNotify->bMuted;

    if ( volume != m_volume ) {
        m_volume = volume;
        DMXStudio::fireEvent( ES_VOLUME, 0L, EA_CHANGED, NULL, volume );
    }

    if ( mute != m_mute ) {
        m_mute = mute;
        DMXStudio::fireEvent( ES_VOLUME_MUTE, 0L, mute ? EA_START : EA_STOP );
    }

    return S_OK;
}
