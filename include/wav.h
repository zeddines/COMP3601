#ifndef WAV
#define WAV

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

//wavefile struct for a wavefile
typedef struct wavefile_s {
    uint32_t num_of_channels;
    uint32_t bits_per_word;
    uint32_t sample_rate;
	// number of words in a frame, 
    uint32_t frame_size;
	// word is a 32 bit sample of either left or right channel
	// words written into wavefile.
    uint32_t word_written;
	// 0 if previous word written is left channel, else 1
	// used to realign LR channels between end of frame and start of next frame
	// as a result of dma synchronsation issues
	uint32_t prev_sample_channel;
	// wavefile pointer
    FILE *file;
} wavefile_s;

typedef wavefile_s *wavefile;

wavefile create_wave(char *fn, uint32_t nc, uint32_t bps, uint32_t sr, uint32_t fs);
void write_frame(uint32_t *frame, wavefile wf);
void close_wave(wavefile wf);


#endif
