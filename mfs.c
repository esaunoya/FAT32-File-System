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

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

// dont want to repeatedly type this
void fnop()
{
  printf("Error: File system image must be opened first.\n");
}

int main()
{

  FILE * fp;
  int fileOpen = 0;

  // info variables
  int bps, spc, rsc, nf, fz32;
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
          // BPB_BytesPerSec
          fseek(fp, 11, SEEK_SET);
          fread(&bps, 2, 1, fp);
          // BPB_SecPerClus
          fseek(fp, 13, SEEK_SET);
          fread(&spc, 1, 1, fp);
          // BPB_RsvdSecCnt
          fseek(fp, 14, SEEK_SET);
          fread(&rsc, 2, 1, fp);
          // BPB_NumFATS
          fseek(fp, 16, SEEK_SET);
          fread(&nf, 1, 1, fp);
          // BPB_FATSz32
          fseek(fp, 36, SEEK_SET);
          fread(&fz32, 4, 1, fp);

          // Set the address of the root directory / first cluster
          rootDir = (nf * fz32 * bps) + (rsc * bps);
          //Set Cluster Size
          cluster = (spc * bps);

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
        printf("BPB_BytesPerSec : %d\n", bps);
        printf("BPB_BytesPerSec : %x\n\n", bps);
        printf("BPB_SecPerClus : %d\n", spc);
        printf("BPB_SecPerClus : %x\n\n", spc);
        printf("BPB_RsvdSecCnt : %d\n", rsc);
        printf("BPB_RsvdSecCnt : %x\n\n", rsc);
        printf("BPB_NumFATS : %d\n", nf);
        printf("BPB_NumFATS : %x\n\n", nf);
        printf("BPB_FATSz32 : %d\n", fz32);
        printf("BPB_FATSz32 : %x\n\n", fz32);
      }
    }

    // stat <filename> or <directory name>
    // This command shall print the attributes and starting cluster number of the file or 
    // directory name. If the parameter is a directory name then the size shall be 0.
    // If the file or directory does not exist then your program shall output “Error: File not found”.
    else if(strcmp(token[0], "stat") == 0)
    {
      if(fileOpen==0)
      {
        fnop();
      }
      else
      {
        printf("Error: stat not yet implemented\n");
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
        printf("Error: cd not yet implemented\n");
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
        printf("Error: ls not yet implemented\n");
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
