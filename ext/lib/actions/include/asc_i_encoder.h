#ifndef  ASC_I_HEADER_FILE
#define	 ASC_I_HEADER_FILE

extern short ROM_ASC_I_Encoder_Init(int samplerate, short samplebit, short channels, int bitrate);
extern short ROM_ASC_I_Encoder(short *indata, short *outdata, short len);
void ASC_I_Encoder_Set_Buffer(int* global);

/**
 * @brief ASC_I encoder init
 *
 * @param short codec: 10, @16:1.
 *
 * @return Sample length, @320 (x 2Bytes).
 */
short ASC_I_Encoder_Init(short codec)
{
	   short Samplelen = 0;
	   if (codec != 10) return 0;
	   
	   Samplelen = ROM_ASC_I_Encoder_Init(16000, 16, 1, 16000);
	   return Samplelen;
}

/**
 * @brief ASC_I encoder 
 *
 * @param short *indata, 320*2 bytes, @16KHz PCM data.
 * @param output, short *outdata, 20*2 bytes
 * @param short len: 320
 *
 * @return Bitstream length, @20 (x 2Bytes)
 */
short ASC_I_Encoder(short *indata, short *outdata, short len)
{
	short Bitstreamlen;
	
	Bitstreamlen = ROM_ASC_I_Encoder(indata, outdata, len);
	
	return Bitstreamlen/2;
}

#endif /* ! ASC_I_HEADER_FILE */



