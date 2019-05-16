

//Header files
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>

#include <stdlib.h>
#include <sys/sysinfo.h>

#include "timer_ioctl.h"
#include "vga_ioctl.h"


//-----------------------------------Funcion declarations--------------------------------

void clear_screen ();
void display();

__u32 read_timer();
void print_timer();
void avg_readtime();
void avg_writetime();
void initialize();
void start_timer();
int time_stat();

//---------------------------------Global Variable-----------------------------
//VGA
int *buffer;
int row, col;
int row_org = 0;//offset of screen
int col_org = 0;
int col_max = 16, row_max = 16;
int img_col, img_row;
int sub_row_org=0;//offset of image
int sub_col_org=0;
const BUFFER_SIZE = 640*480*4;
int ascii;
char c;
char str [50];
int count2=0;//count character loops



//Timer  

int fd; //file descriptor for timer driver
int serial_fd;
int count=0;
float readtime=0;
float writetime=0;
float readtotal=0;
float writetotal=0;
float readarray[100]={0};
float writearray[100]={0};
float avg_read=0;
float avg_write=0;
float starttime;
float stoptime;
float readdif;

char template[8][40]={ {"you entered:"},
			{"read array (sec):"},
		        {"char to read(sec):"},
			{"char to write (cycle):"},
		        //{"Local time and date: " }, { "Program Time (sec)= "}
			};

//-----------------------------------------Structures--------------------------------
struct timer_ioctl_data data; // data structure for ioctl calls

struct termios tio;//The termios functions describe a general terminal interface that is provided to control asynchronous communications ports.
struct image {  int *mem_loc; };
struct image img;
struct timeval start, end;

//----------------------------------Functions Implementation--------------------------

//Function to read the value of the timer
__u32 read_timer()

{
	data.offset = TIMER_REG;
	ioctl(fd, TIMER_READ_REG, &data);
	return data.data;
}

//Function to print the timer data
void print_timer()
{
	printf("timer value = %u\n", read_timer());
}


//Function for the average read time
void avg_readtime()
{
	int i=0;
	for (i=0;i<count;i++)
	{	
		readtotal=readtotal+readarray[i];
		
	}
	printf("readtotal is %d\n", readtotal);

	avg_read= readtotal/count ;
	printf("The average time to read is %f secs\n",avg_read);
}


//Function for the average write time
void avg_writetime()
{
	int j=0;
	for (j=0;j<count;j++)
	{	
		writetotal=writetotal+writearray[j];
		
	}
	printf("writetotal is %d secs\n",writetotal);
	avg_write= writetotal/count/100000;
	printf("The average time to write is %d milisecs\n",avg_write);
	
}

//Initialization
void initialize()
{
	//tcgetattr: Get the parameters associated with the terminal referred to by serial_fd and store them in the termios structure referenced by tio.
	tcgetattr(serial_fd, &tio);

	//cfsetospeed: Set output baud rate 
	cfsetospeed(&tio, B115200);
	cfsetispeed(&tio, B115200);

	///tcsetattr: Set the parameters associated with the terminal referred to by serial_fd from the termios structure referenced by tio. TCSANOW: The change will occur immediately.
	tcsetattr(serial_fd, TCSANOW, &tio);

}


	
//Start timer
void start_timer()
{
	// Rest the counter
	data.offset = LOAD_REG;
	data.data = 0x0;
	ioctl(fd, TIMER_WRITE_REG, &data);

	// Set control bits to load value in load register into counter
	data.offset = CONTROL_REG;
	data.data = LOAD0;
	ioctl(fd, TIMER_WRITE_REG, &data);


	// Set control bits to enable timer, count up
	data.offset = CONTROL_REG;
	data.data = ENT0;
	ioctl(fd, TIMER_WRITE_REG, &data);
	
}

//Prints out metrics
int time_stat()

{
	int ascii_calc;
	int i;
	char str[15];
	sprintf(str, "%f", readdif);
       	for(i=0; i < 10 ; i++)
	{
		int ascii_calc = (int) str [i];

		sub_row_org= (ascii_calc) /16;
		sub_col_org= ascii_calc%16;

		if (count2==40)
			{
			count2 =0;
			row_org= row_org + row_max;
			}
		
		return ascii_calc;
		
			
	}
   


}

//clears the screen; 1. when screen is full 
void clear_screen ()
{

for (row = 0; row < 128 ; row++)
    	{
        	    for (col = 0; col < 640; col++)
  		      {  		
			*(buffer + col + row*640) = 0xff000fff;

  		      }
 	   }

for (row = 128; row < 480; row++)
    	{
        	    for (col = 0; col < 640; col++)
  		      {  		
			*(buffer + col + row*640) = 0xff00ff00;
  		      }
 	   }
}

void clear_half()

{
	for (row = 128; row < 480; row++)
    	{
        	    for (col = 0; col < 640; col++)
  		      {
  		
			*(buffer + col + row*640) = 0xff00ff00;


  		      }
 	   }


}



//Display time stuff

void display_time()

{char str[80];
	int i=0;
	int j=0;
	for ( i=0; i<=7; i++)
	{
		for ( j=0; j<=39; j++)
		{
		
		char temp_c = template[i][j];
		int temp_ascii= (int) temp_c;
		

		printf ("ascii temp is: %i \n", temp_ascii);

		if (temp_ascii==58) { 
				printf ("i: %i\n" , i);
				printf ("j: %i\n" , j);

				
				temp_ascii = time_stat();
				//break;		
					}
		
		sub_row_org= (temp_ascii) /16;
		sub_col_org= (temp_ascii%16) ;

		if (count2==40)
			{
			count2 =0;
			row_org= row_org + row_max;
			}
		
		
		display ();
		count2 ++;

			
		}
	}	
}

//Display the image

void display()

{	
	//shift by character size
	sub_row_org= (sub_row_org+1) * row_max;
	sub_col_org= (sub_col_org+1) * col_max;

	for (img_row = 0 ; img_row < row_max; img_row++)
  	{
		if ((img_row + row_org)>=480)   
	
			{break;} //takes out of loop
	
		else if ((img_row + row_org) < 0)
			{continue;} //takes back to top of loop

        	  for (img_col = 0 ; img_col < col_max; img_col++)
		      {
				if ((img_col+ col_org) < 0)  
					{continue;}
	
			 
				else if ((img_col + col_org) >= 640 )
					{break;}
					
			
				else
					{
					
			*(buffer + (img_col+ (col_org+count2*col_max)) + (img_row + row_org)*640) = (*(img.mem_loc + (img_col+ (sub_col_org-1)) + (img_row + (sub_row_org-1)) *(272)));		

		      			}
  			}


  	}

}


//------------------------>>>>>>>MAIN<<<<<<<<<<-------------------------------
int main() 
{
//-------------Declare variables--------------

int image_fd, fd, fd_timer_driver;

time_t currenttime;
float serial_fd;

float beforewrite;
float afterwrite;
float writedif;


struct stat sb; 

//--------------KERMIT-------------

printf("system time is:%d secs \n",time(&currenttime));

//Fill memory with constant byte 0
memset(&tio, 0, sizeof(tio));
//open serial port

serial_fd = open("/dev/ttyPS0",O_RDWR);
if(serial_fd == -1)printf("Failed to open serial port...  \n");

//-----------Open device file-------------

//VGA driver insertion

fd = open("/dev/vga_driver",O_RDWR);

if (fd ==-1){
	perror("failed to open vga buffer... \n");
	return;
	}

//timer driver insertion

//if (!(fd = open("/dev/timer_driver", O_RDWR))) 

if (!(fd_timer_driver = open("/dev/timer_driver", O_RDWR))) 
	{
		perror("failed to open timer_driver... \n");
		exit(EXIT_FAILURE);
	}

//----------Import raw image----------

image_fd = open("/home/root/asciitable4.raw",O_RDONLY);//*** open (*path, int Oflag)

if (image_fd ==-1){
	perror("failed to open picture>>>>> ");
	return;
	}

//Obtain stats of image, so we can then extract size
  
fstat (image_fd, &sb);

//-----------Map the image in the memory-----------

img.mem_loc = (int*)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, image_fd, 0);

if (img.mem_loc ==-1){
	perror("failed to map pict.>>>>> ");
	return;
	}

printf("size of image is %d \n", sb.st_size);
 
//Map device file with proper buffer size

buffer = (int*)mmap(NULL,BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

if (buffer ==-1){
	perror("failed to map buffer ");
	return;
	}


//Copy fixed color onto the buffer

clear_screen();

printf("location of image %i \n",img.mem_loc);

//initialization
initialize();

printf("passed initializion\n");


//start timer
start_timer();

printf("passed starter\n");

display_time();

// ------------read from Kermit and display-----------

while (1){


printf("in while loop\n");




		//write function hw 
		beforewrite = read_timer();
		printf("%f \n",beforewrite);
		write(serial_fd, &c, 1); 
		afterwrite = read_timer();
		
		printf("%f \n",afterwrite);
		writedif = afterwrite - beforewrite;
		writearray[count]=writedif;
		

		//read function
		//starttime = time(&currenttime);
		starttime = gettimeofday(&start,NULL);
		read(serial_fd, &c, 1);
		//stoptime = time(&currenttime);
		stoptime = gettimeofday(&end,NULL);
		printf("you entered: %c \n",c);
		readdif = stoptime - starttime;
       		readarray[count]=readdif;
		printf("time in read array: %f secs\n",readarray[count]); 

		printf("time between each char to read: %f secs\n",readdif);
		printf("time between each char to write: %f cycles \n",writedif);
		fflush(stdout);
		count++;

 		printf("There are %d inputs \n",count);
		avg_readtime();//sw timer
		avg_writetime();
		 	



printf("--%c",c);
fflush(stdout);

//gconvert ascii chaacter to number
ascii = (int) c;

printf("----> ASCII is %i*** \n", ascii);

//----------->Select Character for SubImage<-------------

//------------------enter button--------


if (ascii == 10) { 
	
printf("Pressed enter\n");
count2= -1;
row_org= row_org + row_max;

	if (row_org >= 480){
  		//clear screan 
		//clear_screen();
		clear_half();
		//start begining of page
		col_org =0;
		row_org=128;
	}
	
}



//------characters-------------
else{


sub_row_org= (ascii) /16;
sub_col_org= ascii%16;

//----check to shift row-----
if (count2==40){

count2 =0;
row_org= row_org + row_max;
		

}

//----------clear page if buttom of page--------

if (row_org >= 480){

 	 //clear screen
	//clear_screen();
	clear_half();	
	col_org =0;
	row_org=128;
}


display();

}
count2++;//next input for loop
}//end of while loop

	return 0;
} 

 //END PROGRAM\\





