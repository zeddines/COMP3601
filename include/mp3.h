#ifndef MP3 
#define MP3
#include "wav.h"


#define BUFFER_SIZE 8192
//generate mp3 file based on existing wavefile and filename
void generate_mp3(wavefile wf, char *s);

//get nsamples of data from wavefile starting from wavefile pointer
//converts samples from 24bit to 32bit and
//fills them in lc and rc
//return number of samples read
//near EOF, return value is less than nsamples
int get_sample(wavefile wf, char buffer[2][4]);

//get one sample based on wavefile file pointer, buffer[0] is lc, buffer[1] is rc
//returns 1 if both lc and rc is retrieved, otherwise return 0
int get_wave_data(wavefile wf, int nsamples, int32_t *lc, int32_t *rc);
#endif
