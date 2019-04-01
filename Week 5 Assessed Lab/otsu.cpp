#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <string>

/* Otsu provides functionality to turn a bitmap image into a binary image using the
Otsu threshold method. */
class Otsu {

    private:

        // Initialize class variables  
        std::vector<char> currentImg = {};
        std::vector<int> currentHist = {};
        int32_t dataOffset = 0;
        int32_t height = 0;
        int32_t width = 0;

        // Update current stored image data.
        void updateCurrentImg(std::vector<char> img) {
            currentImg = img;
            dataOffset = *reinterpret_cast<int32_t *>(&img[10]);
            width = *reinterpret_cast<int32_t *>(&img[18]);
            height = *reinterpret_cast<int32_t *>(&img[22]);

            setHistogram(img);
            return;
        }

        // Creates the histogram of an image
        void setHistogram(std::vector<char> img) {
            std::vector<int> hist(256);

            for (int i = dataOffset; i <= img.size(); i+= 3) {
                hist[int(img[i] & 0xff)]++;
            }

            currentHist = hist;

            return;
        }

        // Retrieves a threshold value using Otsu's method.
        int otsuThreshold() {
            
            // Set histogram and total number of pixels.
            std::vector<int> hist = currentHist;
            int total = height * width;

            // Retrieve sum value to compute average foreground value
            double sum = 0;
            for(int i=0; i < 256; i++) {
                sum += i * hist[i];
            }

            // Initialize remaining variables
            double sumBackground = 0;
            long double weightBackground = 0;
            long double weightForeground = 0;

            double varMax = 0;
            int threshold = 0;

            for(int i=0; i < 256; i++) {
                // Histogram value at current threshold added to background weight.
                weightBackground += hist[i];
                // If background weight = 0 continue to next iteration.
                if(weightBackground == 0) {
                    continue;
                }

                // Foreground weight equals all the other pixels not located in the background.
                weightForeground = total - weightBackground;
                // If foreground weight = 0 then break out of the loop.
                if (weightForeground == 0) {
                    break;
                }
                
                // calculate sum to find average background value.
                sumBackground += i * hist[i];

                // Calculate average values.
                double averagerBackground = sumBackground / weightBackground;
                double averageForeground = (sum - sumBackground) / weightForeground;

                // Calculate between variance.
                double varBetween = weightBackground * weightForeground * 
                                    (averagerBackground - averageForeground) *
                                    (averagerBackground - averageForeground);

                /* If between variance is greater than max variance then set new
                threshold value and set max variance to the between variance. */ 
                if (varBetween > varMax) {
                    varMax = varBetween;
                    threshold = i;
                }
            }

            // Return threshold value.
            std::cout << "Threshold: " << threshold << std::endl;
            return threshold;
        }

        // Open bitmap file and read the contents.
        std::vector<char> openFile(std::string filename) {
            std::ifstream imageFile;
            imageFile.open(filename, std::ios::in | std::ios::binary);
            
            // Ensure file was opened succesfully.
            if(!imageFile.is_open()) {
                std::cout << "Unable to open file." << std::endl;
                return {};
            }

            // Read header data.
            static constexpr size_t HEADER_SIZE = 54;
            std::array<char, HEADER_SIZE> header;

            imageFile.read(header.data(), header.size());

            auto dataOffset = *reinterpret_cast<uint32_t *>(&header[10]);
            auto width = *reinterpret_cast<uint32_t *>(&header[18]);
            auto height = *reinterpret_cast<uint32_t *>(&header[22]);
            
            // Read remaining image file.
            auto dataSize = (dataOffset + ((width * height) * 3));
            std::vector<char> img(dataSize);
            
            imageFile.seekg(0, std::ios::beg);
            imageFile.read(img.data(), img.size());
            
            imageFile.close();
            return img;
        }

        // Writes a bitmap image to a specified file name.
        void writeFile(std::vector<char> img, std::string filename) {
            std::ofstream newImage;
            newImage.open(filename, std::ios::out | std::ios::binary);
            
            // Ensure file was opened successfully.
            if(!newImage.is_open()) {
                std::cout << "Unable to write to file." << std::endl;
                return;
            }

            // Write image data to the file.
            newImage.write(img.data(), img.size());
            newImage.close();

            return;
        }

    public:

        // Creat a greyscale version of the bitmap image.
        void createGrayscale(std::string filename) {
            
            std::vector<char> img = openFile(filename);
            
            // Ensure data was loaded.
            if (img.size() == 0) {
                return;
            }

            auto dataOffset = *reinterpret_cast<uint32_t *>(&img[10]);

            // Transform pixels to their greyscale values.
            for (int i = dataOffset; i < img.size(); i += 3) {
                int grayValue = (int(img[i] & 0xff) * 0.0722) +
                                (int(img[i+1] & 0xff) * 0.7152) +
                                (int(img[i+2] & 0xff) * 0.2126);
                img[i] = grayValue;
                img[i+1] = grayValue;
                img[i+2] = grayValue;
            }
            
            // Write the greyscale image to "grayscale.bmp" and update the currently stored image.
            writeFile(img, "grayscale.bmp");
            updateCurrentImg(img);
            std::cout << "Grayscale Image created. " << std::endl;
            return;
        }

        // Create a binary version of the currently sotred image.
        void createBinary() {
            // Retrieve threshold value from otsuThreshold().
            int threshold = otsuThreshold();
            std::vector<char> img = currentImg;

            // Set binary values based on threshold value.
            for(int i = dataOffset; i < img.size(); i += 3) {
                if (int(img[i] & 0xff) <= threshold) {
                    img[i] = 0x00;
                    img[i+1] = 0x00;
                    img[i+2] = 0x00;
                } else {
                    img[i] = 0xff;
                    img[i+1] = 0xff;
                    img[i+2] = 0xff;
                }
            }

            // Write image data to "binary.bmp".
            writeFile(img, "binary.bmp");
            std::cout << "Binary image created.\n";
            return;
        }
};

// Main function.
int main() {
    Otsu thresholdImage;
    std::string fileName = "";
    std::cout << "Input filename: ";
    std::getline(std::cin, fileName);
    thresholdImage.createGrayscale(fileName);
    thresholdImage.createBinary();
    return 0; 
}