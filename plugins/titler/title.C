
/*
 * CINELERRA
 * Copyright (C) 1997-2016 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

// Originally developed by Heroine Virtual Ltd.
// Support for multiple encodings by 
// Andraz Tori <Andraz.tori1@guest.arnes.si>
// Additional support for UTF-8 by
// Paolo Rampino aka Akirad <info at tuttoainternet.it>




#include "bcsignals.h"
#include "clip.h"
#include "colormodels.h"
#include "filexml.h"
#include "filesystem.h"
#include "language.h"
#include "mwindow.inc"
#include "picon_png.h"
#include "cicolors.h"
#include "title.h"
#include "titlewindow.h"
#include "transportque.inc"


#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include <iconv.h>
#include <fontconfig/fontconfig.h>

#define FONT_SEARCHPATH "/fonts"

REGISTER_PLUGIN(TitleMain)

static YUV yuv;

TitleConfig::TitleConfig()
{
	style = 0;
	color = BLACK;
	outline_color = WHITE;
	alpha = 0xff;
	outline_alpha = 0xff;
	size = 24;
	motion_strategy = NO_MOTION;
	loop = 0;
	hjustification = JUSTIFY_CENTER;
	vjustification = JUSTIFY_MID;
	fade_in = 0.0;
	fade_out = 0.0;
	x = 0.0;
	y = 0.0;
	dropshadow = 10;
	sprintf(font, "fixed");

	text.assign("");

	subs_path.assign("");
	use_subs = 0;

#ifdef X_HAVE_UTF8_STRING
#define DEFAULT_ENCODING "UTF-8"
	encoding[0] = 0;
#else
	sprintf(encoding, "ISO8859-1");
#endif
	pixels_per_second = 1.0;
	timecode = 0;
	timecode_format = TIME_HMS;
	outline_size = 0;
	window_w = 640;
	window_h = 480;
	ucs4text = 0;
	tlen = 0;
	next_keyframe_position = 0;
	prev_keyframe_position = 0;
}

TitleConfig::~TitleConfig()
{
	delete [] ucs4text;
}

// Does not test equivalency but determines if redrawing text is necessary.
int TitleConfig::equivalent(TitleConfig &that)
{
	return dropshadow == that.dropshadow &&
		style == that.style &&
		size == that.size &&
		color == that.color &&
		outline_color == that.outline_color &&
		alpha == that.alpha &&
		outline_alpha == that.outline_alpha &&
		timecode == that.timecode && 
		timecode_format == that.timecode_format && 
		outline_size == that.outline_size && 
		hjustification == that.hjustification &&
		vjustification == that.vjustification &&
		use_subs == that.use_subs &&
		!subs_path.compare(that.subs_path) &&
		EQUIV(pixels_per_second, that.pixels_per_second) &&
		!strcasecmp(font, that.font) &&
		!strcasecmp(encoding, that.encoding) &&
		!text.compare(that.text);
}

void TitleConfig::copy_from(TitleConfig &that)
{
	strcpy(font, that.font);
	style = that.style;
	size = that.size;
	color = that.color;
	outline_color = that.outline_color;
	alpha = that.alpha;
	outline_alpha = that.outline_alpha;
	pixels_per_second = that.pixels_per_second;
	motion_strategy = that.motion_strategy;
	loop = that.loop;
	hjustification = that.hjustification;
	vjustification = that.vjustification;
	fade_in = that.fade_in;
	fade_out = that.fade_out;
	x = that.x;
	y = that.y;
	dropshadow = that.dropshadow;
	timecode = that.timecode;
	timecode_format = that.timecode_format;
	outline_size = that.outline_size;
	text.assign(that.text);
	use_subs = that.use_subs;
	subs_path.assign(that.subs_path);
	strcpy(encoding, that.encoding);
	window_w = that.window_w;
	window_h = that.window_h;

	limits();
}

void TitleConfig::interpolate(TitleConfig &prev, 
	TitleConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	strcpy(font, prev.font);
	strcpy(encoding, prev.encoding);
	style = prev.style;
	size = prev.size;
	color = prev.color;
	outline_color = prev.outline_color;
	alpha = prev.alpha;
	outline_alpha = prev.outline_alpha;
	motion_strategy = prev.motion_strategy;
	loop = prev.loop;
	hjustification = prev.hjustification;
	vjustification = prev.vjustification;
	fade_in = prev.fade_in;
	fade_out = prev.fade_out;
	outline_size = prev.outline_size;
	pixels_per_second = prev.pixels_per_second;
	text.assign(prev.text);
	use_subs = prev.use_subs;
	subs_path.assign(prev.subs_path);

	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


//	this->x = prev.x * prev_scale + next.x * next_scale;
//	this->y = prev.y * prev_scale + next.y * next_scale;
	this->x = prev.x;
	this->y = prev.y;
	timecode = prev.timecode;
	timecode_format = prev.timecode_format;
//	this->dropshadow = (int)(prev.dropshadow * prev_scale + next.dropshadow * next_scale);
	this->dropshadow = prev.dropshadow;
	
	this->window_w = prev.window_w;
	this->window_h = prev.window_h;

	limits();
}





void TitleConfig::limits()
{
	if(window_w < 100) window_w = 100;
	if(window_h < 100) window_h = 100;
}



// this is a little routine that converts no-utf8 text to utf8 // akirad
void TitleMain::convert_encoding()
{
#ifdef X_HAVE_UTF8_STRING
	int utf8 = 1;
#else
	int utf8 = 0;
#endif

	if(strcmp(config.encoding,"UTF-8"))
	{
		iconv_t cd;
		char *utf8text = new char[(config.text.length() + 1) * KEYPRESSLEN];
		FcChar8 return_utf8;
		cd = iconv_open("UTF-8",config.encoding);


		if(cd == (iconv_t)-1)
		{
// Something went wrong.
			printf("TitleMain::convert_encoding: Iconv conversion from %s to UTF-8 not available\n",
				config.encoding);
			config.textutf8.assign(config.text);	
		}
		else
// if iconv is working ok for current encoding
		{
			char *temp_string = new char[config.text.length() + 1];
			char *in_ptr = temp_string;
			strcpy(in_ptr, config.text.c_str());
			char *out_ptr = utf8text;
			size_t inbytes = config.text.length() - 1;
			size_t outbytes = config.text.length() * KEYPRESSLEN;
			int noconv = 0;

//printf("TitleMain::convert_encoding %d %p\n", __LINE__, in_ptr);
			do 
			{
				if(iconv(cd, &in_ptr, &inbytes, &out_ptr, &outbytes) == (size_t)-1)
				{
					printf("iconv failed!\n");
					noconv = 1;
				}
			} while (inbytes > 0 && outbytes > 0);

			outbytes = 0;
			if(!noconv || utf8)
			{
				if(utf8)
				{
//printf("TitleMain::convert_encoding %d\n", __LINE__);
					config.text.assign(utf8text);
					strcpy(config.encoding, "UTF-8");
				}
				else
				{
					if(!noconv)
					{
//printf("TitleMain::convert_encoding %d textutf8=%p\n", __LINE__, config.textutf8);
						config.textutf8.assign(utf8text);
					}
					else
					{
						config.textutf8.assign(config.text);
					}
				}
			}

			int iconv_closed = iconv_close(cd);
			if(iconv_closed != 0)
			{
				fprintf(stderr,"iconv_close failed: %s\n",strerror(errno));
			}

//printf("TitleMain::convert_encoding %d %p\n", __LINE__, in_ptr);
			delete [] temp_string;
		}

		delete [] utf8text;
	}
	else 
	if(!utf8)
	{
		config.textutf8.assign(config.text);	
	}

}







// this is a little routine that converts 8 bit string to FT_ULong array // akirad
void TitleConfig::convert_text()
{
#ifdef X_HAVE_UTF8_STRING
	int utf8 = 1;
#else
	int utf8 = 0;
#endif

	int text_len;
	if(utf8) 
		text_len = text.length();
	else
		text_len = textutf8.length();

	tlen = 0;
	for(int i = 0; i < text_len; i++)
	{
		tlen++;
		int x = 0;  
		int z;

		if(utf8)
			z = (unsigned char)text[i];
		else
			z = (unsigned char)textutf8[i];

		if (!(z & 0x80)) 
		{ 
			x = 0;
	 	} 
		else 
		if (!(z & 0x20)) 
	 	{ 
	 		x = 1;
        } 
		else 
		if (!(z & 0x10)) 
        { 
         		x = 2;
        } 
		else 
		if (!(z & 0x08)) 
        { 
         		x = 3;
        } 
		else 
		if (!(z & 0x04)) 
        { 
         		x = 4;
        } 
		else 
		if (!(z & 0x02)) 
        { 
        	x = 5;
        }
        i += x;
	}
	
	if(ucs4text) delete [] ucs4text;
	ucs4text = new FT_ULong [tlen + 1];
	FcChar32 retunucs4;
	int count = 0;
	for(int i = 0; i < text_len; i++)
	{
		int x = 0;  
		int z;
		if(utf8)
			z = (char)text[i];
		else
			z = (char)textutf8[i];

		FcChar8 loadutf8[KEYPRESSLEN];
		if (!(z & 0x80)) 
		{ 
			x = 0;
	 	} 
		else 
		if (!(z & 0x20)) 
	 	{ 
	 		x = 2;
        } 
		else 
		if (!(z & 0x10)) 
        { 
         	x = 3;
        } 
		else 
		if (!(z & 0x08)) 
        { 
         	x = 4;
        } 
		else 
		if (!(z & 0x04)) 
        { 
         	x = 5;
        } 
		else 
		if (!(z & 0x02)) 
        { 
        	x = 6;
        }

//delete array
		for(int r = 0; r < KEYPRESSLEN; r++)
			loadutf8[r] = 0;

		if( x > 0 )
		{
//fill array
			for(int p = 0; p < x; p++)
			{
				if(utf8)
					loadutf8[p] = text[i+p];
				else
					loadutf8[p] = textutf8[i+p];
			}

			i += (x - 1);
		}
		else
		{
//fill array
		    loadutf8[0] = z;
		}

		FcUtf8ToUcs4 (loadutf8, &retunucs4, KEYPRESSLEN);
		ucs4text[count] = (FT_ULong)retunucs4;
		count++;
	}
}











FontEntry::FontEntry()
{
	image = 0;
	path = 0;
	foundary = 0;
	family = 0;
	weight = 0;
	slant = 0;
	swidth = 0;
	adstyle = 0;
	spacing = 0;
	registry = 0;
	encoding = 0;
	fixed_title = 0;
	fixed_style = 0;
	pixelsize = 0;
	avg_width = 0;
	xres = 0;
	pointsize = 0;
	yres = 0;
}

FontEntry::~FontEntry()
{
	delete image;
	if(path) delete [] path;
	if(foundary) delete [] foundary;
	if(family) delete [] family;
	if(weight) delete [] weight;
	if(slant) delete [] slant;
	if(swidth) delete [] swidth;
	if(adstyle) delete [] adstyle;
	if(spacing) delete [] spacing;
	if(registry) delete [] registry;
	if(encoding) delete [] encoding;
	if(fixed_title) delete [] fixed_title;
}

void FontEntry::dump()
{
	printf("%s: %s %s %s %s %s %s %d %d %d %d %s %d %s %s\n",
		path,
		foundary,
		family,
		weight,
		slant,
		swidth,
		adstyle,
		pixelsize,
		pointsize,
		xres,
		yres,
		spacing,
		avg_width,
		registry,
		encoding);
}

TitleGlyph::TitleGlyph()
{
	char_code = 0;
	c = 0;
	data = 0;
	freetype_index = 0;
	height = 0;
	top = 0;
	width = 0;
	advance_w = 0;
	pitch = 0;
	left = 0;
}


TitleGlyph::~TitleGlyph()
{
//printf("TitleGlyph::~TitleGlyph 1\n");
	if(data) delete data;
}















GlyphPackage::GlyphPackage() : LoadPackage()
{
	glyph = 0;
}

GlyphUnit::GlyphUnit(TitleMain *plugin, GlyphEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
	current_font = 0;
	freetype_library = 0;
	freetype_face = 0;
}

GlyphUnit::~GlyphUnit()
{
	if(freetype_library) FT_Done_FreeType(freetype_library);
}

void GlyphUnit::process_package(LoadPackage *package)
{
	GlyphPackage *pkg = (GlyphPackage*)package;
	TitleGlyph *glyph = pkg->glyph;
	int result = 0;
	int altfont = 0;


	current_font = plugin->get_font();

	char new_path[BCTEXTLEN];
	plugin->check_char_code_path(freetype_library,
					current_font->path,
					glyph->char_code,
					(char *)new_path);

	if(plugin->load_freetype_face(freetype_library,
		freetype_face,
		new_path))
	{
		printf("GlyphUnit::process_package FT_New_Face failed.\n");
		result = 1;
	}
	else
	{
		FT_Set_Pixel_Sizes(freetype_face, plugin->config.size, 0);
	}

	if(!result)
	{

		if(FT_Load_Char(freetype_face, glyph->char_code, FT_LOAD_RENDER))
		{
			printf("GlyphUnit::process_package FT_Load_Char failed.\n");
// Prevent a crash here
			glyph->width = 8;
			glyph->height = 8;
			glyph->pitch = 8;
			glyph->left = 9;
			glyph->top = 9;
			glyph->freetype_index = 0;
			glyph->advance_w = 8;
			glyph->data = new VFrame;
			glyph->data->set_use_shm(0);
			glyph->data->reallocate(0,
				-1,
				0,
				0,
				0,
				8,
				8,
				BC_A8,
				8);
		}
		else
		{
			glyph->width = freetype_face->glyph->bitmap.width;
//			glyph->height = freetype_face->glyph->bitmap.rows;
			glyph->height = freetype_face->glyph->bitmap_top +
				freetype_face->glyph->bitmap.rows;
			glyph->pitch = freetype_face->glyph->bitmap.pitch;
			glyph->left = freetype_face->glyph->bitmap_left;
			glyph->top = freetype_face->glyph->bitmap_top;
			glyph->freetype_index = FT_Get_Char_Index(freetype_face, glyph->char_code);
			glyph->advance_w = (freetype_face->glyph->advance.x >> 6);

//printf("GlyphUnit::process_package 1 width=%d height=%d pitch=%d left=%d top=%d advance_w=%d freetype_index=%d\n", 
//	glyph->width, glyph->height, glyph->pitch, glyph->left, glyph->top, glyph->advance_w, glyph->freetype_index);

//printf("GlyphUnit::process_package 1\n");
			glyph->data = new VFrame;
			glyph->data->set_use_shm(0);
			glyph->data->reallocate(0,
				-1,
				0,
				0,
				0,
				glyph->width,
				glyph->height,
				BC_A8,
				glyph->pitch);
			glyph->data->clear_frame();

//printf("GlyphUnit::process_package 2\n");
			for(int i = 0; i < freetype_face->glyph->bitmap.rows; i++)
			{
				memcpy(glyph->data->get_rows()[i], 
					freetype_face->glyph->bitmap.buffer + glyph->pitch * i,
					glyph->pitch);
			}
		}
	}
}

GlyphEngine::GlyphEngine(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}
void GlyphEngine::init_packages()
{
	int current_package = 0;
	for(int i = 0; i < plugin->glyphs.total; i++)
	{
		if(!plugin->glyphs.values[i]->data)
		{
			GlyphPackage *pkg = (GlyphPackage*)get_package(current_package++);
			pkg->glyph = plugin->glyphs.values[i];
		}
	}
}
LoadClient* GlyphEngine::new_client()
{
	return new GlyphUnit(plugin, this);
}
LoadPackage* GlyphEngine::new_package()
{
	return new GlyphPackage;
}













// Copy a single character to the text mask
TitlePackage::TitlePackage()
 : LoadPackage()
{
	x = 0;
	y = 0;
	c = 0;
}


TitleUnit::TitleUnit(TitleMain *plugin, TitleEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->engine = server;
}

void TitleUnit::draw_glyph(VFrame *output, TitleGlyph *glyph, int x, int y)
{
	int glyph_w = glyph->data->get_w();
	int glyph_h = glyph->data->get_h();
	int output_w = output->get_w();
	int output_h = output->get_h();
	unsigned char **in_rows = glyph->data->get_rows();
	unsigned char **out_rows = output->get_rows();
	int r, g, b, a;
	plugin->get_color_components(&r, &g, &b, &a, 0);
	int outline = plugin->config.outline_size;
	if(outline) a = 0xff;

//printf("TitleUnit::draw_glyph 1 %c %d %d\n", glyph->c, x, y);
	for(int in_y = 0; in_y < glyph_h; in_y++)
	{
		int y_out = y + plugin->ascent + in_y - glyph->top;

		if(y_out >= 0 && y_out < output_h)
		{
			unsigned char *in_row = in_rows[in_y];
			int x1 = x + glyph->left;
			int y1 = y_out;

			if(engine->do_dropshadow)
			{
// Direct copy glyph value to black alpha
				x1 += plugin->config.dropshadow;
				y1 += plugin->config.dropshadow;

				if(y1 < output_h)
				{
					unsigned char *out_row = out_rows[y1];
					for(int in_x = 0; in_x < glyph_w; in_x++)
					{
						int x_out = x1 + in_x;
						if(x_out >= 0 && x_out < output_w)
						{
							if(in_row[in_x] > 0)
								out_row[x_out * 4 + 3] = in_row[in_x];
						}
					}
				}
			}
			else
			{
				unsigned char *out_row = out_rows[y1];
// Blend color value with shadow using glyph alpha.
				for(int in_x = 0; in_x < glyph_w; in_x++)
				{
					int x_out = x1 + in_x;
					if(x_out >= 0 && x_out < output_w)
					{
						int opacity = in_row[in_x] * a / 0xff;
						int transparency = out_row[x_out * 4 + 3] * (0xff - opacity) / 0xff;
//						int transparency = 0xff - opacity;
						if(in_row[in_x] > 0)
						{
							out_row[x_out * 4 + 0] = (r * opacity + 
								out_row[x_out * 4 + 0] * transparency) / 0xff;
							out_row[x_out * 4 + 1] = (g * opacity + 
								out_row[x_out * 4 + 1] * transparency) / 0xff;
							out_row[x_out * 4 + 2] = (b * opacity + 
								out_row[x_out * 4 + 2] * transparency) / 0xff;
							out_row[x_out * 4 + 3] = MAX(opacity,
								out_row[x_out * 4 + 3]);
						}
					}
				}
			}
		}
	}
}


void TitleUnit::process_package(LoadPackage *package)
{
	TitlePackage *pkg = (TitlePackage*)package;
	
	if(pkg->c != 0xa)
	{
		for(int i = 0; i < plugin->glyphs.total; i++)
		{
			TitleGlyph *glyph = plugin->glyphs.values[i];
			if(glyph->c == pkg->c)
			{
				draw_glyph(plugin->text_mask, glyph, pkg->x, pkg->y);
				break;
			}
		}
	}
}

TitleEngine::TitleEngine(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void TitleEngine::init_packages()
{
	int visible_y1 = plugin->visible_row1 * plugin->get_char_height();
	int current_package = 0;
	for(int i = plugin->visible_char1; i < plugin->visible_char2; i++)
	{
		title_char_position_t *char_position = plugin->char_positions + i;
		TitlePackage *pkg = (TitlePackage*)get_package(current_package);
		pkg->x = char_position->x + plugin->config.outline_size;
		pkg->y = char_position->y - visible_y1 + plugin->config.outline_size;
		pkg->c = plugin->config.ucs4text[i];
		current_package++;
	}
}

LoadClient* TitleEngine::new_client()
{
	return new TitleUnit(plugin, this);
}

LoadPackage* TitleEngine::new_package()
{
	return new TitlePackage;
}














// Copy a single character to the text mask
TitleOutlinePackage::TitleOutlinePackage()
 : LoadPackage()
{
}


TitleOutlineUnit::TitleOutlineUnit(TitleMain *plugin, TitleOutlineEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->engine = server;
}

void TitleOutlineUnit::process_package(LoadPackage *package)
{
	TitleOutlinePackage *pkg = (TitleOutlinePackage*)package;
	int r, g, b, outline_a;
	int title_r, title_g, title_b, title_a;
	plugin->get_color_components(&r, &g, &b, &outline_a, 1);
	plugin->get_color_components(&title_r, &title_g, &title_b, &title_a, 0);


	if(engine->pass == 0)
	{


		for(int i = pkg->y1; i < pkg->y2; i++)
		{
			unsigned char *out_row = plugin->outline_mask->get_rows()[i];
			for(int j = 0; j < plugin->text_mask->get_w(); j++)
			{
				int x1 = j - plugin->config.outline_size;
				int x2 = j + plugin->config.outline_size;
				int y1 = i - plugin->config.outline_size;
				int y2 = i + plugin->config.outline_size;
				CLAMP(x1, 0, plugin->text_mask->get_w() - 1);
				CLAMP(x2, 0, plugin->text_mask->get_w() - 1);
				CLAMP(y1, 0, plugin->text_mask->get_h() - 1);
				CLAMP(y2, 0, plugin->text_mask->get_h() - 1);
				int max_r = 0;
				int max_g = 0;
				int max_b = 0;
				int max_a = 0;

				for(int k = y1; k <= y2; k++)
				{
					unsigned char *text_row = plugin->text_mask->get_rows()[k];
					for(int l = x1; l <= x2; l++)
					{
						unsigned char *pixel = text_row + l * 4;
						if(pixel[3] > max_a)
						{
							max_r = pixel[0];
							max_g = pixel[1];
							max_b = pixel[2];
							max_a = pixel[3];
						}
					}
				}

				unsigned char *out_pixel = out_row + j * 4;
				out_pixel[0] = r;
				out_pixel[1] = g;
				out_pixel[2] = b;
				out_pixel[3] = (max_a * outline_a) / 0xff;
			}
		}
	}
	else
	{
// Overlay text mask on top of outline mask
		for(int i = pkg->y1; i < pkg->y2; i++)
		{
			unsigned char *out_row = plugin->text_mask->get_rows()[i];
			unsigned char *in_row = plugin->outline_mask->get_rows()[i];
			for(int j = 0; j < plugin->text_mask->get_w(); j++)
			{
				unsigned char *out_pixel = out_row + j * 4;
				unsigned char *in_pixel = in_row + j * 4;
				int out_a = out_pixel[3];
				int in_a = in_pixel[3];
				int transparency = in_a * (0xff - out_a) / 0xff;
				out_pixel[0] = (out_pixel[0] * out_a + in_pixel[0] * transparency) / 0xff;
				out_pixel[1] = (out_pixel[1] * out_a + in_pixel[1] * transparency) / 0xff;
				out_pixel[2] = (out_pixel[2] * out_a + in_pixel[2] * transparency) / 0xff;
				int temp = in_a - out_a;
				if(temp < 0) temp = 0;
				out_pixel[3] = temp + out_a * title_a / 0xff;
			}
		}
	}
}

TitleOutlineEngine::TitleOutlineEngine(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void TitleOutlineEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		TitleOutlinePackage *pkg = (TitleOutlinePackage*)get_package(i);
		pkg->y1 = plugin->text_mask->get_h() * i / get_total_packages();
		pkg->y2 = plugin->text_mask->get_h() * (i + 1) / get_total_packages();
	}
}

void TitleOutlineEngine::do_outline()
{
	pass = 0;
	process_packages();
	pass = 1;
	process_packages();
}

LoadClient* TitleOutlineEngine::new_client()
{
	return new TitleOutlineUnit(plugin, this);
}

LoadPackage* TitleOutlineEngine::new_package()
{
	return new TitleOutlinePackage;
}











TitleTranslatePackage::TitleTranslatePackage()
 : LoadPackage()
{
	y2 = 0;
	y1 = 0;
}


TitleTranslateUnit::TitleTranslateUnit(TitleMain *plugin, TitleTranslate *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}




static void translation_array_f(transfer_table_f* &table, 
	float out_x1, 
	float out_x2,
	float in_x1,
	float in_x2,
	int in_total, 
	int out_total, 
	int &out_x1_int,
	int &out_x2_int)
{
	int out_w_int;
	float offset = out_x1 - in_x1;
//printf("OverlayFrame::translation_array_f 1 %f %f -> %f %f\n", in_x1, in_x2, out_x1, out_x2);

	out_x1_int = (int)out_x1;
	out_x2_int = MIN((int)ceil(out_x2), out_total);
	out_w_int = out_x2_int - out_x1_int;

	table = new transfer_table_f[out_w_int];
	bzero(table, sizeof(transfer_table_f) * out_w_int);


// printf("OverlayFrame::translation_array_f 2 %f %f -> %f %f scale=%f %f\n", 
// in_x1, 
// in_x2, 
// out_x1, 
// out_x2,
// in_x2 - in_x1,
// out_x2 - out_x1);
// 

	float in_x = in_x1;
	for(int out_x = out_x1_int; out_x < out_x2_int; out_x++)
	{
		transfer_table_f *entry = &table[out_x - out_x1_int];

		entry->in_x1 = (int)in_x;
		entry->in_x2 = (int)in_x + 1;

// Get fraction of output pixel to fill
		entry->output_fraction = 1;

		if(out_x1 > out_x)
		{
			entry->output_fraction -= out_x1 - out_x;
		}

		if(out_x2 < out_x + 1)
		{
			entry->output_fraction = (out_x2 - out_x);
		}

// Advance in_x until out_x_fraction is filled
		float out_x_fraction = entry->output_fraction;
		float in_x_fraction = floor(in_x + 1) - in_x;

		if(out_x_fraction <= in_x_fraction)
		{
			entry->in_fraction1 = out_x_fraction;
			entry->in_fraction2 = 0.0;
			in_x += out_x_fraction;
		}
		else
		{
			entry->in_fraction1 = in_x_fraction;
			in_x += out_x_fraction;
			entry->in_fraction2 = in_x - floor(in_x);
		}

// Clip in_x and zero out fraction.  This doesn't work for YUV.
		if(entry->in_x2 >= in_total)
		{
			entry->in_x2 = in_total - 1;
			entry->in_fraction2 = 0.0;
		}
		
		if(entry->in_x1 >= in_total)
		{
			entry->in_x1 = in_total - 1;
			entry->in_fraction1 = 0.0;
		}
// printf("OverlayFrame::translation_array_f 2 %d %d %d %f %f %f\n", 
// 	out_x, 
// 	entry->in_x1, 
// 	entry->in_x2, 
// 	entry->in_fraction1, 
// 	entry->in_fraction2, 
// 	entry->output_fraction);
 	}
}



#define TRANSLATE(type, max, components) \
{ \
	unsigned char **in_rows = plugin->text_mask->get_rows(); \
	type **out_rows = (type**)plugin->output->get_rows(); \
 \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		if(i + server->out_y1_int >= 0 && \
			i + server->out_y1_int < server->output_h) \
		{ \
			int in_y1, in_y2; \
			float y_fraction1, y_fraction2; \
			float y_output_fraction; \
			in_y1 = server->y_table[i].in_x1; \
			in_y2 = server->y_table[i].in_x2; \
			y_fraction1 = server->y_table[i].in_fraction1; \
			y_fraction2 = server->y_table[i].in_fraction2; \
			y_output_fraction = server->y_table[i].output_fraction; \
			unsigned char *in_row1 = in_rows[in_y1]; \
			unsigned char *in_row2 = in_rows[in_y2]; \
			type *out_row = out_rows[i + server->out_y1_int]; \
 \
			for(int j = server->out_x1_int; j < server->out_x2_int; j++) \
			{ \
				if(j >= 0 && j < server->output_w) \
				{ \
					int in_x1; \
					int in_x2; \
					float x_fraction1; \
					float x_fraction2; \
					float x_output_fraction; \
					in_x1 =  \
						server->x_table[j - server->out_x1_int].in_x1; \
					in_x2 =  \
						server->x_table[j - server->out_x1_int].in_x2; \
					x_fraction1 =  \
						server->x_table[j - server->out_x1_int].in_fraction1; \
					x_fraction2 =  \
						server->x_table[j - server->out_x1_int].in_fraction2; \
					x_output_fraction =  \
						server->x_table[j - server->out_x1_int].output_fraction; \
 \
					float fraction1 = x_fraction1 * y_fraction1; \
					float fraction2 = x_fraction2 * y_fraction1; \
					float fraction3 = x_fraction1 * y_fraction2; \
					float fraction4 = x_fraction2 * y_fraction2; \
					type input_r = (type)(in_row1[in_x1 * 4 + 0] * fraction1 +  \
								in_row1[in_x2 * 4 + 0] * fraction2 +  \
								in_row2[in_x1 * 4 + 0] * fraction3 +  \
								in_row2[in_x2 * 4 + 0] * fraction4); \
					type input_g = (type)(in_row1[in_x1 * 4 + 1] * fraction1 +  \
								in_row1[in_x2 * 4 + 1] * fraction2 +  \
								in_row2[in_x1 * 4 + 1] * fraction3 +  \
								in_row2[in_x2 * 4 + 1] * fraction4); \
					type input_b = (type)(in_row1[in_x1 * 4 + 2] * fraction1 +  \
								in_row1[in_x2 * 4 + 2] * fraction2 +  \
								in_row2[in_x1 * 4 + 2] * fraction3 +  \
								in_row2[in_x2 * 4 + 2] * fraction4); \
					type input_a = (type)(in_row1[in_x1 * 4 + 3] * fraction1 +  \
								in_row1[in_x2 * 4 + 3] * fraction2 +  \
								in_row2[in_x1 * 4 + 3] * fraction3 +  \
								in_row2[in_x2 * 4 + 3] * fraction4); \
/* Plugin alpha is actually 0 - 0x100 */ \
					input_a = input_a * plugin->alpha / 0x100; \
					type transparency; \
 \
 \
					if(components == 4) \
					{ \
						transparency = out_row[j * components + 3] * (max - input_a) / max; \
						out_row[j * components + 0] =  \
							(input_r * input_a + out_row[j * components + 0] * transparency) / max; \
						out_row[j * components + 1] =  \
							(input_g * input_a + out_row[j * components + 1] * transparency) / max; \
						out_row[j * components + 2] =  \
							(input_b * input_a + out_row[j * components + 2] * transparency) / max; \
						out_row[j * components + 3] =  \
							MAX(input_a, out_row[j * components + 3]); \
					} \
					else \
					{ \
						transparency = max - input_a; \
						out_row[j * components + 0] =  \
							(input_r * input_a + out_row[j * components + 0] * transparency) / max; \
						out_row[j * components + 1] =  \
							(input_g * input_a + out_row[j * components + 1] * transparency) / max; \
						out_row[j * components + 2] =  \
							(input_b * input_a + out_row[j * components + 2] * transparency) / max; \
					} \
				} \
			} \
		} \
	} \
}


void TitleTranslateUnit::process_package(LoadPackage *package)
{
	TitleTranslatePackage *pkg = (TitleTranslatePackage*)package;
	TitleTranslate *server = (TitleTranslate*)this->server;

	switch(plugin->output->get_color_model())
	{
		case BC_RGB888:
		{
			TRANSLATE(unsigned char, 0xff, 3);
			break;
		}
		case BC_RGB_FLOAT:
		{
			TRANSLATE(float, 1.0, 3);
			break;
		}
		case BC_YUV888:
		{
			TRANSLATE(unsigned char, 0xff, 3);
			break;
		}
		case BC_RGBA_FLOAT:
		{
			TRANSLATE(float, 1.0, 4);
			break;
		}
		case BC_RGBA8888:
		{	
			TRANSLATE(unsigned char, 0xff, 4);
			break;
		}
		case BC_YUVA8888:
		{	
			TRANSLATE(unsigned char, 0xff, 4);
			break;
		}
	}

//printf("TitleTranslateUnit::process_package 5\n");
}




TitleTranslate::TitleTranslate(TitleMain *plugin, int cpus)
 : LoadServer(1, 1)
{
	this->plugin = plugin;
	x_table = y_table = 0;
	out_y1 = 0;
	out_y2 = 0;
	out_y1_int = 0;
	out_y2_int = 0;
	output_h = 0;
	out_x1 = 0;
	out_x2 = 0;
	out_x1_int = 0;
	out_x2_int = 0;
	output_w = 0;
}

TitleTranslate::~TitleTranslate()
{
	if(x_table) delete [] x_table;
	if(y_table) delete [] y_table;
}

void TitleTranslate::init_packages()
{
//printf("TitleTranslate::init_packages 1\n");
// Generate scaling tables
	if(x_table) delete [] x_table;
	if(y_table) delete [] y_table;
//printf("TitleTranslate::init_packages 1\n");

	output_w = plugin->output->get_w();
	output_h = plugin->output->get_h();
//printf("TitleTranslate::init_packages 1 %f %d\n", plugin->text_x1, plugin->text_w);


	translation_array_f(x_table, 
		plugin->text_x1 - plugin->config.outline_size, 
		plugin->text_x1 + plugin->text_w - plugin->config.outline_size,
		0,
		plugin->text_w,
		plugin->text_w, 
		output_w, 
		out_x1_int,
		out_x2_int);
//printf("TitleTranslate::init_packages 1 %f %f\n", plugin->mask_y1, plugin->mask_y2);

	translation_array_f(y_table, 
		plugin->mask_y1 + plugin->config.outline_size, 
		plugin->mask_y1 + plugin->text_mask->get_h() + plugin->config.outline_size,
		0,
		plugin->text_mask->get_h(),
		plugin->text_mask->get_h(), 
		output_h, 
		out_y1_int,
		out_y2_int);

//printf("TitleTranslate::init_packages 1\n");


	out_y1 = out_y1_int;
	out_y2 = out_y2_int;
	out_x1 = out_x1_int;
	out_x2 = out_x2_int;
	int increment = (out_y2 - out_y1) / get_total_packages() + 1;

//printf("TitleTranslate::init_packages 1 %d %d %d %d\n", 
//	out_y1, out_y2, out_y1_int, out_y2_int);
	for(int i = 0; i < get_total_packages(); i++)
	{
		TitleTranslatePackage *pkg = (TitleTranslatePackage*)get_package(i);
		pkg->y1 = i * increment;
		pkg->y2 = i * increment + increment;
		if(pkg->y1 > out_y2 - out_y1)
			pkg->y1 = out_y2 - out_y1;
		if(pkg->y2 > out_y2 - out_y1)
			pkg->y2 = out_y2 - out_y1;
	}
//printf("TitleTranslate::init_packages 2\n");
}

LoadClient* TitleTranslate::new_client()
{
	return new TitleTranslateUnit(plugin, this);
}

LoadPackage* TitleTranslate::new_package()
{
	return new TitleTranslatePackage;
}















ArrayList<FontEntry*>* TitleMain::fonts = 0;



TitleMain::TitleMain(PluginServer *server)
 : PluginVClient(server)
{
	text_mask = 0;
	outline_mask = 0;
	glyph_engine = 0;
	title_engine = 0;
	freetype_library = 0;
	freetype_face = 0;
	char_positions = 0;
	translate = 0;
	outline_engine = 0;
	need_reconfigure = 1;
	ascent = 0;
	height = 0;
}

TitleMain::~TitleMain()
{
//printf("TitleMain::~TitleMain 1\n");
	
	if(text_mask) delete text_mask;
	if(outline_mask) delete outline_mask;
	if(char_positions) delete [] char_positions;
	clear_glyphs();
	if(glyph_engine) delete glyph_engine;
	if(title_engine) delete title_engine;
	if(freetype_library) FT_Done_FreeType(freetype_library);
	if(translate) delete translate;
	if(outline_engine) delete outline_engine;

	subtitle_db.remove_all_objects();
}

const char* TitleMain::plugin_title() { return N_("Title"); }
int TitleMain::is_realtime() { return 1; }
int TitleMain::is_synthesis() { return 1; }

VFrame* TitleMain::new_picon()
{
	return new VFrame(picon_png);
}

NEW_WINDOW_MACRO(TitleMain, TitleWindow);

void TitleMain::build_fonts()
{
	int debug = 0;

	if(!fonts)
	{
		fonts = new ArrayList<FontEntry*>;
		char find_command[BCTEXTLEN];
		sprintf(find_command, 
			"find %s%s -name 'fonts.dir' -print -exec cat {} \\;", 
			PluginClient::get_plugin_dir(),
			FONT_SEARCHPATH);
		FILE *in = popen(find_command, "r");
		char current_dir[BCTEXTLEN];
		FT_Library freetype_library = 0;      	// Freetype library
		FT_Face freetype_face = 0;

//		FT_Init_FreeType(&freetype_library);
		current_dir[0] = 0;

		while(!feof(in))
		{
			char string[BCTEXTLEN], string2[BCTEXTLEN];
			char *temp = fgets(string, BCTEXTLEN, in);
			if(!strlen(string)) break;

			char *in_ptr = string;
			char *out_ptr;

// Get current directory
			
			if(string[0] == '/')
			{
				out_ptr = current_dir;
				while(*in_ptr != 0 && *in_ptr != 0xa)
					*out_ptr++ = *in_ptr++;
				out_ptr--;
				while(*out_ptr != '/')
					*out_ptr-- = 0;
			}
			else
			{


//printf("TitleMain::build_fonts %s\n", string);
				FontEntry *entry = new FontEntry;

// Path
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa)
				{
					if(*in_ptr == ' ' && *(in_ptr + 1) == '-') break;
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				if(string2[0] == '/')
				{
					entry->path = new char[strlen(string2) + 1];
					sprintf(entry->path, "%s", string2);
				}
				else
				{
					entry->path = new char[strlen(current_dir) + strlen(string2) + 1];
					sprintf(entry->path, "%s%s", current_dir, string2);
				}

// Foundary
				while(*in_ptr != 0 && *in_ptr != 0xa && (*in_ptr == ' ' || *in_ptr == '-'))
					in_ptr++;

				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != ' ' && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->foundary = new char[strlen(string2) + 1];
				strcpy(entry->foundary, string2);
				if(*in_ptr == '-') in_ptr++;


// Family
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->family = new char[strlen(string2) + 1];
				strcpy(entry->family, string2);
				if(*in_ptr == '-') in_ptr++;

// Weight
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->weight = new char[strlen(string2) + 1];
				strcpy(entry->weight, string2);
				if(*in_ptr == '-') in_ptr++;

// Slant
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->slant = new char[strlen(string2) + 1];
				strcpy(entry->slant, string2);
				if(*in_ptr == '-') in_ptr++;

// SWidth
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->swidth = new char[strlen(string2) + 1];
				strcpy(entry->swidth, string2);
				if(*in_ptr == '-') in_ptr++;

// Adstyle
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->adstyle = new char[strlen(string2) + 1];
				strcpy(entry->adstyle, string2);
				if(*in_ptr == '-') in_ptr++;

// pixelsize
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->pixelsize = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// pointsize
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->pointsize = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// xres
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->xres = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// yres
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->yres = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// spacing
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->spacing = new char[strlen(string2) + 1];
				strcpy(entry->spacing, string2);
				if(*in_ptr == '-') in_ptr++;

// avg_width
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->avg_width = atol(string2);
				if(*in_ptr == '-') in_ptr++;

// registry
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa && *in_ptr != '-')
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->registry = new char[strlen(string2) + 1];
				strcpy(entry->registry, string2);
				if(*in_ptr == '-') in_ptr++;

// encoding
				out_ptr = string2;
				while(*in_ptr != 0 && *in_ptr != 0xa)
				{
					*out_ptr++ = *in_ptr++;
				}
				*out_ptr = 0;
				entry->encoding = new char[strlen(string2) + 1];
				strcpy(entry->encoding, string2);



// Add to list
				if(strlen(entry->foundary))
				{
//printf("TitleMain::build_fonts 1 %s\n", entry->path);
// This takes a real long time to do.  Instead just take all fonts
// 					if(!load_freetype_face(freetype_library, 
// 						freetype_face,
// 						entry->path))
//					if(1)
					if(entry->family[0])
					{
// Fix parameters
						sprintf(string, "%s (%s)", entry->family, entry->foundary);
						entry->fixed_title = new char[strlen(string) + 1];
						strcpy(entry->fixed_title, string);

						if(!strcasecmp(entry->weight, "demibold") ||
							!strcasecmp(entry->weight, "bold")) 
							entry->fixed_style |= FONT_BOLD;
						if(!strcasecmp(entry->slant, "i") ||
							!strcasecmp(entry->slant, "o")) 
							entry->fixed_style |= FONT_ITALIC;
						fonts->append(entry);
						if(debug) printf("TitleMain::build_fonts %d %s: success\n",
							__LINE__,
							entry->path);
//printf("TitleMain::build_fonts 2\n");
					}
					else
					{
						if(debug) printf("TitleMain::build_fonts %d %s: FT_New_Face failed\n",
							__LINE__,
							entry->path);
//printf("TitleMain::build_fonts 3\n");
						delete entry;
					}
				}
				else
				{
					if(debug) printf("TitleMain::build_fonts %d path=%s no foundary\n", __LINE__, entry->path);
					delete entry;
				}
			}
		}
		pclose(in);


// Load all the fonts from fontconfig
		FcPattern *pat;
		FcFontSet *fs;
		FcObjectSet *os;
		FcChar8 *family,
			*file,
			*foundry,
			*style,
			*format;
		int slant,
			spacing,
			width,
			weight;
		int force_style = 0;
		int limit_to_trutype = 0; // if you want limit search to TrueType put 1
		FcConfig *config;
		FcBool resultfc;
		int i;
		char tmpstring[BCTEXTLEN];
		resultfc = FcInit();
		config = FcConfigGetCurrent();
		FcConfigSetRescanInterval(config, 0);

		pat = FcPatternCreate();
		os = FcObjectSetBuild ( FC_FAMILY,
					FC_FILE,
					FC_FOUNDRY,
					FC_WEIGHT,
					FC_WIDTH,
					FC_SLANT,
					FC_FONTFORMAT,
					FC_SPACING,
					FC_STYLE,
					(char *) 0);
		fs = FcFontList(config, pat, os);
		FcPattern *font;

		for (i = 0; fs && i < fs->nfont; i++)
		{
			font = fs->fonts[i];
			force_style = 0;
			FcPatternGetString(font, FC_FONTFORMAT, 0, &format);
			if((!strcmp((char *)format, "TrueType")) || limit_to_trutype) //on this point you can limit font search
			{
				sprintf(tmpstring, "%s", format);

				FontEntry *entry = new FontEntry;

				if(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
				{
					sprintf(tmpstring, "%s", file);
					entry->path = new char[strlen(tmpstring) + 1];
					sprintf(entry->path, "%s", tmpstring);
				}

				if(FcPatternGetString(font, FC_FOUNDRY, 0, &foundry) == FcResultMatch)
				{
					sprintf(tmpstring, "%s", foundry);
					entry->foundary = new char[strlen(tmpstring) + 2];
					strcpy(entry->foundary, tmpstring);
				}

				if(FcPatternGetInteger(font, FC_WEIGHT, 0, &weight) == FcResultMatch)
				{
					switch(weight)
					{
						case FC_WEIGHT_THIN:
						case FC_WEIGHT_EXTRALIGHT:
						case FC_WEIGHT_LIGHT:
						case FC_WEIGHT_BOOK:
							force_style = 1;
							entry->weight = new char[strlen("medium") + 1];
							strcpy(entry->weight, "medium");
							break;

						case FC_WEIGHT_NORMAL:
						case FC_WEIGHT_MEDIUM:
						default:
							entry->weight = new char[strlen("medium") + 1];
							strcpy(entry->weight, "medium");
							break;

						case FC_WEIGHT_BLACK:
						case FC_WEIGHT_SEMIBOLD:
						case FC_WEIGHT_BOLD:
							entry->weight = new char[strlen("bold") + 1];
							strcpy(entry->weight, "bold");
							entry->fixed_style |= FONT_BOLD;
							break;

						case FC_WEIGHT_EXTRABOLD:
						case FC_WEIGHT_EXTRABLACK:
							force_style = 1;
							entry->weight = new char[strlen("bold") + 1];
							strcpy(entry->weight, "bold");
							entry->fixed_style |= FONT_BOLD;
							break;
						break;
					}
				}

				if(FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch)
				{
					sprintf(tmpstring, "%s", family);
					entry->family = new char[strlen(tmpstring) + 2];
					strcpy(entry->family, tmpstring);
				}

				if(FcPatternGetInteger(font, FC_SLANT, 0, &slant) == FcResultMatch)
				{
					switch(slant)
					{
						case FC_SLANT_ROMAN:
						default:
							entry->slant = new char[strlen("r") + 1];
							strcpy(entry->slant, "r");
							break;
						case FC_SLANT_ITALIC:
							entry->slant = new char[strlen("i") + 1];
							strcpy(entry->slant, "i");
							entry->fixed_style |= FONT_ITALIC;
							break;
						case FC_SLANT_OBLIQUE:
							entry->slant = new char[strlen("o") + 1];
							strcpy(entry->slant, "o");
							entry->fixed_style |= FONT_ITALIC;
							break;
					}
				}

				if(FcPatternGetInteger(font, FC_WIDTH, 0, &width) == FcResultMatch)
				{
					switch(width)
					{
						case FC_WIDTH_ULTRACONDENSED:
							entry->swidth = new char[strlen("ultracondensed") + 1];
							strcpy(entry->swidth, "ultracondensed");
							break;

						case FC_WIDTH_EXTRACONDENSED:
							entry->swidth = new char[strlen("extracondensed") + 1];
							strcpy(entry->swidth, "extracondensed");
							break;

						case FC_WIDTH_CONDENSED:
							entry->swidth = new char[strlen("condensed") + 1];
							strcpy(entry->swidth, "condensed");
							break;
						case FC_WIDTH_SEMICONDENSED:
							entry->swidth = new char[strlen("semicondensed") + 1];
							strcpy(entry->swidth, "semicondensed");
							break;

						case FC_WIDTH_NORMAL:
						default:
							entry->swidth = new char[strlen("normal") + 1];
							strcpy(entry->swidth, "normal");
							break;

						case FC_WIDTH_SEMIEXPANDED:
							entry->swidth = new char[strlen("semiexpanded") + 1];
							strcpy(entry->swidth, "semiexpanded");
							break;

						case FC_WIDTH_EXPANDED:
							entry->swidth = new char[strlen("expanded") + 1];
							strcpy(entry->swidth, "expanded");
							break;

						case FC_WIDTH_EXTRAEXPANDED:
							entry->swidth = new char[strlen("extraexpanded") + 1];
							strcpy(entry->swidth, "extraexpanded");
							break;

						case FC_WIDTH_ULTRAEXPANDED:
							entry->swidth = new char[strlen("ultraexpanded") + 1];
							strcpy(entry->swidth, "ultraexpanded");
							break;
					}
				}

				if(FcPatternGetInteger(font, FC_SPACING, 0, &spacing) == FcResultMatch)
				{
					switch(spacing)
					{
						case 0:
						default:
							entry->spacing = new char[strlen("p") + 1];
							strcpy(entry->spacing, "p");
							break;

						case 90:
							entry->spacing = new char[strlen("d") + 1];
							strcpy(entry->spacing, "d");
							break;

						case 100:
							entry->spacing = new char[strlen("m") + 1];
							strcpy(entry->spacing, "m");
							break;

						case 110:
							entry->spacing = new char[strlen("c") + 1];
							strcpy(entry->spacing, "c");
							break;
					}

				}

				// Add fake stuff for compatibility
				entry->adstyle = new char[strlen(" ") + 1];
				strcpy(entry->adstyle, " ");
				entry->pixelsize = 0;
				entry->pointsize = 0;
				entry->xres = 0;
				entry->yres = 0;
				entry->avg_width = 0;
				entry->registry = new char[strlen("utf") + 1];
				strcpy(entry->registry, "utf");
				entry->encoding = new char[strlen("8") + 1];
				strcpy(entry->encoding, "8");

				if(!FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch) force_style = 0;

				// If font has a style unmanaged by titler plugin, force style to be displayed on name
				// in this way we can shown all available fonts styles.
				if(force_style)
				{
					sprintf(tmpstring, "%s (%s)", entry->family, style);
					entry->fixed_title = new char[strlen(tmpstring) + 1];
					strcpy(entry->fixed_title, tmpstring);
				}
				else
				{
					if(strcmp(entry->foundary, "unknown"))
					{
						sprintf(tmpstring, "%s (%s)", entry->family, entry->foundary);
						entry->fixed_title = new char[strlen(tmpstring) + 1];
						strcpy(entry->fixed_title, tmpstring);
					}
					else
					{
						sprintf(tmpstring, "%s", entry->family);
						entry->fixed_title = new char[strlen(tmpstring) + 1];
						strcpy(entry->fixed_title, tmpstring);
					}

				}
				fonts->append(entry);
			}
		}
		if(fs) FcFontSetDestroy(fs);






		if(freetype_library) FT_Done_FreeType(freetype_library);
	}


// for(int i = 0; i < fonts->total; i++)
//	fonts->values[i]->dump();


}


void TitleMain::build_previews(TitleWindow *gui)
{
	for(int font_number = 0; font_number < fonts->size(); font_number++)
	{
		FontEntry *font_entry = fonts->get(font_number);
// already have examples
		if(font_entry->image) return;
	}

// create example bitmaps
	FT_Library freetype_library = 0;      	// Freetype library
	FT_Face freetype_face = 0;
	const char *test_string = "Aa";
	char new_path[BCTEXTLEN];
	int text_height = gui->get_text_height(LARGEFONT);
	int text_color = BC_WindowBase::get_resources()->default_text_color;
	int r = (text_color >> 16) & 0xff;
	int g = (text_color >> 8) & 0xff;
	int b = text_color & 0xff;
// dimensions for each line
	int height[fonts->size()];
	int ascent[fonts->size()];

// pass 1 gets the extents for all the fonts
// pass 2 draws the image
	int total_w = 0;
	int total_h = 0;
	int total_ascent = 0;
	for(int pass = 0; pass < 2; pass++)
	{
//printf("TitleMain::build_previews %d %d %d\n", 
//__LINE__, 
//text_height,
//total_h);
		for(int font_number = 0; font_number < fonts->size(); font_number++)
		{
			FontEntry *font_entry = fonts->get(font_number);

// test if font of same name has been processed
			int skip = 0;
			for(int i = 0; i < font_number; i++)
			{
				if(!strcasecmp(fonts->get(i)->fixed_title, 
					font_entry->fixed_title))
				{
					if(pass == 1) 
					{
						font_entry->image = fonts->get(i)->image;
					}
					
					skip = 1;
					break;
				}
			}


			if(skip) continue;
			
			int current_x = 0;
			int current_w = 0;
			int current_ascent = 0;
			int current_h = 0;
			if(pass == 1)
			{
				font_entry->image = new VFrame;
				font_entry->image->set_use_shm(0);
				font_entry->image->reallocate(0,
					-1,
					0,
					0,
					0,
					total_w,
					total_h,
					BC_RGBA8888,
					-1);
				font_entry->image->clear_frame();
			}

			current_x = 0;
			current_w = 0;
			for(int j = 0; j < strlen(test_string); j++)
			{
				FT_ULong c = test_string[j];
				check_char_code_path(freetype_library,
					font_entry->path,
					c,
					(char *)new_path);
				if(!load_freetype_face(freetype_library,
					freetype_face,
					new_path))
				{
					FT_Set_Pixel_Sizes(freetype_face, text_height, 0);

					if(!FT_Load_Char(freetype_face, c, FT_LOAD_RENDER))
					{
						if(pass == 0)
						{
							current_w = current_x + freetype_face->glyph->bitmap.width;
							if(freetype_face->glyph->bitmap_top > current_ascent)
								current_ascent = freetype_face->glyph->bitmap_top;
							if(freetype_face->glyph->bitmap.rows > total_h)
								total_h = freetype_face->glyph->bitmap.rows;
							if(freetype_face->glyph->bitmap.rows > current_h)
								current_h = freetype_face->glyph->bitmap.rows;
						}
						else
						{
// copy 1 row at a time
// center vertically
							int out_y = (total_h - height[font_number]) / 2 +
								ascent[font_number] - freetype_face->glyph->bitmap_top;
							for(int in_y = 0; 
								in_y < freetype_face->glyph->bitmap.rows &&
									out_y < total_h; 
								in_y++, out_y++)
							{
								unsigned char *out_row = font_entry->image->get_rows()[out_y] +
									current_x * 4;
								unsigned char *in_row = freetype_face->glyph->bitmap.buffer + 
									freetype_face->glyph->bitmap.pitch * in_y;

								for(int out_x = 0; out_x < freetype_face->glyph->bitmap.width &&
									out_x < total_w;
									out_x++)
								{
									*out_row++ = (*in_row * r + 
										(0xff - *in_row) * *out_row) / 0xff;
									*out_row++ = (*in_row * g +
										(0xff - *in_row) * *out_row) / 0xff;
									*out_row++ = (*in_row * b +
										(0xff - *in_row) * *out_row) / 0xff;
									*out_row++ = MAX(*in_row, *out_row);
									in_row++;
								}
							}
						}


						current_x += freetype_face->glyph->advance.x >> 6;


// if(pass == 0)
// {
// printf("TitleMain::build_fonts %d %c %d %d\n", 
// __LINE__, 
// c,
// freetype_face->glyph->advance.x >> 6,
// freetype_face->glyph->bitmap.width);
// }
					}
				}
			}
			
			height[font_number] = current_h;
			ascent[font_number] = current_ascent;
			if(pass == 0 && current_w > total_w) total_w = current_w;

		}
	}

	if(freetype_library) FT_Done_FreeType(freetype_library);
}



//This checks if char_code is on the selected font, else it changes font to the first compatible //Akirad
int TitleMain::check_char_code_path(FT_Library &freetype_library,
	char *path_old, 
	FT_ULong &char_code,
	char *path_new)
{
	FT_Face temp_freetype_face;
	FcPattern *pat;
	FcFontSet *fs;
	FcObjectSet *os;
	FcChar8 *file, *format;
	FcConfig *config;
	FcBool resultfc;
	int i;

	resultfc = FcInit();
	config = FcConfigGetCurrent();
	FcConfigSetRescanInterval(config, 0);

	pat = FcPatternCreate();
	os = FcObjectSetBuild ( FC_FILE, FC_FONTFORMAT, (char *) 0);
	fs = FcFontList(config, pat, os);
	FcPattern *font;
	int notfindit = 1;
	char tmpstring[BCTEXTLEN];
	int limit_to_truetype = 0; //if you want to limit search to truetype put 1
	if(!freetype_library) FT_Init_FreeType(&freetype_library);
	if(!FT_New_Face(freetype_library,
					path_old,
					0,
					&temp_freetype_face))
	{
		FT_Set_Pixel_Sizes(temp_freetype_face, 128, 0);
		int gindex = FT_Get_Char_Index(temp_freetype_face, char_code);
		if((!gindex == 0) && (!char_code != 10)) 
		{
			strcpy(path_new, path_old);
			notfindit = 0;
		}
	}

	if(notfindit)
	{
		for (i=0; fs && i < fs->nfont; i++)
		{
			font = fs->fonts[i];
			FcPatternGetString(font, FC_FONTFORMAT, 0, &format);
			if((!strcmp((char *)format, "TrueType")) || limit_to_truetype)
			{
				if(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
				{

					sprintf(tmpstring, "%s", file);
					if(!FT_New_Face(freetype_library,
								tmpstring,
								0,
								&temp_freetype_face))
					{
						FT_Set_Pixel_Sizes(temp_freetype_face, 128, 0);
						int gindex = FT_Get_Char_Index(temp_freetype_face, char_code);
						if((!gindex == 0) && (!char_code != 10))
						{
							sprintf(path_new, "%s", tmpstring);
							notfindit = 0;
							goto done;

						}
					}
				}
			}
		}
	}

done:
	if(fs) FcFontSetDestroy(fs);
	if(temp_freetype_face) FT_Done_Face(temp_freetype_face);
	temp_freetype_face = 0;

	if(notfindit)
	{
		strcpy(path_new, path_old);
		return 1;
	}

	return 0;
}








int TitleMain::load_freetype_face(FT_Library &freetype_library,
	FT_Face &freetype_face,
	char *path)
{
//printf("TitleMain::load_freetype_face 1\n");
	if(!freetype_library) FT_Init_FreeType(&freetype_library);
	if(freetype_face) FT_Done_Face(freetype_face);
	freetype_face = 0;
//printf("TitleMain::load_freetype_face 2\n");

// Use freetype's internal function for loading font
	if(FT_New_Face(freetype_library, 
		path, 
		0,
		&freetype_face))
	{
		fprintf(stderr, "TitleMain::load_freetype_face %s failed.\n", path);
		freetype_face = 0;
		return 1;
	} else
	{
		return 0;
	}

//printf("TitleMain::load_freetype_face 4\n");
}

FontEntry* TitleMain::get_font_entry(char *title,
	int style,
	int size)
{
//printf("TitleMain::get_font_entry %s %d %d\n", title, style, size);
	FontEntry *result = 0;
	int got_title = 0;

	for(int i = 0; i < fonts->total; i++)
	{
		FontEntry *entry = fonts->values[i];

		if(!result) result = entry;

		if(!strcmp(title, entry->fixed_title))
		{
			if(!got_title) result = entry;
			got_title = 1;

// Not every font has a size but every font has a style
			if(entry->fixed_style == style)
				result = entry;

			if(entry->fixed_style == style && entry->pointsize == size) 
				result = entry;

		}
	}

	return result;
}


FontEntry* TitleMain::get_font()
{
	return get_font_entry(config.font,
		config.style,
		config.size);
}




int TitleMain::get_char_width(FT_ULong c)
{
	if(c == 0xa) return 0;

	for(int i = 0; i < glyphs.size(); i++)
	{
		if(glyphs.get(i)->c == c)
		{
			return glyphs.get(i)->width;
		}
	}
	
	return 0;
}

int TitleMain::get_char_height()
{
//	return config.size;
	return height;
}

int TitleMain::get_char_advance(int current, int next)
{
	FT_Vector kerning;
	int result = 0;
	TitleGlyph *current_glyph = 0;
	TitleGlyph *next_glyph = 0;

	if(current == 0xa) return 0;

	for(int i = 0; i < glyphs.size(); i++)
	{
		if(glyphs.get(i)->c == current)
		{
			current_glyph = glyphs.get(i);
			break;
		}
	}

	for(int i = 0; i < glyphs.size(); i++)
	{
		if(glyphs.get(i)->c == next)
		{
			next_glyph = glyphs.get(i);
			break;
		}
	}

	if(current_glyph)
		result = current_glyph->advance_w;

//printf("TitleMain::get_char_advance 1 %c %c %p %p\n", current, next, current_glyph, next_glyph);
	if(next_glyph)
		FT_Get_Kerning(freetype_face, 
				current_glyph->freetype_index,
				next_glyph->freetype_index,
				ft_kerning_default,
				&kerning);
	else
		kerning.x = 0;
//printf("TitleMain::get_char_advance 2 %d %d\n", result, kerning.x);

	return result + (kerning.x >> 6);
}


void TitleMain::draw_glyphs()
{
// Build table of all glyphs needed
	int total_packages = 0;

// now convert text to FT_Ulong array
#ifndef X_HAVE_UTF8_STRING
//temp conversion to utf8
	convert_encoding();
#endif
	config.convert_text();
	for(int i = 0; i < config.tlen; i++)
	{
		FT_ULong char_code;	
		int c = config.ucs4text[i];
		int exists = 0;
		char_code = config.ucs4text[i];



		for(int j = 0; j < glyphs.total; j++)
		{
			if(glyphs.values[j]->char_code == char_code)
			{
				exists = 1;
				break;
			}
		}

		if(!exists)
		{
			total_packages++;
//printf("TitleMain::draw_glyphs 1\n");
			TitleGlyph *glyph = new TitleGlyph;
//printf("TitleMain::draw_glyphs 2\n");
			glyphs.append(glyph);
			glyph->c = c;
			glyph->char_code = char_code;
		}
	}

	if(!glyph_engine)
		glyph_engine = new GlyphEngine(this, PluginClient::smp + 1);

	glyph_engine->set_package_count(total_packages);
//printf("TitleMain::draw_glyphs 3 %d\n", glyphs.total);
	glyph_engine->process_packages();
//printf("TitleMain::draw_glyphs 4\n");
}

void TitleMain::get_total_extents()
{
// Determine extents of total text
	int current_w = 0;
	int row_start = 0;
	int max_char_w = 0;
	text_len = config.tlen;

	if(!char_positions) char_positions = new title_char_position_t[text_len];
	text_rows = 0;
	text_w = 0;
	ascent = 0;
	height = 0;

	for(int i = 0; i < glyphs.size(); i++)
	{
		TitleGlyph *glyph = glyphs.get(i);
		if(glyph->top > ascent) ascent = glyph->top;
		if(glyph->height > height) height = glyph->height;
	}
//printf("TitleMain::get_total_extents %d\n", ascent);


	for(int i = 0; i < text_len; i++)
	{
		char_positions[i].x = current_w;
		char_positions[i].y = text_rows * get_char_height();
		int char_advance = get_char_advance(config.ucs4text[i], 
			config.ucs4text[i + 1]);
		char_positions[i].w = char_advance;
		if(char_advance > max_char_w) max_char_w = char_advance;


// printf("TitleMain::get_total_extents 1 %c %d %d %d\n", 
// 	config.text[i], 
// 	char_positions[i].x, 
// 	char_positions[i].y, 
// 	char_positions[i].w);

		if(config.ucs4text[i] == 0xa || i == text_len - 1)
		{
			text_rows++;
// expand line by last character's width
			if(i > 0)
			{
				current_w += get_char_width(config.ucs4text[i - 1]);
			}
			
			if(current_w > text_w) text_w = current_w;
			current_w = 0;
		}
		else
		{
// expand line by last character's advance
			current_w += char_positions[i].w;
		}
	}
	text_w += config.dropshadow + config.outline_size * 4 + max_char_w;
	text_h = text_rows * get_char_height() + 
		config.dropshadow + 
		config.outline_size * 4;

// Now that text_w is known
// Justify rows based on configuration
	row_start = 0;
	for(int i = 0; i < text_len; i++)
	{
		if(config.ucs4text[i] == 0xa || i == text_len - 1)
		{
			for(int j = row_start; j <= i; j++)
			{
				switch(config.hjustification)
				{
					case JUSTIFY_LEFT:
						break;

					case JUSTIFY_MID:
						char_positions[j].x += (text_w - 
								char_positions[i].x - 
								char_positions[i].w) /
							2;
						break;

					case JUSTIFY_RIGHT:
						char_positions[j].x += (text_w - 
							char_positions[i].x - 
							char_positions[i].w);
						break;
				}
			}
			row_start = i + 1;
		}
	}


//printf("TitleMain::get_total_extents 2 %d %d\n", text_w, text_h);
}

int TitleMain::draw_mask()
{
	int old_visible_row1 = visible_row1;
	int old_visible_row2 = visible_row2;


// Determine y of visible text
	if(config.motion_strategy == BOTTOM_TO_TOP)
	{
// printf("TitleMain::draw_mask 1 %d %d %d %d\n", 
// 	config.motion_strategy,
// 	get_source_position(), 
// 	get_source_start(),
// 	config.prev_keyframe_position);
		float magnitude = config.pixels_per_second * 
			((get_source_position() - get_source_start()) - 
				(config.prev_keyframe_position - get_source_start())) / 
			PluginVClient::project_frame_rate;
		if(config.loop)
		{
			int loop_size = text_h + input->get_h();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_y1 = config.y + input->get_h() - magnitude;
	}
	else
	if(config.motion_strategy == TOP_TO_BOTTOM)
	{
		float magnitude = config.pixels_per_second * 
			(get_source_position() - 
				get_source_start() -
				config.prev_keyframe_position) / 
			PluginVClient::project_frame_rate;
		if(config.loop)
		{
			int loop_size = text_h + input->get_h();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_y1 = config.y + magnitude;
		text_y1 -= text_h;
	}
	else
	if(config.vjustification == JUSTIFY_TOP)
	{
		text_y1 = config.y;
	}
	else
	if(config.vjustification == JUSTIFY_MID)
	{
		text_y1 = config.y + input->get_h() / 2 - text_h / 2;
	}
	else
	if(config.vjustification == JUSTIFY_BOTTOM)
	{
		text_y1 = config.y + input->get_h() - text_h;
	}

	text_y2 = text_y1 + text_h + 0.5;

// Determine x of visible text
	if(config.motion_strategy == RIGHT_TO_LEFT)
	{
		float magnitude = config.pixels_per_second * 
			(get_source_position() - 
				get_source_start() - 
				config.prev_keyframe_position) / 
			PluginVClient::project_frame_rate;
		if(config.loop)
		{
			int loop_size = text_w + input->get_w();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_x1 = config.x + (float)input->get_w() - magnitude;
	}
	else
	if(config.motion_strategy == LEFT_TO_RIGHT)
	{
		float magnitude = config.pixels_per_second * 
			(get_source_position() - 
				get_source_start() - 
				config.prev_keyframe_position) / 
			PluginVClient::project_frame_rate;
		if(config.loop)
		{
			int loop_size = text_w + input->get_w();
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_x1 = config.x + -(float)text_w + magnitude;
	}
	else
	if(config.hjustification == JUSTIFY_LEFT)
	{
		text_x1 = config.x;
	}
	else
	if(config.hjustification == JUSTIFY_MID)
	{
		text_x1 = config.x + input->get_w() / 2 - text_w / 2;
	}
	else
	if(config.hjustification == JUSTIFY_RIGHT)
	{
		text_x1 = config.x + input->get_w() - text_w;
	}





// Determine y extents just of visible text
	visible_row1 = (int)(-text_y1 / get_char_height());
	if(visible_row1 < 0) visible_row1 = 0;

	visible_row2 = (int)((float)text_rows - (text_y2 - input->get_h()) / get_char_height() + 1);
	if(visible_row2 > text_rows) visible_row2 = text_rows;


	if(visible_row2 <= visible_row1) return 1;


	mask_y1 = text_y1 + visible_row1 * get_char_height();
	mask_y2 = text_y1 + visible_row2 * get_char_height();
	text_x1 += config.x;


//printf("TitleMain::draw_mask %d %d\n", visible_row1, visible_row2);
	visible_char1 = visible_char2 = 0;
	int got_char1 = 0;
	for(int i = 0; i < text_len; i++)
	{
		title_char_position_t *char_position = char_positions + i;
		int char_row = char_position->y / get_char_height();
		if(char_row >= visible_row1 &&
			char_row < visible_row2)
		
		{
			if(!got_char1)
			{
				visible_char1 = i;
				got_char1 = 1;
			}
			visible_char2 = i;
		}
	}
	visible_char2++;



	int visible_rows = visible_row2 - visible_row1;
	int need_redraw = 0;
	if(text_mask &&
		(text_mask->get_w() != text_w ||
		text_mask->get_h() != visible_rows * get_char_height()))
	{
		delete text_mask;
		text_mask = 0;
	}

	if(!text_mask)
	{
// Always use 8 bit because the glyphs are 8 bit
// Need to set YUV to get clear_frame to set the right chroma.
		int color_model = BC_RGBA8888;
		if(cmodel_is_yuv(get_output()->get_color_model()))
			color_model = BC_YUVA8888;
		text_mask = new VFrame;
		text_mask->set_use_shm(0);
//printf("TitleMain::draw_mask %d %d\n", __LINE__, color_model);
		text_mask->reallocate(0,
			-1,
			0,
			0,
			0,
			text_w,
			text_h,
			color_model,
			-1);
		need_redraw = 1;
	}



// Draw on text mask if it has changed
	if(old_visible_row1 != visible_row1 ||
		old_visible_row2 != visible_row2 ||
		need_redraw)
	{
		text_mask->clear_frame();


		if(!title_engine)
			title_engine = new TitleEngine(this, PluginClient::smp + 1);


// Draw dropshadow first
		if(config.dropshadow)
		{
			title_engine->do_dropshadow = 1;
			title_engine->set_package_count(visible_char2 - visible_char1);
			title_engine->process_packages();
		}

// Then draw foreground
		title_engine->do_dropshadow = 0;
		title_engine->set_package_count(visible_char2 - visible_char1);
		title_engine->process_packages();




// Convert to text outlines
		if(config.outline_size > 0)
		{
			if(outline_mask &&
				(text_mask->get_w() != outline_mask->get_w() ||
				text_mask->get_h() != outline_mask->get_h()))
			{
				delete outline_mask;
				outline_mask = 0;
			}

			if(!outline_mask)
			{
				outline_mask = new VFrame;
				outline_mask->set_use_shm(0);
				outline_mask->reallocate(0,
					-1,
					0,
					0,
					0,
					text_mask->get_w(),
					text_mask->get_h(),
					text_mask->get_color_model(),
					-1);
			}

			if(!outline_engine) outline_engine = 
				new TitleOutlineEngine(this, PluginClient::smp + 1);
			outline_engine->do_outline();
		}
	}

	return 0;
}

void TitleMain::get_color_components(int *r, int *g, int *b, int *a, int is_outline)
{
	int r_in, g_in, b_in, a_in;

	if(is_outline)
	{
		r_in = (config.outline_color & 0xff0000) >> 16;
		g_in = (config.outline_color & 0xff00) >> 8;
		b_in = config.outline_color & 0xff;
		a_in = config.outline_alpha;
	}
	else
	{
		r_in = (config.color & 0xff0000) >> 16;
		g_in = (config.color & 0xff00) >> 8;
		b_in = config.color & 0xff;
		a_in = config.alpha;
	}
	*r = r_in;
	*g = g_in;
	*b = b_in;
	*a = a_in;

	switch(output->get_color_model())
	{
		case BC_YUV888:
			yuv.rgb_to_yuv_8(r_in, g_in, b_in, *r, *g, *b);
			break;
		case BC_YUVA8888:
			yuv.rgb_to_yuv_8(r_in, g_in, b_in, *r, *g, *b);
			break;
	}
}

void TitleMain::overlay_mask()
{

//printf("TitleMain::overlay_mask 1\n");
	alpha = 0x100;
	if(!EQUIV(config.fade_in, 0))
	{
		int fade_len = (int)(config.fade_in * PluginVClient::project_frame_rate);
		int fade_position = get_source_position() - 
			config.prev_keyframe_position;

// printf("TitleMain::overlay_mask %d %d %d\n", 
// get_source_position(), 
// get_source_start(), 
// config.prev_keyframe_position);

		if(fade_position >= 0 && fade_position < fade_len)
		{
			alpha = (int)((float)0x100 * fade_position / fade_len);
		}
	}
//printf("TitleMain::overlay_mask 1\n");

	if(!EQUIV(config.fade_out, 0))
	{
		int fade_len = (int)(config.fade_out * 
			PluginVClient::project_frame_rate);
		int fade_position = config.next_keyframe_position - 
			get_source_position();


// printf("TitleMain::overlay_mask %d %d %d\n", 
// get_source_start() + config.next_keyframe_position,
// get_source_position(), 
// source_end);

		if(fade_position >= 0 && fade_position < fade_len)
		{
			alpha = (int)((float)0x100 * fade_position / fade_len);
		}
	}
//printf("TitleMain::overlay_mask 1\n");


	if(text_x1 < input->get_w() && text_x1 + text_w > 0 &&
		mask_y1 < input->get_h() && mask_y2 > 0)
	{
		if(!translate) translate = new TitleTranslate(this, PluginClient::smp + 1);
		translate->process_packages();
	}
}

void TitleMain::clear_glyphs()
{
//printf("TitleMain::clear_glyphs 1\n");
	glyphs.remove_all_objects();
}

const char* TitleMain::motion_to_text(int motion)
{
	switch(motion)
	{
		case NO_MOTION: return "No motion"; break;
		case BOTTOM_TO_TOP: return "Bottom to top"; break;
		case TOP_TO_BOTTOM: return "Top to bottom"; break;
		case RIGHT_TO_LEFT: return "Right to left"; break;
		case LEFT_TO_RIGHT: return "Left to right"; break;
	}
	return "";
}

int TitleMain::text_to_motion(const char *text)
{
	for(int i = 0; i < TOTAL_PATHS; i++)
	{
		if(!strcasecmp(motion_to_text(i), text)) return i;
	}
	return 0;
}


// read time from a subtitle file
static double read_time(char *ptr, char *end)
{
	int numbers[4];
	int i;
	for(i = 0; i < 4; i++)
	{
		numbers[i] = 0;
	}

// char *ptr2 = ptr;
// printf("read_time %d '", __LINE__);
// while(ptr2 < end)
// {
// printf("%c", *ptr2++);
// }
// printf("'\n");

	for(i = 0; i < 4; i++)
	{
// start of number
		while(ptr < end && 
			(*ptr < '0' || *ptr > '9'))
		{
			ptr++;
		}

// {		
// char *ptr2 = ptr;
// printf("read_time %d '", __LINE__);
// while(ptr2 < end)
// {
// printf("%c", *ptr2++);
// }
// printf("'\n");
// }

		if(ptr < end)
		{
			numbers[i] = atoi(ptr);
		}
		
// end of number
		while(ptr < end &&
			*ptr >= '0' && 
			*ptr <= '9')
		{
			ptr++;
		}
// {
// char *ptr2 = ptr;
// printf("read_time %d '", __LINE__);
// while(ptr2 < end)
// {
// printf("%c", *ptr2++);
// }
// printf("'\n");
// }
	}


// printf("read_time %d %d %d %d %d\n", __LINE__, 
// numbers[0],
// numbers[1],
// numbers[2],
// numbers[3]);
// 	
	return (double)(numbers[0] * 3600) +
		(numbers[1] * 60) +
		(numbers[2]) +
		(double)numbers[3] / 1000;
}


// this parses a file of the form:
// 1
// 00:01:04,440 --> 00:01:07,318
// <i>It's okay, son...</i>
// 
// 2
// 00:01:08,611 --> 00:01:13,491
// <i>I know you want this to be over.</i>
// 
// 3
// 00:01:19,998 --> 00:01:22,125
// <i>I'm right here.</i>
// 
// 4
// 00:01:26,504 --> 00:01:29,173
// <i>I will be right here...</i>


void TitleMain::load_subtitle_db()
{
// test for change in path
	if(!prev_subs_path.compare(config.subs_path))
	{
		return;
	}



// try to load it
	if(config.subs_path.length() > 0)
	{
		subtitle_db.remove_all_objects();
		prev_subs_path.assign(config.subs_path);
		
		FILE *fd = fopen(config.subs_path.c_str(), "r");
		if(!fd)
		{
			printf("TitleMain::load_subtitle_db %d %s not found\n", 
				__LINE__, 
				config.subs_path.c_str());
		}
		else
		{
			fseek(fd, 0, SEEK_END);
			int size = ftell(fd);
			fseek(fd, 0, SEEK_SET);

			char *buffer = new char[size + 1];
			char *end = buffer + size;
			int temp = fread(buffer, 1, size, fd);
			fclose(fd);
			buffer[size] = 0;
			
// strip carriage returns
			char *dst = buffer;
			char *src = buffer;
			while(src < end)
			{
				if(*src == 0x0d)
				{
					src++;
				}
				else
				{
					*dst++ = *src++;
				}
			}
			*dst = 0;

//printf("TitleMain::load_subtitle_db %d %d\n", __LINE__, size);
			char *ptr = buffer;
			int state = 0;
			double time1 = 0;
			double time2 = 0;
			string text;

			while(ptr < end)
			{
				char *line_start = ptr;
				char *line_end = ptr;
				while(line_end < end && 
					*line_end != 0x0a)
				{
					line_end++;
				}

//printf("TitleMain::load_subtitle_db %d %d %d %d\n", __LINE__, ptr - buffer, end - buffer, state);

				ptr = line_end;
				if(ptr < end && 
					*ptr == 0x0a)
				{
					ptr++;
				}

// printf("TitleMain::load_subtitle_db %d %d %d\n", 
// __LINE__, 
// line_end - line_start, 
// state);
// printf("TitleMain::load_subtitle_db %d state=%d '", __LINE__, state);
// for(int i = 0; i < line_end - line_start; i++)
// {
// printf("%02x ", line_start[i]);
// }
// printf("\n");

				switch(state)
				{
					case 0:
// standalone number marks the beginning of a title
					{
// find start of number
						char *ptr2 = line_start;
						while(ptr2 < line_end &&
							(*ptr2 < '0' || *ptr2 > 'z'))
						{
							ptr2++;
						}

// find end of number
						char *ptr3 = ptr2;
						while(ptr3 < line_end &&
							*ptr3 >= '0' && *ptr3 <= '9')
						{
							ptr3++;
						}

// find end of whitespace
						char *ptr4 = ptr3;
						while(ptr4 < line_end &&
							(*ptr4 == ' ' ||
							*ptr3 == '\t'))
						{
							ptr4++;
						}

// is a standalone number
// printf("TitleMain::load_subtitle_db %d %d %d %d %d\n", 
// __LINE__,
// ptr2 - line_start,
// ptr3 - line_start,
// ptr4 - line_start,
// line_end - line_start);
						if(ptr2 < line_end &&
							ptr3 <= line_end &&
							ptr2 < ptr3 && 
							ptr4 >= line_end &&
							*ptr2 >= '0' &&
							*ptr2 <= '9')
						{
							state++;
						}
						break;
					}
					
					case 1:
// time range
					{
						char *ptr2 = strstr(line_start, "-->");
						if(ptr2 < line_end)
						{
							time1 = read_time(line_start, ptr2);
						}
						else
						{
							state = 0;
							break;
						}
						
						while(ptr2 < line_end &&
							*ptr2 < '0' &&
							*ptr2 > '9')
						{
							ptr2++;
						}
						
						if(ptr2 < line_end)
						{
							time2 = read_time(ptr2, line_end);
						}
						else
						{
							state = 0;
							break;
						}
						
						state++;
						break;
					}

// text
					case 2:
					{
						char *ptr2 = line_start;
						while(ptr2 < line_end &&
							(*ptr2 == ' ' ||
							*ptr2 == '\t'))
						{
							ptr2++;
						}

//printf("TitleMain::load_subtitle_db %d %d %d\n", __LINE__, ptr2 - line_start, line_end - line_start);
// got non whitespace
						if(ptr2 < line_end)
						{
							char temp_string[line_end - ptr2 + 1];
							memcpy(temp_string, ptr2, line_end - ptr2);
							temp_string[line_end - ptr2] = 0;
							if(text.length() > 0)
							{
								text.append("\n");
							}
//printf("TitleMain::load_subtitle_db %d '%s'\n", __LINE__, temp_string);
							text.append(temp_string);
						}
						else
						{
//printf("TitleMain::load_subtitle_db %d %p %p %f %f %s\n", __LINE__, ptr, end, time1, time2, text.c_str());
// got all whitespace.  done with title
							Subtitle *subtitle = new Subtitle;
							subtitle->start_time = time1;
							subtitle->end_time = time2;
							subtitle->text.assign(text);
							subtitle_db.append(subtitle);
							
							
							state = 0;
							time1 = 0;
							time2 = 0;
							text.erase();
						}
						break;
					}
				}
			}
			
			
			delete [] buffer;
		}
	}


// 	for(int i = 0; i < subtitle_db.size(); i++)
// 	{
// 		printf("TitleMain::load_subtitle_db %d %f %f '%s'\n",
// 			__LINE__,
// 			subtitle_db.get(i)->start_time,
// 			subtitle_db.get(i)->end_time,
// 			subtitle_db.get(i)->text.c_str());
// 	}
}


int TitleMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	int result = 0;
	input = input_ptr;
	output = output_ptr;
	build_fonts();

	need_reconfigure |= load_configuration();


// Always synthesize text and redraw it for timecode
	if(config.timecode)
	{
		int64_t current_frame = get_source_position();
		if (get_direction() == PLAY_REVERSE)
		{
			current_frame -= 1;
		}
		char string[BCTEXTLEN];
		Units::totext(string, 
				(double)current_frame / PluginVClient::project_frame_rate, 
				config.timecode_format, 
				PluginVClient::get_project_samplerate(),
				PluginVClient::get_project_framerate(), 
// no-one ever uses this
				16);
		config.text.assign(string);
		need_reconfigure = 1;
	}
	else
// synthesize & redraw text from subtitle file
	if(config.use_subs)
	{
		load_subtitle_db();
		
		int64_t current_frame = get_source_position();
		if (get_direction() == PLAY_REVERSE)
		{
			current_frame -= 1;
		}
		double current_time = (double)current_frame / PluginVClient::project_frame_rate;
		CLAMP(current_subtitle, 0, subtitle_db.size() - 1);

		string prev_text;
		prev_text.assign(config.text);

		if(current_subtitle < subtitle_db.size())
		{
// don't hide it after the end_time, because they're always too short
			while(current_subtitle < subtitle_db.size() &&
				subtitle_db.get(current_subtitle)->start_time < current_time)
			{
				current_subtitle++;
			}
			
			while(current_subtitle > 0 &&
				subtitle_db.get(current_subtitle)->start_time > current_time)
			{
				current_subtitle--;
			}
			


			if(current_subtitle >= 0 && 
				current_subtitle < subtitle_db.size() &&
				subtitle_db.get(current_subtitle)->start_time <= current_time 
				/* &&
				subtitle_db.get(current_subtitle)->end_time > current_time */ )
			{
				config.text.assign(subtitle_db.get(current_subtitle)->text);
// Force the bottom descenders to be drawn.
//				config.text.append("\n ");
			}
			else
			{
				config.text.assign("");
			}
		}
		else
		{
			config.text.assign("");
		}
		
		if(prev_text.compare(config.text))
		{
			need_reconfigure = 1;
		}
	}

	if(config.size <= 0 || config.size >= 2048) return 0;
	if(!config.text.length()) return 0;
	if(!strlen(config.encoding)) return 0;

// printf("TitleMain::process_realtime %d need_reconfigure=%d\n",
// __LINE__,
// need_reconfigure);

// Handle reconfiguration
	if(need_reconfigure)
	{
		if(text_mask) delete text_mask;
		text_mask = 0;
		if(freetype_face) FT_Done_Face(freetype_face);
		freetype_face = 0;
		if(glyph_engine) delete glyph_engine;
		glyph_engine = 0;
		if(char_positions) delete [] char_positions;
		char_positions = 0;
		clear_glyphs();
		visible_row1 = 0;
		visible_row2 = 0;
		ascent = 0;

		if(!freetype_library) 
			FT_Init_FreeType(&freetype_library);

		if(!freetype_face)
		{
			FontEntry *font = get_font();
			if(load_freetype_face(freetype_library,
				freetype_face,
				font->path))
			{
				printf("TitleMain::process_realtime %s: FT_New_Face failed.\n",
					font->fixed_title);
				result = 1;
			}

			if(!result) FT_Set_Pixel_Sizes(freetype_face, config.size, 0);
		}


		if(!result)
		{
//PRINT_TRACE
			draw_glyphs();
			get_total_extents();
			need_reconfigure = 0;
		}
	}

	if(!result)
	{
//PRINT_TRACE
// Determine region of text visible on the output and draw mask
		result = draw_mask();
	}


// Overlay mask on output
	if(!result)
	{
//PRINT_TRACE
		overlay_mask();
	}

	return 0;
}

void TitleMain::update_gui()
{
	if(thread)
	{
		int reconfigure = load_configuration();
		if(reconfigure)
		{
			thread->window->lock_window("TitleMain::update_gui");
			((TitleWindow*)thread->window)->update();
			((TitleWindow*)thread->window)->unlock_window();
			((TitleWindow*)thread->window)->color_thread->update_gui(config.color, 0);
			thread->window->unlock_window();
		}
	}
}






int TitleMain::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	next_keyframe = get_next_keyframe(get_source_position());
	int64_t prev_position = edl_to_local(prev_keyframe->position);
	int64_t next_position = edl_to_local(next_keyframe->position);

	TitleConfig old_config, prev_config, next_config;
	old_config.copy_from(config);
	read_data(prev_keyframe);
	prev_config.copy_from(config);
	read_data(next_keyframe);
	next_config.copy_from(config);

	config.prev_keyframe_position = prev_position;
	config.next_keyframe_position = next_position;
	if(config.next_keyframe_position == config.prev_keyframe_position)
		config.next_keyframe_position = get_source_start() + get_total_len();
	if(config.prev_keyframe_position == 0)
		config.prev_keyframe_position = get_source_start();



	config.interpolate(prev_config, 
		next_config, 
		(next_position == prev_position) ?
			get_source_position() :
			prev_position,
		(next_position == prev_position) ?
			get_source_position() + 1 :
			next_position,
		get_source_position());

	if(!config.equivalent(old_config))
		return 1;
	else
		return 0;
}












void TitleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
#ifdef X_HAVE_UTF8_STRING
	convert_encoding();
#endif
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("TITLE");
	output.tag.set_property("FONT", config.font);
	output.tag.set_property("ENCODING", config.encoding);
	output.tag.set_property("STYLE", (int64_t)config.style);
	output.tag.set_property("SIZE", config.size);
//printf("TitleMain::save_data %d %d\n", __LINE__, config.size);
	output.tag.set_property("COLOR", config.color);
	output.tag.set_property("OUTLINE_COLOR", config.outline_color);
	output.tag.set_property("ALPHA", config.alpha);
	output.tag.set_property("OUTLINE_ALPHA", config.outline_alpha);
	output.tag.set_property("MOTION_STRATEGY", config.motion_strategy);
	output.tag.set_property("LOOP", config.loop);
	output.tag.set_property("PIXELS_PER_SECOND", config.pixels_per_second);
	output.tag.set_property("HJUSTIFICATION", config.hjustification);
	output.tag.set_property("VJUSTIFICATION", config.vjustification);
	output.tag.set_property("FADE_IN", config.fade_in);
	output.tag.set_property("FADE_OUT", config.fade_out);
	output.tag.set_property("TITLE_X", config.x);
	output.tag.set_property("TITLE_Y", config.y);
	output.tag.set_property("DROPSHADOW", config.dropshadow);
	output.tag.set_property("OUTLINE_SIZE", config.outline_size);
	output.tag.set_property("TIMECODE", config.timecode);
	output.tag.set_property("TIMECODE_FORMAT", config.timecode_format);
	output.tag.set_property("WINDOW_W", config.window_w);
	output.tag.set_property("WINDOW_H", config.window_h);
	output.tag.set_property("USE_SUBS", config.use_subs);
	output.tag.set_property("SUBS_PATH", config.subs_path.c_str());
	output.append_tag();
	output.append_newline();
	
	output.encode_text(config.text.c_str());

	output.tag.set_title("/TITLE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void TitleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	int new_interlace = 0;
	int new_horizontal = 0;
	int new_luminance = 0;

	config.prev_keyframe_position = edl_to_local(keyframe->position);
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("TITLE"))
			{
				input.tag.get_property("FONT", config.font);
				input.tag.get_property("ENCODING", config.encoding);
				config.style = input.tag.get_property("STYLE", (int64_t)config.style);
				config.size = input.tag.get_property("SIZE", config.size);
//printf("TitleMain::read_data %d %d\n", __LINE__, config.size);
				config.color = input.tag.get_property("COLOR", config.color);
				config.outline_color = input.tag.get_property("OUTLINE_COLOR", config.outline_color);
				config.alpha = input.tag.get_property("ALPHA", config.alpha);
				config.outline_alpha = input.tag.get_property("OUTLINE_ALPHA", config.outline_alpha);
				config.motion_strategy = input.tag.get_property("MOTION_STRATEGY", config.motion_strategy);
				config.loop = input.tag.get_property("LOOP", config.loop);
				config.pixels_per_second = input.tag.get_property("PIXELS_PER_SECOND", config.pixels_per_second);
				config.hjustification = input.tag.get_property("HJUSTIFICATION", config.hjustification);
				config.vjustification = input.tag.get_property("VJUSTIFICATION", config.vjustification);
				config.fade_in = input.tag.get_property("FADE_IN", config.fade_in);
				config.fade_out = input.tag.get_property("FADE_OUT", config.fade_out);
				config.x = input.tag.get_property("TITLE_X", config.x);
				config.y = input.tag.get_property("TITLE_Y", config.y);
				config.dropshadow = input.tag.get_property("DROPSHADOW", config.dropshadow);
				config.outline_size = input.tag.get_property("OUTLINE_SIZE", config.outline_size);
				config.timecode = input.tag.get_property("TIMECODE", config.timecode);
				config.timecode_format = input.tag.get_property("TIMECODE_FORMAT", config.timecode_format);
				config.window_w = input.tag.get_property("WINDOW_W", config.window_w);
				config.window_h = input.tag.get_property("WINDOW_H", config.window_h);
				config.use_subs = input.tag.get_property("USE_SUBS", config.use_subs);
				config.subs_path.assign(input.tag.get_property_text("SUBS_PATH"));
				config.text.assign(input.read_text());
			}
			else
			if(input.tag.title_is("/TITLE"))
			{
				result = 1;
			}
		}
	}
#ifdef X_HAVE_UTF8_STRING
	convert_encoding();
#endif
}






