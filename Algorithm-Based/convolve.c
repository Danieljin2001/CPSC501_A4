#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#define SWAP(a,b)  tempr=(a);(a)=(b);(b)=tempr


//Algorithm-based
//inspo from D2L: CPSC 501 L01 > Table of Contents > Tutorials > Kimiya - T03/T04 > CPSC501_F23_FFT_convolution_overlap-add

//Struct for Wav headers (used to store wav haeader data)
//to hold all data up until the end of subchunk1
typedef struct {
   char chunkId[4];
   int chunkSize;
   char format[4];
   
   char subchunk1Id[4];
   int subchunk1Size;
   short audioFormat;
   short numChannels;
   int sampleRate;
   int byteRate;
   short blockAlign;
   short bitsPerSample;
} WavHeader;


//file paths received from command arguments
char* INPUT_FILE_PATH;
char* IR_FILE_PATH;
char* OUTPUT_FILE_PATH;

//short arrays to hold audio data
short* INPUT_AUDIO_DATA;
short* IR_AUDIO_DATA;
short* OUTPUT_AUDIO_DATA;

//audio data in bytes
int INPUT_SIZE;
int IR_SIZE;
int OUTPUT_SIZE;

//initialize headers
WavHeader INPUT_HEADER;
WavHeader IR_HEADER;
WavHeader OUTPUT_HEADER;

//inspo from: https://www.learnc.net/c-tutorial/c-file-exists/
//Function checks if file exists
bool file_exists(char *filename)
{
    FILE *fp = fopen(filename, "r");
    bool exist = false;
    if (fp != NULL)
    {
        exist = true;
        fclose(fp); // close the file
    }
    return exist;
}

//Function checks if arguments are valid (correct number of arguments and correct file extension, .wav)
void checkArguments(int count, char* arg[]){
   if (count != 4){
      printf("Please input in the correct amount of command line arguments");
      printf("\nExample: a.exe inputfile.wav IRfile.wav outputfile.wav");
      exit(0);
   }
   for (int i = 1; i < count; i++) {
      if (i == 1){
         INPUT_FILE_PATH = arg[i];
      } else if (i == 2) {
         IR_FILE_PATH = arg[i];
      } else {
         OUTPUT_FILE_PATH = arg[i];
      }

      if(i < 3 && !file_exists(arg[i])) {
         printf("File %s doesn't exist.", arg[i]);
         exit(0);
      }

      if(i == 3){
         int length = strlen(arg[i]);
         if(!(arg[i][length-1] == 'v' && arg[i][length-2] == 'a' && arg[i][length-3] == 'w' && arg[i][length-4] == '.')){
            printf("Output file %s must be in .wav format.", arg[i]);
            exit(0);
         }
      }
   }
}

//Function to read the .wav file header and its audio data. inputType = 0 to read sample audio, anything else to read impulse response audio
void readTone(int inputType) {
   FILE * fileStream;
   if(inputType == 0){ //input
      fileStream = fopen(INPUT_FILE_PATH, "rb");
   } else { //IR
      fileStream = fopen(IR_FILE_PATH, "rb");
   }

   WavHeader header;

   fread(&header, sizeof(header), 1 , fileStream);

   
   if (header.subchunk1Size != 16){
        int remainder = header.subchunk1Size -16;
        char randomVar[remainder];
        fread(randomVar, remainder, 1, fileStream);
    }

   char subchunk2Id[4];
   int subchunk2Size; // an integer is 4 bytes
   fread(&subchunk2Id, sizeof(subchunk2Id), 1, fileStream);
   fread(&subchunk2Size, sizeof(subchunk2Size), 1, fileStream);

   
   int num_samples = subchunk2Size / (header.bitsPerSample / 8);
   size_t data_size = subchunk2Size;

   if(inputType == 0){
      INPUT_HEADER = header;
      INPUT_SIZE = data_size;
      INPUT_AUDIO_DATA = malloc(data_size); // allocate a bunch of space to hold all of the audio data
      fread(INPUT_AUDIO_DATA, sizeof(short), data_size/sizeof(short), fileStream);


   } else {
      IR_HEADER = header;
      IR_SIZE = data_size;
      IR_AUDIO_DATA = malloc(data_size); // allocate a bunch of space to hold all of the audio data
      fread(IR_AUDIO_DATA, sizeof(short), data_size/sizeof(short), fileStream);
   }
   

   // Close the files
   fclose(fileStream);
}

// Function to convert a short to one float in the range -1 to 1
double shortToDouble(short value) {
    return value / 32768.0;
}

//  The four1 FFT from Numerical Recipes in C,
//  p. 507 - 508.
//  Note:  changed float data types to double.
//  nn must be a power of 2, and use +1 for
//  isign for an FFT, and -1 for the Inverse FFT.
//  The data is complex, so the array size must be
//  nn*2. This code assumes the array starts
//  at index 1, not 0, so subtract 1 when
//  calling the routine (see main() below).
void four1(double data[], int nn, int isign)
{
    unsigned long n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;

    n = nn << 1;
    j = 1;

    for (i = 1; i < n; i += 2) {
	if (j > i) {
	    SWAP(data[j], data[i]);
	    SWAP(data[j+1], data[i+1]);
	}
	m = nn;
	while (m >= 2 && j > m) {
	    j -= m;
	    m >>= 1;
	}
	j += m;
    }

    mmax = 2;
    while (n > mmax) {
	istep = mmax << 1;
	theta = isign * (6.28318530717959 / mmax);
	wtemp = sin(0.5 * theta);
	wpr = -2.0 * wtemp * wtemp;
	wpi = sin(theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 1; m < mmax; m += 2) {
	    for (i = m; i <= n; i += istep) {
		j = i + mmax;
		tempr = wr * data[j] - wi * data[j+1];
		tempi = wr * data[j+1] + wi * data[j];
		data[j] = data[i] - tempr;
		data[j+1] = data[i+1] - tempi;
		data[i] += tempr;
		data[i+1] += tempi;
	    }
	    wr = (wtemp = wr) * wpr - wi * wpi + wr;
	    wi = wi * wpr + wtemp * wpi + wi;
	}
	mmax = istep;
    }
    
}





// Function to pad zeros to the input array to make its length M
void pad_zeros_to(double *arr, int current_length, int M) {
    int padding = M - current_length;
    for (int i = 0; i < padding; ++i) {
        arr[current_length + i] = 0.0;
    }
}

void convolution(double *x, int K, double *h, double *y) {
    // Perform the DFT
    for (int k = 0, nn = 0; k < K; k++, nn += 2)
    {
	    y[nn] = ((x[nn] * h[nn]) - (x[nn+1] * h [nn+1]));
	    y[nn+1] = ((x[nn] * h[nn+1]) + (x[nn+1] * h[nn]));
	}
    
}

//Function to write the convolved audio into an output .wav file. 
void writeTone(double y[], int K){
   //create header for output file
   OUTPUT_HEADER.chunkId[0] = 'R';
   OUTPUT_HEADER.chunkId[1] = 'I';
   OUTPUT_HEADER.chunkId[2] = 'F';
   OUTPUT_HEADER.chunkId[3] = 'F';
 

   OUTPUT_HEADER.chunkSize = 36 + K*sizeof(short); //36 + Subchunk2Size
   OUTPUT_HEADER.format[0] = 'W';
   OUTPUT_HEADER.format[1] = 'A';
   OUTPUT_HEADER.format[2] = 'V';
   OUTPUT_HEADER.format[3] = 'E';
   OUTPUT_HEADER.subchunk1Id[0] = 'f';
   OUTPUT_HEADER.subchunk1Id[1] = 'm';
   OUTPUT_HEADER.subchunk1Id[2] = 't';
   OUTPUT_HEADER.subchunk1Id[3] = ' ';
   OUTPUT_HEADER.subchunk1Size = 16;
   OUTPUT_HEADER.audioFormat = 1;
   OUTPUT_HEADER.numChannels = 1;
   OUTPUT_HEADER.sampleRate = 44100;
   OUTPUT_HEADER.byteRate = 88200; //SampleRate * NumChannels * BitsPerSample/8
   OUTPUT_HEADER.blockAlign = 2;
   OUTPUT_HEADER.bitsPerSample = 16;
   
   FILE* fileStream = fopen(OUTPUT_FILE_PATH, "wb");
   fwrite(&OUTPUT_HEADER, sizeof(OUTPUT_HEADER), 1, fileStream);

   char subchunk2Id[4] = {'d','a','t','a'};
   int subchunk2Size = K*sizeof(short); // an integer is 4 bytes
   fwrite(&subchunk2Id, sizeof(subchunk2Id), 1, fileStream);
   fwrite(&subchunk2Size, sizeof(subchunk2Size), 1, fileStream);

   double largestNum = 0.0;
   for(int i = 0; i < K*2; i= i+2){
      double value = y[i];
      if(value < 0){
         value = value * -1;
      }
      if (value > largestNum){
         largestNum = value;
      }
   }

   double scaleTo = 32768.0 / largestNum;
   short data;
   for (size_t i = 0; i < K*2; i=i+2) {
         data = (short) (y[i] *  scaleTo);
         fwrite(&data, sizeof(data), 1, fileStream);
   }


   fclose(fileStream);
   printf("Finished creating %s", OUTPUT_FILE_PATH);

}

int main(int argc, char* argv[])
{
    clock_t time;
    time = clock(); //start timer

    checkArguments(argc, argv);
    readTone(0); //read input file
    readTone(1); //read IR file

    int N = INPUT_SIZE / sizeof(short); //x[n] length
    int M = IR_SIZE / sizeof(short); //h[m] length

    int largerLength; //size of X[] and H[]
    int smallerLength;

    if (N>M){
        largerLength = N;
        smallerLength = M;
    } else{
        largerLength = M;
        smallerLength = N;
    }
   

    int K = 2*largerLength; // compute the next highest power of 2 of 32-bit K
    //from: https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
    K--;
    K |= K >> 1;
    K |= K >> 2;
    K |= K >> 4;
    K |= K >> 8;
    K |= K >> 16;
    K++;
    
    double *x = (double *)calloc(K*2, sizeof(double));
    double *h = (double *)calloc(K*2, sizeof(double));
    double *y = (double *)calloc(K*2, sizeof(double));


    //setting real and imaginary numbers for the array
    for (size_t i = 0; i < 2*largerLength; i++) {
        if(i % 2 == 0){
            x[i] = shortToDouble(INPUT_AUDIO_DATA[i/2]);
        } else {
            x[i] = 0.0;
        }
        if ( i < 2*smallerLength){
            if(i % 2 == 0){
                h[i] = shortToDouble(IR_AUDIO_DATA[i/2]);
            } else {
                h[i] = 0.0;
            }
        }
    }


    //pad zeros
    pad_zeros_to(x, 2*N, K*2);
    pad_zeros_to(h, 2*M, K*2);
   
    four1(x-1, K, 1);
 
    four1(h-1, K, 1);

    convolution(x, K, h, y); //complex multiplication

    four1(y-1, K, -1);

    writeTone(y, N+M-1);

    time = clock() - time; //end timer
    double timeTaken = ((double)time)/CLOCKS_PER_SEC;
    printf("\nThis application using FFT took %f seconds to execute \n", timeTaken); 


    return 0;
}