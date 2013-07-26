Photomosaic
===========

Command-line photomosaic generator

Date:  6/24/2009

Photomosaic uses wxWidgets 2.8 (www.wxwidgets.org).  It has been tested under MSW and GTK.

How to use the program:
I recommend a batch/shell script (a sample is included), but the command prompt works just fine, too.  Note that all images will be cropped to be square (chopped evenly on both sides, or both top and bottom).  Here are the arguments (quotes are required for arguments with spaces, as demonstrated for -dir).  All-caps indicates an argument that you, the user, must specify:

REQUIRED
-dir "C:\COMPLETE\PATH\TO FOLDER\CONTAINING PICTURES"
    All source images must be in this directory.

-big-pic FILENAME.JPG
    This must be in the directory specified by -dir argument, or it must be a full path to the image file.  .Jpg and .bmp only.

-tilesize WIDTH
    Where WIDTH is an integer number describing the size of the tiles.  All images are re-scaled to be width x width square.  It is recommended that this number is less than the original size of the image (I use something around 75 or 100).  Use larger numbers if you're going to be making a large poster (better resolution), smaller numbers for reduced file size.

-subsize SIZE
    This is the number of pixels to use to break up the big picture into samples.  This is different from tilesize.  I recommend values between 15 and 30, depending on the resolution of the source image.  For example, if the big picture is 800 x 600, and subsize is specified to be 20, the final image will be 40 x 30 tiles (800 / 20 x 600 / 20).  If tilesize is specified to be 100, the final image resolution will be 4000 x 3000 (40 * 100 x 30 * 100).  Use smaller numbers for more resolution, larger numbers for reduced file size.

OPTIONAL
-o FILENAME.JPG
    Output image file name.  .jpg or .bmp formats supported.  If left unspecified, a default file name is used.

-subsamples N
    Where N is the number of subsamples.  If left unspecified, a default value of 1 is used (one sample per tile).  If more sub-samples are specified, each tile will be broken into smaller pieces during the color matching.  This can aid in brining out finer detail in the big picture.  I recommend 2 - 4 sub-samples.

-r
    Specifies that the search for picture files (in the directory specified with -dir) is to recurse through sub-directories.

-no-repeats
    Specifies that no image is to be used more than once in the creation of the mosaic.  The application exits with an error in the event that not enough image files exist to fulfill this requirement.

-save-thumbs C:\PATH\TO\THUMBS\DIR
    Specifies a directory to which the scaled-down thumbnails are saved.  If this is not specified, the images are not saved to disk.  This is useful because after the thumbnails are created, you can specify this directory with the -dir argument and the images will not have to be re-scaled, which will reduce execution time.

-seed SEED
	Seeds the random number generator with SEED.  If omitted, the time is used as the seed.
