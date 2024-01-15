# AUDIO VISUALIZER

# NOTE: 
Sampling rate refers to sample frames per second. For stereo, 1 frame consists of **2 channels** (1 for left channel and 1 for right channel).

SDL's default samples per cycle(spc) is 4096, 4096spc. So frames per cycle(fpc) is 4096 == 4096fpc.

SDL's audio device will change the default samples cycle:

```c++
SDL_AudioSpec               wavSpec, have;                //SDL data type to analyze WAV file.

SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &wavSpec, &have,
            SDL_AUDIO_ALLOW_ANY_CHANGE);
```
So we must check and update our audio information:

```c++
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
```

The program only works with .wav audio files that contain **signed 16 bit data** and has 2 channels (Left and Right).  However anyone can easily add code so that it can run a wav file that contains a different bit width, a different number of channels and/or uses unsigned data values. 

 
## Software:
Install fftw and libsndfile.  The source code for these programs are provided in the repository.
- Install libsndfile in ubuntu:
  ```bash
  sudo apt install sndfile-programs
  ```

Install SDL2 (https://wiki.libsdl.org/Installation)
- Note: make sure to install sdl2, alsa, and pulseaudio development (or any development files like pipewire that your systems uses to play sound ) files before installing SDL2.

Install GDB; used for debugging purposes if necessary.

Install g++ compiler

Install Make

Install avconv ( https://libav.org/download/  if running debian based distro run "sudo apt-get install libav-tools")

FFTW: http://www.fftw.org/download.html

pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/

## howto run:

after cloning the repo go to "src" directory and run **make**.

```bash
git clone https://github.com/Crelloc/terminal-music-visualizer.git && cd terminal-music-visualizer && make
```

to run the program:
```bash
./program -f path/to/wav/file
```
make sure the path to the wav file contains no blank spaces. 
also make sure that the wav file name doesnt contain any blank spaces also.

for ex: /home/user/**music files**/song.wav    ---will not work

        /home/user/**music_files**/song.wav    ---will work

for ex: /home/user/music_files/**song 2.wav**    ---will not work
       
       /home/user/music_files/**song_2.wav**    ---will work

### While the program is running:

You are able to pause, start, restart, and rewind the song. 

Press p to pause, s to start, r to restart, and q to quit or b <sec> to rewind song...

![music_visualizer.jpg](https://bitbucket.org/repo/zbG9rd/images/3398881550-music_visualizer.jpg)

## Brief overview about how the program works:

Analyzes the digital information in the wav file.

Computes the fourier transformation every 2048 frames. Eg. computes the fourier transformation (FT) on 2048 L channel data and also computes the FT on 2048 R channel data.

Gathers frequency and magnitude information from the results of the fourier transformation and records it by storing into an array.

After analysis is finished the program will play music and display the corresponding information at the terminal.

## Documentation
http://www.fftw.org/#documentation

https://wiki.libsdl.org/FrontPage
