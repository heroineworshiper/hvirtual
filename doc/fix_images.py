#!/usr/bin/python


# Texinfo doesn't support linking image thumbnails to full size images.
# This scales all the images given & links them to the originals.
# Only works with a single html file, 1 IMG SRC per line


import os
import sys
import fileinput
from PIL import Image




THUMB_W = 320

# input & scaled images
IMAGES = [
    'chromakey.png', 'chromakeys.png',
    'chromakeyhsv1.png', 'chromakeyhsv1s.png',
    'chromakeyhsv2.png', 'chromakeyhsv2s.png',
    'chromakeyhsv3.png', 'chromakeyhsv3s.png',
    'chromakeyhsv4.png', 'chromakeyhsv4s.png',
    'chromakeyhsv5.png', 'chromakeyhsv5s.png',
    'chromakeyhsv6.png', 'chromakeyhsv6s.png',
    'chromakeyhsv7.png', 'chromakeyhsv7s.png',
    'chromakeyhsv8.png', 'chromakeyhsv8s.png'
]












filename = sys.argv[1]
print 'Reading', filename
# read it
file = open(filename, 'r')

# output file string
dst = []
global gotOne
gotOne = False

while True:
    line = file.readline()
    if line == "":
        break
    global new_line
    new_line = line

# get the <img src...> section
    if '<img src=' in line:
        offset1 = line.find('<img src=\"')
        if offset1 >= 0:
            line2 = line[offset1:]
            offset2 = line2.find('\">')

            if offset2 >= 0:
                offset2 = offset1 + offset2 + 2
# get the image path
                img_path = line[(offset1 + 10):]
                offset3 = img_path.find('\"')
                img_path = img_path[0:offset3]

                gotIt = False
                for i in range(0, len(IMAGES), 2):
                    if img_path == IMAGES[i]:
                        gotIt = True
                        break

                if gotIt:
                    img_path2 = IMAGES[i + 1]
                    print("Got %s -> %s" % (img_path, img_path2))

                    image = Image.open(img_path)
                    w, h = image.size
                    print("size %dx%d" % (w, h))

                    new_w = THUMB_W
                    new_h = h * THUMB_W / w

                    print("new size %dx%d" % (new_w, new_h))
                    resized_image = image.resize((new_w, new_h), Image.ANTIALIAS)
                    resized_image.save(img_path2)

                    new_line = line[0:offset1]
                    new_line += "<A HREF=\"%s\"><IMG SRC=\"%s\"></A>" % \
                        (img_path, img_path2)
                    new_line += line[offset2:]
                    print("%s offset1=%d offset2=%d" % (line, offset1, offset2))
                    print("%s" % new_line)
                    gotOne = True

    dst += new_line

# write it
if gotOne:
    file = open(filename, 'w')
    for i in dst:
        file.write(i)
    print 'Wrote file'
