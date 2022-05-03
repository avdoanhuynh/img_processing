#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>

/////////////////////////////////////////////////////////////////////////////// 
// settings and notes
/////////////////////////////////////////////////////////////////////////////// 

// gcc commandline: gcc -std=c99 -pg -fno-inline -mfpu=neon -o img_proc img_proc.c 

// enable the define "FILE_IO" for file I/O,
// otherwise use the following command line for "live" camera video processing (it requires package mplayer : sudo apt-get install mplayer2)
// due to performance issues image size is reduced to 320x240
// command line: raspivid -n -t 500000 -w 320 -h 240 -fps 10 --raw - -rf gray  -o video.264 | ./img_proc | mplayer -demuxer rawvideo -rawvideo w=320:h=240:fps=10:format=y8 -vo x11 -vf scale -cache 32 -



#define FILE_IO

char* SETTINGS_FILENAME="./settings.txt"; // file for storing settings, IMPORTANT: use the same as in the other programm

// define width and height of the image / video
#ifdef FILE_IO	
  #define W 1280  // image width
  #define H 960  // image height
  char* INPUT_FILENAME="./Bilder/test_bild_original.raw"; // input file (raw image data = pgm file without header)
  char* OUTPUT_FILENAME="./Bilder/out.pgm";                      // processed output file (pgm file)
#else
  #define W 360  // video width
  #define H 240  // video height
#endif


// define FIR filter settings (select one)

#define IMAGE_COPY
//#define LOWPASS
//#define HOCHPASS
//#define BOXCAR
//#define SCHARR

#ifdef IMAGE_COPY
  // change brightness
  #define K 3     // size of filter window
  int c[K][K] = { // coefficients
          {0, 0, 0},
          {0, 1, 0},
          {0, 0, 0}
          };
  int g=1;   // scaling
  int h=0; // offset
#endif	

#ifdef LOWPASS
  // change brightness
  #define K 5     // size of filter window
  int c[K][K] = { // coefficients
          {1, 4, 6, 4, 1},
          {4, 16, 24, 16, 4},
          {6, 24, 36, 24, 6},
          {4, 16, 24, 16, 4},
          {1, 4, 6, 4, 1}
          };
  int g=256;   // scaling
  int h=0; // offset
#endif	

#ifdef HOCHPASS
  // change brightness
  #define K 5     // size of filter window
  int c[K][K] = { // coefficients
          {0, 0, -1, 0, 0},
          {0, -1, -2, -1, 0},
          {-1, -2, 16, -2, -1},
		  {0, -1, -2, -1, 0},
          {0, 0, -1, 0, 0},
          };
  int g=1;   // scaling
  int h=128; // offset
#endif	

#ifdef BOXCAR
  // change brightness
  #define K 3     // size of filter window
  int c[K][K] = { // coefficients
          {1, 1, 1},
          {1, 1, 1},
          {1, 1, 1}
          };
  int g=9;   // scaling
  int h=0; // offset
#endif	

#ifdef SCHARR
  // change brightness
  #define K 3     // size of filter window
  int c[K][K] = { // coefficients
          {3, 10, 3},
          {0, 0, 0},
          {-3, -10, -3}
          };
  int g=1;   // scaling
  int h=128; // offset
#endif	


// for estimation of realtime processing of Full-HD@30fps video

//#define REALTIME_PROCESSING_SIMULATION
const int realtime_factor = (1920*1080*30)/(W*H); 

/////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////// 



typedef unsigned char uint8_t;
FILE *log_file;  // file for writing status messages



/////////////////////////////////////////////////////////////////////////////// 
// time measurement functions
/////////////////////////////////////////////////////////////////////////////// 
#ifdef _WIN32

  #include <windows.h>
  LARGE_INTEGER start,stop,freq;

  void start_count() {
    QueryPerformanceFrequency(&freq);
    fprintf(log_file,"%f MHz\n", freq.QuadPart/1000.0);
    QueryPerformanceCounter(&start);
  }
    

  void stop_count() {
    QueryPerformanceCounter(&stop);
  }

  double get_time_ms() {
    return (double) (stop.QuadPart - start.QuadPart) / (freq.QuadPart/1000);
  }
#else

  #include <sys/time.h>
  struct timeval start,stop,freq;
  double diffi;
  
  //clock_gettime(CLOCK_REALTIME,&freq);
  
  void start_count() {
    gettimeofday(&start,NULL);
  }
    

  void stop_count() {
    gettimeofday(&stop,NULL);
  }

  double get_time_ms() {
	return ( stop.tv_sec * 1000 + (double)stop.tv_usec/1000 )
	      - (start.tv_sec * 1000 + (double)start.tv_usec/1000); 
  }
#endif
/////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////// 

/////////////////////////////////////////////////////////////////////////////// 
// file I/O functions
/////////////////////////////////////////////////////////////////////////////// 
FILE *open_file(char *filename, char *flags) 
{
	FILE *img = fopen(filename, flags);  // open raw file
	if (img == NULL)                        // check if opening worked fine
	{
		fprintf(log_file,"Error opening file %s ==> exit.\n",filename);
		exit(-1);
	}
	return img;
}
	
int read_image(uint8_t img_array[H][W], FILE* img) 
{
	int length = fread(img_array,1,W*H,img);  // read file data to img_array
	if (length != W*H)                        // check if reading worked fine
	{
		fprintf(log_file,"no more data in input image\n");
		return 0;
	}
	return 1;
}

void write_pgm_header(FILE *img) 
{
	char header[32];	
	
	sprintf(header,"P5\n%d %d\n255\n",W,H);     // create header
	fwrite(header,1, strlen(header),img);     // copy header to pgm file
}

void write_image(uint8_t img_array[H][W], FILE *img) 
{
	int length = fwrite(img_array,1,W*H,img); // write image data to output file
	if (length != W*H)                        // check if writing worked fine
	{
		fprintf(log_file,"Error writing image ==> exit.\n");
		exit (-1);
	}
}


/////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////// 


/////////////////////////////////////////////////////////////////////////////// 
// image processing funtions
/////////////////////////////////////////////////////////////////////////////// 

void fir_filter(uint8_t out[H][W], uint8_t in[H][W])
{
	int k,l,x,y;
	int sum;
	
	for (y=(K>>1); y < H-(K>>1); y++)  // loop over all lines of frame
	{
		for (x=(K>>1); x < W-(K>>1); x++)  // loop over all rows of frame
		{
			// perform FIR filtering for each output pixel
			sum = 0;	
			for (k=0; k< K; k++)   // loop over all lines of filter window
			{
				for (l=0; l< K; l++) // loop over all rows of filter window
				{
					sum += c[k][l] * in[y-(K>>1)+k][x-(K>>1)+l];  // process each pixel in window
					
				}
				
			
			}
			sum = sum/g + h;  // scaling and offset

			if (sum < 0) sum = 0; //clipping
			else if (sum > 255) sum=255;  
			
			out[y][x] = sum;  // write to output
		}
	}
}

void flip_horizontal(uint8_t out[H][W], uint8_t in[H][W]) // spiegeln
{
	int x,y;
	
	for (y=0; y < H; y++)  // loop over all lines
	{
		for (x=0; x < (W/4)*4; x+=4)  // loop over all rows , Optimierung: 4-fach Loop-Unrolling
		{
			out[y][x] = in[y][(W-1)-x];  // process each pixel
			out[y][x+1] = in[y][(W-1)-(x+1)];
			out[y][x+2] = in[y][(W-1)-(x+2)];
			out[y][x+3] = in[y][(W-1)-(x+3)];
			
			
		}
		for (x = (W/4)*4; x < W; x++) //Optimierung: Restschleife von Loop-unrolling
		{
			out[y][x] = in[y][(W-1)-x];
		}
	}
}

void change_brightness(uint8_t out[H][W], uint8_t in[H][W], int c)
{
	int x,y;
	int temp;
	
	for (y=0; y < H; y++)  // loop over all lines
	{
		for (x=0; x < W; x++)  // loop over all rows
		{
			temp = in[y][x] + c;
			if (temp > 0)
			{
				if (temp < 255)
				{
					out[y][x] = temp;  
				}
				else
				{
					out[y][x] = 255;
				}
			}
			else
			{
				out[y][x] = 0;
			}
			
		}
	}
}

//Quelle: http://homepages.inf.ed.ac.uk/rbf/BOOKS/PHILLIPS/
void rotation(uint8_t out[H][W], uint8_t in[H][W],int angle_grad)
{
	int x,y,x_out,y_out;
	int temp;
	double c,s;
	double angle_rad = (double)angle_grad*3.14159265359/180;
	c = cos(angle_rad);
	s = sin(angle_rad);
	double center_x = (double)W/2;
	double center_y = (double)H/2;
	
	for (y=0; y < H; y++)  // loop over all lines
	{
		for (x=0; x < W; x++)  // loop over all rows
		{
			x_out = center_x + ((double)(y)-center_y)*s+((double)(x)-center_x)*c;
			y_out = center_y + ((double)(y)-center_y)*c-((double)(x)-center_x)*s;
			
			temp = 0; //in case the original pixel is not available
			if(x_out >= 0 && x_out <= W && y_out >= 0 && y_out <= H)
			{
				 temp = in[y_out][x_out];
			}
			out[y][x] = temp;
		}
	}
}

//Quelle: http://homepages.inf.ed.ac.uk/rbf/BOOKS/PHILLIPS/
void zoom(uint8_t out[H][W], uint8_t in[H][W], int faktor)
{
	  
	   int x,y;
	   int A,B,C,D;	   
	   int hf = H/faktor;
	   int wf = W/faktor;
	  	
	   	
	for(A=0; A<faktor; A++) // loop over factor
	{
       for(B=0; B<faktor; B++) // loop over factor
       {
        for(y=0; y<(hf); y++) // loop over all lines of frame
        {
         for(x=0; x<(wf); x++) // loop over all rows of frame
         {
          for(C=0; C<faktor; C++) // loop over factor
          {
           for(D=0; D<faktor; D++) // loop over factor
           {
             out[faktor*y+C][faktor*x+D] = in[y+A*((hf)>>1)][x+B*((wf)>>1)]; // replicate every element and write to output
           } 
          }  
         }  
        }   
	   }
	}
}



void median_filter(uint8_t out[H][W], uint8_t in[H][W])
{
	int s = 3; //size of filter window
	int ds=s>>1;
	int x,y, j, k ;
	int pixel[9]; // Declare the chosen 3x3 Pixels
	int tmp;
	
	
	for (y=ds; y < H - ds; y ++)  // loop over all lines of frame
	{
		for (x=ds; x < W-ds; x ++)  // loop over all rows of frame
		{
			// 3x3 will be taken
			pixel[0] = in[y-1][x-1];
			pixel[1] = in[y-1][x];
			pixel[2] = in[y-1][x+1];
		  
			pixel[3] = in[y][x-1];
			pixel[4] = in[y][x];
			pixel[5] = in[y][x+1];
		 
			pixel[6] = in[y+1][x-1];
			pixel[7] = in[y+1][x];
			pixel[8] = in[y+1][x+1];
			
			// Sort-Algorithm: the chosen 3x3 Pixels will be sort
			for ( j = 0; j < 9; j ++)			  
		    {  
			    for( k = j+1; k< 9 ; k ++)			   
				{
					if( pixel[j] < pixel[k])			      
					{
						tmp = pixel[j];
						pixel[j]=pixel[k];
						pixel[k]=tmp;
   
					}
			    }
			}
			 
			out[y][x] = pixel[4];
			 	    
		}
	}
}

void array_copy(uint8_t out[H][W], uint8_t in[H][W])
{
	int x,y;
	
	for (y=0; y < H; y++)  // loop over all lines
	{
		for (x=0; x < W; x++)  // loop over all rows
		{
			out[y][x] = in[y][x];  // process each pixel
		}
		
	}
}


uint8_t inp[H][W], out[H][W], temp1[H][W], temp2[H][W], temp3[H][W], temp4[H][W], temp5[H][W];  // define arrays for input and output image


int main () 
{	
	int i=0;
	FILE *in_file,*out_file;
	 
#ifdef FILE_IO	
	log_file = stdout;                             // write log messages to stdout
	in_file  = open_file(INPUT_FILENAME, "rb");     // open input file (raw image data = pgm file without header)
	out_file = open_file(OUTPUT_FILENAME, "wb");    // open/create output file (pgm file)
	write_pgm_header(out_file);                    // write PGM header (for grayscale: P5, dimensions and max pixel value)
#else
	log_file = open_file("performance.log", "w");  // open log file for writing status messages
	in_file  = stdin;                              // read raw grayscale video from stdin
	out_file = stdout;                             // write raw grayscale video to stdout 
#endif	 
	 
	size_t size=0;
	char *buffer=NULL; 
	int paramFir=0, paramMedian=0, paramZoom=0, paramBrightness=0, paramFlip=0, paramRotation=0;
    FILE *settings_file;
	struct stat fileInfo;
	time_t last_time=0;


	fprintf(log_file,"process images\n");	
	
		
		while(read_image(inp,in_file)) // loop until no more input data is available
		{
			start_count(); // start time measurement
			
			stat(SETTINGS_FILENAME,&fileInfo);
			
			if (fileInfo.st_mtime!=last_time) 
			{
				settings_file = open_file(SETTINGS_FILENAME, "r");  // open settings file 
				getline(&buffer,&size,settings_file);               // read from settings file
				sscanf(buffer, "%d", &paramFir);                    // convert the line to an integer value of the parameter
				getline(&buffer,&size,settings_file);               // read from settings file
				sscanf(buffer, "%d", &paramMedian);                 // convert the line to an integer value of the parameter
				getline(&buffer,&size,settings_file);               // read from settings file
				sscanf(buffer, "%d", &paramZoom);                   // convert the line to an integer value of the parameter
				getline(&buffer,&size,settings_file);               // read from settings file
				sscanf(buffer, "%d", &paramBrightness);             // convert the line to an integer value of the parameter
				getline(&buffer,&size,settings_file);               // read from settings file
				sscanf(buffer, "%d", &paramFlip);                   // convert the line to an integer value of the parameter
				getline(&buffer,&size,settings_file);               // read from settings file
				sscanf(buffer, "%d", &paramRotation);               // convert the line to an integer value of the parameter
				fclose(settings_file); 
			
			
				//print out the parameter   
				if(paramFir!=1)
				{
					fprintf(log_file,"FIR Filter: nein\n");
				}
				else
				{
					fprintf(log_file,"FIR Filter: ja\n");
				}
			
				if(paramMedian!=1)
				{
					fprintf(log_file,"Median Filter: nein\n");
				}
				else
				{
					fprintf(log_file,"Median Filter: ja\n");
				}
			
				fprintf(log_file,"Zoom Faktor ist: %d\n",paramZoom);
				fprintf(log_file,"Helligkeitaederung Parameter ist: %d\n",paramBrightness);
			
				if(paramFlip!=1)
				{
					fprintf(log_file,"Um vertikale Achse spiegeln: nein\n");
				}
				else
				{
					fprintf(log_file,"Um vertikale Achse spiegeln: ja\n");
				}
			
				fprintf(log_file,"Rotation um %d Grad\n",paramRotation);
			
			

				last_time=fileInfo.st_mtime;
			}
			
			#ifdef REALTIME_PROCESSING_SIMULATION  	
			fprintf(log_file,"realtime estimation : process image %d times (this might take a while ...)\n", realtime_factor);
			for (int j=0;j<realtime_factor;j++) // repeat execution for simulating realtime requirements  
			#endif 	
			{	
				//execute image processing
				
				
				if(paramFir!=1)
				{
					array_copy(temp1,inp);
				}
				else
				{
					fir_filter(temp1,inp);
				}
			
				if(paramMedian!=1)
				{
					array_copy(temp2,temp1);
				}
				else
				{
					median_filter(temp2,temp1);
				}
				
				if(paramZoom==0)
				{
					array_copy(temp3,temp2);
				}
				else
				{
					zoom(temp3,temp2,paramZoom);
				}
				
				change_brightness(temp4,temp3,paramBrightness);
			
				if(paramFlip!=1)
				{
					array_copy(temp5,temp4);
				}
				else
				{
					flip_horizontal(temp5,temp4);
				}
				
				rotation(out,temp5,paramRotation);
				
							
			}
		
			stop_count(); // stop time measurement
			fprintf(log_file,"%f msec for processing image %d\n", get_time_ms(),i);
			write_image(out,out_file);  
			i++;
		}
		fprintf(log_file,"done\n");

		// close files
		fclose(in_file);    
		fclose(out_file);  
		fclose(log_file);  
	
		sleep(1);  
		free(buffer);
	return 0;
}


