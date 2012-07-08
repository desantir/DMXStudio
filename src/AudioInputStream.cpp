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


#include "AudioInputStream.h"
#include "Functiondiscoverykeys_devpkey.h"

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IAudioEndpointVolume = __uuidof(IAudioEndpointVolume);

AudioCaptureDeviceArray AudioInputStream::audioCaptureDevices;

#define AUDIO_INPUT_ASSERT( hr, ... ) \
    if ( !SUCCEEDED(hr) ) { \
        CString message( "Audio input stream " ); \
        message.AppendFormat( __VA_ARGS__ ); \
        message.AppendFormat( " (0x%lx)", hr ); \
        throw StudioException( __FILE__, __LINE__, (LPCSTR)message ); \
    }

// ----------------------------------------------------------------------------
//
AudioInputStream* AudioInputStream::createAudioStream( LPCSTR capture_device, unsigned sample_size, 
                                             float scale_factor, float audio_sample_min )
{
    AudioInputStream* audio_stream;

    if ( StrCmpI( capture_device, "none" ) == 0 ) {
        audio_stream = new NullAudioStream();
    }
    else {
        LPCWSTR endpoint_id = NULL;
        bool isDefault = ( strlen(capture_device) == 0 || StrCmpI( capture_device, "default" ) == 0 );

        for ( AudioCaptureDeviceArray::iterator it=audioCaptureDevices.begin();
                it != audioCaptureDevices.end(); it++ ) {

            if ( (isDefault && (*it).m_isDefault) ||
                 (!isDefault && _stricmp( capture_device, (*it).m_friendly_name ) == 0) ) {
                endpoint_id = (LPCWSTR)((*it).m_id);
                break;
            }
        }

        STUDIO_ASSERT( endpoint_id != NULL, "Cannot start unknown audio capture device [%s]", capture_device );

        audio_stream = new AudioInputStream( endpoint_id, sample_size, scale_factor, audio_sample_min );
    }

    audio_stream->openAudioStream();

    return audio_stream;
}

// ----------------------------------------------------------------------------
//
void AudioInputStream::releaseAudioStream( AudioInputStream* audio_stream )
{
    if ( audio_stream ) {
        audio_stream->closeAudioStream();
        delete audio_stream;
    }
}

// ----------------------------------------------------------------------------
//
AudioInputStream::AudioInputStream( LPCWSTR endpoint_id, unsigned sample_size, float scale_factor, float audio_sample_min ) :
    m_endpoint_id( endpoint_id ),
    m_scale_factor( scale_factor ),
    m_audio_sample_min( audio_sample_min ),
    m_pEnumerator( NULL ),
    m_pDevice( NULL ),
    m_pAudioClient( NULL ),
    m_pCaptureClient( NULL ),
    m_pwfx( NULL ),
    m_sample_size( sample_size ),
    m_fft_object( sample_size )
{
    m_sample_left = new float[sample_size];
    m_sample_right = new float[sample_size];

    m_fft_channels[LEFT_CHANNEL] = m_fft_channels[RIGHT_CHANNEL] = NULL;

    memset( &m_format, 0, sizeof(WAVEFORMATEXTENSIBLE) );
}

// ----------------------------------------------------------------------------
//
AudioInputStream::~AudioInputStream(void)
{
    closeAudioStream();

    delete m_sample_left;
    delete m_sample_right;

    if ( m_fft_channels[LEFT_CHANNEL] )
        delete m_fft_channels[LEFT_CHANNEL];
    if ( m_fft_channels[RIGHT_CHANNEL] )
        delete m_fft_channels[RIGHT_CHANNEL];
}

//-----------------------------------------------------------
// Record an audio stream from the default audio capture
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// capture device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

HRESULT AudioInputStream::openAudioStream( )
{
    HRESULT hr;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;

    hr = CoCreateInstance(
           CLSID_MMDeviceEnumerator, NULL,
           CLSCTX_ALL, IID_IMMDeviceEnumerator,
           (void**)&m_pEnumerator);

    AUDIO_INPUT_ASSERT( hr, "Cannot create COM device enumerator instance" );

    hr = m_pEnumerator->GetDevice( m_endpoint_id, &m_pDevice );

    AUDIO_INPUT_ASSERT( hr, "GetDevice %S failed", m_endpoint_id );

    hr = m_pDevice->Activate(
                    IID_IAudioClient, CLSCTX_ALL,
                    NULL, (void**)&m_pAudioClient);
    
    AUDIO_INPUT_ASSERT( hr, "Activate failed" );

    hr = m_pAudioClient->GetMixFormat(&m_pwfx);

    AUDIO_INPUT_ASSERT( hr, "GetMixFormat failed" );

    hr = m_pAudioClient->Initialize(
                         AUDCLNT_SHAREMODE_SHARED,
                         0,
                         hnsRequestedDuration,
                         0,
                         m_pwfx,
                         NULL);

    AUDIO_INPUT_ASSERT( hr, "Initialize failed" );

    // Get the size of the allocated buffer.
    hr = m_pAudioClient->GetBufferSize(&m_bufferFrameCount);

    AUDIO_INPUT_ASSERT( hr, "GetBufferSize failed" );

    hr = m_pAudioClient->GetService(
                         IID_IAudioCaptureClient,
                         (void**)&m_pCaptureClient);

    AUDIO_INPUT_ASSERT( hr, "GetService failed" );

    SetFormat( m_pwfx );

    bool started = startThread();

    STUDIO_ASSERT( started, "Cannot start audio thread" );

    return 0;
}

// ----------------------------------------------------------------------------
//
HRESULT AudioInputStream::closeAudioStream()
{
    stopThread();

    if ( m_pwfx ) {
        CoTaskMemFree(m_pwfx);
        m_pwfx = NULL;
    }

    SAFE_RELEASE(m_pEnumerator)
    SAFE_RELEASE(m_pDevice)
    SAFE_RELEASE(m_pAudioClient)
    SAFE_RELEASE(m_pCaptureClient)

    return 0;
}

// ----------------------------------------------------------------------------
//
LPCSTR AudioInputStream::getEndpointDeviceName( LPCWSTR endpoint_id )
{
    for ( AudioCaptureDeviceArray::iterator it=audioCaptureDevices.begin();
            it != audioCaptureDevices.end(); it++ ) {
        if ( wcscmp( endpoint_id, (*it).m_id ) == 0 )
            return (*it).m_friendly_name;
    }

    return "UNKNOWN";
}

// ----------------------------------------------------------------------------
//
UINT AudioInputStream::run(void) 
{
    LPCSTR audio_capture_device = getEndpointDeviceName( m_endpoint_id );

    DMXStudio::log_status( "Audio input stream started [%s, %d channel(s) @ %dHz, format %X]", 
        audio_capture_device, m_format.Format.nChannels, m_format.Format.nSamplesPerSec, m_format.Format.wFormatTag );

    try {
        while ( isRunning() )
            recordAudioStream( );
    }
    catch ( std::exception& ex ) {
        DMXStudio::log( ex );
        return -1;
    }

    DMXStudio::log_status( "Audio input stream stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
bool AudioInputStream::addAudioProcessor( IAudioProcessor* sink, ProcessorFormat& format ) 
{
    CSingleLock lock( &m_sink_mutex, TRUE );

    m_sinks[ sink ] = format;

    return true;
}

// ----------------------------------------------------------------------------
//
bool AudioInputStream::removeAudioProcessor( IAudioProcessor* sink ) 
{
    CSingleLock lock( &m_sink_mutex, TRUE );

    AudioProcessorMap::iterator it = m_sinks.find( sink );
    if ( it != m_sinks.end() )
        m_sinks.erase( it );

    return true;
}

// ----------------------------------------------------------------------------
//
HRESULT AudioInputStream::recordAudioStream( )
{
    HRESULT hr;
    UINT32 numFramesAvailable;
    UINT32 packetLength = 0;
    REFERENCE_TIME hnsActualDuration;
    BYTE *pData;
    DWORD flags;

    // Setup for sample collection
    m_data_index = 0;

    hr = m_pAudioClient->Start();  // Start recording.

    AUDIO_INPUT_ASSERT( hr, "Start failed");

    // Calculate the actual duration of the allocated buffer.
    hnsActualDuration = REFTIMES_PER_SEC *
                     m_bufferFrameCount / m_pwfx->nSamplesPerSec;

    // Each loop fills about half of the shared buffer.
    while ( isRunning() )
    {
        // Sleep for half the buffer duration.
        Sleep( (DWORD)(hnsActualDuration/(REFTIMES_PER_MILLISEC)/2) );

        hr = m_pCaptureClient->GetNextPacketSize(&packetLength);
        AUDIO_INPUT_ASSERT( hr,  "GetNextPacketSize failed" );

        while (packetLength != 0 && isRunning() )
        {
            // Get the available data in the shared buffer.
            hr = m_pCaptureClient->GetBuffer(
                                   &pData,
                                   &numFramesAvailable,
                                   &flags, NULL, NULL);
            AUDIO_INPUT_ASSERT( hr, "GetBuffer failed" );

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                pData = NULL;  // Tell CopyData to write silence.
            }

            // Copy the available capture data to the audio sink.
            hr = CopyData( pData, numFramesAvailable );
            AUDIO_INPUT_ASSERT( hr, "CopyData failed" );

            hr = m_pCaptureClient->ReleaseBuffer(numFramesAvailable);
            AUDIO_INPUT_ASSERT( hr, "ReleaseBuffer failed" );

            hr = m_pCaptureClient->GetNextPacketSize(&packetLength);
            AUDIO_INPUT_ASSERT( hr, "GetNextPacketSize failed" );
        }
    }

    hr = m_pAudioClient->Stop();  // Stop recording.
    AUDIO_INPUT_ASSERT( hr, "Stop failed" );

    hr = m_pAudioClient->Reset();  // Reset stream's clock
    AUDIO_INPUT_ASSERT( hr, "Reset failed" );

    return hr;
}

// ----------------------------------------------------------------------------
//
void AudioInputStream::SetFormat( WAVEFORMATEX * pwfx )
{
    unsigned size = ( WAVE_FORMAT_EXTENSIBLE == pwfx->wFormatTag ) ?
                        sizeof(WAVEFORMATEXTENSIBLE) : sizeof(WAVEFORMATEX);
    memcpy( &m_format, pwfx, size );

    STUDIO_ASSERT( WAVE_FORMAT_EXTENSIBLE == m_format.Format.wFormatTag,		// 0xFFFE
                   "Unsupported audio format %04x (must be 0xFFFE)", m_format.Format.wFormatTag );

    STUDIO_ASSERT( m_format.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,
                   "Unsupported audio format subtype (must be IEEE FLOAT)\n" );

    if ( m_fft_channels[LEFT_CHANNEL] )
        delete m_fft_channels[LEFT_CHANNEL];
    if ( m_fft_channels[RIGHT_CHANNEL] )
        delete m_fft_channels[RIGHT_CHANNEL];

    m_fft_channels[LEFT_CHANNEL] = new FFT_Result( m_sample_size, m_format.Format.nSamplesPerSec );
    m_fft_channels[RIGHT_CHANNEL] = new FFT_Result( m_sample_size, m_format.Format.nSamplesPerSec );

    // TODO: Verify incoming values
}

// ----------------------------------------------------------------------------
//
HRESULT AudioInputStream::CopyData( BYTE *pData, UINT32 numFramesAvailable )
{
    float* data = reinterpret_cast<float *>(pData);

    WORD channels = m_format.Format.nChannels;

    for ( unsigned frame=0; frame < numFramesAvailable; frame++ ) {
        if ( data ) {
            m_sample_left[m_data_index] = *data++;

            if ( channels > 1 ) 
                m_sample_right[m_data_index] = *data++;
            else
                m_sample_right[m_data_index] = 0.0F;

            if ( channels > 2 )				// Skip any other channels
                data = &data[channels-2];
        }
        else {
            m_sample_left[m_data_index] = m_sample_right[m_data_index] = 0.0F;
        }

        m_data_index++;

        if ( m_data_index >= m_sample_size ) {
            CSingleLock lock( &m_sink_mutex, TRUE );

            scaleSample( m_sample_size, m_sample_left );
            applyWindowFunction( m_sample_size, m_sample_left );

            if (channels > 1 ) {
                scaleSample( m_sample_size, m_sample_right );
                applyWindowFunction( m_sample_size, m_sample_right );
            }

            bool left_processed = false;
            bool right_processed = false;

            for ( AudioProcessorMap::iterator it=m_sinks.begin(); it != m_sinks.end(); it++ ) {
                IAudioProcessor* sink = it->first;
                ProcessorFormat& format = it->second;

                if ( format.m_fft ) {
                    if ( !left_processed ) {
                        m_fft_channels[LEFT_CHANNEL]->clear();
                        m_fft_channels[LEFT_CHANNEL]->process( m_fft_object, m_sample_left );
                        left_processed = true;
                    }

                    if ( !right_processed && format.m_channels > 1 && m_format.Format.nChannels > 1 ) {
                        m_fft_channels[RIGHT_CHANNEL]->clear();
                        m_fft_channels[RIGHT_CHANNEL]->process( m_fft_object, m_sample_right );
                        right_processed = true;
                    }

                    sink->ProcessFFT( std::min<WORD>( channels, format.m_channels ), m_fft_channels );
                }
                else {
                    float * sample_data[] = { m_sample_left, m_sample_right };

                    sink->ProcessAmplitudes( channels, m_sample_size, sample_data );
                }
            }

            m_data_index = 0;
        }
    }

    return 0;
};

// ----------------------------------------------------------------------------
// Apply Hann window function
//
void AudioInputStream::applyWindowFunction( unsigned m_sample_size, float* sampleSet )
{	
    static const double TWO_PI = M_PI * 2.0;

    for ( unsigned bin=0; bin < m_sample_size; bin++ ) {
        sampleSet[bin] *= ( 0.5f * (1.0F - (float) cos(TWO_PI * bin / (m_sample_size - 1.0F))) );
    }
}

// ----------------------------------------------------------------------------
// Scale the sample
//
void AudioInputStream::scaleSample( unsigned m_sample_size, float* sampleSet )
{	
    if ( m_scale_factor > 0.0 ) {
        for ( unsigned bin=0; bin < m_sample_size; bin++ ) {
            bool neg = sampleSet[bin] < 0;
            float value = ( neg ) ? -sampleSet[bin] : sampleSet[bin];

            if ( value < m_audio_sample_min )          // Establish a floor for audio signal
                value = 0.0f;

            value = value * m_scale_factor;
            if ( value > .999f )
                value = .999f;

            sampleSet[bin] = ( neg ) ? -value : value;
        }
    }
}

// ----------------------------------------------------------------------------
// Collect audio capture devices
//
void AudioInputStream::collectAudioCaptureDevices( )
{
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDeviceCollection *pDevices = NULL;
    IMMDevice *pDevice = NULL;
    IPropertyStore *pProperties = NULL;
    LPWSTR pstrId = NULL;
    LPWSTR pstrDefaultId = NULL;
    UINT cDevices = 0;

    audioCaptureDevices.clear();

    try {
        hr = CoCreateInstance(
               CLSID_MMDeviceEnumerator, NULL,
               CLSCTX_ALL, IID_IMMDeviceEnumerator,
               (void**)&pEnumerator);
        AUDIO_INPUT_ASSERT( hr, "Cannot create COM device enumerator instance" );

        hr = pEnumerator->EnumAudioEndpoints( eCapture, DEVICE_STATE_ACTIVE, &pDevices );
        AUDIO_INPUT_ASSERT( hr, "Cannot enumerate audio devices" );

        // Get the default audio endpoint (if we don't get one its not an error)
        hr = pEnumerator->GetDefaultAudioEndpoint( eCapture, eConsole, &pDevice );
        if ( SUCCEEDED(hr) ) {
            pDevice->GetId( &pstrDefaultId );
            SAFE_RELEASE( pDevice );
        }

        // Get count of audio capture devices
        hr = pDevices->GetCount( &cDevices );
        AUDIO_INPUT_ASSERT( hr, "Cannot get audio device count" );

        DMXStudio::log_status( "Found %d audio capture devices", cDevices );

        // Suck up the capture device names and default
        for ( UINT i=0; i < cDevices; i++ ) {
            hr = pDevices->Item( i, &pDevice );
            AUDIO_INPUT_ASSERT( hr, "Cannot get IMMDevice" );

            hr = pDevice->GetId( &pstrId );
            AUDIO_INPUT_ASSERT( hr, "Cannot get IMMDevice Id" );

            hr = pDevice->OpenPropertyStore( STGM_READ, &pProperties );
            AUDIO_INPUT_ASSERT( hr, "Cannot open IMMDevice property store" );

            PROPVARIANT varName;
            // Initialize container for property value.
            PropVariantInit(&varName);

            // Get the endpoint's friendly-name property.
            hr = pProperties->GetValue( PKEY_Device_DeviceDesc , &varName);
            AUDIO_INPUT_ASSERT( hr, "Cannot open IMMDevice name property" );

            bool isDefault = pstrDefaultId != NULL && wcscmp( pstrId, pstrDefaultId ) == 0;
            CW2A friendly_name( varName.pwszVal );

            audioCaptureDevices.push_back( AudioCaptureDevice( pstrId, friendly_name.m_psz, isDefault) ); 

            DMXStudio::log_status( "Registering audio capture device '%s'%s", 
                friendly_name.m_psz, isDefault ? " [Default]" : "" );

            CoTaskMemFree( pstrId );
            pstrId = NULL;

            PropVariantClear(&varName);

            SAFE_RELEASE( pProperties );
            SAFE_RELEASE( pDevice );
        }

        SAFE_RELEASE( pDevices );
        SAFE_RELEASE( pEnumerator );

        CoTaskMemFree( pstrDefaultId );
    }
    catch ( ... ) {
        CoTaskMemFree( pstrDefaultId );
        CoTaskMemFree( pstrId );

        SAFE_RELEASE( pProperties );
        SAFE_RELEASE( pDevice );
        SAFE_RELEASE( pDevices );
        SAFE_RELEASE( pEnumerator );

        throw;
    }
}
