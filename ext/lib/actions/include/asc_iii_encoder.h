#ifndef ASC_III_HEADER_FILE
#define	ASC_III_HEADER_FILE

/**
 * @brief ASC_III encoder init
 *
 * @param short codec: 30 or 31,  @4:1,  30@8KHz PCM data,  31 @16KHz PCM data
 *
 * @return Sample length,  @40 (x 2Bytes).
 */
short ASC_III_Encoder_Init(short codec);

/**
 * @brief ASC_III encoder 
 *
 * @param short *indata, 40*2 bytes,  @8KHz PCM data @16KHz PCM data.
 * @param short *outdata, 10*2 bytes
 * @param short len: 40
 *
 * @return Bitstream length,  @10 (x 2Bytes)
 */
short ASC_III_Encoder(short *indata, short *outdata, short len);

#endif /* !ASC_III_HEADER_FILE */



