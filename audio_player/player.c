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
    //fclose(fp);
    fclose(fifo);
    snd_pcm_drain(pcm_handle); 
    snd_pcm_close(pcm_handle); 
    i2s_disable_tx();
    exit(0);
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

    // Print the number of users on system
    int num_users = get_num_users();
    if (num_users < 0) {
        fprintf(stderr, "Failed to get number of users\n");
        return 1;
    }
    printf("Number of users on system: %d\n", num_users);

    // Print the IP address of the system
    char ip_address[16];
    get_ip_address(ip_address);
    printf("IP address of the system: %s\n", ip_address);

    // Print the duration of the WAV file 
    int total_seconds = hdr.Subchunk2Size / hdr.ByteRate;
    printf("Duration of WAV file: ");
    print_time(total_seconds);

    const char *input_file = argv[1];
    const char *output_file = "output.wav";
    unsigned int start_cut;
    printf("Enter the start time in seconds: ");
    scanf("%d", &start_cut);
    start_cut *= hdr.SampleRate; // Convert to samples
    unsigned int end_cut;
    printf("Enter the end time in seconds (-1 for end of file): ");
    scanf("%d", &end_cut);
    if (end_cut != -1) {
        end_cut *= hdr.SampleRate; // Convert to samples
    }
    if (cut_wav_file_inverse(input_file, hdr, output_file, start_cut, end_cut) != 0) {
        fprintf(stderr, "Failed to cut WAV file\n");
        return 1;
    }

    printf("WAV file cut successfully\n");
    
    // Ask user if they want to play the file in reverse
    char reverse;
    printf("Do you want to play the file in reverse? (y/n): ");
    scanf(" %c", &reverse);

    // Get user input for start and end time in seconds and convert to samples
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
    
    // Get user input for loop count
    unsigned int loop;
    printf("Enter the number of times to loop (-1 for infinite): ");
    scanf("%d", &loop);

    // Print instructions for pausing/resuming playback
    printf("Press Ctrl+C to pause/resume playback\n");
    
    // Play the WAV file samples normally or reversed based on user input
    if (reverse == 'y') {
        play_wave_samples_reverse(fp, hdr, start, end, loop);
    } else {
        play_wave_samples(fp, hdr, start, end, loop);
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