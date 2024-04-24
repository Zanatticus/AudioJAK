#include "audio_player.h"
#include "../hdmi/include/visualizer.h"
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

// Signal handler function for SIGINT (Ctrl+C)
static void signal_handler(int num)
{
  pause_playback = !pause_playback;
  if (pause_playback)
    printf("Paused playback\n");
  else
    printf("Resumed playback\n");
}

// Signal handler function for SIGTSTP (Ctrl+Z)
static void sigtstp_handler(int num)
{
    // Cleanup before terminating the code
    printf("Cleaning up before termination...\n");
    stopVisuals();
    //fclose(fp);
    fclose(fifo);
    snd_pcm_drain(pcm_handle); 
    snd_pcm_close(pcm_handle); 
    i2s_disable_tx();
    exit(0);
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
  if(hdr.NumChannels != 1 && hdr.NumChannels !=2)
    return -EINVAL;

  // calculate starting point and move there
  int x=fseek(fp, 44 + start, SEEK_SET);

  // continuously read frames/samples and use fifo_transmit_word to simulate transmission
  int8_t buf[(hdr.BitsPerSample/8) * hdr.NumChannels];

  if(sample_count == -1) //Play the whole file through
  {
    sample_count = (hdr.ChunkSize + 8) - start;
  }

  *len = sample_count;
  *samples = (uint32_t *)malloc(*len * sizeof(uint32_t));

  int i = 0;
  while (sample_count > 0)
  {
    // read chunk (whole frame)
    x=fread(buf, sizeof(int8_t), ((hdr.BitsPerSample/8)) * hdr.NumChannels, fp);

    if(hdr.NumChannels == 2) //Seperate into two different buffers for left and right, for 2-channel audio
    {
    //IGNORE 2 channel audio for now
      sample_count -= 1;
    }
    else
    {
        (*samples)[(*len)-sample_count] = audio_word_from_buf(hdr, buf); //For the left channel
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

int main(int argc, char** argv) {
    if (argc < 2) {
        pr_usage(argv[0]);
        return 1;
    }

    // handle SIGINT (ctrl-c)
    signal(SIGINT, signal_handler);
    // handle SIGTSTP (ctrl-z)
    signal(SIGTSTP, sigtstp_handler);

    // Initialize ALSA variables
    snd_pcm_hw_params_t *params = NULL;
    i2s_enable_tx();
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Error opening file");
        snd_pcm_close(pcm_handle); // Make sure to close the ALSA PCM device
        return -1;
    }

    fifo = fopen("/dev/zedaudio0", "w");
    if (!fifo) {
        perror("Error opening FIFO");
        fclose(fp);
        snd_pcm_close(pcm_handle); // Make sure to close the ALSA PCM device
        return -1;
    }

    struct wave_header hdr;
    if (read_wave_header(fp, &hdr) < 0) {
        fprintf(stderr, "Failed to read WAV header.\n");
        close(fp);
        snd_pcm_close(pcm_handle); // Close the ALSA PCM device on failure
        return -1;
    }

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

    // Get the number of users on system
    char num_users[50];
    get_num_users(num_users);

    // Get the IP address of the system
    char ip_address[50];
    get_ip_address(ip_address);

    // Visual intialization
    int len;
    uint32_t *waveform = NULL;
    char *wav_file = argv[1];
    getSamples(wav_file, &waveform, &len, -1, 0);
    initVisuals(wav_file, ip_address, num_users, &waveform, len, hdr.SampleRate, 0x3232C8, 0x000000, 0xC0C0C0);

    // Check for flags passed in command-line arguments
    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-p") == 0){
            unsigned int start, end, loop;
            start = 0;
            end = -1;
            loop = 0;
            // Parse command-line arguments
            for (int i = 1; i < argc; i++) {
                char *arg = argv[i];
                if (strncmp(arg, "start=", 6) == 0) {
                    start = atoi(arg + 6);
                } else if (strncmp(arg, "end=", 4) == 0) {
                    end = atoi(arg + 4);
                } else if (strncmp(arg, "loop=", 5) == 0) {
                    loop = atoi(arg + 5);
                }
            }

            // Get start and end time in seconds and convert to samples
            start *= hdr.SampleRate; // Convert to samples
            if (end != -1) {
                end *= hdr.SampleRate; // Convert to samples
            }
            // Print instructions for pausing/resuming playback
            printf("Press Ctrl+C to pause/resume playback\n");
            
            // Call the function play_wave_samples with appropriate arguments
            play_wave_samples(fp, hdr, start, end, loop);
            printf("Finished playing WAV file\n");
        } else if (strcmp(argv[i], "-r") == 0){
            unsigned int loop;
            loop = 0;
            // Parse command-line arguments
            for (int i = 1; i < argc; i++) {
                char *arg = argv[i];
                if (strncmp(arg, "loop=", 5) == 0) {
                    loop = atoi(arg + 5);
                }
            }
            // Print instructions for pausing/resuming playback
            printf("Press Ctrl+C to pause/resume playback\n");
            // Call the function play_wave_samples_reverse with appropriate arguments
            play_wave_samples_reverse(fp, hdr, 0, -1, loop);
            printf("Finished playing WAV file\n");
        }
    }

    // Main menu loop
    int choice;
    while (1) {
        printf("Choose an option:\n");
        printf("1. Play audio file\n");
        printf("2. Play audio file in reverse\n");
        printf("3. Cut audio file\n");
        printf("4. Inverse cut audio file\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                // Get start and end time in seconds and convert to samples
                unsigned int start;
                unsigned int end;
                printf("Enter the start time in seconds: ");
                scanf("%d", &start);
                start *= hdr.SampleRate; // Convert to samples
                printf("Enter the end time in seconds (-1 for end of file): ");
                scanf("%d", &end);
                if (end != -1) {
                    end *= hdr.SampleRate; // Convert to samples
                }
                // Get loop count
                unsigned int loop;
                printf("Enter the number of times to loop (-1 for infinite): ");
                scanf("%d", &loop);
                // Print instructions for pausing/resuming playback
                printf("Press Ctrl+C to pause/resume playback\n");
                play_wave_samples(fp, hdr, start, end, loop);
                printf("Finished playing WAV file\n");
                break;
            case 2:
                // Play the WAV file samples in reverse
                unsigned int loop_2;
                printf("Enter the number of times to loop (-1 for infinite): ");
                scanf("%d", &loop_2);
                play_wave_samples_reverse(fp, hdr, 0, -1, loop_2);
                break;
            case 3:
                // Cut the WAV file based on user input
                const char *input_file = argv[1];
                const char *output_file;
                int file_name_valid = 0;
                // loop to get user input for output_file name until valid one is entered
                while (file_name_valid == 0) {
                    output_file = (char *)malloc(100);
                    printf("Enter the output file name: ");
                    scanf("%s", output_file);
                    printf("Output file name: %s\n", output_file);
                    // Error handling for valid .wav file
                    if (strstr(output_file, ".wav") == NULL) {
                        printf("Output file must be a .wav file\n");
                    } else {
                        file_name_valid = 1;
                    }
                }
                int cursors_confirmed = 0;
                unsigned int start_cut;
                unsigned int end_cut;
                while (cursors_confirmed == 0) {
                    printf("Enter the start time in seconds: ");
                    scanf("%d", &start_cut);
                    start_cut *= hdr.SampleRate; // Convert to samples
                    printf("Enter the end time in seconds (-1 for end of file): ");
                    scanf("%d", &end_cut);
                    if (end_cut != -1) {
                        end_cut *= hdr.SampleRate; // Convert to samples
                    }
                    updateCursorValues(start_cut, start_cut, (end_cut == -1 ? hdr.Subchunk2Size: end_cut));
                    printf("Is this correct? (1 for yes, 0 for no): ");
                    scanf("%d", &cursors_confirmed);
                }
                if (cut_wav_file(input_file, hdr, output_file, start_cut, end_cut) != 0) {
                    fprintf(stderr, "Failed to cut WAV file\n");
                    return 1;
                }
                printf("WAV file cut successfully\n");
                fp = fopen(output_file, "rb");
                read_wave_header(fp, &hdr);
                getSamples(output_file, &waveform, &len, -1, 0);
                get_ip_address(ip_address);
                get_num_users(num_users);
                updateWaveform(output_file, &waveform, len, hdr.SampleRate, ip_address, num_users);
                break;
            case 4:
                // Cut the WAV file based on user input
                const char *input_file_2 = argv[1];
                const char *output_file_2;
                int file_name_valid_2 = 0;
                // loop to get user input for output_file name until valid one is entered
                while (file_name_valid_2 == 0) {
                    output_file_2 = (char *)malloc(100);
                    printf("Enter the output file name: ");
                    scanf("%s", output_file_2);
                    printf("Output file name: %s\n", output_file_2);
                    // Error handling for valid .wav file
                    if (strstr(output_file_2, ".wav") == NULL) {
                        printf("Output file must be a .wav file\n");
                    } else {
                        file_name_valid_2 = 1;
                    }
                }
                int cursors_confirmed_2 = 0;
                unsigned int start_cut_2;
                unsigned int end_cut_2;
                while (cursors_confirmed_2 == 0) {
                    printf("Enter the start time in seconds: ");
                    scanf("%d", &start_cut_2);
                    start_cut_2 *= hdr.SampleRate; // Convert to samples
                    printf("Enter the end time in seconds (-1 for end of file): ");
                    scanf("%d", &end_cut_2);
                    if (end_cut_2 != -1) {
                        end_cut_2 *= hdr.SampleRate; // Convert to samples
                    }
                    updateCursorValues(start_cut_2, start_cut_2, (end_cut_2 == -1 ? hdr.Subchunk2Size: end_cut_2));
                    printf("Is this correct? (1 for yes, 0 for no): ");
                    scanf("%d", &cursors_confirmed_2);
                }
                if (cut_wav_file_inverse(input_file_2, hdr, output_file_2, start_cut_2, end_cut_2) != 0) {
                    fprintf(stderr, "Failed to cut WAV file\n");
                    return 1;
                }
                printf("WAV file cut successfully\n");
                fp = fopen(output_file_2, "rb");
                read_wave_header(fp, &hdr);
                getSamples(output_file_2, &waveform, &len, -1, 0);
                get_ip_address(ip_address);
                get_num_users(num_users);
                updateWaveform(output_file_2, &waveform, len, hdr.SampleRate, ip_address, num_users);
                break;
            case 5:
                // Exit the program
                printf("Exiting program\n");
                stopVisuals();
                fclose(fp);
                fclose(fifo);
                snd_pcm_drain(pcm_handle); // Wait for all pending audio to play
                snd_pcm_close(pcm_handle); // Close the PCM device
                i2s_disable_tx();
                return 0;
            default:
                printf("Invalid choice. Please enter a valid choice.\n");
        }
    }

    // Cleanup and deinitialize
    stopVisuals();
    fclose(fp);
    fclose(fifo);
    snd_pcm_drain(pcm_handle); // Wait for all pending audio to play
    snd_pcm_close(pcm_handle); // Close the PCM device
    i2s_disable_tx();

    return 0;
}