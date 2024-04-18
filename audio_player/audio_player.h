#ifndef audio_player_H
#define audio_player_H

#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

#define SND_CARD "default"
//FILE* fifo;
extern FILE* fifo;
extern snd_pcm_t *pcm_handle;
//snd_pcm_t *pcm_handle = NULL;

// stop infinite loop flag
static unsigned char pause_playback = 0;

struct wave_header
{
  // TODO STUDENT: populate this struct correctly
  char ChunkID[4]; // "RIFF"
  uint32_t ChunkSize; // total size of file. 36 + SubChunk2Size
  char Format[4]; // "WAVE"
  char Subchunk1ID[4];// "fmt "
  uint32_t Subchunk1Size; // 16 for PCM. This is the size of the rest of the Subchunk which follows this number.
  uint16_t AudioFormat; // PCM = 1 (linear quantization) values other than 1 indicate some form of compression
  uint16_t NumChannels; // Mono=1 or Stereo=2
  uint32_t SampleRate; // 8000, 44100, etc
  uint32_t ByteRate; // SampleRate * NumChannels * BitsPerSample/8
  uint16_t BlockAlign; // NumChannels * BitsPerSample/8
  uint16_t BitsPerSample; // 8, 16, etc
  char Subchunk2ID[4]; // "data"
  uint32_t Subchunk2Size; // NumSamples * NumChannels * BitsPerSample/8
};

int i2s_enable_tx(void);
int i2s_disable_tx(void);
int configure_codec(unsigned int sample_rate, snd_pcm_format_t format, snd_pcm_t* handle, snd_pcm_hw_params_t* params);
void pr_usage(char* pname);
int read_wave_header(FILE* fp, struct wave_header* dest);
int parse_wave_header(struct wave_header hdr);
uint32_t audio_word_from_buf(struct wave_header hdr, uint8_t* buf);
void fifo_transmit_word(uint32_t word);
int play_wave_samples(FILE* fp, struct wave_header hdr, unsigned int start, unsigned int end, unsigned int loop);
int play_wave_samples_reverse(FILE* fp, struct wave_header hdr, unsigned int start, unsigned int end, unsigned int loop);
void print_time(int total_seconds);
int cut_wav_file(const char *input_file, struct wave_header hdr, const char *output_file, unsigned int start, unsigned int end);
int cut_wav_file_inverse(const char *input_file, struct wave_header hdr, const char *output_file, unsigned int start, unsigned int end);
int get_num_users();
void get_ip_address(char* ip_address);

#endif /* audio_player_H */
