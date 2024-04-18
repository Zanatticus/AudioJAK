#include "audio_player.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <alsa/pcm.h>
#include <unistd.h>
#include <sys/signal.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

snd_pcm_t *pcm_handle = NULL;
FILE* fifo = NULL;

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
}

/* @brief Play sound samples
   @param fp file pointer
   @param hdr WAVE header
   @param sample_count how many samples to play or -1 plays to end of file
   @param start starting point in file for playing
   @return 0 if successful, < 0 otherwise */
int play_wave_samples(FILE* fp, struct wave_header hdr, unsigned int start, unsigned int end, unsigned int loop)
{
    if (!fp) return -EINVAL;

    int bytesPerSample = hdr.BitsPerSample / 8;
    int frameSize = bytesPerSample * hdr.NumChannels; // Frame size in bytes
    long dataOffset = start * frameSize; // Calculate offset in bytes
    fseek(fp, 44 + dataOffset, SEEK_SET); // Skip header + offset

    uint8_t* buffer = (uint8_t*)malloc(frameSize);
    if (!buffer) return -ENOMEM;

    int samplesPlayed = start;
    int loopCount = 0;
    int total_seconds_played = start / hdr.SampleRate;

    // Print starting point timestamp
    printf("Current timestamp: ");
    print_time(total_seconds_played);

    while (1)
    {
        // Check if playback is paused
        if (pause_playback) {
            usleep(100000);
            continue;
        }

        if (fread(buffer, 1, frameSize, fp) < frameSize || (end != -1 && samplesPlayed >= end))
        {
            if (loopCount < loop || loop == -1)
            {
                fseek(fp, 44 + dataOffset, SEEK_SET); // Go back to start of data
                loopCount++;
                total_seconds_played = start / hdr.SampleRate;
                samplesPlayed = start;
                // Print starting point timestamp
                printf("Current timestamp: ");
                print_time(total_seconds_played);
                continue;
            }
            else
            {
                // End of file or read error
                printf("End of file or read error\n");
                break;
            }
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
        if (samplesPlayed % hdr.SampleRate == 0) {
            total_seconds_played += 1;
            printf("Current timestamp: ");
            print_time(total_seconds_played);
        }
    }
    free(buffer);
    return 0;
}

// play_wave_samples() function modified to play in reverse
int play_wave_samples_reverse(FILE* fp, struct wave_header hdr, unsigned int start, unsigned int end, unsigned int loop)
{
    if (!fp) return -EINVAL;

    int bytesPerSample = hdr.BitsPerSample / 8;
    int frameSize = bytesPerSample * hdr.NumChannels; // Frame size in bytes
    long dataOffset = start * frameSize; // Calculate offset in bytes
    fseek(fp, 44 + dataOffset, SEEK_SET); // Skip header + offset

    uint8_t* buffer = (uint8_t*)malloc(frameSize);
    if (!buffer) return -ENOMEM;

    // Move to the end of the file
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);

    // Calculate the total number of frames in the data section
    long totalFrames = hdr.Subchunk2Size / frameSize;

    // Set the initial position to the last frame
    long currentPosition = totalFrames - 1;

    int samplesPlayed = 0;
    int loopCount = 0;
    int total_seconds_played = hdr.Subchunk2Size / hdr.ByteRate;

    // Print starting point timestamp
    printf("Current timestamp: ");
    print_time(total_seconds_played);

    while (1)
    {
        // Check if playback is paused
        if (pause_playback) {
            usleep(100000);
            continue;
        }

        // Check if we've reached the end of the data section
        if (currentPosition < 0) {
            if (loopCount < loop || loop == -1) {
                currentPosition = totalFrames - 1; // Go back to the last frame
                loopCount++;
                total_seconds_played = hdr.Subchunk2Size / hdr.ByteRate;
                samplesPlayed = start;
                // Print starting point timestamp
                printf("Current timestamp: ");
                print_time(total_seconds_played);
                continue;
            } else {
                // End of file or read error
                printf("End of file or read error\n");
                break;
            }
        }

        // Set the file pointer position to the current frame
        fseek(fp, 44 + currentPosition * frameSize, SEEK_SET);

        // Read the audio data
        if (fread(buffer, 1, frameSize, fp) < frameSize || (end != -1 && samplesPlayed <= end)) {
            if (loopCount < loop || loop == -1) {
                currentPosition = totalFrames - 1; // Go back to the last frame
                loopCount++;
                total_seconds_played = start / hdr.SampleRate;
                samplesPlayed = start;
                continue;
            } else {
                // End of file or read error
                printf("End of file or read error\n");
                break;
            }
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

        // Update play count and timestamp
        samplesPlayed += (hdr.NumChannels == 1 ? 2 : 1); // Adjusting play count based on mono or stereo
        if (samplesPlayed % hdr.SampleRate == 0) {
            total_seconds_played -= 1;
            printf("Current timestamp: ");
            print_time(total_seconds_played);
        }

        // Move to the previous frame
        currentPosition--;
    }
    free(buffer);
    return 0;
}


// function to print minutes and seconds 
void print_time(int total_seconds){
  int minutes = total_seconds / 60;
  int seconds = total_seconds % 60;
  if (seconds < 10 && minutes < 10){
    printf("0%d:0%d\n", minutes, seconds);
  }
  else if (minutes < 10){
    printf("0%d:%d\n", minutes, seconds);
  }
  else if (seconds < 10){
    printf("%d:0%d\n", minutes, seconds);
  }
  else {
    printf("%d:%d\n", minutes, seconds);
  }
}

// Cut WAV file from start to end samples and copy to output_file
int cut_wav_file(const char *input_file, struct wave_header hdr, const char *output_file, unsigned int start, unsigned int end) {
    FILE *input_fp, *output_fp;

    // Open input WAV file for reading
    input_fp = fopen(input_file, "rb");
    if (!input_fp) {
        perror("Error opening input file");
        return -1;
    }

    // Open output WAV file for writing
    output_fp = fopen(output_file, "wb");
    if (!output_fp) {
        perror("Error opening output file");
        fclose(input_fp);
        return -1;
    }

    // Calculate header size
    size_t header_size = sizeof(hdr);

    // Write header from input file to output file
    char buffer[1024];
    size_t bytes_written = 0;
    size_t bytes_to_copy = header_size;
    size_t bytes_read;
    while (bytes_to_copy > 0 && (bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
        size_t bytes_written_current = fwrite(buffer, 1, bytes_to_copy < bytes_read ? bytes_to_copy : bytes_read, output_fp);
        if (bytes_written_current != bytes_to_copy) {
            perror("Error writing WAV header");
            fclose(input_fp);
            fclose(output_fp);
            return -1;
        }
        bytes_written += bytes_written_current;
        bytes_to_copy -= bytes_written_current;
    }

    // Calculate start and end positions in bytes
    int bytesPerSample = hdr.BitsPerSample / 8;
    int frameSize = bytesPerSample * hdr.NumChannels; // Frame size in bytes
    long start_pos = header_size + (start * frameSize); // Calculate offset in bytes
    long end_pos = end == -1 ? -1 : header_size + (end * frameSize); // Calculate offset in bytes

    // Seek to start position in input file
    if (fseek(input_fp, start_pos, SEEK_SET) != 0) {
        perror("Error seeking in input file");
        fclose(input_fp);
        fclose(output_fp);
        return -1;
    }

    // Copy data from input file to output file
    bytes_to_copy = (end_pos == -1) ? -1 : end_pos - start_pos;
    while ((end_pos == -1 || ftell(input_fp) < end_pos) && (bytes_read = fread(buffer, 1, frameSize, input_fp)) > 0) {
        size_t bytes_written_current = fwrite(buffer, 1, bytes_to_copy == -1 ? bytes_read : (bytes_to_copy < bytes_read ? bytes_to_copy : bytes_read), output_fp);
        if (bytes_written_current != bytes_read) {
            perror("Error writing to output file");
            fclose(input_fp);
            fclose(output_fp);
            return -1;
        }
        if (bytes_to_copy != -1) {
            bytes_to_copy -= bytes_written_current;
        }
    }

    // Close files
    fclose(input_fp);
    fclose(output_fp);

    return 0;
}

// Cut WAV file from start of file to start pos, and from end pos to end of file and copy to output_file
int cut_wav_file_inverse(const char *input_file, struct wave_header hdr, const char *output_file, unsigned int start, unsigned int end) {
    FILE *input_fp, *output_fp;

    // Open input WAV file for reading
    input_fp = fopen(input_file, "rb");
    if (!input_fp) {
        perror("Error opening input file");
        return -1;
    }

    // Open output WAV file for writing
    output_fp = fopen(output_file, "wb");
    if (!output_fp) {
        perror("Error opening output file");
        fclose(input_fp);
        return -1;
    }

    // Calculate header size
    size_t header_size = sizeof(hdr);

    // Write header from input file to output file
    char buffer[1024];
    size_t bytes_written = 0;
    size_t bytes_to_copy = header_size;
    size_t bytes_read;
    while (bytes_to_copy > 0 && (bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
        size_t bytes_written_current = fwrite(buffer, 1, bytes_to_copy < bytes_read ? bytes_to_copy : bytes_read, output_fp);
        if (bytes_written_current != bytes_to_copy) {
            perror("Error writing WAV header");
            fclose(input_fp);
            fclose(output_fp);
            return -1;
        }
        bytes_written += bytes_written_current;
        bytes_to_copy -= bytes_written_current;
    }

    // Calculate start and end positions in bytes
    int bytesPerSample = hdr.BitsPerSample / 8;
    int frameSize = bytesPerSample * hdr.NumChannels; // Frame size in bytes
    long start_pos = header_size + (start * frameSize); // Calculate start position in bytes
    long end_pos = header_size + (end * frameSize); // Calculate end position in bytes

    // Seek to start position in input file
    if (fseek(input_fp, header_size, SEEK_SET) != 0) {
        perror("Error seeking in input file");
        fclose(input_fp);
        fclose(output_fp);
        return -1;
    }

    // Copy data from beginning of file to start position
    bytes_to_copy = start_pos - header_size;
    while (bytes_to_copy > 0 && (bytes_read = fread(buffer, 1, bytes_to_copy < sizeof(buffer) ? bytes_to_copy : sizeof(buffer), input_fp)) > 0) {
        size_t bytes_written_current = fwrite(buffer, 1, bytes_read, output_fp);
        if (bytes_written_current != bytes_read) {
            perror("Error writing to output file");
            fclose(input_fp);
            fclose(output_fp);
            return -1;
        }
        bytes_to_copy -= bytes_read;
    }

    // Seek to end position in input file
    if (fseek(input_fp, end_pos, SEEK_SET) != 0) {
        perror("Error seeking in input file");
        fclose(input_fp);
        fclose(output_fp);
        return -1;
    }

    // Copy data from end position to end of file
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
        size_t bytes_written_current = fwrite(buffer, 1, bytes_read, output_fp);
        if (bytes_written_current != bytes_read) {
            perror("Error writing to output file");
            fclose(input_fp);
            fclose(output_fp);
            return -1;
        }
    }

    // Close files
    fclose(input_fp);
    fclose(output_fp);

    return 0;
}

// function to get number of users on the system
int get_num_users() {
    FILE* fp = popen("who | wc -l", "r");
    if (!fp) {
        perror("Error opening pipe");
        return -1;
    }

    int num_users;
    if (fscanf(fp, "%d", &num_users) != 1) {
        perror("Error reading number of users");
        pclose(fp);
        return -1;
    }

    pclose(fp);
    return num_users;
}

// function to get IP address of the system
void get_ip_address(char *ip_address) {
    struct ifaddrs *ifap, *ifa;

    if (getifaddrs(&ifap) == -1) {
        perror("Error getting interface addresses");
        strcpy(ip_address, "Unknown");
        return;
    }

    // Traverse the linked list of interface addresses
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
        // Check for IPv4 address and skip loopback interface
        if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip_address, INET_ADDRSTRLEN);
            freeifaddrs(ifap);
            return;
        }
    }

    // No suitable interface found
    strcpy(ip_address, "Unknown");
    freeifaddrs(ifap);
}