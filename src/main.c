/** 22T3 COMP3601 Design Project A
 * File name: main.c
 * Description: Example main file for using the audio_i2s driver for your Zynq audio driver.
 *
 * Distributed under the MIT license.
 * Copyright (c) 2022 Elton Shih
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "audio_i2s.h"
#include "wav.h"
#include "mp3.h"
#define NUM_CHANNELS 2
#define BPS 24
#define SAMPLE_RATE 45956 //count = 16

void bin(uint8_t n) {
    uint8_t i;
     for (i = 1 << 7; i > 0; i = i >> 1)
         (n & i) ? printf("1") : printf("0");
    //for (i = 0; i < 8; i++) // LSB first
    //    (n & (1 << i)) ? printf("1") : printf("0");
}

void parsemem(void* virtual_address, int word_count) {
    uint32_t *p = (uint32_t *)virtual_address;
    char *b = (char*)virtual_address;
    int offset;
	
    uint32_t sample_count = 0;
    uint32_t sample_value = 0;
    int32_t sc_2comp = 0;
    for (offset = 0; offset < word_count; offset++) {
        sample_value = p[offset] & ((1<<18)-1);
        //sample_count = p[offset] >> 18;
	sample_count = p[offset] >> 8;

//        for (int i = 0; i < 4; i++) {
//           bin(b[offset*4+i]);
//            printf(" ");
//        }

        for (int i = 3; i >= 0; i--) {
            bin(b[offset*4+i]);
            printf(" ");
        }

	sc_2comp = sample_count << 8;
	sc_2comp >>= 8;
        printf(" -> [%d] [%d]: %02x (%dp)\n", sample_count, sc_2comp, sample_value, sample_value*100/((1<<18)-1));
    }

}

int main(int argc, char* argv[]) {
    printf("Entered main\n");
    int opt;
    uint32_t record_duration = atoi(argv[1]); 
    char *file_name = argv[2];
	char *mp3_name = argv[3];
    int print = 0;
	int mp3 = 0;
	//parse flags
    while ((opt = getopt(argc, argv, "f:t:m:p")) != -1){
        switch(opt){
            case 'f':
                file_name = optarg;
                break;
            case 't':
                record_duration = atoi(optarg);
                break;
			case 'm':
				mp3 = 1;
				mp3_name = optarg;
				break;
            case 'p':
                print = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-f wave_filename] [-t duration] [-m mp3_filename] [-p print to stdout]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }        
    audio_i2s_t my_config;
    if (audio_i2s_init(&my_config) < 0) {
        printf("Error initializing audio_i2s\n");
        return -1;
    }

    printf("mmapped address: %p\n", my_config.v_baseaddr);
    printf("Before writing to CR: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_CR));
    audio_i2s_set_reg(&my_config, AUDIO_I2S_CR, 0x1);
    printf("After writing to CR: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_CR));
    printf("SR: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_SR));
    printf("Key: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_KEY));
    printf("Before writing to gain: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_GAIN));
    audio_i2s_set_reg(&my_config, AUDIO_I2S_GAIN, 0x1);
    printf("After writing to gain: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_GAIN));
    printf("Initialized audio_i2s\n");
    printf("Starting audio_i2s_recv\n");
	//calculate number of frames needed based on record duration and sample rate
    uint32_t frames_needed = (uint32_t)record_duration*SAMPLE_RATE*NUM_CHANNELS/TRANSFER_LEN;
	//create wavefile and fill in headers
    wavefile wf = create_wave(file_name, (uint32_t)NUM_CHANNELS, (uint32_t)BPS, (uint32_t)SAMPLE_RATE, (uint32_t)TRANSFER_LEN);
    printf("Recording for %d seconds\n", record_duration);
	//get frame from dma and write it into wavefile
    for (uint32_t i = 0; i < frames_needed; i++) {
        int32_t *samples = audio_i2s_recv(&my_config);
        uint32_t frame[TRANSFER_LEN] = {0}; 
        memcpy(frame, samples, TRANSFER_LEN*sizeof(uint32_t));
        if (print) {
            printf("Frame %d:\n", i);
            parsemem(frame, TRANSFER_LEN);
            printf("==============================\n");
        }
    	write_frame(frame, wf);
    }
	//wave data written, generate mp3 if flag if m flag is set
	if (mp3){
		printf("going into generate_mp3\n");
		generate_mp3(wf, mp3_name);
	}
	//fill in remaining fields in header, close file and release unused resources
    close_wave(wf);
    audio_i2s_release(&my_config);
    printf("Done\n");
    return 0;
}

