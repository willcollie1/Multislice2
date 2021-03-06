#This program uses the output file from "multislice.c" to create a greyscale image of the unit cell.

import subprocess
import os #Run linux commands from Python program
from PIL import Image #Image Processing
import numpy as np #Poisson Distribution

def sort_file(input_filename, output_filename):
    subprocess.call(['sort -n -k3,3 -k2,2 -k1,1 <"$1" >"$2"', '_',
                     input_filename, output_filename],
                    shell=True)

def sort_fileside(input_filename, output_filename):
    subprocess.call(['sort -n -k1,1 -k2,2 -k3,3 <"$1" >"$2"', '_',
                     input_filename, output_filename],
                    shell=True)

def sort_filetop(input_filename, output_filename):
    subprocess.call(['sort -n -k2,2 -k3,3 -k1,1 <"$1" >"$2"', '_',
                     input_filename, output_filename],
                    shell=True)


subprocess.call("gcc multislice.c -o multislice -lfftw3 -lm", shell = True)

subprocess.call("python GUI.py", shell = True)

parameters = open('userinput.txt', 'r').readlines()
edose = int(parameters[7])
retile = int(parameters[11])
view = int(parameters[12])

if(view == 0):
    sort_file(
        parameters[10].rstrip('\n'),
        'orderedfile.txt',
    )

if(view == 1):
    sort_fileside(
        parameters[10].rstrip('\n'),
        'orderedfile.txt',
    )

if(view == 2):
    sort_filetop(
        parameters[10].rstrip('\n'),
        'orderedfile.txt',
    )


#Runs the multislice C program
subprocess.call("./multislice", shell = True)

#Reads in array parameters from multislice
parameters = open("size.txt").readlines()
height = int(parameters[0])
width = int(parameters[1]) 
size = height*int(width)

#Read in intensity values from multislice.c and create a list
List = open("output.txt").readlines()


#Drawing a random number from a Poission distribution centred on edose
for j in range (0,size):
  List[j] = np.random.poisson(float(List[j])*edose) 

#Normalise intensity values so that they fill the range 0-255 (8 bit binary)
maximum = max(List)
minimum = min(List)
for i in range (0,size):
    List[i] =  255*(float(List[i])-float(minimum))/(float(maximum) - float(minimum))
    List[i] = int(List[i])


# If the image needs a verticle retile
New = [None] * size
if(retile == 1):
    number1 = int(0.7*size)
    number2 = int(0.3*size)
    number3 = int(0.5*size)
    number4 = int(0.2*size)
    number5 = int(0.8*size)

    for i in range (number1,size):
        New[i-number3] = List[i]
    for i in range (0,number2):
        New[i+ number3] = List[i]     
    for i in range (0,number4):
        New[i] = 0 
    for i in range (number5,size):
        New[i] = 0 
    for i in range (0,size):
        List[i] = New[i]

# If the image needs a Qudrant retile
if(retile == 2):
    for i in range (0,(width/2)):
        for j in range(0,(height/2)):
            New[(size/2+width/2)+i+(j*width)] = List[i+j*width]
            New[i+(j*width)] = List[(size/2+width/2)+i+(j*width)]

    for i in range ((width/2),(width)):
        for j in range(0,(height/2)):
            New[(size/2)+(i-(width/2))+(j*width)] = List[i+j*width]
            New[i+(j*width)] = List[(size/2)+(i-(width/2))+(j*width)]

    for i in range (0,size):
        List[i] = New[i]


#Create greyscale image with the given intensity values.
newim = Image.new('L', (height,width))
pixels1 = newim.load()
for i in range (0,(height-1)):
    for j in range(0,(width-1)):
        l = List[i+height*j] 
 
        pixels1[i,j] = (l) 

newim.save("newim", "JPEG")