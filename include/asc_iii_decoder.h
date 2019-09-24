#ifndef ASC_III_A_HEADER_FILE
#define	ASC_III_A_HEADER_FILE


short ASC_III_Decoder_Init(int samplerate, short samplebit, short channels, int bitrate);
/*
**   int samplerate, 8000/16000
**   short samplebit, 16
**   short channels, 1
**   int bitrate, 32000/64000
**   short return, 10 - Bit stream length(words); If it's zero, then it's wrong.
*/

short ASC_III_Decoder(short *indata, short *outdata, short len, short channels);
/*
**   short *indata, 40*2 bytes, 8KHz, 5ms; 40*2 bytes, 16KHz, 2.5ms;
**   short *outdata, 10*2 bytes
**   short len, 40
**   short channels, 1 or 2
**   short return, 40 or 80 - Sample length(words); If it's zero, then it's wrong.
*/

short ASC_III_Decoder_Set_Volume(short level); 

#endif  /* !ASC_III_A_HEADER_FILE */
