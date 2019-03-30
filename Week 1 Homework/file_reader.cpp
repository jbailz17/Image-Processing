#include <iostream>
#include <fstream>
#include <array>
#include <vector>

using namespace std;

int main () {
    
    ifstream imageFile;
    imageFile.open("image.bmp", ios::in | ios::binary);

    ofstream newImage;
    newImage.open("newImage.bmp", ios::out | ios::binary);
    
    if (imageFile.is_open() && newImage.is_open()) {

        // Create an array for header data
        static constexpr size_t HEADER_SIZE = 54;
        array<char, HEADER_SIZE> header;

        // Read header data
        imageFile.read(header.data(), header.size());

        // Display important header info
        auto fileSize = *reinterpret_cast<uint32_t *>(&header[2]);
        auto dataOffset = *reinterpret_cast<uint32_t *>(&header[10]);
        auto width = *reinterpret_cast<uint32_t *>(&header[18]);
        auto height = *reinterpret_cast<uint32_t *>(&header[22]);
        auto depth = *reinterpret_cast<uint32_t *>(&header[28]);

        cout << "FileSize: " << fileSize << endl;
        cout << "dataOffset: " << dataOffset << endl;
        cout << "width: " << width << endl;
        cout << "height: " << height << endl;
        cout << "depth: " << depth << endl;

        // Write header data to new image file.
        newImage.write(header.data(), header.size());

        // Create a vector for the rest of the data in the file.
        vector<char> img(dataOffset - HEADER_SIZE);
        // Read the data between the header and the dataOffset.
        imageFile.read(img.data(), img.size());
        newImage.write(img.data(), img.size());

        // Read the pixel data
        auto dataSize = (width * height) * 3;
        img.resize(dataSize);
        imageFile.read(img.data(), img.size());

        char temp = 0;

        // Bitmap stores pixel data as BGR. Use for loop to change it too RGB.
        for (auto i = 0; i < img.size(); i += 3) {
            temp = img[i];
            img[i] = img[i + 2];
            img[i+2] = temp;
            cout << "-> R: " << int(img[i] & 0xff) << " G: " << int(img[i+1] & 0xff) << " B: " << int(img[i+2] & 0xff) << endl;
        }

        newImage.write(img.data(), img.size());

    }

    newImage.close();
    imageFile.close();
    cout << "Image File Closed!" << endl;
    return 0;
}