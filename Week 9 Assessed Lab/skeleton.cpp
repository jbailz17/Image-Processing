#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <string>

class Image {
    private:
        // Initialize class variables 
        std::vector<char> currentImgHeader = {}; 
        std::vector< std::vector<char> > currentImgData = {{}};
        std::vector<int> currentHist = {};
        int32_t dataOffset = 0;
        int32_t height = 0;
        int32_t width = 0;

        bool skeletonComplete = false;

        // Update current stored image data.
        void setImg(std::vector<char> img) {
            dataOffset = *reinterpret_cast<int32_t *>(&img[10]);
            width = *reinterpret_cast<int32_t *>(&img[18]);
            height = *reinterpret_cast<int32_t *>(&img[22]);

            currentImgHeader.resize(dataOffset);
            std::copy(img.begin(), img.begin() + (dataOffset + 1), currentImgHeader.begin());

            int index = 0 + dataOffset;
            currentImgData.resize(height);

            for (int i = 0; i < height; i++) {
                currentImgData[i].resize(width * 3);
                for (int j = 0; j < (width * 3); j++) {
                    currentImgData[i][j] = img[index++];
                }
            }
            setHistogram();
            return;
        }

        // Creates the histogram of an image
        void setHistogram() {
            std::vector< std::vector<char> > img = currentImgData;
            std::vector<int> hist(256);

            for (int i = 0; i < img.size(); i++) {
                for (int j = 0; j < img[i].size(); j += 3){
                    hist[int(img[i][j] & 0xff)]++;
                }
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
            return threshold;
        }

        void addImagePadding() {
            // Create new row to insert at the top and bottom of image.
            std::vector<char> newRow((width * 3), 0x00);

            // Set new height and width in image header.
            width = width + 2;
            height = height + 2;
            currentImgHeader[18] = width;
            currentImgHeader[22] = height;

            // Insert padding rows to the top and bottom of image.
            currentImgData.insert(currentImgData.begin(), newRow);
            currentImgData.push_back(newRow);

            // Insert padding to both sides of the image.
            for (int i = 0; i < currentImgData.size(); i++) {

                currentImgData[i].push_back(0x00);
                currentImgData[i].push_back(0x00);
                currentImgData[i].push_back(0x00);

                currentImgData[i].insert(currentImgData[i].begin(), 0x00);
                currentImgData[i].insert(currentImgData[i].begin() + 1, 0x00);
                currentImgData[i].insert(currentImgData[i].begin() + 2, 0x00);

                while(currentImgData[i].size() % 4 != 0) {
                    currentImgData[i].push_back(0x00);
                }
            }
            return;
        }

        void applyMask(std::vector< std::vector<int> > mask) {

            bool fit = true;

            std::vector< std::vector<char> > img = currentImgData;
            
            // Iterate through each pixel.
            for (int i = 0; i < img.size(); i++) {
                for (int j = 0; j < img[i].size(); j += 3) {
                    
                    // Don't apply mask to the padding around the border.
                    if ( i == 0 || i == (img.size() - 1) || j == 0 || j == (img[i].size() - 1 )){
                        continue;
                    }
                    
                    // Iterate through each mask element.
                    if (int(img[i][j] & 0xff) == 255) {
                        fit = true;
                        for (int x = 0; x < mask.size(); x++) {
                            for (int y = 0; y < mask[x].size(); y ++) {
                                // Find the index for the corresponding pixel value.
                                int index1 = i + (x - 1);
                                int index2 = j + ((y - 1) * 3);
                                if (mask[x][y] != 1) {
                                    // Compare mask value with corresponding pixel value.
                                    if (int(currentImgData[index1][index2] & 0xff) != mask[x][y]){
                                        // If mask doesn't fit, break from loop.
                                        fit = false;
                                        break;
                                    }
                                }

                            }
                            if (!fit) {
                                break;
                            }
                        }
                        // If mask fits set pixel to background.
                        if (fit) {
                            img[i][j] = 0x00;
                            img[i][j+1] = 0x00;
                            img[i][j+2] = 0x00;
                        }
                    }
                }
            }
            // Set current image data to resulting image.
            currentImgData = img;
            return;
        }

        void thinningItr() {
            // Create masks to apply to each individual pixel.
            std::vector< std::vector<int> > structEl1 = {{0,0,0}, 
                                                         {1,255,1},
                                                         {255,255,255}};

            std::vector< std::vector<int> > structEl2 = {{1,0,0},
                                                         {255,255,0},
                                                         {1,255,1}};

            std::vector< std::vector<int> > structEl3 = {{255,1,0}, 
                                                         {255,255,0},
                                                         {255,1,0}};
                                                         
            std::vector< std::vector<int> > structEl4 = {{1,255,1},
                                                         {255,255,0},
                                                         {1,0,0}};

            std::vector< std::vector<int> > structEl5 = {{255,255,255}, 
                                                         {1,255,1},
                                                         {0,0,0}};
                                                         
            std::vector< std::vector<int> > structEl6 = {{1,255,1},
                                                         {0,255,255},
                                                         {0,0,1}};

            std::vector< std::vector<int> > structEl7 = {{0,1,255}, 
                                                         {0,255,255},
                                                         {0,1,255}};
                                                         
            std::vector< std::vector<int> > structEl8 = {{0,0,1},
                                                         {0,255,255},
                                                         {1,255,1}};

            // Create an original copy of the image.
            std::vector< std::vector<char> > original = currentImgData;
            
            // Apply all the mask to each pixel.
            applyMask(structEl1);
            applyMask(structEl2);
            applyMask(structEl3);
            applyMask(structEl4);
            applyMask(structEl5);
            applyMask(structEl6);
            applyMask(structEl7);
            applyMask(structEl8);

            // If the orignal image is the same as the new image set skeleton to complete.
            if (original == currentImgData) {
                skeletonComplete = true;
            }
            return;
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
            
            std::cout << "original width: " << width << " : " << width*4 << std::endl;

            // Read remaining image file.
            auto dataSize = (dataOffset + ((width * height) * 4));
            std::vector<char> img(dataSize);
            
            imageFile.seekg(0, std::ios::beg);
            imageFile.read(img.data(), img.size());
            
            imageFile.close();

            setImg(img);
            return img;
        }

        // Writes a bitmap image to a specified file name.
        void writeFile(std::string filename) {
            std::ofstream newImage;
            newImage.open(filename, std::ios::out | std::ios::binary);
            
            // Ensure file was opened successfully.
            if(!newImage.is_open()) {
                std::cout << "Unable to write to file." << std::endl;
                return;
            }

            auto dataSize = (dataOffset + ((width * height) * 4));
            std::vector<char> newImg(dataSize);

            std::copy(currentImgHeader.begin(), currentImgHeader.end(), newImg.begin());

            int index = 0 + dataOffset;
            std::vector< std::vector<char> > img = currentImgData;
            for (int i = 0; i < img.size(); i++){
                for (int j = 0; j < img[i].size(); j++){
                    newImg[index++] = img[i][j];
                }
            }
            
            // Write image data to the file.
            // std::cout << "Size: " << currentImgData.size() * currentImgData[0].size() << " : " << (((height * width) * 3)) << std::endl;
            newImage.write(newImg.data(), newImg.size());
            newImage.close();
            return;
        }

    public:
        // Create a greyscale version of the bitmap image.
        void createGrayscale(std::string filename) {
            
            openFile(filename);
            
            std::vector< std::vector<char> > img = currentImgData;

            // Ensure data was loaded.
            if (img.size() == 0) {
                return;
            }

            // Transform pixels to their greyscale values.
            for (int i = 0; i < img.size(); i++) {
                for (int j = 0; j < img[i].size(); j += 3) {
                    int grayValue = (int(img[i][j] & 0xff) * 0.0722) +
                                (int(img[i][j+1] & 0xff) * 0.7152) +
                                (int(img[i][j+2] & 0xff) * 0.2126);
                    img[i][j] = grayValue;
                    img[i][j+1] = grayValue;
                    img[i][j+2] = grayValue;
                }
            }
            
            // Write the greyscale image to "grayscale.bmp" and update the currently stored image.
            currentImgData = img;
            writeFile("grayscale.bmp");
            std::cout << "Grayscale Image created. " << std::endl;
            return;
        }

        // Create a binary version of the currently sotred image.
        void createBinary() {
            // Retrieve threshold value from otsuThreshold().
            int threshold = otsuThreshold();
            std::vector< std::vector<char> > img = currentImgData;

            // Set binary values based on threshold value.
            for(int i = 0; i < img.size(); i++) {
                for(int j = 0; j < img[i].size(); j += 3){
                    if (int(img[i][j] & 0xff) <= threshold) {
                        img[i][j] = 0x00;
                        img[i][j+1] = 0x00;
                        img[i][j+2] = 0x00;
                    } else {
                        img[i][j] = 0xff;
                        img[i][j+1] = 0xff;
                        img[i][j+2] = 0xff;
                    }
                }
            }

            // Write image data to "binary.bmp".
            currentImgData = img;
            writeFile("binary.bmp");
            std::cout << "Binary image created.\n";
            return;
        }

        // Create a skeleton version of the currently stored binary image.
        void createSkeleton() {
            std::cout << "Creating skeleton...\n";
            // Add paddinng around the border of the image.
            addImagePadding();
            do {
                // Thin the image until a skeleton is created.
                thinningItr();
            }
            while (skeletonComplete == false);
            // Write the resulting image to "skeleton.bmp".
            writeFile("skeleton.bmp");
            std::cout << "Skeleton created.\n";
            return;
        }
};

// Main function
int main() {
    Image skeletonImg;
    std::string fName = "";

    // Retrieve image file name.
    std::cout << "Enter image file name: ";
    std::getline(std::cin, fName);

    // Create images in correct order.
    skeletonImg.createGrayscale(fName);
    skeletonImg.createBinary();
    skeletonImg.createSkeleton();
    return 0;
}