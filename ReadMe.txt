Extract Images from a Video - Desktop software - FFMPEG Add to Watch List
Usage:
FrameExtractor input_file [nb_milliSec] [output_folder] [output_file_extension] [width] [height]
Program extract frames from an input file every nb_milliSec milliseconds.              
This program reads frames from a file, decodes them, and write a frame every nb_milliSec milliseconds.
input_file: path to be read.
nb_milliSec:  Optional,time to write a frame; default 0, all images will be write
output_folder: Optional, output folder, default current path
output_file_extension: Optional,image file type; 0 for bmp, 1 for jpg;default bmp
width: Optional,image width, default the video width
height: Optional,image height, default the video height