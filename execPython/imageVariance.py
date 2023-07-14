from PIL import Image
import numpy as np

def compute_image_variances(image_path):
    # Open the image
    image = Image.open(image_path)

    # Convert the image to a numpy array
    image_array = np.array(image)

    # Convert the image to the linear RGB color space
    image_linear_rgb = image_array.astype(np.float32) / 255.0

    # Compute the variances for each color channel
    channel_variances = np.var(image_linear_rgb, axis=(0, 1))

    # Compute the luminance channel, using RGB->Luma from Photometric/digital ITU BT.709:
    luminance = 0.2126 * image_linear_rgb[:, :, 0] + 0.7152 * image_linear_rgb[:, :, 1] + 0.0722 * image_linear_rgb[:, :, 2]

    # Compute the variance of the luminance channel
    luminance_variance = np.var(luminance)

    return channel_variances, luminance_variance

# Provide the path to your PPM image file
image_paths = [
    '../docs/assets/appendixD_result_8.png',
    '../docs/assets/appendixD_result_40.png',
    '../docs/assets/appendixD_result_200.png',
    '../docs/assets/appendixD_result_1000.png',
    '../docs/assets/appendixD_result_5k.png',
    '../docs/assets/appendixD_result_25k.png'
]

for path in image_paths:
    # Compute the variances for each channel and luminance
    channel_variances, luminance_variance = compute_image_variances(path)

    # Print the variances for each channel
    print(path+":")
    print("Red channel variance:", channel_variances[0])
    print("Green channel variance:", channel_variances[1])
    print("Blue channel variance:", channel_variances[2])
    print("Luminance variance:", luminance_variance)
