CC       ?= gcc
CXX      ?= g++
LANGFLAG = -x c++
CPPFLAGS += -I include/
CFLAGS   += -g -Wall -O2 -std=c++17
LDFLAGS  += $(LIBS) -lpthread -lz -rdynamic
BUILD_DIR = build

BINARY = sample256
OBJ = $(BUILD_DIR)/main.o \
      $(BUILD_DIR)/audio_i2s.o \
      $(BUILD_DIR)/axi_dma.o \
      $(BUILD_DIR)/wav.o\
	  $(BUILD_DIR)/mp3.o\

.PHONY: clean

all: $(BINARY) upload_cloud ctrl_bus_test 

ctrl_bus_test: src/ctrl_bus.c
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^

$(BINARY): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) -L$(SDKTARGETSYSROOT)/lib/ -lmp3lame -lm 

$(BUILD_DIR)/main.o: src/main.c include/audio_i2s.h include/wav.h
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

$(BUILD_DIR)/audio_i2s.o: src/audio_i2s.c include/audio_i2s.h include/axi_dma.h include/misc.h
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

$(BUILD_DIR)/axi_dma.o: src/axi_dma.c include/axi_dma.h include/misc.h
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

$(BUILD_DIR)/wav.o: src/wav.c include/wav.h 
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

$(BUILD_DIR)/mp3.o: src/mp3.c include/mp3.h include/wav.h lame-3.100/include/lame.h
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

upload_cloud: src/upload_cloud_homo.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ -lcurl

clean:
	rm -rf $(BINARY) $(BUILD_DIR)/*.o ctrl_bus upload_cloud
