#include <lame/lame.h>
#include <stdio.h>
#include <stdlib.h>
#include "wav.h"
#include <sys/types.h>
#include <unistd.h>
#include "mp3.h"
#include <errno.h>
#include <error.h>
#include <unistd.h>

//generate mp3 file based on existing wavefile and filename
void generate_mp3(wavefile wf, char *fn){
	printf("Creating mp3 file\n");
	//create wavefile
    FILE *mfp = fopen(fn, "wb");
    if (mfp == NULL) {
        perror("Error opening file");
        exit(errno);
	} 
	printf("lame version %s\n", get_lame_version());
	lame_t lf = lame_init();
	//setting encoder
	lame_set_num_samples(lf, wf->word_written/2);
	lame_set_in_samplerate(lf, wf->sample_rate);
	lame_set_num_channels(lf, wf->num_of_channels);
	lame_set_out_samplerate(lf, 48000);
	lame_set_quality(lf, 5);
	MPEG_mode mode = wf->num_of_channels == 2 ? STEREO : MONO;
	lame_set_mode(lf, mode);
	if(lame_init_params(lf) == -1)
		printf("setting internal parameters failed.\n");
	//fseeking to the data portion of the wavefile	
	fseek(wf->file, 44, SEEK_SET);
	//prepare buffer for left and right channel
	//lame require left right channel to be seperate inputs
	int *lc = (int *)malloc(sizeof(int)*BUFFER_SIZE);
	int *rc = (int *)malloc(sizeof(int)*BUFFER_SIZE);
	int samples_received;
	int mp3buf_size;
	int mp3buf_written;
	unsigned char* mp3buf = (unsigned char *)malloc(sizeof(unsigned char)*BUFFER_SIZE+72000);
	//read BUFFER_SIZE of data from wave file
	//encode the data in mp3
	//and write it in mp3 file until all data in wavefile is read
	while ((samples_received = get_wave_data(wf, BUFFER_SIZE, lc, rc))){
		mp3buf_size = 1.25*samples_received+72000;
		mp3buf_written = lame_encode_buffer_int(lf, lc, rc, samples_received, mp3buf, mp3buf_size);
		printf("samples_received=%d, mp3buf_written=%d\n", samples_received, mp3buf_written);
		fwrite(mp3buf, mp3buf_written, 1, mfp);		
	}
	//flushing the encoder
	mp3buf_written = lame_encode_flush(lf, mp3buf, 72000);
	fwrite(mp3buf, mp3buf_written, 1, mfp);
	//freeing resources
	fclose(mfp);
	free(lc);
	free(rc);
	free(mp3buf);
	lame_close(lf);
}

//get nsamples of data from wavefile starting from wavefile pointer
//converts samples from 24bit to 32bit and
//fills them in lc and rc
//return number of samples read
//near EOF, return value is less than nsamples
int get_wave_data(wavefile wf, int nsamples, int *lc, int *rc){
	printf("getting wave frame\n");
	char buffer[2][4];
	int *temp;
	int i;
	for (i = 0; i < nsamples; i++){
		if (get_sample(wf, buffer) == 0){
			printf("no samples left\n");
			return i;
		}	
		temp = (int *)buffer[0];
		lc[i] = (*temp << 8);
		temp = (int *)buffer[1];
		rc[i] = (*temp << 8);
		printf("got a sample, %d, %d\n", lc[i], rc[i]);
	}		
	printf("frame completed\n");
	return i;
}

//get one sample based on wavefile file pointer, buffer[0] is lc, buffer[1] is rc
//returns 1 if both lc and rc is retrieved, otherwise return 0
int get_sample(wavefile wf, char buffer[2][4]){
	printf("getting_sample_left\n");
	for (uint32_t j = 0; j < wf->bits_per_word/8; j++){
		if (fread(buffer[0]+j, 1, 1, wf->file) == 0){
			return 0;	
		}
	}
	printf("getting_sample_right\n");
	for (uint32_t j = 0; j < wf->bits_per_word/8; j++){
		if (fread(buffer[1]+j, 1, 1, wf->file) == 0){
			return 0;	
		}
	}
	return 1;
}	
