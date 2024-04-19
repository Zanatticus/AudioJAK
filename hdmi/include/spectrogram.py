import numpy as np
import struct
import matplotlib
matplotlib.use("Agg", force=True)
import matplotlib.pyplot as plt

# Disable the cache
matplotlib.rcParams['agg.path.chunksize'] = 0
plt.rcParams['agg.path.chunksize'] = 0

from scipy import signal
from scipy.io import wavfile


def plt_to_pixels(plt):
    from matplotlib.backends.backend_agg import FigureCanvasAgg
    """
    Convert a matplotlib plot (plt) to an array of pixels.

    Parameters:
    - plt: Matplotlib plot object

    Returns:
    - Array of pixels representing the image
    """
    # Get the figure from the current plot
    fig = plt.gcf()
    
    # Create a canvas
    canvas = FigureCanvasAgg(fig)
    
    # Render the figure to the canvas
    canvas.draw()
    
    # Get the width and height of the figure
    width, height = fig.get_size_inches() * fig.get_dpi()
    
    # Convert the canvas to an array of pixels
    pixels = np.frombuffer(canvas.tostring_rgb(), dtype='uint8').reshape(int(height) * int(width), 3)
    
    pixels = [ (((int)(p[0])) << 16) + (((int)(p[1])) << 8) + (((int)(p[2])) << 0) for p in pixels]

    return pixels

def read_uint32_array_from_file(filename):
    """
    Read uint32_t samples from a file and store them in a list.

    Parameters:
    - filename: Name of the file to read from.

    Returns:
    - List containing the uint32_t samples.
    """
    samples = []

    with open(filename, "rb") as file:
        while True:
            # Read 4 bytes from the file
            data = file.read(4)
            if not data:
                break  # Reached end of file
            # Unpack the bytes into a uint32_t value
            sample = struct.unpack('<I', data)[0]
            samples.append(sample)

    return samples#np.array([abs((i>>24) - 127) for i in samples]).astype(np.uint8)

def getclim(log):
    flts = np.sort(np.array([float(i) for i in np.ndarray.flatten(log)]))
    min = np.nanmin(flts[flts != -np.inf])
    
    for i, x in enumerate(flts):
        if(abs(x) == np.inf):
            flts[i] = min
        if(abs(x) < np.inf):
            break
    
    qs = np.quantile(flts, [0, 0.25, 0.5, 0.75, 1])

    for i, arr in enumerate(log):
        for x, item in enumerate(arr):
            if(abs(item) == np.inf):
                log[i][x] = qs[1]
    return qs


def plot_spectrogram(samples, sample_rate, h, w, background_color):

    
    samples = np.array([abs((i>>24) - 127) for i in samples]).astype(np.uint8)


    frequencies, times, spectrogram = signal.spectrogram(samples, sample_rate)

    del samples

    np.seterr(divide = 'ignore')
    spectrogram = np.log(spectrogram)
    np.seterr(divide = 'warn')

    #print("Just logged")
    
    qs = getclim(spectrogram)

    #print("Just climmed")
    
    color = "#" + str(hex(background_color))[2:]
    plt.figure(facecolor=color, figsize=(w/100, h/100), dpi=100)
    #print("figure")
    try:
        plt.pcolormesh(times, frequencies, spectrogram, cmap="inferno")
    except Exception as e:
        print(e)
    
    #print("colormesh")
    plt.ylabel('Frequency [Hz]')
    plt.xlabel('Time [sec]')
    plt.title("Audio Spectrogram")
    plt.clim(qs[1], qs[-1])
    plt.colorbar(label='Intensity (dB)')

    print("Finished plotting")

    pixels = plt_to_pixels(plt)

    #plt.savefig("spec.png", dpi=100)
    #print(pixels)

    plt.cla()
    plt.clf()
    plt.close()

    return pixels

def write_integers_to_file(integers, filepath):
    with open(filepath, 'wb') as file:
        for integer in integers:
            file.write(integer.to_bytes(4, byteorder='little', signed=False))

def plot_spectrogram_filename(filename, h, w, background_color):

    sample_rate, samples = wavfile.read(filename)
    #samples = np.array([abs((i>>24) - 127) for i in samples]).astype(np.uint8)


    frequencies, times, spectrogram = signal.spectrogram(samples, sample_rate)

    del samples

    np.seterr(divide = 'ignore')
    spectrogram = np.log(spectrogram)
    np.seterr(divide = 'warn')

    #print("Just logged")
    
    qs = getclim(spectrogram)

    #print("Just climmed")
    
    color = "#" + str(hex(background_color))[2:]
    plt.figure(facecolor=color, figsize=(w/100, h/100), dpi=100)
    #print("figure")
    try:
        plt.pcolormesh(times, frequencies, spectrogram, cmap="inferno")
    except Exception as e:
        print(e)
    #print("colormesh")
    plt.ylabel('Frequency [Hz]')
    plt.xlabel('Time [sec]')
    plt.title("Audio Spectrogram")
    plt.clim(qs[1], qs[-1])
    plt.colorbar(label='Intensity (dB)')

    ##print("Finished plotting")

    pixels = plt_to_pixels(plt)
    write_integers_to_file(pixels, "spec.data")

    #plt.savefig("spec.png", dpi=100)
    #print(pixels)

    plt.cla()
    plt.clf()
    plt.close()

    return 1#pixels


if __name__ == "__main__":
    # Example usage:
    # Read the audio file
    samples1 = read_uint32_array_from_file("samples.txt")
    #print(samples)
    #exit()
    sample_rate, samples = wavfile.read('testsounds.wav')

    #print(samples1[500:520])
    #print(samples[500:520])
    #print(type(samples), samples, type(samples[500]), sample_rate)
    # Plot the spectrogram
    print("Plotting....")
    plot_spectrogram(samples1, 44100, 700, 1000, 0xFFFF00)
    print("Finished plotting")
    plot_spectrogram(samples1, 44100, 700, 1000, 0xFFFF00)
    print("Plotting..2..")