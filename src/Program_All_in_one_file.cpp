/*
    Author: T. Turner 
*/


#include <iostream>
#include <SDL2/SDL.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <fftw3.h>
#include <string>
#include <fstream>
#include <cstdint>
#include <limits>
#include <cstring>

using std::fstream;
using std::cout;
using std::endl;
using std::cin;
using std::string;
using std::flush;


static const uint8_t    SUPPORTED_CHANNELS = 2;
static const uint8_t    GRIDS = 5;
static const uint8_t    CHAR_THRESHOLD = 1;
static const uint16_t   MAX_CHAR_LEN = 1000;
const int               I = 1;

#define __IsBigEndianMachine() (*(char*)&I == 0)

/*                                      -DOCUMENTATION-

                FOR SDL2 API:       https://wiki.libsdl.org/CategoryAPI
                FOR FFTW2 API:      http://www.fftw.org/#documentation
*/



struct FFTW
{
    fftw_complex *in;                 //used for channel data before fftw operation
    fftw_complex *out;                //will contain real and imaginary data for left and right channel after fftw operation.
    fftw_plan p;                 		//fftw_plan is a fftw3 data type that allocates memory for fftw
                                  	      //read the '2.3 One-Dimensional DFTs of Real Data' section for more information:
                                        //http://www.fftw.org/#documentation
    double* magnitude;                 //calculating magnitude from real and imaginary parts after fftw operation. ex: sqrt(re*re+im*im);
   	int index;
    
};

struct FFT_results
{
    float peakfreq[SUPPORTED_CHANNELS];                    //represents the peak frequency for 2056 frame samples
    float peakmag[SUPPORTED_CHANNELS];                     //represents the peak maximum magnitude or (amplitude) for 2056 samples 
    double magInDB[SUPPORTED_CHANNELS][GRIDS];                    //represents the amplitude at each of the 6 frequencies thresholds.
    size_t* raw;                                            //need to implement later
    struct Wavform{

    	char spectrum[GRIDS][MAX_CHAR_LEN];						//Array to print out waveform on the terminal

    }wav[SUPPORTED_CHANNELS]; 
    
    
    
};


struct AudioData
{
    Uint8*      pos;                    //pointer to the WAV data
    Uint8*      beginning;              //pointer to the first position of the WAV data
    uint32_t    data_size;              //size of the music data in bytes
    Uint32      length;                 //contains the size of music data in real time
    int32_t     SamplesFrequency;       //sample frame rate frequency for WAV file. typically 44.1k sample frames / sec (stereo)
    int32_t     Samples;                //number of buffer samples which is by default 4096. The total number of sample frames would be 4096/2
                                        //because there are 2 channels; 1 sample each for left and right channel
    
};

//Global variables

FFTW                        *fftw;           
FFT_results                 *fft_results;
AudioData                   audio;
SDL_AudioSpec               wavSpec, have;                //SDL data type to analyze WAV file.
                                                    //A structure that contains the audio output format. 
int                         g_array_limit;           //It also contains a callback that is called when the audio device needs more data.
int                         gc = 0;                 //global counter that increments every 4096 samples 
bool                        time_to_exit = false;   //flag to exit thread function
pthread_mutex_t             
    work_mutex = PTHREAD_MUTEX_INITIALIZER;         
const char                        vis[]= "|";          //character to print waveform

char                        *filename;

// Function prototypes
void initializer_vars();
void create_wav_graph(int); 
void printwaveform();
char* getfilepath();
void file_info();                                   //uses sndfile-info program to display wav header information
void printstats();                                  //prints the statistics of waveform after it undergoes fft. and also 
                                                    //control information of the audio player
int getFileSize(FILE*);                             //returns filesize in bytes
void MyAudioCallback(void*,Uint8*, int);//callback function. Its called when the audio device needs more data.
void PressEnterToContinue();                        //function that only returns when '\n' is pressed on the keyboard. 
void parse_wav_samples(int8_t , size_t , int&, int&);//Analyze audio wave file and extract information for fft. Splits information into 2 for left and right channels
void analyze_data(int, int, int, FFTW);                   //Analyzes fft data for left and right channels. calculates frequencies and magnitudes 
void PARSE_COMPUTE_ANALYZE_WAVEFILE();                                //Function that starts the WAVE analysis.  ie, calls parse_wav_samples() and analyze_data()
void* mainthread(void *arg);                        //pthread function --NOTHING IS USING THIS THREAD FUNCTION AT THE MOMENT
int handle_command_line_args(int, char**);
int INITIALIZE_SDL_AND_WAV_VARIABLES();
void AUDIO_DEVICE_CONTROL(SDL_AudioDeviceID);
void CLEANUPMESS(SDL_AudioDeviceID);




int main(int argc, char** argv)
{

    if(handle_command_line_args(argc, argv))
        return 1;

    if(INITIALIZE_SDL_AND_WAV_VARIABLES())
        return 1;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &wavSpec, &have,
            SDL_AUDIO_ALLOW_ANY_CHANGE);
    if(device == 0)
    {
        // TODO: Proper error handling
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    //Update audio information if device has changed any default settings
    if(wavSpec.format != have.format){
		wavSpec.format = have.format;
        cout << "wavSpec.format updated!: " << std::hex << wavSpec.format << endl;
    }
    if(wavSpec.samples != have.samples){
		audio.Samples = wavSpec.samples = have.samples;
        cout << "wavSpec.samples updated!: " << wavSpec.samples << endl;
    }
    if(wavSpec.freq != have.freq){
		audio.SamplesFrequency = wavSpec.freq = have.freq;
        cout << "wavSpec.freq updated!: " << wavSpec.freq << endl;
    }
    if(wavSpec.size != have.size){
        wavSpec.size = have.size;
        cout << "wavSpec.size updated!: " << wavSpec.size << endl;
    }
    PARSE_COMPUTE_ANALYZE_WAVEFILE();
    AUDIO_DEVICE_CONTROL(device);
    CLEANUPMESS(device);
  
    return 0;
}

void AUDIO_DEVICE_CONTROL(SDL_AudioDeviceID device){

    //pthread_t id1;
    //pthread_create(&id1, NULL, mainthread, NULL);

    SDL_PauseAudioDevice(device, 0);
  
    int c = 0;
    int cps;        //cycles per second
    int sec;        //seconds
    // while (audio.length > 0){ SDL_Delay(100);}
    while(c != 'q')
    {
          
            c = getchar();
            switch(c)
            {
                case 's':
start:               SDL_PauseAudioDevice(device, 0);
                    //Pause = false;
                    break;
                
                case 'p':
                    SDL_PauseAudioDevice(device, 1);
                    //Pause = true;
                    break;
                case 'r':
restart:            SDL_PauseAudioDevice(device, 1);
                    //Pause = true;
                    gc = 0;
                    audio.pos = audio.beginning;
                    audio.length = audio.data_size;
                    SDL_PauseAudioDevice(device, 0);
                    //Pause = false;
                    break;
                case 'b': //b 10 ~ rewind 10 sec
                    SDL_PauseAudioDevice(device, 1);
                    //Pause = true;
                    cin >> sec; 
                    //cps = cycles per sec
                    cps = audio.SamplesFrequency/audio.Samples; //we have frames/sec and cycles/frame. | 8192 buffer size per cycle ==> 2048 sample frames per cycle: (8192 bytes = 2 bytes (byte_size for 16 bit audio samples) X 2 channels (for stereo) X 2048 samples( or frames)), same as [2048L's + 2048R's]
                    if((gc - sec*cps) < 0){//we want cycles given seconds
                                                                    
                        goto restart;
                    }
                    else{
                        
                        gc = gc - sec*cps;
                        audio.pos-=(sec*cps*wavSpec.size); //we want bytes, so we use the fact that there are 8192 bytes per cycle 
                        audio.length+=(sec*cps*wavSpec.size);
                        goto start;
                    }
                    break;
                case 'f':
                    SDL_PauseAudioDevice(device, 1);
                    cin >> sec;
                    cps = audio.SamplesFrequency/audio.Samples;

                    if((gc + sec*cps) >= g_array_limit){
                        system("clear");
                        printf("error: Forward past length of file. Press 'q' to quite or 'r' to restart...\n");
                        c = getchar();
                    }
                    else{
                        gc = gc + sec*cps;
                        audio.pos+=(sec*cps*wavSpec.size); //we want bytes, so we use the fact that there are 8192 bytes per cycle 
                        audio.length-=(sec*cps*wavSpec.size);
                        goto start;
                    }
                    break;

                   default:
                    break;
           }//end switch
  

         
    }//end while
}
void MyAudioCallback(void* userdata, Uint8* stream, int streamLength)
{

    AudioData* audio = (AudioData*)userdata;
    if(audio->length == 0)
    {
        printstats();
        gc = 0;
        return;
    }
    else{
        printstats();
        printwaveform();
        
    }
    
    Uint32 length = (Uint32)streamLength;
    length = (length > audio->length ? audio->length : length);

    
    SDL_memcpy(stream, audio->pos, length);

    audio->pos += length;
    audio->length -= length;
    gc++;
   

}
void initializer_vars(){

    int N, n_frames ;

    switch((int)SDL_AUDIO_BITSIZE(wavSpec.format)){

            
        case (16):   
        
            N = (int)(ceil((float)audio.length/(wavSpec.size)));
            break;
           
    }
    fftw = new FFTW[ wavSpec.channels ];
    fftw[0].index = 0;
    fftw[1].index = 1;
    fft_results = new FFT_results[ N ];
    g_array_limit = N;
    n_frames = audio.Samples;
    cout << "before fftw_malloc" << endl;
    for(int c=0; c<wavSpec.channels; c++){
    	fftw[c].in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n_frames);
    	fftw[c].out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n_frames);
    	fftw[c].magnitude = new double[n_frames];
    }
             
       
}

void parse_wav_samples(int8_t* buffer, size_t bytesRead, int* M, int* F){

    int l = 0;
    int r = 0;

    if(wavSpec.channels == 2){
	    if ((int)SDL_AUDIO_BITSIZE(wavSpec.format) == 16)
        {
             
            *F = (int)bytesRead/4; /* To get number of frames divide total bytes by number of channels and bytewidth of audio data*/

            fftw[0].p = fftw_plan_dft_1d(*F, fftw[0].in, fftw[0].out, FFTW_FORWARD, FFTW_MEASURE);
            fftw[1].p = fftw_plan_dft_1d(*F, fftw[1].in, fftw[1].out, FFTW_FORWARD, FFTW_MEASURE);

            for(int c=0; c<(int)bytesRead; c+=4)

            { //right now assuming signed 16 bit little endian
                
                uint16_t temp16 = 0x0000;

                if(!__IsBigEndianMachine()){

                    if(SDL_AUDIO_ISLITTLEENDIAN(wavSpec.format)){
                        temp16 = temp16 | (uint16_t)buffer[c];
                        temp16 = temp16 | ((uint16_t)buffer[c+1] << 8 );  
                                                      
                    }
                    else if(SDL_AUDIO_ISBIGENDIAN(wavSpec.format)){
                        temp16 = temp16 | ((uint16_t)buffer[c] << 8);
                        temp16 = temp16 | (uint16_t)buffer[c+1];
                    }

                    /*
                    store first calculation in fftw[1] because Little endian machine reverses data
                        Ex: 
                            say that we have 16bit Data values in a file that represents Left and 
                            Right channel respectively ---> 0x 1234 5678
                            So Left channel data equals 1234 and Right channel data equals 5678

                            But Little endian machine stores values in reverse because the least 
                            significant byte is stored in the smallest memory address:

                            Little endian memory byte addresses from 0 to 4 has the data stored in 
                            this order: 78 56 34 12 where 78 is position 0, 56 is position 1, and so on. 

                            So we can see that 78 56 data bytes represents the Right channel data values

                            For completeness of explanation

                            Big endian memory byte addresses stores data in this order: 12 34 56 78
                    */
                    if(SDL_AUDIO_ISSIGNED(wavSpec.format))
                   	   fftw[1].in[r++][0] = ((int16_t)temp16)/32768.0;
                    else
                       fftw[1].in[r++][0] = temp16/65535.0;

                	fftw[1].in[r-1][1] = 0.0;
                                   
                    
                    temp16 = 0x0000;
                    if(SDL_AUDIO_ISLITTLEENDIAN(wavSpec.format)){

                        temp16 = temp16 | (uint16_t)buffer[c+2];
                        temp16 = temp16 | ((uint16_t)buffer[c+3] << 8 );
                    }
                    else if(SDL_AUDIO_ISBIGENDIAN(wavSpec.format)){

                        temp16 = temp16 | ((uint16_t)buffer[c+2] << 8);
                        temp16 = temp16 | (uint16_t)buffer[c+3];
                    }

                
                    if(SDL_AUDIO_ISSIGNED(wavSpec.format))
                    	fftw[0].in[l++][0] = ((int16_t)temp16)/32768.0;
                    else
                        fftw[0].in[l++][0] = temp16/65535.0;

                	fftw[0].in[l-1][1] = 0.0;
                }	                        
                

//other formats can be implemented
                //----------------------------------------------
            
                
    
            }//end for
            
            
           
	    }//end if
	}

}

void analyze_data(int M, int F, int cc, FFTW fftw){


    double max[5] = {  
            1.7E-308,
            1.7E-308,
            1.7E-308,
            1.7E-308,
            1.7E-308
    };

    double re, im; 
    double peakmax = 1.7E-308 ;
    int max_index = -1;


    for (int m=0 ; m< F/2; m++){  
        re = fftw.out[m][0];
        im = fftw.out[m][1];
      
        fftw.magnitude[m] = sqrt(re*re+im*im);
        
        float freq = m * (float)wavSpec.freq / F;

        if(freq > 19 && freq<= 140){ 
            if(fftw.magnitude[m] > max[0]){ 
                max[0] = fftw.magnitude[m];
            }
        }
        else if(freq > 140 && freq<= 400){
            if(fftw.magnitude[m] > max[1]){ 
                max[1] = fftw.magnitude[m];
            }
        }
        else if(freq >400 && freq<= 2600){
            if(fftw.magnitude[m] > max[2]){ 
                max[2] = fftw.magnitude[m];
            }
        }
        else if(freq > 2600 && freq<= 5200){
            if(fftw.magnitude[m] > max[3]){ 
                max[3] = fftw.magnitude[m];
            }
        }
        else if(freq > 5200 && freq<= audio.SamplesFrequency/2){
            if(fftw.magnitude[m] > max[4]){ 
                max[4] = fftw.magnitude[m];
            }
        }
        if(fftw.magnitude[m] > peakmax){ 
            peakmax = fftw.magnitude[m];
            max_index = m;
        }
    }//end for

    if(fftw.index == 0){
    		fft_results[cc].peakfreq[0] = max_index*wavSpec.freq/F;
    		fft_results[cc].peakmag[0] = peakmax;
    	}
    	else if(wavSpec.channels > 1){
    	fft_results[cc].peakfreq[fftw.index] = max_index*wavSpec.freq/F;
    	fft_results[cc].peakmag[fftw.index] = peakmax;
    	}




    for(int copy=0; copy < GRIDS; copy++){

    	if(fftw.index == 0)
        	fft_results[cc].magInDB[0][copy] = 10*(log10(max[copy]));
        else if(wavSpec.channels > 1)
        	fft_results[cc].magInDB[fftw.index][copy] = 10*(log10(max[copy]));

    }




}
void PARSE_COMPUTE_ANALYZE_WAVEFILE(){

 

    size_t bytesRead;
   
    uint32_t BUFFER_SIZE = wavSpec.size;
    int8_t* buffer ;   
    int M;
    int F; // used for number of frames

    
    int cc=0;
   
    FILE* wavFile = fopen(filename, "r");
    int filesize = getFileSize(wavFile);

    initializer_vars();

    buffer = new int8_t[BUFFER_SIZE];
    bytesRead = fread(buffer, sizeof buffer[0], filesize-audio.data_size, wavFile); //Skip header information in .WAV file

    while ((bytesRead = fread(buffer, sizeof buffer[0], BUFFER_SIZE / (sizeof buffer[0]), wavFile)) > 0) //Reading actual audio data
    {
        parse_wav_samples(buffer, bytesRead, &M, &F);

        for(int c=0; c< wavSpec.channels; ++c)
        	fftw_execute(fftw[c].p);

        for(int c=0; c< wavSpec.channels; ++c)
        	analyze_data(M, F, cc, fftw[c]);
                
   		create_wav_graph(cc);

   		for(int c=0; c< wavSpec.channels; ++c)
	        fftw_destroy_plan(fftw[c].p);

        
        cc++;     
         
    }//end while(fread)

    delete [] buffer;
    delete [] fftw[0].magnitude;
    delete [] fftw[1].magnitude;
    buffer = nullptr;

    fftw_free(fftw[0].in); 
    fftw_free(fftw[1].in);
    fftw_free(fftw[0].out);
	fftw_free(fftw[1].out);
	delete [] fftw;
	fftw = nullptr;

    fclose(wavFile);
  
    file_info();
}

// find the file size 
int getFileSize(FILE *inFile){
    int fileSize = 0;
    fseek(inFile,0,SEEK_END);

    fileSize=ftell(inFile);

    fseek(inFile,0,SEEK_SET);
    return fileSize;
}

void PressEnterToContinue()
{
  std::cout << "Press ENTER to continue... " << flush;
  std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
  std::system("clear");
}

void* mainthread(void *arg){

    while(!time_to_exit) 
        {
             
               // while(!Pause && (audio.length > 0) && !time_to_exit){} //gonna be used for opengl 3.x

        }

    pthread_exit(NULL);
}


void printstats(){
 
    std::system("clear");
    printf("%s%s", "FILE_PATH : ",filename);
    putchar('\n');
    putchar('\n');
    printf( "Press: \np to pause \ns to start \nr to restart\nq to quit\nb <sec> to rewind song in sec\nf <sec> to fast forward in sec" );
    putchar('\n');
    putchar('\n');
   
    printf("%s%d", "Sample Rate : ", audio.SamplesFrequency );
    putchar('\n');
   
    printf("%s%d", "Frames (samples) per Period : ", audio.Samples);
    putchar('\n');
    
    double val = audio.length/wavSpec.channels;
    
    if (SDL_AUDIO_BITSIZE(wavSpec.format) == 16) 
        val = val / 2; 

    val = val / audio.SamplesFrequency ;

  /* val is in seconds ===>   X [bytes]    frame        channel      sec 
                             --------- * ----------  * ------- * -------------
                                 1       2 channels    2 bytes   44.1k frames  */

    printf("%s%.02lf", "TIME Remaining (sec) : ", val);
    putchar('\n');

    if(wavSpec.channels == 2){
        float AvgdBPeakMag = 10*log10((fft_results[gc].peakmag[0] + fft_results[gc].peakmag[1])/2);
        printf("%s%.2lf", "peak Magn. (dB)\t: ", AvgdBPeakMag > 0 ? AvgdBPeakMag : 0);
    }
    
    else{
        float dBPeakMag = 10*log10((fft_results[gc].peakmag[0]));
        printf("%s%.2lf", "peak Magn. (dB)\t: ", dBPeakMag > 0 ? dBPeakMag : 0);
    }
    putchar('\n');
    
    printf( "=============================================================\n");

}

void file_info()
{
  
    system("clear");
    char CMD[] = "sndfile-info ";
    char buffer[128];


    snprintf(buffer, 128, "%s %s", CMD,filename);
    system(buffer);

    PressEnterToContinue();
}

void create_wav_graph(int cc){


			for(int g=0; g<GRIDS; ++g){
				for (int c = 0; c < wavSpec.channels; ++c){
					strcpy(fft_results[cc].wav[c].spectrum[g], "");
				}
			}

          for(int chs=0; chs<wavSpec.channels; chs++){
            	for(int g=0; g<GRIDS; g++){
            		for(int A=0; A<fft_results[cc].magInDB[chs][g]; A+=CHAR_THRESHOLD){
                        strcat (fft_results[cc].wav[chs].spectrum[g],vis);
    
                	}
              	}
            }

     

}
void printwaveform(){
		for(int c=0; c< wavSpec.channels; c++){
			for(int out=0; out<GRIDS; out++){
				if(c==0){
					cout << "L" << out << fft_results[gc].wav[c].spectrum[out] << endl;
					cout << "L" << out << fft_results[gc].wav[c].spectrum[out] << endl;
				}
				else{
					cout << "R" << GRIDS-1-out << fft_results[gc].wav[c].spectrum[GRIDS-1-out] << endl;
					cout << "R" << GRIDS-1-out << fft_results[gc].wav[c].spectrum[GRIDS-1-out] << endl;

				}
			}
	        cout << "\n\n\n";
		}
		
		

 }

char path[1024];
char* getfilepath(){

	system("clear");
	char buffer[1024];
	FILE *fp;
  	
    cout << "getfilepath()" << endl;
  	snprintf(buffer, 1024, " avconv -i %s -f wav temp.wav",filename); //convert mp3 to wav file
  	system(buffer);
    /* Open the command for reading. */
    fp = popen("pwd", "r"); //get current/working directory
    if (fp == NULL) {
     printf("Failed to run command\n" );
     return NULL;
    }

	  /*get terminal output and find null character position.  This position is the start of concatenation of temporary file name*/
	  fgets(path, sizeof(path), fp); 
	  int c=0;
	  while(path[c] != '\0'){c++;} //
  
	  c--; //move position to the newline character of string (very end before null character)
	  char t[] = "/temp.wav";
	  //copy chars from t array and append to path array 
	  for(int i=0; i<= (int)sizeof(t); i++){
	  	path[c+i]= t[i]; 
	  }
	  /* close */
	  pclose(fp);

	return path; 

}

int handle_command_line_args(int argc, char** argv){

    int opt;

    while((opt = getopt(argc, argv, "f:")) != -1){
        switch(opt){

            case 'f':
                        filename = optarg;
                        break;
            case '?':
usage:
                        fprintf(stderr, "usage: %s [-f PATH_TO_FILE]\n", argv[0] );
                        return 1;
        }
    }

    if(argc != 3) goto usage;                               // error check to make sure there are 3 command line parameters

    int len = strlen(filename);
    const char *last_four = &filename[len-4];
    
    if(strcmp(last_four, ".mp3") == 0)
        filename = getfilepath();
        if(filename == nullptr)
            return 1;
    
    return 0;
}

int INITIALIZE_SDL_AND_WAV_VARIABLES(){
    
    SDL_Init(SDL_INIT_AUDIO);                                
    Uint8* wavStart;                                        //pointer to audio data 
    Uint32 wavLength;                                       //length of audio data
    
    if(SDL_LoadWAV(filename, &wavSpec, &wavStart, &wavLength) == NULL)
    {
        // TODO: Proper error handling
        std::cerr << "Error: " << filename
                    << " could not be loaded as an audio file" << std::endl;
        return 1;
    }

    if(wavSpec.channels < 2){

         std::cerr << "Error! Number of channels: " << wavSpec.channels 
                    << " isnt supported in program yet" << std::endl;
         return 1;
    }
   
    else if(wavSpec.channels > 2){

         std::cerr << "Error! Number of channels: " << wavSpec.channels 
                    << " isnt supported in program yet" << std::endl;
         return 1;
    }   
    
    audio.beginning = wavStart;                             
    audio.data_size = wavLength;
    audio.pos = wavStart;
    audio.length = wavLength;
    audio.Samples = wavSpec.samples;
    audio.SamplesFrequency = wavSpec.freq;

    wavSpec.callback = MyAudioCallback;
    wavSpec.userdata = &audio;

    
/*
    Debugging values of wavSpec 
    

    std::cout << "wavSpec.freq = " << (int)wavSpec.freq << std::endl
          << "wavSpec.format = 0x" << std::hex << wavSpec.format << std::dec << std::endl
          << "wavSpec.channels = " << (int)wavSpec.channels << std::endl
          << "wavSpec.samples = "   << (int)wavSpec.samples << std::endl;

*/
    return 0;
}

void CLEANUPMESS(SDL_AudioDeviceID device){
    time_to_exit = true;
    //  pthread_join(id1, NULL);
    //  pthread_mutex_destroy(&work_mutex);
    SDL_CloseAudioDevice(device);
    SDL_FreeWAV(audio.beginning);
    SDL_Quit();

    
    char buffer[1024];
    snprintf(buffer, 1024, "rm -f temp.wav"); 
    system(buffer);


}