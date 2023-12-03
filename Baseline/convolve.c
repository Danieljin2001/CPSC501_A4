#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
//Baseline



char* INPUT_FILE_PATH;
char* IR_FILE_PATH;
char* OUTPUT_FILE_PATH;





//inspo from: https://www.learnc.net/c-tutorial/c-file-exists/
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


int main(int argc, char* argv[])
{
   checkArguments(argc, argv);

   return 0;
}