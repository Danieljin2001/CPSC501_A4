/*
- test checks if headers and audio is the same between two files
- when checking audio data theres a margin of error allowed (delta=1)
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* FILE_PATH_1;
char* FILE_PATH_2;

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


typedef struct {
    char subchunk2Id[4];
    int subchunk2size;
} SubchunkHeader;

WavHeader HEADER_1;
WavHeader HEADER_2;
SubchunkHeader SUBHEADER_1;
SubchunkHeader SUBHEADER_2;

short* AUDIO_DATA_1;
short* AUDIO_DATA_2;

size_t SIZE_1;
size_t SIZE_2;

short delta;

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

void checkArguments(int count, char* arg[]){
   if (count != 3){
      printf("Please input in the correct amount of command line arguments");
      printf("\nExample: a.exe first.wav second.wav");
      exit(0);
   }
   for (int i = 1; i < count; i++) {
    if (i == 1){
        FILE_PATH_1 = arg[i];
    } else if (i == 2) {
        FILE_PATH_2 = arg[i];
    } 

    if(!file_exists(arg[i])) {
        printf("File %s doesn't exist.", arg[i]);
        exit(0);
    }


    int length = strlen(arg[i]);
    if(!(arg[i][length-1] == 'v' && arg[i][length-2] == 'a' && arg[i][length-3] == 'w' && arg[i][length-4] == '.')){
        printf("Output file %s must be in .wav format.", arg[i]);
        exit(0);
    }

   }
}


//Function to read the .wav file header and its audio data. inputType = 0 to read sample audio, anything else to read impulse response audio
void readTone() {
    FILE *fileStream1 = fopen(FILE_PATH_1, "rb");
    FILE *fileStream2 = fopen(FILE_PATH_2, "rb");


    fread(&HEADER_1, sizeof(HEADER_1), 1 , fileStream1);
    fread(&HEADER_2, sizeof(HEADER_2), 1 , fileStream2);


    if (HEADER_1.subchunk1Size != 16){
        int remainder = HEADER_1.subchunk1Size -16;
        char randomVar[remainder];
        fread(randomVar, remainder, 1, fileStream1);
    }


    if (HEADER_2.subchunk1Size != 16){
        int remainder = HEADER_2.subchunk1Size -16;
        char randomVar[remainder];
        fread(randomVar, remainder, 1, fileStream2);
    }


    fread(&SUBHEADER_1, sizeof(SUBHEADER_1), 1, fileStream1);
    fread(&SUBHEADER_2, sizeof(SUBHEADER_2), 1, fileStream2);

    SIZE_1 =  SUBHEADER_1.subchunk2size;
    AUDIO_DATA_1 = malloc(SIZE_1);
    fread(AUDIO_DATA_1, sizeof(short), SIZE_1/sizeof(short), fileStream1);


    SIZE_2 =  SUBHEADER_2.subchunk2size;
    AUDIO_DATA_2 = malloc(SIZE_2);
    fread(AUDIO_DATA_2, sizeof(short), SIZE_2/sizeof(short), fileStream2);

   // Close the file
   fclose(fileStream1);
   fclose(fileStream2);

}

int main(int argc, char* argv[])
{
    delta = 1; //difference that is allowed
    checkArguments(argc, argv);
    readTone();

    int headerErrorCount = 0;
    int subheaderErrorCount = 0;
    int dataErrorCount = 0;



    FILE *fp = fopen("TestResults.txt", "w");
    fprintf(fp, "File One: %s\n", FILE_PATH_1);
    fprintf(fp, "File Two: %s\n", FILE_PATH_2);
    fprintf(fp, "Delta for error: %d\n", delta);
    
    fprintf(fp, "--------------------------------------------STARTING TEST--------------------------------------------");

    if(HEADER_1.chunkId[0] != HEADER_2.chunkId[0] || HEADER_1.chunkId[1] != HEADER_2.chunkId[1] || HEADER_1.chunkId[2] != HEADER_2.chunkId[2] || HEADER_1.chunkId[3] != HEADER_2.chunkId[3]){
        fprintf(fp, "\nchunkId does not match.\nOne: %s\nTwo: %s", HEADER_1.chunkId, HEADER_2.chunkId);
        headerErrorCount++;
    }

    if(HEADER_1.chunkSize != HEADER_2.chunkSize){
        fprintf(fp, "\nchunkSize does not match.\nOne: %d\nTwo: %d", HEADER_1.chunkSize, HEADER_2.chunkSize);
        headerErrorCount++;
    }

    if(HEADER_1.format[0] != HEADER_2.format[0] || HEADER_1.format[1] != HEADER_2.format[1] || HEADER_1.format[2] != HEADER_2.format[2] || HEADER_1.format[3] != HEADER_2.format[3]){
        fprintf(fp, "\nformat does not match.\nOne: %s\nTwo: %s", HEADER_1.format, HEADER_2.format);
        headerErrorCount++;
    }

    if(HEADER_1.subchunk1Id[0] != HEADER_2.subchunk1Id[0] || HEADER_1.subchunk1Id[1] != HEADER_2.subchunk1Id[1] || HEADER_1.subchunk1Id[2] != HEADER_2.subchunk1Id[2] || HEADER_1.subchunk1Id[3] != HEADER_2.subchunk1Id[3]){
        fprintf(fp, "\nsubchunk1Id does not match.\nOne: %s\nTwo: %s", HEADER_1.subchunk1Id, HEADER_2.subchunk1Id);
        headerErrorCount++;
    }

    if(HEADER_1.subchunk1Size != HEADER_2.subchunk1Size){
        fprintf(fp, "\nsubchunk1Size does not match.\nOne: %d\nTwo: %d", HEADER_1.subchunk1Size, HEADER_2.subchunk1Size);
        headerErrorCount++;
    }

    if(HEADER_1.audioFormat != HEADER_2.audioFormat){
        fprintf(fp, "\naudioFormat does not match.\nOne: %d\nTwo: %d", HEADER_1.audioFormat, HEADER_2.audioFormat);
        headerErrorCount++;
    }

    if(HEADER_1.numChannels != HEADER_2.numChannels){
        fprintf(fp, "\numChannels does not match.\nOne: %d\nTwo: %d", HEADER_1.numChannels, HEADER_2.numChannels);
        headerErrorCount++;
    }

    if(HEADER_1.sampleRate != HEADER_2.sampleRate){
        fprintf(fp, "\nsampleRate does not match.\nOne: %d\nTwo: %d", HEADER_1.sampleRate, HEADER_2.sampleRate);
        headerErrorCount++;
    }

    if(HEADER_1.byteRate != HEADER_2.byteRate){
        fprintf(fp, "\nbyteRate does not match.\nOne: %d\nTwo: %d", HEADER_1.byteRate, HEADER_2.byteRate);
        headerErrorCount++;
    }

    if(HEADER_1.blockAlign != HEADER_2.blockAlign){
        fprintf(fp, "\nblockAlign does not match.\nOne: %d\nTwo: %d", HEADER_1.blockAlign, HEADER_2.blockAlign);
        headerErrorCount++;
    }

    if(HEADER_1.bitsPerSample != HEADER_2.bitsPerSample){
        fprintf(fp, "\nbitsPerSample does not match.\nOne: %d\nTwo: %d", HEADER_1.bitsPerSample, HEADER_2.bitsPerSample);
        headerErrorCount++;
    }

    if(SUBHEADER_1.subchunk2Id[0] != SUBHEADER_2.subchunk2Id[0] || SUBHEADER_1.subchunk2Id[1] != SUBHEADER_2.subchunk2Id[1] || SUBHEADER_1.subchunk2Id[2] != SUBHEADER_2.subchunk2Id[2] || SUBHEADER_1.subchunk2Id[3] != SUBHEADER_2.subchunk2Id[3]){
        fprintf(fp, "\nsubchunk2Id does not match.\nOne: %s\nTwo: %s", SUBHEADER_1.subchunk2Id, SUBHEADER_2.subchunk2Id);
        subheaderErrorCount++;
    }

    if(SUBHEADER_1.subchunk2size != SUBHEADER_2.subchunk2size){
        fprintf(fp, "\nsubchunk2size does not match.\nOne: %d\nTwo: %d", SUBHEADER_1.subchunk2size, SUBHEADER_2.subchunk2size);
        subheaderErrorCount++;
    }

    if(SIZE_1 == SIZE_2){
        int difference;
        for(size_t i = 0; i < SIZE_1/sizeof(short); i++){
            difference = AUDIO_DATA_1[i] - AUDIO_DATA_2[i];
            
            //make difference value absolute
            if(difference < 0) {
                difference = difference*-1;
            }

            if(difference > delta){
                fprintf(fp, "\nAudio data does not match at %d. \n\tOne: %d\n\tTwo: %d", i, AUDIO_DATA_1[i], AUDIO_DATA_2[i]);
                dataErrorCount++;
            }
        }


    } else{
        fprintf(fp, "\nSIZE does not match.");
    }
    
    fprintf(fp, "\n--------------------------------------------FINISHED TEST--------------------------------------------");

    fprintf(fp, "\nHeader errors: %d", headerErrorCount);
    fprintf(fp, "\nSubheader errors: %d", subheaderErrorCount);
    fprintf(fp, "\nData errors: %d", dataErrorCount);
    fclose(fp);


    return 0;

}