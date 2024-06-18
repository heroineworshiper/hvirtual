#!/usr/bin/python


# Texinfo doesn't support linking image thumbnails to full size images.
# This scales all the images given & links them to the originals.
# Only works with a single html file, 1 IMG SRC per line


import os
import sys
import fileinput
from PIL import Image




THUMB_W = 320

# input images
IMAGES = [
    'align_after.png',
    'align_before.png',
    'align_extend.png',
    'align_glitch.png',
    'align_silence.png',
    'align_sync.png',
    'bobcast5.png',
    'broadcast.png',
    'broadcast21.png',
    'chromakey.png',
    'chromakeyhsv1.png', 
    'chromakeyhsv2.png', 
    'chromakeyhsv3.png', 
    'chromakeyhsv4.png', 
    'chromakeyhsv5.png', 
    'chromakeyhsv6.png', 
    'chromakeyhsv7.png', 
    'chromakeyhsv8.png', 
    'compositing_pipeline.png', 
    'compositing_pipeline2.png', 
    'compressor1.png', 
    'compressor2.png', 
    'linear.png', 
    'locked_bezier.png', 
    'program.png',
    'unlocked_bezier.png', 
    'mask2.png',
    'recording.png'
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
                for i in range(0, len(IMAGES)):
                    if img_path == IMAGES[i]:
                        gotIt = True
                        break

                if gotIt:
# get the filename extension
                    offset4 = img_path.rfind('.')
# add _s before extension
                    img_path2 = img_path[0:offset4] + '_s' + img_path[offset4:]
                    print("Got %s -> %s" % (img_path, img_path2))

# test if _s image already exists & scale it
                    if not os.path.exists(img_path2):
                        image = Image.open(img_path)
                        w, h = image.size
    #                    print("size %dx%d" % (w, h))

                        new_w = THUMB_W
                        new_h = h * THUMB_W / w

    #                    print("new size %dx%d" % (new_w, new_h))
                        resized_image = image.resize((new_w, new_h), Image.ANTIALIAS)
                        resized_image.save(img_path2)

                    new_line = line[0:offset1]
                    new_line += "<A HREF=\"%s\"><IMG SRC=\"%s\"></A>" % \
                        (img_path, img_path2)
                    new_line += line[offset2:]
#                    print("%s offset1=%d offset2=%d" % (line, offset1, offset2))
#                    print("%s" % new_line)
                    gotOne = True

    dst += new_line

# write it
if gotOne:
    file = open(filename, 'w')
    for i in dst:
        file.write(i)
    print 'Wrote file'
