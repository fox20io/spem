/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

#include "StdAfx.h"
#include "speaker.h"
#include "machine.h"
#include <cstdint>

Speaker::Speaker()
{
}

Speaker::~Speaker()
{
	if (m_buffer != NULL)
		delete[] m_buffer;
}

BOOL Speaker::Initialize()
{
	memset(&m_format, 0, sizeof(WAVEFORMATEX));
	m_format.wFormatTag = WAVE_FORMAT_PCM;
	m_format.nChannels = 1;
	m_format.wBitsPerSample = 8;
	m_format.nSamplesPerSec = 22050;
	m_format.nBlockAlign = m_format.nChannels * m_format.wBitsPerSample / 8;
	m_format.nAvgBytesPerSec = m_format.nBlockAlign * m_format.nSamplesPerSec;
	m_format.cbSize = 0;

	if (DirectSoundCreate8(NULL, &m_dsound, NULL) != DS_OK)
		return FALSE;
	if (m_dsound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY) != DS_OK)
		return FALSE;

	memset(&m_buf_format, 0, sizeof(DSBUFFERDESC));
	m_buf_format.dwSize = sizeof(m_buf_format);
	m_buf_format.dwFlags =
		DSBCAPS_GETCURRENTPOSITION2 |
		DSBCAPS_STICKYFOCUS | 
		DSBCAPS_GLOBALFOCUS |
		DSBCAPS_LOCSOFTWARE |
		//DSBCAPS_CTRLPOSITIONNOTIFY |
		DSBCAPS_CTRLVOLUME;
	m_buf_format.dwBufferBytes = (double)m_format.nSamplesPerSec * BufferLengthInMs / 1000.0;
	m_buf_format.dwReserved = 0;
	m_buf_format.lpwfxFormat = &m_format;

	if (m_dsound->CreateSoundBuffer(&m_buf_format, &m_dsbuf, NULL) != DS_OK)
		return FALSE;

	uint8_t* data1, * data2;
	uint32_t size1, size2;

	if (IDirectSoundBuffer_Lock(m_dsbuf, 0, m_buf_format.dwBufferBytes, (LPVOID*)&data1, (LPDWORD)&size1,
		(LPVOID*)&data2, (LPDWORD)&size2, DSBLOCK_ENTIREBUFFER) != DS_OK)
		return FALSE;

	for (uint32_t i = 0; i < size1; i++)
		data1[i] = 0;

	IDirectSoundBuffer_Unlock(m_dsbuf, (LPVOID)data1, (DWORD)size1, (LPVOID)data2, (DWORD)size2);

	return TRUE;
}

BOOL Speaker::Play()
{
	if (m_dsbuf != NULL)
	{
		if (IDirectSoundBuffer_Play(m_dsbuf, 0, 0, DSBPLAY_LOOPING) != DS_OK)
			return FALSE;
	}
	return TRUE;
}


BOOL Speaker::Stop()
{
	if (m_dsbuf != NULL)
	{
		if (IDirectSoundBuffer_Stop(m_dsbuf) != DS_OK)
			return FALSE;

		m_last_dsbuff_pos = 0;
	}
	return TRUE;
}

void Speaker::CreateBuffer(int size)
{
	if (m_buffer_size != size || m_buffer == NULL)
	{
		if (m_buffer != NULL)
			delete[] m_buffer;
		m_buffer = new BYTE[size];
		m_buffer_size = size;
	}
	::memset(m_buffer, 0, size);
	m_buffer_pos = 0;
	m_last_dsbuff_pos = 0;
}

void Speaker::WriteNextBufferBit(BOOL pcmBit)
{
	if (m_buffer_pos < m_buffer_size - 1)
	{
		m_buffer[m_buffer_pos] = pcmBit;
		m_buffer_pos++;
	}
}

BOOL AppWriteDataToBuffer(
	LPDIRECTSOUNDBUFFER lpDsb,  // The buffer.
	DWORD dwOffset,              // Our own write cursor.
	LPBYTE lpbSoundData,         // Start of our data.
	DWORD dwSoundBytes)          // Size of block to copy.
{
	LPVOID  lpvPtr1;
	DWORD dwBytes1;
	LPVOID  lpvPtr2;
	DWORD dwBytes2;
	HRESULT hr;

	// Obtain memory address of write block. This will be in two parts
	// if the block wraps around.

	hr = lpDsb->Lock(dwOffset, dwSoundBytes, &lpvPtr1,
		&dwBytes1, &lpvPtr2, &dwBytes2, 0);

	// If the buffer was lost, restore and retry lock. 

	if (DSERR_BUFFERLOST == hr)
	{
		lpDsb->Restore();
		hr = lpDsb->Lock(dwOffset, dwSoundBytes,
			&lpvPtr1, &dwBytes1,
			&lpvPtr2, &dwBytes2, 0);
	}
	if (SUCCEEDED(hr))
	{
		// Write to pointers. 

		CopyMemory(lpvPtr1, lpbSoundData, dwBytes1);
		if (NULL != lpvPtr2)
		{
			CopyMemory(lpvPtr2, lpbSoundData + dwBytes1, dwBytes2);
		}

		// Release the data back to DirectSound. 

		hr = lpDsb->Unlock(lpvPtr1, dwBytes1, lpvPtr2,
			dwBytes2);
		if (SUCCEEDED(hr))
		{
			// Success. 
			return TRUE;
		}
	}

	// Lock, Unlock, or Restore failed. 

	return FALSE;
}

void Speaker::ApplyBuffer(int runtimeSpanMs)
{
	if (m_dsbuf != NULL)
	{
		// calc the size of buffer for runtime span
		double bytesPerMs = (double)m_format.nSamplesPerSec / BufferLengthInMs;
		DWORD dsBuffSizeForSpan = runtimeSpanMs * bytesPerMs;
		if (dsBuffSizeForSpan > m_buf_format.dwBufferBytes)
			dsBuffSizeForSpan = m_buf_format.dwBufferBytes;
		if (dsBuffSizeForSpan == 0)
			return;
		BYTE* soundData = new BYTE[dsBuffSizeForSpan];
		int stepInBuff = m_buffer_size / dsBuffSizeForSpan;
		for (int i = 0, j = 0; i < dsBuffSizeForSpan; i ++)
		{
			soundData[i] = m_buffer[j] ? 50 : 0;
			j += stepInBuff;
		}

		if (m_last_dsbuff_pos == 0)
		{
			DWORD dwCurrentWriteCursor;
			IDirectSoundBuffer_GetCurrentPosition(m_dsbuf, NULL, &dwCurrentWriteCursor);
			m_last_dsbuff_pos = dwCurrentWriteCursor;
		}

		AppWriteDataToBuffer(m_dsbuf, m_last_dsbuff_pos, soundData, dsBuffSizeForSpan);

		m_last_dsbuff_pos += dsBuffSizeForSpan;
		if (m_last_dsbuff_pos >= m_buf_format.dwBufferBytes)
			m_last_dsbuff_pos -= m_buf_format.dwBufferBytes;
	}
}
