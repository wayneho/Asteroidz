''' this script takes a color palette and an image and returns a look up table for that color palette '''

import string

fr = open("C:/Users/wayne/Desktop/new images/start.c","r")
fp = open("C:/Users/wayne/Desktop/new images/palette.txt","r")
fw = open("C:/Users/wayne/Desktop/new images/converted/start.txt","w")

data = fr.read()
palette = fp.read()
data = "".join(data.split()).split(",")
palette = "".join(palette.split()).split(",")

lut = []
count = 0

for x in range(len(data)):
	lut.insert(x,palette.index(data[x]))


fw.write("lut: " + str(len(lut)) + "\n\n")

fw.write(str(lut).replace("[","{").replace("]","}"))

print ("done")
fr.close()
fw.close()


