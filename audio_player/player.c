#include "audio_player.h"
// #include "include/visualizer.h"
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

    // Visual intialization
    //int len;
    //uint32_t *waveform = NULL;
    //char *wav_file = argv[1];
    //getSamples(wav_file, &waveform, &len, -1, 0);
    //initVisuals(wav_file, &waveform, len, 44100, 0x3232C8, 0x000000, 0xC0C0C0);

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
                // updateCursorValues(start, end);
                printf("Finished playing WAV file\n");
                break;
            case 2:
                // Play the WAV file samples in reverse
                unsigned int loop_2;
                printf("Enter the number of times to loop (-1 for infinite): ");
                scanf("%d", &loop_2);
                play_wave_samples_reverse(fp, hdr, 0, -1, loop_2);
                // updateCursorValues(start, end);
                break;
            case 3:
                // Cut the WAV file based on user input
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
                if (cut_wav_file(input_file, hdr, output_file, start_cut, end_cut) != 0) {
                    fprintf(stderr, "Failed to cut WAV file\n");
                    return 1;
                }
                printf("WAV file cut successfully\n");
                break;
            case 4:
                // Inverse cut the WAV file based on user input
                const char *input_file_2 = argv[1];
                const char *output_file_2 = "output.wav";
                unsigned int start_cut_2;
                printf("Enter the start time in seconds: ");
                scanf("%d", &start_cut_2);
                start_cut_2 *= hdr.SampleRate; // Convert to samples
                unsigned int end_cut_2;
                printf("Enter the end time in seconds (-1 for end of file): ");
                scanf("%d", &end_cut_2);
                if (end_cut_2 != -1) {
                    end_cut_2 *= hdr.SampleRate; // Convert to samples
                }
                if (cut_wav_file_inverse(input_file_2, hdr, output_file_2, start_cut_2, end_cut_2) != 0) {
                    fprintf(stderr, "Failed to cut WAV file\n");
                    return 1;
                }
                printf("WAV file cut successfully\n");
                break;
            case 5:
                // Exit the program
                printf("Exiting program\n");
                //stopVisuals();
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
    //stopVisuals();
    fclose(fp);
    fclose(fifo);
    snd_pcm_drain(pcm_handle); // Wait for all pending audio to play
    snd_pcm_close(pcm_handle); // Close the PCM device
    i2s_disable_tx();

    return 0;
}