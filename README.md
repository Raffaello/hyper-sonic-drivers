# Hyper-Sonic Drivers

`hyper-sonic-drivers` is `RtAudio` based, supporting `sdl2` or `sdl3` as alternatives, emulating audio of '90s hardware PCs (adlib, OPL3, ...)
reading some old files too as utility and give the capability to render them in digital format
or play them in real time

To compile with either SDL2 or SDL3 use the CMake switch: `USE_SDL2` or `USE_SDL3`.

**NOTE:** On `MSYS2`. `SDL3` is currently preferred because `std::chrono` microsecond timing is not precise at all. MIDI playback is still timer-based (polling at microseconds intervals) instead of interrupt/callback driven.

## Digital sound files

Partially reading these types of files, depending of their format.

- [x] VOC files
- [x] WAV files

## Music sound files

- [x] ADL files
- [ ] XMI files
- [x] MID files
- [x] MUS files

### MIDI Files

MIDI files Format 1 are converted to single track before playing them.

Format 2 file type is not supported as there is no sample available,
but each tracks should be like a format 0 file (single track).

### MUS Files

Are converted to MIDI.


### XMI Files

eXtended MIDI files will be converted to MIDI Format 2 when
there is more than 1 track.
each track is like a format 0 MIDI file with eXtensions.

- [ ] OPL XMI
- [ ] MT32 XMI
- [ ] PCSpeaker XMI
- [ ] Tandy XMI


## Emulators

At the moment the only implemented emulators are OPL chips.

### Adlib/Sound Blaster PRO 2.0 chips

- [x] Mame OPL2
- [ ] Mame OPL3
- [x] DosBox OPL2
- [x] DosBox Dual OPL2
- [x] DosBox OPL3
- [x] Nuked OPL3
- [x] "woody" OPL2/Surround OPL2 (adding a sort of chorus effect)

### Roland MT-32

Roland MT-32 is emulated through [MUNT](https://github.com/munt/munt).

ROMs are required to use MT-32.

## Render Audio to Disk

It is possible to render the audio buffer to disk into a 16 bits PCM WAVE file of:

- [x] OPL emulated chips (ADL Files only at the moment)

## Usage

see the `examples` folder for some various basic usage.

## Credits

Most of the core emulation code is part of these projects:

- [ScummVM](https://www.scummvm.org/) mostly all emulators
- [Mame](https://www.mamedev.org/) 
- [DosBox](https://www.dosbox.com/)
- [AIL](http://www.vgmpf.com/Wiki/index.php?title=Audio_Interface_Library) reverse engineering


## Notes

## TODO

- GM
- GUS
- PC Speaker (8 bit, 16 bit, 32-bit processing)
- XMI format (xmidi) (Westwood .PCS .C55 .TAN are XMI files)
- render to digital wave or mp3
