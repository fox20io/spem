/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

#if !defined(ZX_SPECTRUM_SPEAKER_V0100_INCLUDED)
#define ZX_SPECTRUM_SPEAKER_V0100_INCLUDED

class Speaker
{
protected:
	// DS buffer management members
	LPDIRECTSOUND8 m_dsound;
	WAVEFORMATEX m_format;
	LPDIRECTSOUNDBUFFER m_dsbuf;
	DSBUFFERDESC m_buf_format;

	const double BufferLengthInMs = 310.0;
	
	// virtual buffer management members
	LPBYTE m_buffer = NULL;
	int m_buffer_size = 0;
	int m_buffer_pos = 0;
	int m_last_dsbuff_pos = 0;

public:
	Speaker();
	~Speaker();

	BOOL Initialize();
	BOOL Play();
	BOOL Stop();
	void ApplyBuffer(int runtimeSpanMs);
	void CreateBuffer(int size);
	void WriteNextBufferBit(BOOL pcmBit);
};

#endif