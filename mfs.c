/*
    Name: Esau Noya
    ID: 1001301929
*/

// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <ctype.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

FILE * fp;

char    BS_OEMName[8];
int16_t BPB_BytesPerSec;
int8_t  BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t  BPB_NumFATs;
int16_t BPB_RootEntCnt;
char    BS_VolLab[11];
int32_t BPB_FATSz32;
int32_t BPB_RootClus;

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FirstSectorofCluster = 0;


struct __attribute__((__packed__)) DirectoryEntry 
{
  char      DIR_Name[11];
  uint8_t   DIR_Attr;
  uint8_t   Unused1[8];
  uint16_t  DIR_FirstClusterHigh;
  uint8_t   Unused2[4];
  uint16_t  DIR_FirstClusterLow;
  uint32_t  DIR_FileSize;
};

struct DirectoryEntry dir[16];

int LBAToOffset(int32_t sector)
{
  return ((sector-2)*BPB_BytesPerSec)+(BPB_BytesPerSec*BPB_RsvdSecCnt)
          +(BPB_NumFATs*BPB_FATSz32*BPB_BytesPerSec);
}

int16_t NextLB(uint32_t sector)
{
  uint32_t FATAddress = (BPB_BytesPerSec*BPB_RsvdSecCnt)+(sector*4);
  int16_t val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}

// dont want to repeatedly type this
void fnop()
{
  printf("Error: File system image must be opened first.\n");
}

// compare filenames, returns 1 if filenames match, 0 if not
int compare(char * input, char * IMG_Name)
{
  int match = 0;

  char expanded_name[12];
  memset( expanded_name, ' ', 12 );

  char *token = strtok( input, "." );

  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "." );

  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }

  expanded_name[11] = '\0';

  int i;
  for( i = 0; i < 11; i++ )
  {
    expanded_name[i] = toupper( expanded_name[i] );
  }

  if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
  {
    match = 1;
  }

  return match;
}

int main()
{

  FILE * fp;
  int fileOpen = 0;

  // Root Directory and Cluster Size
  int rootDir, cluster;


  char * filename;
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality
    // int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index] );  
    // }

    if(token[0] == NULL)
    {
      // In case of no input... DO NOTHING!!!
    }
    // "exit" or "quit" terminates program with status 0
    else if(strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)
    {
      if(fileOpen==1)
      {
        fclose(fp);
      }
      exit(0);  
    }

    // open <filename>
    // Open a fat32 image
    else if(strcmp(token[0], "open") == 0)
    {
      if(fileOpen)
      {
        printf("Error: File system image already open.\n");
      }
      else
      {
        fp = fopen(token[1],"r");

        if(fp==NULL)
        {
          printf("Error: File system image not found.\n");
        }
        else
        {
          // get info values
          fseek(fp, 11, SEEK_SET);
          fread(&BPB_BytesPerSec, 2, 1, fp);
          fseek(fp, 13, SEEK_SET);
          fread(&BPB_SecPerClus, 1, 1, fp);
          fseek(fp, 14, SEEK_SET);
          fread(&BPB_RsvdSecCnt, 2, 1, fp);
          fseek(fp, 16, SEEK_SET);
          fread(&BPB_NumFATs, 1, 1, fp);
          fseek(fp, 36, SEEK_SET);
          fread(&BPB_FATSz32, 4, 1, fp);

          // Set the address of the root directory / first cluster
          rootDir = (BPB_NumFATs*BPB_FATSz32*BPB_BytesPerSec) + (BPB_RsvdSecCnt*BPB_BytesPerSec);
          //Set Cluster Size
          cluster = (BPB_SecPerClus * BPB_BytesPerSec);

          fseek(fp, rootDir, SEEK_SET);
          fread(&dir[0], 16, sizeof(struct DirectoryEntry), fp);        

          fileOpen = 1;
          filename = token[1];
        }
      }
    }

    // close
    // Close the fat 32 image.
    else if(strcmp(token[0], "close") == 0)
    {
      if(fileOpen == 0)
      {
        printf("Error: File system not open\n");
      }
      else
      {
        fclose(fp);
        fileOpen = 0;
        filename = NULL;
      }
    }

    // info
    // Prints out information about file system in both hexadecimal and base 10
    // BPB_BytesPerSec, BPB_SecPerClus, BPB_RsvdSecCnt, BPB_NumFATS, BPB_FATSz32
    else if(strcmp(token[0], "info") == 0)
    {
      if(fileOpen==0)
      {
        fnop();
      }
      else
      {
        // print hexadecimal and decimal values
        printf("BPB_BytesPerSec : %d\n", BPB_BytesPerSec);
        printf("BPB_BytesPerSec : %x\n\n", BPB_BytesPerSec);
        printf("BPB_SecPerClus : %d\n", BPB_SecPerClus);
        printf("BPB_SecPerClus : %x\n\n", BPB_SecPerClus);
        printf("BPB_RsvdSecCnt : %d\n", BPB_RsvdSecCnt);
        printf("BPB_RsvdSecCnt : %x\n\n", BPB_RsvdSecCnt);
        printf("BPB_NumFATS : %d\n", BPB_NumFATs);
        printf("BPB_NumFATS : %x\n\n", BPB_NumFATs);
        printf("BPB_FATSz32 : %d\n", BPB_FATSz32);
        printf("BPB_FATSz32 : %x\n\n", BPB_FATSz32);
      }
    }

    // stat <filename> or <directory name>
    // This command shall print the attributes and starting cluster number of the file or 
    // directory name. If the parameter is a directory name then the size shall be 0.
    else if(strcmp(token[0], "stat") == 0)
    {
      if(fileOpen==0)
      {
        fnop();
      }
      else
      {
        int i;
        int fileFound = 0;
        if(token[1] != NULL)
        {
          for(i = 0; i < 16; i++)
          {

            char input[12];
            memset(input, 0, 12);
            strncpy(input, token[1], 12);
            char file[12];
            memset(file, 0, 12);
            strncpy(file, dir[i].DIR_Name, 12);

            if(compare(input, file))
            {
              printf("Attribute\tSize\tStarting Cluster Number\n");
              printf("%d\t\t%d\t%d\n",
                    dir[i].DIR_Attr, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
              fileFound = 1;
            }
          }
        }
        // If the file or directory does not exist then your program shall output 
        // “Error: File not found”.
        if(fileFound == 0)
        {
          printf("Error: File not found\n");
        }

      }
    }

    // cd <directory>
    // This command shall change the current working directory to the given directory.
    // Your program shall support relative paths, e.g cd ../name and absolute paths.
    else if(strcmp(token[0], "cd") == 0)
    {
      if(fileOpen==0)
      {
        fnop();
      }
      else
      {
        // input is "cd" go to root
        if(token[1] == NULL)
        {
         fseek(fp, rootDir, SEEK_SET);
         fread(&dir[0], 16, sizeof(struct DirectoryEntry), fp);
        }
      }
    }

    // ls
    // Lists the directory contents. Your program shall support listing “.” and
    // “..” . Your program shall not list deleted files or system volume names.
    else if(strcmp(token[0], "ls") == 0)
    {
      if(fileOpen==0)
      {
        fnop();
      }
      else
      {
        int i;
        
        for(i = 0; i < 16; i++)
        {
          // print files and subdirectories which are not deleted
          if(dir[i].DIR_Name[0] != (char)0xe5 && (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20))
          {
            printf("%.11s\n", dir[i].DIR_Name);
          }
        }
      }
    }

    // get <filename>
    // This command shall retrieve the file from the FAT 32 image and place
    // it in your current working directory. If the file or directory does 
    // not exist then your program shall output “Error: File not found”.
    else if(strcmp(token[0], "get") == 0)
    {
      if(fileOpen==0)
      {
        fnop();
      }
      else
      {
        printf("Error: get not yet implemented\n");
      }
    }

    // read <filename> <position> <number of bytes>
    // Reads from the given file at the position, in bytes, specified 
    // by the position parameter and output the number of bytes specified
    else if(strcmp(token[0], "read") == 0)
    {
      if(fileOpen==0)
      {
        fnop();
      }
      else
      {
        printf("Error: read not yet implemented\n");
      }
    }

    // invalid command entered
    else
    {
      printf("Error: Command %s not found.\n", token[0]);
    }

    free( working_root );

  }
  return 0;
}
