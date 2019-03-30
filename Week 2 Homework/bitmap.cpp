#include <iostream>
#include <fstream>
#include <array>
#include <vector>

using namespace std;

class Bitmap {

    private:

        vector<char> currentImg = {};
        vector<int> currentHist = {};
        int32_t dataOffset;
        int32_t height = 0;
        int32_t width = 0;
        int totalPvalue = 0;

        void updateCurrentImg(vector<char> img) {
            currentImg = img;
            dataOffset = *reinterpret_cast<int32_t *>(&img[10]);
            width = *reinterpret_cast<int32_t *>(&img[18]);
            height = *reinterpret_cast<int32_t *>(&img[22]);

            for(auto i = dataOffset; i < img.size(); i += 3) {
                totalPvalue += img[i];
            }

            setHistogram(img);
            return;
        }

        void setHistogram(vector<char> img) {
            vector<int> hist(256);

            for (int i = dataOffset; i < img.size(); i+= 3) {
                hist[int(img[i] & 0xff)] += 1;
            }

            currentHist = hist;

            return;
        }

        vector<char> openFile(char * filename) {
            ifstream imageFile;
            imageFile.open(filename, ios::in | ios::binary);

            if(!imageFile.is_open()) {
                cout << "Unable to open file." << endl;
                return {};
            }

            static constexpr size_t HEADER_SIZE = 54;
            array<char, HEADER_SIZE> header;

            imageFile.read(header.data(), header.size());

            auto dataOffset = *reinterpret_cast<uint32_t *>(&header[10]);
            auto width = *reinterpret_cast<uint32_t *>(&header[18]);
            auto height = *reinterpret_cast<uint32_t *>(&header[22]);

            auto dataSize = (dataOffset + ((width * height) * 3));
            vector<char> img(dataSize);
            
            imageFile.seekg(0, ios::beg);
            imageFile.read(img.data(), img.size());
            
            imageFile.close();
            return img;

        }

        void writeFile(vector<char> img) {
            ofstream grayscaleImage;
            grayscaleImage.open("grayscale.bmp", ios::out | ios::binary);

            if(!grayscaleImage.is_open()) {
                cout << "Unable to write to file." << endl;
                return;
            }

            grayscaleImage.write(img.data(), img.size());
            grayscaleImage.close();

            return;
        }

    public:

        void saveGrayscale(char * filename) {
            
            vector<char> img = openFile(filename);
            
            if (img.size() == 0) {
                return;
            }

            auto dataOffset = *reinterpret_cast<uint32_t *>(&img[10]);

            for (auto i = dataOffset; i < img.size(); i += 3) {
                int grayValue = (int(img[i] & 0xff) * 0.0722) +
                                (int(img[i+1] & 0xff) * 0.7152) +
                                (int(img[i+2] & 0xff) * 0.2126);
                img[i] = grayValue;
                img[i+1] = grayValue;
                img[i+2] = grayValue;
            }
            
            writeFile(img);
            updateCurrentImg(img);
            cout << "Grayscale Image created. " << endl;
            return;
        }

        void brighten(int amt) {

            vector<char> img = currentImg;

            if (amt >= 100) {
                for (auto i = dataOffset; i < img.size(); i += 3) {
                    img[i] = 0xff;
                    img[i+1] = 0xff;
                    img[i+2] = 0xff;
                }
                writeFile(img);
            } else if (amt <= 0) {
                for (auto i = dataOffset; i < img.size(); i += 3) {
                    img[i] = 0x00;
                    img[i+1] = 0x00;
                    img[i+2] = 0x00;
                }
                writeFile(img);
            } else {
                amt = ((amt * 2.55) * (height * width));
                int expectedTotal = amt;
                int total = totalPvalue;
                while (amt != 0) {
                    amt = ((expectedTotal - total) / (height * width));
                    total = 0;
                    for (auto i = dataOffset; i < img.size(); i += 3) {
                        if ((int(img[i] & 0xff) + amt) > 255) {
                            img[i] = 0xff;
                            img[i+1] = 0xff;
                            img[i+2] = 0xff;
                        } else if ((int(img[i] & 0xff) + amt) < 0){
                            img[i] = 0x00;
                            img[i+1] = 0x00;
                            img[i+2] = 0x00;
                        } else {
                            img[i] += amt;
                            img[i+1] += amt;
                            img[i+2] += amt;
                        }
                        total += int(img[i] & 0xff);
                    }
                }
            }

            writeFile(img);
            setHistogram(img);
            return;

        }

        void clamp(int low, int high) {
            vector<char> img = currentImg;

            for (int i = dataOffset; i < img.size(); i += 3) {
                if ((int(img[i] & 0xff) < low)) {
                    img[i] = low;
                    img[i + 1] = low;
                    img[i + 2] = low;
                } else if ((int(img[i] & 0xff) > high)) {
                    img[i] = high;
                    img[i + 1] = high;
                    img[i + 2] = high;
                }
            }

            writeFile(img);
            setHistogram(img);
            return;
        }

        void intensityWindow(int low, int high) {
            vector<char> img = currentImg;

            for (int i = dataOffset; i < img.size(); i += 3) {
                if ((int(img[i] & 0xff) < low)) {
                    img[i] = 0x00;
                    img[i + 1] = 0x00;
                    img[i + 2] = 0x00;
                } else if ((int(img[i] & 0xff) > high)) {
                    img[i] = 0xff;
                    img[i + 1] = 0xff;
                    img[i + 2] = 0xff;
                } else {
                    double p = double(img[i] & 0xff);
                    int value = 255 * ((p - low) / (high - low));
                    img[i] = value;
                    img[i + 1] = value;
                    img[i + 2] = value;
                }
            }

            writeFile(img);
            setHistogram(img);
            return;
        }

        void getHistogram() {
            int increment = (width * height) / 10;
            int totalValue = 0;
             for (int i = 0; i < currentHist.size(); i += 5) {
                cout << i << " | ";
                totalValue += currentHist[i];
                if (i < 255) {
                    totalValue += currentHist[i+1];
                    totalValue += currentHist[i+2];
                    totalValue += currentHist[i+3];
                    totalValue += currentHist[i+4];
                }
                for(int j = 0; j < totalValue; j += increment){
                    cout << "*";
                }
                cout << endl;
            }
            return;
        }

};

int main() {
    
    Bitmap bmap;
    int selected_option;
    bool quit = false;

    while (quit == false) {
        cout << "Select operation:\n";
        cout << "1. Create grayscale image.\n";
        cout << "2. Adjust brightness.\n";
        cout << "3. Clamp image.\n";
        cout << "4. Window image.\n";
        cout << "5. Get histogram.\n";
        cout << "6. Exit.\n";
        cout << "->: ";
        cin >> selected_option;
        cout << endl;

        switch(selected_option) {
            case 1:
                char fname[100];
                cout << "Enter filename: ";
                cin >> fname;
                bmap.saveGrayscale(fname);
                cout << endl;
                break;
            case 2:
                int brightness;
                cout << "Set Brightness: ";
                cin >> brightness;
                bmap.brighten(brightness);
                cout << endl;
                break; 
            case 3:
                int clow;
                int chigh;
                cout << "Enter lowest value: ";
                cin >> clow;
                cout << "Enter highest value: ";
                cin >> chigh;
                bmap.clamp(clow, chigh);
                cout << endl;
                break;
            case 4:
                int wlow;
                int whigh;
                cout << "Enter lowest value: ";
                cin >> wlow;
                cout << "Enter highest value: ";
                cin >> whigh;
                bmap.intensityWindow(wlow, whigh);
                cout << endl;
                break;
            case 5:
                bmap.getHistogram();
                cout << endl;
                break;
            case 6:
                quit = true;
                break;
        }
    }

    return 0;
}