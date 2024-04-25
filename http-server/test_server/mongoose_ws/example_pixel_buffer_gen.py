# THIS FILE IS JUST TO MAKE EXAMPLE PIXEL DATA FOR TESTING PURPOSES #

import struct

def write_pixel_data(file_path):
    width = 1920
    height = 1080

    with open(file_path, 'wb') as file:
        for y in range(height):
            for x in range(width):
                # Generate RGB values (24 bits)
                red = x % 256
                green = y % 256
                blue = (x + y) % 256

                # Combine RGB values into a 32-bit pixel value
                pixel_value = (red << 16) | (green << 8) | blue

                # Write the pixel value to the file
                file.write(struct.pack('I', pixel_value))

    print(f"Pixel data written to {file_path}")

# Usage example
write_pixel_data('./mongoose_client/web_root/test_pixels.data')