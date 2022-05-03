#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/////////////////////////////////////////////////////////////////////////////// 
// general settings 
/////////////////////////////////////////////////////////////////////////////// 
// enable the define "FILE_IO" for file I/O
#define FILE_IO

// file for storing settings 
#define SETTINGS_FILENAME   "./settings.txt"    // IMPORTANT: use the same when reading the seting in the image/audio processing programm
/////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////// 



// Quick guide for important options of convert (for more options see: http://www.imagemagick.org/Usage/text/)
// Convert is part of imagemagick, download for Windows or MAC here: http://www.imagemagick.org/script/download.php, for installation on Linux use e.g. "sudo apt-get install imagemagick"
//   size: WIDTHxHEIGHT in pixel
//   canvas: background color or pattern
//   depth: color/grayscale depth (number of bits/pixel)
//   pointsize: font size
//   fill: font color
//   font: font used for the text (available fonts differ in Linux and Windows, see all available fonts on your system with "convert -list font")
//   gravity: location of text e.g. northwest means top-left (see all available directions with "convert -list gravity")
//   annotate +x+y @text.txt : write text at a location with offset x,y (measured in pixel starting at the location set with gravity) 
// example commandline: convert -size 1280x960 -depth 8 canvas:black -font Helvetica -pointsize 48 -fill white -gravity northwest -annotate +0+0 @text1.txt -gravity southeast -annotate +0+0 @text2.txt text_image.pgm";
#ifdef _WIN32
#define FONT "Arial"
#else
#define FONT "Helvetica"
#endif

#ifdef FILE_IO    
#define IMAGE_SIZE "1280x960"
#define FONT_SIZE "48"
#else
#define IMAGE_SIZE "320x240"
#define FONT_SIZE "12"
#endif



/////////////////////////////////////////////////////////////////////////////// 
// file I/O functions
/////////////////////////////////////////////////////////////////////////////// 
FILE *open_file(char *filename, char *flags) 
{
    FILE *img = fopen(filename, flags);  // open raw file
    if (img == NULL)                        // check if opening worked fine
    {
        printf("Error opening file %s ==> exit.\n",filename);
        exit(-1);
    }
    return img;
}
/////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////// 


int main () 
{    
  
  
  size_t size_paramFir=0;
  size_t size_paramMedian=0;
  size_t size_paramZoom=0;
  size_t size_paramBrightness=0;
  size_t size_paramFlip=0;
  size_t size_paramRotation=0;
  
  
  char *buffer_paramFir = NULL; 
  char *buffer_paramMedian = NULL; 
  char *buffer_paramZoom = NULL; 
  char *buffer_paramBrightness = NULL; 
  char *buffer_paramFlip = NULL; 
  char *buffer_paramRotation = NULL; 
  
  FILE *settings_file;

  printf("  Control-Programm  \n");  
  printf("====================\n");  
    
  while(1) // loop forever
  {

    printf("\nMoechten Sie den FIR Filter benutzen (Ja:1; Nein:0): ");
    getline(&buffer_paramFir,&size_paramFir,stdin);     // read input from console
    printf("\nMoechten Sie den Median Filter benutzen (Ja:1; Nein:0): ");
    getline(&buffer_paramMedian,&size_paramMedian,stdin);     // read input from console
    printf("\nGeben Sie den Vergroesserung Faktor an (Zahl): ");
    getline(&buffer_paramZoom,&size_paramZoom,stdin);     // read input from console
    printf("\nGeben Sie den Parameter fuer die Helligkeitaenderung an (Zahl): ");
    getline(&buffer_paramBrightness,&size_paramBrightness,stdin);     // read input from console
    printf("\nMoechten Sie das Bild um den vertikale Achse spiegeln (Ja:1; Nein:0): ");
    getline(&buffer_paramFlip,&size_paramFlip,stdin);     // read input from console
    printf("\nUm wie viel Grad moechten Sie das Bild drehen (Zahl): ");
    getline(&buffer_paramRotation,&size_paramRotation,stdin);     // read input from console

    printf("Schreibe Parameter in Settings-Datei ... ");
    settings_file = open_file(SETTINGS_FILENAME, "w");  // open text file for storing settings
    fputs(buffer_paramFir, settings_file);                // write Parameter
    fputs(buffer_paramMedian, settings_file);                // write Parameter
    fputs(buffer_paramZoom, settings_file);                // write Parameter
    fputs(buffer_paramBrightness, settings_file);                // write Parameter
    fputs(buffer_paramFlip, settings_file);                // write Parameter
    fputs(buffer_paramRotation, settings_file);                // write Parameter
    
    fclose(settings_file);        
    printf("fertig\n");

  }

 
  free(buffer_paramFir);
  free(buffer_paramMedian);
  free(buffer_paramZoom);
  free(buffer_paramBrightness);
  free(buffer_paramFlip);
  free(buffer_paramRotation);
  return 0;
}
