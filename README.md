# EmbeddedDrumMachine
808/909 style step sequencer drum machine running on the ARM A9 HPS part of a Altera DE1-SoC dev board. Loads wav files from the SD card and uses the 
onboard switches to sequence a basic 8-step pattern, which plays out of onboard the line out jack.

Features:

- 8-step patterns
- BPM on 7 segment display, adjustable
- Waveform plot on LT24 LCD

TODO:

- Swing
- More UI stuff
- Saving/recall patterns
- Get audio to work on interrupts/implement patterns task scheduler 
