# SimpleWaveGen
makes sine/square waves based off a 88 note piano

Usage
arguments [file out name] [file in name]

Notes formatting for input:
[Note Start time in milliseconds] [Note A – G] [Note number (0 – 8)] [(S)harp (F)lat (N)on][Note duration in milliseconds][S for sine wave, Q for square wave]
Each note needs to be on a new line.

Example note:
2000D3N2000Q

Output is a raw 64 bit stream of sound at 8372hz
