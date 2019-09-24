#ifndef  ASC_II_HEADER_FILE
#define	 ASC_II_HEADER_FILE

/**
 * @brief ASC_II encoder init
 *
 * @param short codec: 20,  @8:1.
 *
 * @return Sample length,  @80 (x 2Bytes).
 */
short ASC_II_Encoder_Init(short codec);

/**
 * @brief ASC_II encoder 
 *
 * @param short *indata, 80*2 bytes,  @16KHz PCM data.
 * @param short *outdata, 10*2 bytes
 * @param short len: 80
 *
 * @return Bitstream length,  @10 (x 2Bytes)
 */
short ASC_II_Encoder(short *indata, short *outdata, short len);

/**
 * @brief ASC_II_B encoder init
 *
 * @param short codec: 21,  @8:1.
 *
 * @return Sample length,  @80 (x 2Bytes).
 */
short ASC_II_B_Encoder_Init(short codec);

/**
 * @brief ASC_II_B encoder 
 *
 * @param short *indata, 80*2 bytes,  @16KHz PCM data.
 * @param short *outdata, 10*2 bytes
 * @param short len: 80
 *
 * @return Bitstream length,  @10 (x 2Bytes)
 */
short ASC_II_B_Encoder(short *indata, short *outdata, short len);

#endif /* ! ASC_II_HEADER_FILE */



