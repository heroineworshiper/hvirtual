/*
png2yuv
========

  Converts a collection of PNG images to a YUV4MPEG stream.
  (see png2yuv -h for help (or have a look at the function "usage"))
  
  PNG (Portable Network Graphics) is a lossless 2D image format. 
  See www.libpng.org for more information. 

  Based on rpf2yuv.c.

  Copyright (C) 1999, 2002 Gernot Ziegler (gz@lysator.liu.se)
  Copyright (C) 2001, 2004 Matthew Marjanovic (maddog@mir.com)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>

#include <sys/types.h>

#include <png.h>

#include "mjpeg_logging.h"
#include "mjpeg_types.h"

#include "yuv4mpeg.h"
#include "mpegconsts.h"

#include "subsample.h"
#include "colorspace.h"
//#include "mplexconsts.hh"

#define DEFAULT_CHROMA_MODE Y4M_CHROMA_420JPEG

#define MAXPIXELS (2800*1152)  /**< Maximum size of final image */

typedef struct _parameters 
{
  char *pngformatstr;
  uint32_t begin;       /**< the video frame start */
  int32_t numframes;   /**< -1 means: take all frames */
  y4m_ratio_t framerate;
  int interlace;   /**< will the YUV4MPEG stream be interlaced? */
  int interleave;  /**< are the PNG frames field-interleaved? */
  int verbose; /**< the verbosity of the program (see mjpeg_logging.h) */

  png_uint_32 width;
  png_uint_32 height;
  int ss_mode; /**< subsampling mode (based on ssm_id from subsample.h) */

  int new_width; /// new MPEG2 width, in case the original one is uneven
} parameters_t;


struct _parameters *sh_param; 
png_structp png_ptr;
png_infop info_ptr, end_info;
uint8_t *raw0, *raw1, *raw2;  /* buffer for RGB first, and then Y/Cb/Cr planes of decoded PNG */

/*
 * The User Interface parts 
 */

/* usage
 * Prints a short description of the program, including default values 
 * in: prog: The name of the program 
 */
static void usage(char *prog)
{
  char *h;
  
  if (NULL != (h = (char *)strrchr(prog,'/')))
    prog = h+1;
  
  fprintf(stderr, 
	  "usage: %s [ options ]\n"
	  "\n"
	  "where options are ([] shows the defaults):\n"
	  "  -v num        verbosity (0,1,2)                  [1]\n"
	  "  -b framenum   starting frame number              [0]\n"
	  "  -f framerate  framerate for output stream (fps)     \n"
	  "  -n numframes  number of frames to process        [-1 = all]\n"
	  "  -j {1}%%{2}d{3} Read PNG frames with the name components as follows:\n"
	  "               {1} PNG filename prefix (e g rendered_ )\n"
	  "               {2} Counting placeholder (like in C, printf, eg 06 ))\n"
	  "  -I x  interlacing mode:  p = none/progressive\n"
	  "                           t = top-field-first\n"
	  "                           b = bottom-field-first\n"
	  "  -L x  interleaving mode:  0 = non-interleaved (two successive\n"
	  "                                 fields per PNG file)\n"
	  "                            1 = interleaved fields\n"
          "  -S mode  chroma subsampling mode [%s]\n"
	  "\n"
	  "%s pipes a sequence of PNG files to stdout,\n"
	  "making the direct encoding of MPEG files possible under mpeg2enc.\n"
	  "Any 8bit PNG format supported by libpng can be read.\n"
	  "stdout will be filled with the YUV4MPEG movie data stream,\n"
	  "so be prepared to pipe it on to mpeg2enc or to write it into a file.\n"
	  "\n"
	  "\n"
	  "examples:\n"
	  "  %s -j in_%%06d.png -f 25 -I p -b 100000 > result.yuv\n"
	  "  | combines all the available PNGs that match \n"
	  "    in_??????.png, starting with 100000 (in_100000.png, \n"
	  "    in_100001.png, etc...) into the uncompressed YUV4MPEG videofile result.yuv\n"
	  "    The videofile has 25 frames per second and does not use any interlacing.\n"
	  "  %s -Ip -L0 -j abc_%%04d.png | mpeg2enc -f3 -o out.m2v\n"
	  "  | combines all the available PNGs that match \n"
	  "    abc_??????.png, starting with 0000 (abc_0000.png, \n"
	  "    abc_0001.png, etc...) and pipes it to mpeg2enc which encodes\n"
	  "    an MPEG2-file called out.m2v out of it\n"
	  "\n",
	  prog, y4m_chroma_keyword(DEFAULT_CHROMA_MODE), prog, prog, prog);
}



/** parse_commandline
 * Parses the commandline for the supplied parameters.
 * in: argc, argv: the classic commandline parameters
 */
static void parse_commandline(int argc, char ** argv, parameters_t *param)
{
  int c;
  
  param->pngformatstr = NULL;
  param->begin = 0;
  param->numframes = -1;
  param->framerate = y4m_fps_UNKNOWN;
  param->interlace = Y4M_UNKNOWN;
  param->interleave = -1;
  param->verbose = 1;
  param->ss_mode = DEFAULT_CHROMA_MODE;
  //param->mza_filename = NULL;
  //param->make_z_alpha = 0;

  /* parse options */
  for (;;) {
    if (-1 == (c = getopt(argc, argv, "I:hv:L:b:j:n:f:z:S:")))
      break;
    switch (c) {
    case 'j':
      param->pngformatstr = strdup(optarg);
      break;
#if 0 
    case 'z':
      param->mza_filename = strdup(optarg);
      param->make_z_alpha = 1;
      break;
#else
    case 'z':
      mjpeg_error("Z encoding currently unsupported !\n");
      exit(-1);
      break;
#endif
    case 'S':
      param->ss_mode = y4m_chroma_parse_keyword(optarg);
      if (param->ss_mode == Y4M_UNKNOWN) {
	mjpeg_error_exit1("Unknown subsampling mode option:  %s", optarg);
      } else if (!chroma_sub_implemented(param->ss_mode)) {
	mjpeg_error_exit1("Unsupported subsampling mode option:  %s", optarg);
      }
      break;
    case 'b':
      param->begin = atol(optarg);
      break;
    case 'n':
      param->numframes = atol(optarg);
      break;
    case 'f':
      param->framerate = mpeg_conform_framerate(atof(optarg));
      break;
    case 'I':
      switch (optarg[0]) {
      case 'p':
	param->interlace = Y4M_ILACE_NONE;
	break;
      case 't':
	param->interlace = Y4M_ILACE_TOP_FIRST;
	break;
      case 'b':
	param->interlace = Y4M_ILACE_BOTTOM_FIRST;
	break;
      default:
	mjpeg_error_exit1 ("-I option requires arg p, t, or b");
      }
      break;
    case 'L':
      param->interleave = atoi(optarg);
      if ((param->interleave != 0) &&
	  (param->interleave != 1)) 
	mjpeg_error_exit1 ("-L option requires arg 0 or 1");
      break;
    case 'v':
      param->verbose = atoi(optarg);
      if (param->verbose < 0 || param->verbose > 2) 
	mjpeg_error_exit1( "-v option requires arg 0, 1, or 2");    
      break;     
    case 'h':
    default:
      usage(argv[0]);
      exit(1);
    }
  }
  if (param->pngformatstr == NULL) 
    { 
      mjpeg_error("%s:  input format string not specified. (Use -j option.)",
		  argv[0]); 
      usage(argv[0]); 
      exit(1);
    }

  if (Y4M_RATIO_EQL(param->framerate, y4m_fps_UNKNOWN)) 
    {
      mjpeg_error("%s:  framerate not specified.  (Use -f option)",
		  argv[0]); 
      usage(argv[0]); 
      exit(1);
    }
}

void png_separation(png_structp png_ptr, png_row_infop row_info, png_bytep data)
{
  int row_nr = png_ptr->row_number; // internal variable ? 
  int i, width = row_info->width; 
  int new_width = sh_param->new_width;

  /* contents of row_info:
   *  png_uint_32 width      width of row
   *  png_uint_32 rowbytes   number of bytes in row
   *  png_byte color_type    color type of pixels
   *  png_byte bit_depth     bit depth of samples
   *  png_byte channels      number of channels (1-4)
   *  png_byte pixel_depth   bits per pixel (depth*channels)
   */

  //mjpeg_debug("PNG YUV transformation callback; color_type is %d row_number %d\n", 
  //	 row_info->color_type, row_nr);

  if(row_info->color_type == PNG_COLOR_TYPE_GRAY) // only Z available
    {
      //mjpeg_debug("Grayscale to YUV, row %d", row_nr);
      for (i = 0; i < width; i++)
	{
	  raw0[i + row_nr * new_width] = data[i];
	  raw1[i + row_nr * new_width] = data[i];
	  raw2[i + row_nr * new_width] = data[i];
	}
      return;
    }

  if(row_info->color_type == PNG_COLOR_TYPE_RGB) // Z and Alpha available
    {
      //mjpeg_info("RGB to YUV, row %d", row_nr);
      for (i = 0; i < width; i++)
	{
	  raw0[i + row_nr * new_width] = data[i*3];
	  raw1[i + row_nr * new_width] = data[i*3 + 1];
	  raw2[i + row_nr * new_width] = data[i*3 + 2];
	}
      return;
    }

  mjpeg_error_exit1("mpegz: UNKNOWN COLOR FORMAT %d in PNG transformation !\n", row_info->color_type);
}


/*
 * The file handling parts 
 */
/** 
Reads one PNG file. 
@param process Process the image data (0 for initial parameter determination)
@returns -1 on failure, 1 on sucess
*/
int decode_png(const char *pngname, int process, parameters_t *param)
{
  int num_pass = 1;
  int bit_depth, color_type;
  FILE *pngfile;
  //png_byte hdptr[8];

  /* Now open this PNG file, and examine its header to retrieve the 
     YUV4MPEG info that shall be written */
  pngfile = fopen(pngname, "rb");
  if (!pngfile)
    {
      perror("PNG file open failed:");
      return -1;
    }

  //fread(hdptr, 1, 8, pngfile);

#if 0 
  bool is_png = !png_sig_cmp(hdptr, 0, 8);
  if (!is_png)
    {
      mjpeg_error("%s is _no_ PNG file !\n");
      return -1;
    }
#endif
  
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    mjpeg_error_exit1("%s: Could not allocate PNG read struct !", pngname);
  
  png_init_io(png_ptr, pngfile);
  //png_set_sig_bytes(png_ptr, 8);
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
      png_destroy_read_struct(&png_ptr,
			      (png_infopp)NULL, (png_infopp)NULL);
      mjpeg_error_exit1("%s: Could not allocate PNG info struct !", pngname);
    }
  
  end_info = png_create_info_struct(png_ptr);
  if (!end_info)
    {
      png_destroy_read_struct(&png_ptr, &info_ptr,
			      (png_infopp)NULL);
      mjpeg_error_exit1("%s: Could not allocate PNG end info struct !", pngname);
    }
  
  if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr,
			      &end_info);
      mjpeg_error("%s: Corrupted PNG file !", pngname);
      return -1;
    }
  
  if (process)
    png_set_read_user_transform_fn(png_ptr, png_separation);
  png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA, NULL);
  
  if (png_get_IHDR(png_ptr, info_ptr, &param->width, &param->height, &bit_depth,
		       //   &color_type, &interlace_type, &compression_type, &filter_type))
		   &color_type, NULL, NULL, NULL))	
    num_pass = png_set_interlace_handling(png_ptr);
  else
    mjpeg_error_exit1("PNG header reading failed !!\n");
#if 0 
  mjpeg_info("Reading info struct...\n");
  png_read_info(png_ptr, info_ptr);
  mjpeg_info("Done...\n");
  
  if (png_get_IHDR(png_ptr, info_ptr, &param->width, &param->height, &bit_depth,
		       //   &color_type, &interlace_type, &compression_type, &filter_type))
		   &color_type, NULL, NULL, NULL))	
    num_pass = png_set_interlace_handling(png_ptr);
  else
    mjpeg_error_exit1("PNG header reading failed !!\n");
  
  if (process)
    {
      printf("%d passes needed\n\n", num_pass);
      
      if (bit_depth != 8 && bit_depth != 16)
	{
	  mjpeg_error_exit1("Invalid bit_depth %d, only 8 and 16 bit allowed !!\n", bit_depth);
	}
      
      png_set_strip_16(png_ptr); // always has to strip the 16bit input, MPEG can't handle it	  
      png_set_strip_alpha(png_ptr); // Alpha can't be processed until Z/Alpha is integrated
      
      printf("\nAllocating row buffer...");
      png_set_read_user_transform_fn(png_ptr, png_separation);
      png_bytep row_buf = (png_bytep)png_malloc(png_ptr,
						png_get_rowbytes(png_ptr, info_ptr));
      
      for (int n=0; n < num_pass; n++)
	for (int y=0; y < sh_param->height; y++)
	  {
	    printf("Writing row data for pass %d\n", n);
	    png_read_rows(png_ptr, (png_bytepp)&row_buf, NULL, 1);
	  }
      
      png_free(png_ptr, row_buf);	      
    }
  png_read_end(png_ptr, info_ptr);
#endif  
  if (setjmp(png_ptr->jmpbuf)) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return 2;
    }

  fclose(pngfile);

  return 1;
}


/** init_parse_files
 * Verifies the PNG input files and prepares YUV4MPEG header information.
 * @returns 0 on success
 */
static int init_parse_files(parameters_t *param)
{ 
  char pngname[255];

  snprintf(pngname, sizeof(pngname), 
	   param->pngformatstr, param->begin);
  mjpeg_debug("Analyzing %s to get the right pic params", pngname);
  
  if (decode_png(pngname, 0, param) == -1)
    mjpeg_error_exit1("Reading of %s failed.\n", pngname);

  mjpeg_info("Image dimensions are %ux%u",
	     param->width, param->height);
  
  mjpeg_info("Movie frame rate is:  %f frames/second",
	     Y4M_RATIO_DBL(param->framerate));

  switch (param->interlace) 
    {
    case Y4M_ILACE_NONE:
      mjpeg_info("Non-interlaced/progressive frames.");
      break;
    case Y4M_ILACE_BOTTOM_FIRST:
      mjpeg_info("Interlaced frames, bottom field first.");      
      break;
    case Y4M_ILACE_TOP_FIRST:
      mjpeg_info("Interlaced frames, top field first.");      
      break;
    default:
      mjpeg_error_exit1("Interlace has not been specified (use -I option)");
      break;
    }

  if ((param->interlace != Y4M_ILACE_NONE) && (param->interleave == -1))
    mjpeg_error_exit1("Interleave has not been specified (use -L option)");

  if (!(param->interleave) && (param->interlace != Y4M_ILACE_NONE)) 
    {
      param->height *= 2;
      mjpeg_info("Non-interleaved fields (image height doubled)");
    }
  mjpeg_info("Frame size:  %u x %u", param->width, param->height);

  return 0;
}

static int generate_YUV4MPEG(parameters_t *param)
{
  uint32_t frame;
  //size_t pngsize;
  char pngname[FILENAME_MAX];
  uint8_t *yuv[3];  /* buffer for Y/U/V planes of decoded PNG */
  y4m_stream_info_t streaminfo;
  y4m_frame_info_t frameinfo;

  if ((param->width % 2) == 0)
    param->new_width = param->width;
  else
    {
      param->new_width = ((param->width >> 1) + 1) << 1;
      printf("Setting new, even image width %d", param->new_width);
    }

  mjpeg_info("Now generating YUV4MPEG stream.");
  y4m_init_stream_info(&streaminfo);
  y4m_init_frame_info(&frameinfo);

  y4m_si_set_width(&streaminfo, param->new_width);
  y4m_si_set_height(&streaminfo, param->height);
  y4m_si_set_interlace(&streaminfo, param->interlace);
  y4m_si_set_framerate(&streaminfo, param->framerate);
  y4m_si_set_chroma(&streaminfo, param->ss_mode);

  yuv[0] = (uint8_t *)malloc(param->new_width * param->height * sizeof(yuv[0][0]));
  yuv[1] = (uint8_t *)malloc(param->new_width * param->height * sizeof(yuv[1][0]));
  yuv[2] = (uint8_t *)malloc(param->new_width * param->height * sizeof(yuv[2][0]));

  y4m_write_stream_header(STDOUT_FILENO, &streaminfo);

  for (frame = param->begin;
       (frame < param->numframes + param->begin) || (param->numframes == -1);
       frame++) 
    {
      //      if (frame < 25)
      //      else      
      //snprintf(pngname, sizeof(pngname), param->pngformatstr, frame - 25);
      snprintf(pngname, sizeof(pngname), param->pngformatstr, frame);
            
      raw0 = yuv[0];
      raw1 = yuv[1];
      raw2 = yuv[2];
      if (decode_png(pngname, 1, param) == -1)
	{
	  mjpeg_info("Read from '%s' failed:  %s", pngname, strerror(errno));
	  if (param->numframes == -1) 
	    {
	      mjpeg_info("No more frames.  Stopping.");
	      break;  /* we are done; leave 'while' loop */
	    } 
	  else 
	    {
	      mjpeg_info("Rewriting latest frame instead.");
	    }
	} 
      else 
	{
#if 0 
	  mjpeg_debug("Preparing frame");
	  
	  /* Now open this PNG file, and examine its header to retrieve the 
	     YUV4MPEG info that shall be written */

	  if ((param->interlace == Y4M_ILACE_NONE) || (param->interleave == 1)) 
	    {
	      mjpeg_info("Processing non-interlaced/interleaved %s.", 
			 pngname, pngsize);

	      decode_png(imagedata, 0, 420, yuv[0], yuv[1], yuv[2], 
			 param->width, param->height, param->new_width);
	      
#if 0 
	      if (param->make_z_alpha)
		{
		  mjpeg_info("Writing Z/Alpha data.\n");
		  za_write(real_z_imagemap, param->width, param->height,z_alpha_fp,frame);
		}
#endif
	    } 
	  else 
	    {
	      mjpeg_error_exit1("Can't handle interlaced PNG information (yet) since there is no standard for it.\n"
				"Use interleaved mode (-L option) to create interlaced material.");

	      switch (param->interlace) 
		{		  
		case Y4M_ILACE_TOP_FIRST:
		  mjpeg_info("Processing interlaced, top-first %s", pngname);
#if 0 
		  decode_jpeg_raw(jpegdata, jpegsize,
				  Y4M_ILACE_TOP_FIRST,
				  420, param->width, param->height,
				  yuv[0], yuv[1], yuv[2]);
#endif
		  break;
		case Y4M_ILACE_BOTTOM_FIRST:
		  mjpeg_info("Processing interlaced, bottom-first %s", pngname);
#if 0 
		  decode_jpeg_raw(jpegdata, jpegsize,
				  Y4M_ILACE_BOTTOM_FIRST,
				  420, param->width, param->height,
				  yuv[0], yuv[1], yuv[2]);
#endif
		  break;
		default:
		  mjpeg_error_exit1("FATAL logic error?!?");
		  break;
		}
	    }
#endif
	  mjpeg_debug("Converting frame to YUV format.");
	  /* Transform colorspace, then subsample (in place) */
	  convert_RGB_to_YCbCr(yuv, param->height *  param->new_width);
	  chroma_subsample(param->ss_mode, yuv, param->new_width, param->height);

	  mjpeg_debug("Frame decoded, now writing to output stream.");
	}
      
      mjpeg_debug("Frame decoded, now writing to output stream.");
      y4m_write_frame(STDOUT_FILENO, &streaminfo, &frameinfo, yuv);
    }

#if 0 
  if (param->make_z_alpha)
    {
      za_write_end(z_alpha_fp);
      fclose(z_alpha_fp);
    }
#endif

  y4m_fini_stream_info(&streaminfo);
  y4m_fini_frame_info(&frameinfo);
  free(yuv[0]);
  free(yuv[1]);
  free(yuv[2]);

  return 0;
}



/* main
 * in: argc, argv:  Classic commandline parameters. 
 * returns: int: 0: success, !0: !success :-)
 */
int main(int argc, char ** argv)
{ 
  parameters_t param;
  sh_param = &param;

  y4m_accept_extensions(1);

  parse_commandline(argc, argv, &param);
  mjpeg_default_handler_verbosity(param.verbose);

  mjpeg_info("Parsing & checking input files.");
  if (init_parse_files(&param)) {
    mjpeg_error_exit1("* Error processing the PNG input.");
  }

  if (generate_YUV4MPEG(&param)) { 
    mjpeg_error_exit1("* Error processing the input files.");
  }

  return 0;
}










