# spem
ZX Spectrum 48K emulator

The source code can be built using Visual Studio 2015, 2017 or 2019.

Target platform: Windows 8.1
Platform toolset: Visual Studio 2015 (v140)

Other dependencies:
- DirectX (DirectDraw, DirectInput, DirectSound)
- MFC


I developed this software to be my thesis work in 2000 for Windows 98. I've found the source code in my archive and decided to share and make it to be open source.

The emulator supports the following features:

- 48KB of RAM
- Snapshot file format for loading existing ZX Spectrum programs and saving the current state of the machine into this format; as well.
- Reseting
- Built in Z80 disassembler and debugger
- Different zoom levels: 1x1, 2x2, Full screen mode
- Turning on or off the screen border
- Interlace or non-interlace video rendering
- Different speed options: real, synchronized to video and full speed
- Video speed test
- Keyboard assistant to help using the Spectrum's tokenized keyboard

Note: The Z80 CPU emulation supports only the public instruction set.

Issues have been found:
- Full screen mode breaks
- Sound generation doesn't work

