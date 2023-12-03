#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
//Baseline
//inspo from D2L: CPSC 501 L01 > Table of Contents > Tutorials > Ali - T06/T07 > Week 10 > Week 10 - Session 2 - Updated


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

   // printf("\n\n\nchunk id: %s\n", header.chunkId);
   // printf("chunk size: %d\n", header.chunkSize);
   // printf("format: %s\n", header.format);
   // printf("subchunk1 id: %s\n", header.subchunk1Id);
   // printf("subchunk1 size: %d\n", header.subchunk1Size);
   // printf("audio format: %d\n", header.audioFormat);
   // printf("number of channels: %d\n", header.numChannels);
   // printf("sample rate: %d\n", header.sampleRate);
   // printf("byte rate: %d\n", header.byteRate);
   // printf("block align: %d\n", header.blockAlign);
   // printf("bits per sample: %d\n", header.bitsPerSample);
   
      // the remaining bytes in subchunk1 will be null bytes if there is more than 16
      // so read the junk!
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

   //  printf("subchunk2 size: %d\n", subchunk2Size);
   //  printf("number of samples: %d\n", num_samples);

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
   

   // Close the file
   fclose(fileStream);
}

// Function to convert a short to one float in the range -1 to 1
float shortToFloat(short value) {
    return value / 32768.0f;
}

/*
    The function convolve takes six arguments: 
        Two input arrays x[] and h[], their respective sizes N and M, and an output array y[] with size P.

    The first loop initializes the output array y[] to zero. 
        This is necessary because the convolution operation involves accumulating values in y[].

    The second loop (outer loop) iterates over each element of the input array x[].

    The third loop (inner loop) iterates over each element of the array h[]. 
        For each pair of elements x[n] and h[m], it adds their sum to the corresponding element in y[].
*/
void convolve(float x[], int N, float h[], int M, float y[], int P)
{
    int n,m;

    /* Clear Output Buffer y[] */
    for (n=0; n < P; n++)
    {
        y[n] = 0.0;
    }

    /* Outer Loop: process each input value x[n] in turn */
    for (n=0; n<N; n++){
        /* Inner loop: process x[n] with each sample of h[n] */
        for (m=0; m<M; m++){
            y[n+m] += x[n] * h[m];
        }
    }
}

//Function to write the convolved audio into an output .wav file. 
void writeTone(float y[], int P){
   //create header for output file
   OUTPUT_HEADER.chunkId[0] = 'R';
   OUTPUT_HEADER.chunkId[1] = 'I';
   OUTPUT_HEADER.chunkId[2] = 'F';
   OUTPUT_HEADER.chunkId[3] = 'F';

   OUTPUT_HEADER.chunkSize = 36 + P*sizeof(short); //36 + Subchunk2Size
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

   // printf("OUTEER HEADER STUFF---------------------------------------");
   // printf("\n\n\nchunk id: %s\n", OUTPUT_HEADER.chunkId);
   // printf("chunk size: %d\n", OUTPUT_HEADER.chunkSize);
   // printf("format: %s\n", OUTPUT_HEADER.format);
   // printf("subchunk1 id: %s\n", OUTPUT_HEADER.subchunk1Id);
   // printf("subchunk1 size: %d\n", OUTPUT_HEADER.subchunk1Size);
   // printf("audio format: %d\n", OUTPUT_HEADER.audioFormat);
   // printf("number of channels: %d\n", OUTPUT_HEADER.numChannels);
   // printf("sample rate: %d\n", OUTPUT_HEADER.sampleRate);
   // printf("byte rate: %d\n", OUTPUT_HEADER.byteRate);
   // printf("block align: %d\n", OUTPUT_HEADER.blockAlign);
   // printf("bits per sample: %d\n", OUTPUT_HEADER.bitsPerSample);

   char subchunk2Id[4] = {'d','a','t','a'};
   int subchunk2Size = P*sizeof(short); // an integer is 4 bytes
   fwrite(&subchunk2Id, sizeof(subchunk2Id), 1, fileStream);
   fwrite(&subchunk2Size, sizeof(subchunk2Size), 1, fileStream);

   // printf("subchunk2Id: %s\n", subchunk2Id);
   // printf("subchunk2Size: %d\n", subchunk2Size);

   float largestNum = 0.0f;
   for(int i = 0; i < P; i++){
      float value = y[i];
      if(value < 0){
         value = value * -1;
      }
      if (value > largestNum){
         largestNum = value;
      }
   }

   float scaleTo = 32768.0f / largestNum;


   short data;
   for (int i = 0; i < P; i++){
      data = (short) (y[i] *  scaleTo);
      fwrite(&data, sizeof(data), 1, fileStream);
   }

   fclose(fileStream);
   printf("\nFinished creating %s\n", OUTPUT_FILE_PATH);




}

int main(int argc, char* argv[])
{
   clock_t time;
   time = clock();

   checkArguments(argc, argv);
   readTone(0); //read input file
   readTone(1); //read IR file

   //get array lengths
   int N = INPUT_SIZE / sizeof(short);
   int M = IR_SIZE / sizeof(short);
   int P = N+M-1;

   //create float arrays
   float* x = malloc(N * sizeof(float));
   float* h = malloc(M * sizeof(float));
   float* y = malloc(P * sizeof(float));
   
   //setting up float array for input file
   for (size_t i = 0; i < N; i++) {
      x[i] = shortToFloat(INPUT_AUDIO_DATA[i]);
   }

   for (size_t i = 0; i < M; i++) {
      h[i] = shortToFloat(IR_AUDIO_DATA[i]);
   }

   convolve(x, N, h, M, y, P);
   

   writeTone(y, P);

   time = clock() - time;
   double timeTaken = ((double)time)/CLOCKS_PER_SEC;
   printf("This applicaition using time domain convolution took %f seconds to execute \n", timeTaken); 
   return 0;
   
}