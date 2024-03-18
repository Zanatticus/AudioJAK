#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <unistd.h>


#define SND_CARD "default"
FILE* fifo;

// NOTE use sizes from STDINT
// NOTE verify data alignment!
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


int i2s_enable_tx(void)
{
  // enable the TX portion of the I2S Controller
  // by writing the character "1" to /sys/devices/soc0/amba_pl/77600000.axi_i2s_adi/tx_enabled
  // returns 0 on success or negative value otherwise
  FILE* fp = fopen("/sys/devices/soc0/amba_pl/77600000.axi_i2s_adi/tx_enabled", "w");
  if (fp == NULL)
    {
      return -1;
    }
  fprintf(fp, "1");
  if (ferror(fp))
    {
      fclose(fp);
      return -1;
    }
  fclose(fp);

  return 0;
}

int i2s_disable_tx(void)
{
  // disable the TX portion of the I2S Controller
  // by writing the character "0" to /sys/devices/soc0/amba_pl/77600000.axi_i2s_adi/tx_enabled
  // returns 0 on success or negative value otherwise
  FILE* fp = fopen("/sys/devices/soc0/amba_pl/77600000.axi_i2s_adi/tx_enabled", "w");
  if (fp == NULL)
    {
      return -1;
    }
  fprintf(fp, "0");
  if (ferror(fp))
    {
      fclose(fp);
      return -1;
    }
  fclose(fp);

  return 0;
}


int configure_codec(unsigned int sample_rate, 
                    snd_pcm_format_t format, 
                    snd_pcm_t* handle,
                    snd_pcm_hw_params_t* params)
{
  int err;

  // initialize parameters 
  err = snd_pcm_hw_params_any(handle, params);
  if (err < 0)
  {
      printf("Failed to initialize pcm hw parameters\n");
      return err;
  }

  // set format
  // NOTE: the codec only supports one audio format, this should be constant
  //       and not read from the WAVE file. You must convert properly to this 
  //       format, regardless of the format in your WAVE file 
  //       (bits per sample and alignment).
  err = snd_pcm_hw_params_set_format(handle, params, format);
  if (err < 0)
  {
      printf("Sample format not available: %s\n", snd_strerror(err));
      return err;
  }

  // set channel count
  err = snd_pcm_hw_params_set_channels(handle, params, 2);
  if (err < 0)
  {
      printf("Channels count (2) not available: %s\n", snd_strerror(err));
      return err;
  }

  // set sample rate
  err = snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, 0);
  if (err < 0)
  {
      printf("Rate %iHz not available: %s\n", sample_rate, snd_strerror(err));
      return err;
  }

  // write parameters to device
  err = snd_pcm_hw_params(handle, params);
  if (err < 0)
  {
      printf("Unable to set hw params: %s\n", snd_strerror(err));
      return err;
  }

  return 0;
}

void pr_usage(char* pname)
{
  printf("usage: %s WAV_FILE\n", pname);
}

/* @brief Read WAVE header
   @param fp file pointer
   @param dest destination struct
   @return 0 on success, < 0 on error */
int read_wave_header(FILE* fp, struct wave_header* dest)
{
  if (!dest || !fp)
    {
      return -ENOENT;
    }

  // NOTE do not assume file pointer is at its starting point
  // read header
  fread(dest, sizeof(struct wave_header), 1, fp);
  printf("sizeof read wave_header: %zu\n", sizeof(*dest));
  printf("ChunkID: %s\n", dest->ChunkID);
  printf("ChunkSize: %u\n", dest->ChunkSize);
  printf("Format: %s\n", dest->Format);
  printf("Subchunk1ID: %s\n", dest->Subchunk1ID);
  printf("Subchunk1Size: %u\n", dest->Subchunk1Size);
  printf("AudioFormat: %u\n", dest->AudioFormat);
  printf("NumChannels: %u\n", dest->NumChannels);
  printf("SampleRate: %u\n", dest->SampleRate);
  printf("ByteRate: %u\n", dest->ByteRate);
  printf("BlockAlign: %u\n", dest->BlockAlign);
  printf("BitsPerSample: %u\n", dest->BitsPerSample);
  printf("Subchunk2ID: %s\n", dest->Subchunk2ID);
  printf("Subchunk2Size: %u\n", dest->Subchunk2Size);

  return 0;
}

/* @brief Parse WAVE header and print parameters
   @param hdr a struct wave_header variable
   @return 0 on success, < 0 on error or if not WAVE file*/
int parse_wave_header(struct wave_header hdr)
{
  // verify that this is a RIFF file header
  assert(strncmp(hdr.ChunkID, "RIFF", 4) == 0);
  // verify that this is WAVE file
  assert(strncmp(hdr.Format, "WAVE", 4) == 0);
  // verify that this is PCM file
  assert(hdr.AudioFormat == 1);

  return 0;
}

/* @brief Build a 32-bit audio word from a buffer
   @param hdr WAVE header
   @param buf a byte array
   @return 32-bit word */
uint32_t audio_word_from_buf(struct wave_header hdr, uint8_t* buf)
{
    uint32_t audio_word = 0;
    switch (hdr.BitsPerSample)
    {
        case 8: // LSB, shift left to align
            audio_word = ((uint32_t)buf[0] - 127) << 24;
            break;
        case 16: // Two bytes, need to shift left to align
            audio_word = ((uint32_t)buf[1] << 8 | buf[0]) << 8;
            break;
        case 24: // Already 24 bits, just reorder and align
            audio_word = (uint32_t)buf[2] << 16 | (uint32_t)buf[1] << 8 | buf[0];
            break;
        default:
            printf("Unsupported BPS: %d\n", hdr.BitsPerSample);
            break;
    }
    return audio_word;
}

/* @brief Transmit a word (put into FIFO)
   @param word a 32-bit word */
void fifo_transmit_word(uint32_t word)
{
    // fwrite() implementation
    int ret = fwrite(&word, 1, sizeof(uint32_t), fifo);
    if (ret != sizeof(uint32_t))
    {
      perror("Failed to write word to FIFO\n");
    }

    // // write() implementation
    // int ret = write(fileno(fifo), &word, sizeof(uint32_t));
    // if (ret != sizeof(uint32_t))
    // {
    //   perror("Failed to write word to FIFO\n");
    // }
}

/* @brief Play sound samples
   @param fp file pointer
   @param hdr WAVE header
   @param sample_count how many samples to play or -1 plays to end of file
   @param start starting point in file for playing
   @return 0 if successful, < 0 otherwise */
int play_wave_samples(FILE* fp, struct wave_header hdr, unsigned int start, unsigned int end)
{
    if (!fp) return -EINVAL;

    int bytesPerSample = hdr.BitsPerSample / 8;
    int frameSize = bytesPerSample * hdr.NumChannels; // Frame size in bytes
    long dataOffset = start * frameSize; // Calculate offset in bytes
    fseek(fp, 44 + dataOffset, SEEK_SET); // Skip header + offset

    uint8_t* buffer = (uint8_t*)malloc(frameSize);
    if (!buffer) return -ENOMEM;

    int samplesPlayed = 0;
    while (samplesPlayed < end - start || end == -1)
    {
        if (fread(buffer, 1, frameSize, fp) < frameSize)
        {
            // End of file or read error
            break;
        }

        // For mono files, duplicate the sample for left and right channels
        if (hdr.NumChannels == 1)
        {
            uint32_t sample = audio_word_from_buf(hdr, buffer);
            fifo_transmit_word(sample); // Left channel
            fifo_transmit_word(sample); // Right channel
        }
        else // For stereo, assume interleaved samples
        {
            for (int channel = 0; channel < 2; ++channel)
            {
                uint32_t sample = audio_word_from_buf(hdr, &buffer[channel]);
                fifo_transmit_word(sample);
            }
        }

        samplesPlayed += (hdr.NumChannels == 1 ? 2 : 1); // Adjusting play count based on mono or stereo
        //printf("samples played: %d\n", samplesPlayed);
    }

    
    free(buffer);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        pr_usage(argv[0]);
        return 1;
    }

    // Initialize ALSA variables
    snd_pcm_t *pcm_handle = NULL;
    snd_pcm_hw_params_t *params = NULL;
    i2s_enable_tx();
    printf("Open WAV file\n");
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Error opening file");
        snd_pcm_close(pcm_handle); // Make sure to close the ALSA PCM device
        return -1;
    }

    printf("Open zedaudio FIFO\n"); 
    fifo = fopen("/dev/zedaudio0", "w");
    if (!fifo) {
        perror("Error opening FIFO");
        fclose(fp);
        snd_pcm_close(pcm_handle); // Make sure to close the ALSA PCM device
        return -1;
    }

    printf("Read WAV header\n");
    struct wave_header hdr;
    if (read_wave_header(fp, &hdr) < 0) {
        fprintf(stderr, "Failed to read WAV header.\n");
        close(fp);
        snd_pcm_close(pcm_handle); // Close the ALSA PCM device on failure
        return -1;
    }

    printf("Parse WAV header\n");
    if (parse_wave_header(hdr) < 0) {
        close(fp);
        snd_pcm_close(pcm_handle); // Close the ALSA PCM device on failure
        return -1;
    }

    // Allocate the hardware parameters object
    snd_pcm_hw_params_alloca(&params);

    // Open the ALSA PCM device for playback.
    int err = snd_pcm_open(&pcm_handle, SND_CARD, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to open PCM device: %s\n", snd_strerror(err));
        return -1;
    }

    // Configure the CODEC for playback
    if (configure_codec(hdr.SampleRate, SND_PCM_FORMAT_S32_LE, pcm_handle, params) < 0) {
        close(fp);
        snd_pcm_close(pcm_handle); // Make sure to close the ALSA PCM device on failure
        return -1; // Exit if configuring the CODEC fails
    }

    // Get user input for the start and end samples to play 
    unsigned int start;
    unsigned int end;
    printf("Enter the number of samples to start playing from: ");
    scanf("%d", &start);
    printf("Enter the number of samples to play (-1 for entire file): ");
    scanf("%d", &end);
    
    // Play the WAV file samples
    if (play_wave_samples(fp, hdr, start, end) < 0) {
        printf(stderr, "Failed to play WAV samples\n");
    }
    printf("Finished playing WAV file\n"); 

    // Cleanup and deinitialize
    fclose(fp);
    fclose(fifo);
    snd_pcm_drain(pcm_handle); // Wait for all pending audio to play
    snd_pcm_close(pcm_handle); // Close the PCM device
    i2s_disable_tx();

    return 0;
}
