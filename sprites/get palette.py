''' this script takes an image and returns the colour palette'''
import string

fr = open("c:/Users/wayne/Desktop/new images/palette.c","r")
fw = open("c:/Users/wayne/Desktop/new images/palette.txt","w")


''' remove whitespaces and commas '''
data = fr.read()
data = "".join(data.split()).split(",")

''' remove duplicate from image '''
palette = sorted(set(data))

fw.write("palette: " + str(len(palette)) + "\n")
fw.write(str(palette).replace("'",""))

fr.close()
fw.close()

print("done")

