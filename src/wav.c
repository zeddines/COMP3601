#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "wav.h"
#include <errno.h>

void write_big_endian(uint32_t w, int nb, FILE* file);
void write_big_endian_str(const char *s, int nb, FILE* file);
void write_little_endian(uint32_t w, int nb, FILE* file);

// creates wavefile and wavefile struct. Filling in details in wavefile headers based on provided arguments
// returns wavefile struct 
wavefile create_wave(char *fn, uint32_t nc, uint32_t bps, uint32_t sr, uint32_t fs){
    printf("Creating wave file\n");
	//create wavefile
    FILE *file = fopen(fn, "wb+");
    if (file == NULL) {
        perror("Error opening file");
        exit(errno);
	} 
	//create wavefile struct
    wavefile wf =  (wavefile_s*)malloc(sizeof(wavefile_s));
    wf->num_of_channels = nc;
	wf->bits_per_word = bps;
	wf->sample_rate = sr;
	wf->frame_size = fs;
    wf->word_written = 0;
    wf->file = file;
	wf->prev_sample_channel = 1;
	uint32_t byte_rate = sr*nc*bps/8;
    uint32_t block_align = nc*bps/8;
	//write wavefile header details, file size and data size left unfilled
    write_big_endian_str("RIFF", 4, file);
    fseek(file, 4, SEEK_CUR); // file size unfilled
    write_big_endian_str("WAVE", 4, file);
    write_big_endian_str("fmt ", 4, file);
    write_little_endian(16, 4, file); //subchucksize
    write_little_endian(1, 2, file); //pcm format
    write_little_endian(nc, 2, file); //number of channels
    write_little_endian(sr, 4, file); //sample rate
    write_little_endian(byte_rate, 4, file); //byte_rate
    write_little_endian(block_align, 2, file); //block align
    write_little_endian(bps, 2, file); //bits per sample
    write_big_endian_str("data", 4, file);
    fseek(file, 4, SEEK_CUR); // chunk size unfilled
    return wf;
}

// writes a frame of samples into wavefile specified in wavfile struct
void write_frame(uint32_t *frame, wavefile wf){ 
	uint8_t lsb = *(uint8_t *)frame;
	uint32_t first_sample_channel = (lsb & (1 << 1)) ? 1 : 0;
 	// Some words at the start of the frame aren't transfered due to synchronisation issue
	// from dma. As a result, LR channels maybe misaligned between ending frame word and next frame
	// starting word. If misalign occured, skip the first word in a frame.	
	uint32_t si = (wf->prev_sample_channel == first_sample_channel) ? 1 : 0;
    for (uint32_t j = si; j < wf->frame_size; j++){
      uint32_t s = frame[j];
      uint8_t *p = (uint8_t *)(&s); 
	  // if LSB is 0, the word isn't coming from audio pipeline and is the all 0 samples 
	  // coming from dma. Prevent writing 0s into wavefile
	  if (*p << 7 == 0)
		 break;
	  // write the most 3 MS byte into wave
      for (int k = 1; k <= 3; k++){
          fwrite(&(p[k]), 1, 1, wf->file);
	  }
	  // record channel of the word 
	  wf->prev_sample_channel = (*p & (1 << 1)) ? 1 : 0; 
	  wf->word_written++;
    }
}

// fill out chunk and file size in wavefile header
// closes wavefile and release resources
void close_wave(wavefile wf){
	// fill out chunk size fields in wavefile header
    uint32_t num_of_samples = wf->word_written/wf->num_of_channels;
    uint32_t sub_chunk2_size = num_of_samples*wf->num_of_channels*wf->bits_per_word/8;
    uint32_t chunk_size = sub_chunk2_size+36;
    fseek(wf->file, 4, SEEK_SET);
    write_little_endian(chunk_size, 4, wf->file);
    fseek(wf->file, 40, SEEK_SET);
    write_little_endian(sub_chunk2_size, 4, wf->file);
	// close file and release resources
    fclose(wf->file);
    free(wf);    
}

////////////// helper functions//////////////////////
void write_little_endian(uint32_t w, int nb, FILE* file){
    fwrite(&w, 1, nb, file);
}

void write_big_endian(uint32_t w, int nb, FILE* file){
    uint32_t a;
    for (int shift_b = nb-1; shift_b >= 0; shift_b--){
        a = 0xff & (w >> (shift_b*8));
        fwrite(&a, 1, 1, file);
    }
}

void write_big_endian_str(const char *s, int nb, FILE* file){
    fwrite(s, nb, 1, file);
}
