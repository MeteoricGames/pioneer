#-------------------------------------------------------------------------------
# Name:        spacescape2dds
# Purpose:     Convert SpaceScape generated cubemaps to DXT1-compressed cube DDS
#
# Author:      Salwan
#
# Created:     02/12/2013
#-------------------------------------------------------------------------------
from __future__ import generators
import os
import shlex
import subprocess

CUBEMAPS_PATH = "../../data/textures/cube/"

def generateCommand(path, name, ext):
    right = path + name + "_right1." + ext
    left = path + name + "_left2." + ext
    top = path + name + "_top3." + ext
    bottom = path + name + "_bottom4." + ext
    front = path + name + "_front5." + ext
    back = path + name + "_back6." + ext
    output = path + "../" + name + ".dds"
    command = ("""CubeMapGen -importFaceXPos:"%s" -importFaceXNeg:"%s" -importFaceYPos:"%s" -importFaceYNeg:"%s" -importFaceZPos:"%s" -importFaceZNeg:"%s" -exportFilename:"%s" -exportCubeDDS -exportPixelFormat:DXT1 -exportMipChain -consoleErrorOutput -exit"""
        % (right, left, top, bottom, front, back, output))
    return command

def convertDirectory(directory):
    print(("> Processing directory \"" + CUBEMAPS_PATH + directory + "\""))
    print("> Determining cubemap file extension")
    test_file = CUBEMAPS_PATH + directory + "/" + directory + "_right1."
    extension = ""
    if os.path.isfile(test_file + "png"):
        extension = "png"
    elif os.path.isfile(test_file + "jpg"):
        extension = "jpg"
    else:
        print(">     Folder does not contain a SpaceScape generated cubemap\n")
        return
    print("> Running CubeMapGen\n")
    command = generateCommand(CUBEMAPS_PATH + directory + "/", directory, extension)
    command_args = shlex.split(command)
    process = subprocess.check_call(command_args)

def main():
    print("This script will parse all directories found in data/textures/cube ")
    print("and convert any SpaceScape cubemaps it finds to DDS cubemaps using ")
    print("CubeMapGen.                                                        ")
    print("All cubemaps are compressed to DXT1 and include mipmaps.         \n")
    directories = next(os.walk(CUBEMAPS_PATH))[1]
    for d in directories:
        convertDirectory(d)

if __name__ == '__main__':
    main()
