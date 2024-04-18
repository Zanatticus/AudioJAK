//#include "include/audiovisuals.h"
//#include "include/hdmi.h"
#include "include/visualizer.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/signal.h>
#include <unistd.h>

#include <errno.h>
#include <sys/stat.h>

volatile int stop = 0;

void sigint_handler(int signum) { 
    printf("\nStopping....\n");
    stop = 1;
}

struct wave_header
{
  uint32_t chunkID;
  uint32_t chunkSize;
  uint32_t format;

  uint32_t subchunk1ID;
  uint32_t subchunk1Size;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t SampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;

  uint32_t subchunk2ID;
  uint32_t subchunk2Size;
};

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

  rewind(fp); //Resets the file pointer

  struct stat st;
  fstat(fileno(fp), &st);
  if(!(S_ISREG(st.st_mode) && st.st_size > 44))
    return -1;


  //read header
  int x = 0;
  x=fseek(fp, 0, SEEK_SET);
  x=fread(&(dest->chunkID), sizeof(uint32_t), 1, fp);  
  x=fseek(fp, 4, SEEK_SET);
  x=fread(&(dest->chunkSize), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 8, SEEK_SET);
  x=fread(&(dest->format), sizeof(uint32_t), 1, fp);

  x=fseek(fp, 12, SEEK_SET);
  x=fread(&(dest->subchunk1ID), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 16, SEEK_SET);
  x=fread(&(dest->subchunk1Size), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 20, SEEK_SET);
  x=fread(&(dest->audioFormat), sizeof(uint16_t), 1, fp);
  x=fseek(fp, 22, SEEK_SET);
  x=fread(&(dest->numChannels), sizeof(uint16_t), 1, fp);
  x=fseek(fp, 24, SEEK_SET);
  x=fread(&(dest->SampleRate), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 28, SEEK_SET);
  x=fread(&(dest->byteRate), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 32, SEEK_SET);
  x=fread(&(dest->blockAlign), sizeof(uint16_t), 1, fp);
  x=fseek(fp, 34, SEEK_SET);
  x=fread(&(dest->bitsPerSample), sizeof(uint16_t), 1, fp);

  x=fseek(fp, 36, SEEK_SET);
  x=fread(&(dest->subchunk2ID), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 40, SEEK_SET);
  x=fread(&(dest->subchunk2Size), sizeof(uint32_t), 1, fp);

  if(x == -1)
    printf("There was an error.\n");

  //printf("Expected file size: %d, Actual file size: %ld\n", (dest->chunkSize + 8), st.st_size);

  return !((dest->chunkSize + 8) == st.st_size);
}

/* @brief Parse WAVE header and print parameters
   @param hdr a struct wave_header variable
   @return 0 on success, < 0 on error or if not WAVE file*/
int parse_wave_header(struct wave_header hdr)
{
  // verify that this is a RIFF file header
  if((hdr.chunkID != (0x46464952)))
    return -1;
  //verify that this is WAVE file
  if(hdr.format != (0x45564157))
    return -1;

  if(hdr.audioFormat != 1)
    return -1;

  //print out information: number of channels, sample rate, total size
  //printf("Number of channels: %d, Sample Rate: %dHz, Total Size: %d bytes\n", hdr.numChannels, hdr.SampleRate, (hdr.chunkSize + 8));

  return 0;
}

uint32_t audio_word_from_buf(struct wave_header hdr, int8_t* buf)
{
  //build word depending on bits per sample, etc
  uint32_t audio_word = 0;
  //printf("BPS: %d\n", hdr.bitsPerSample);

  for(int i = 0 ; i < hdr.bitsPerSample/8; i++)
    audio_word |= (uint32_t)((buf[i] + 127) << (8*((24/hdr.bitsPerSample-1) - i)));
  //for(int i = 0 ; i < 24/hdr.bitsPerSample; i++)
    //audio_word |= (uint32_t)buf[i] << (8*((24/hdr.bitsPerSample-1) - i));
  return audio_word << 8;
}

int loadAudioSamples(FILE* fp,
                      struct wave_header hdr,
                      int sample_count,
                      unsigned int start, uint32_t **samples, int *len)
{
  if (!fp)
    {
      return -EINVAL;
    }

  // NOTE reject if number of channels is not 1 or 2
  if(hdr.numChannels != 1 && hdr.numChannels !=2)
    return -EINVAL;

  // calculate starting point and move there
  int x=fseek(fp, 44 + start, SEEK_SET);

  // continuously read frames/samples and use fifo_transmit_word to
  //      simulate transmission
  int8_t buf[(hdr.bitsPerSample/8) * hdr.numChannels];
  //int8_t lbuf[(hdr.bitsPerSample/8)];
  //int8_t rbuf[(hdr.bitsPerSample/8)];

  if(sample_count == -1) //Play the whole file through
  {
    sample_count = (hdr.chunkSize + 8) - start;
  }

  *len = sample_count;
  *samples = (uint32_t *)malloc(*len * sizeof(uint32_t));

  int i = 0;
  while (sample_count > 0)
  {
    // read chunk (whole frame)
    x=fread(buf, sizeof(int8_t), ((hdr.bitsPerSample/8)) * hdr.numChannels, fp);

    if(hdr.numChannels == 2) //Seperate into two different buffers for left and right, for 2-channel audio
    {
      //for(int i = 0; i < (hdr.bitsPerSample/8) * hdr.numChannels; i+=2)
      //{
        //lbuf[i] = buf[i];
        //rbuf[i] = buf[i + 1];
      //}

    //IGNORE 2 channel audio for now
      //fifo_transmit_word(audio_word_from_buf(hdr, lbuf));
      //fifo_transmit_word(audio_word_from_buf(hdr, rbuf));
      sample_count -= 1;
    }
    else
    {
        (*samples)[(*len)-sample_count] = audio_word_from_buf(hdr, buf); //For the left channel
      //fifo_transmit_word(audio_word_from_buf(hdr, buf)); //For the right channel
    }
    
    sample_count -= 1;
    i += 2;
  }

  return 0 * x;
}

void getSamples(char *filename, uint32_t **samples, int *len, int sample_count, int start)
{
    int err;
    FILE* fp;
    struct wave_header hdr;

    // open file
    fp = fopen(filename, "r");
    /*
    if(())
    {
        fclose(fp);
        printf("File %s does not exist.\n", filename);
    }*/


    // read file header
    if(read_wave_header(fp, &hdr) != 0)
    {
        printf("Error: Incorrect File Format\n");
    }

    // parse file header, verify that is wave
    if(parse_wave_header(hdr) != 0)
    {
        printf("Error: Incorrect WAV format\n");
    }

    err = loadAudioSamples(fp, hdr, sample_count, start, samples, len);
    if(err != 0)
        printf("There was an error!\n");
}

void write_uint32_array_to_file(const char *filename, uint32_t *array, size_t size) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    size_t elements_written = fwrite(array, sizeof(uint32_t), size, file);
    if (elements_written != size) {
        fprintf(stderr, "Error writing to file\n");
    }

    fclose(file);
}

int main()
{
  //Signal handler to stop the program safely
  signal(SIGINT, sigint_handler);


  int len;
  uint32_t *waveform = NULL;

  //Function defined above to get the samples from a .wav file as an uint32_t*, not included in the visualizer library
  getSamples("testsounds.wav", &waveform, &len, -1, 0);


  //printf("Screen W: %d\nScreen H: %d\n", getScreenWidth(), getScreenHeight());
  
  //This function Initializes the screen and the waveform. Needs to be called before everything else.
  initVisuals("testsounds.wav", &waveform, len, 44100, 0x3232C8, 0x000000, 0xC0C0C0);

  int loop_start = 230000;
  int loop_end = len - 50000;
  int i = loop_start;

  while(!stop)
  {
    //This function is non-blocking and updates the cursor values. The values passed are the sample number being played, starting at 0
    updateCursorValues(i, loop_start, loop_end);

    //This function updates the waveform if the samples are updated. Should not be called frequently since it is slow. Leaving commented because it is just to show what a call would look like
    //updateWaveform(&waveform, len, 44100);

    i+=10;
    if(i >= loop_end)
    {
      i = loop_start;
      updateWaveform(&waveform, len, 44100);
    }
    
  }

  //Needs to be called to stop the HDMI display and other threads
  stopVisuals();
}