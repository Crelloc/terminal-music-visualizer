#UNDER CONSTRUCTION...CODE IS A MESS lol

#AUDIO VISUALIZER

#NOTE: 
Sampling rate refers to sample frames per second. For stereo, 1 frame consists of **2 channels** (1 for left channel and 1 for right channel).

SDL's default samples per cycle(spc) is 4096, 4096spc. So frames per cycle(fpc) is 4096/2 == 2048fpc.


The program only works with .wav audio files that contain **signed 16 bit data** and has 2 channels (Left and Right).  However anyone can easily add code so that it can run a wav file that contains a different bit width, a different number of channels and/or uses unsigned data values. 

 
##Software:
Install fftw and libsndfile.  The source code for these programs are provided in the repository.
- Install libsndfile in ubuntu: sudo apt install sndfile-programs

Install SDL2 (https://wiki.libsdl.org/Installation)

Install GDB; used for debugging purposes if necessary.

Install g++ compiler



Install avconv ( https://libav.org/download/  if running debian based distro run "sudo apt-get install libav-tools")

##howto run:

after cloning the repo go to "src" directory and run **make**.

to run the program: **./program -f path/to/wav/file
**
make sure the path to the wav file contains no blank spaces. 
also make sure that the wav file name doesnt contain any blank spaces also.

for ex: /home/user/**music files**/song.wav    ---will not work

        /home/user/**music_files**/song.wav    ---will work

for ex: /home/user/music_files/**song 2.wav**    ---will not work
       
       /home/user/music_files/**song_2.wav**    ---will work

###While the program is running:

You are able to pause, start, restart, and rewind the song. 

Press p to pause, s to start, r to restart, and q to quit or b <sec> to rewind song...

![music_visualizer.jpg](https://bitbucket.org/repo/zbG9rd/images/3398881550-music_visualizer.jpg)

##Brief overview about how the program works:

Analyzes the digital information in the wav file.

Computes the fourier transformation every 2048 frames. Eg. computes the fourier transformation (FT) on 2048 L channel data and also computes the FT on 2048 R channel data.

Gathers frequency and magnitude information from the results of the fourier transformation and records it by storing into an array.

After analysis is finished the program will play music and display the corresponding information at the terminal.

##Documentation
http://www.fftw.org/#documentation

https://wiki.libsdl.org/FrontPage