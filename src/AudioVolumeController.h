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

#pragma once

#include "DMXStudio.h"

class AudioVolumeController : public IAudioEndpointVolumeCallback 
{
    IAudioEndpointVolume*   m_endpointVolume;
    CString                 m_render_name;
    ULONG                   m_reference_count;

    UINT                    m_volume;
    BOOL                    m_mute;

protected:
    AudioVolumeController( IAudioEndpointVolume*, LPCSTR endpoint_name );
    ~AudioVolumeController(void);

public:
    static AudioVolumeController* createVolumeController( );
    static void releaseVolumeController( AudioVolumeController* volume_controller );

    UINT getMasterVolume( void );
    void setMasterVolume( UINT volume );

    bool isMute( void );
    void setMute( bool mute );

    LPCSTR getRenderDeviceName( ) const {
        return m_render_name;
    }

    HRESULT STDMETHODCALLTYPE OnNotify( PAUDIO_VOLUME_NOTIFICATION_DATA pNotify );

    ULONG STDMETHODCALLTYPE AddRef() {
        return ++m_reference_count;
    }

    ULONG STDMETHODCALLTYPE Release() {
        return --m_reference_count;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject ) {
        if ( ppvObject == NULL )
            return E_POINTER;

        if ( riid == IID_IAudioEndpointVolume )
            return E_NOINTERFACE;

        *ppvObject = this;

        AddRef();
        
        return S_OK;
    }
};

