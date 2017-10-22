
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "aboutprefs.h"
#include "bcsignals.h"
#include "language.h"
#include "libmpeg3.h"
#include "mwindow.h"
#include "quicktime.h"
#include "theme.h"
#include "vframe.h"



AboutPrefs::AboutPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
}

AboutPrefs::~AboutPrefs()
{
	credits.remove_all_objects();
}

void AboutPrefs::create_objects()
{
	int x, y;


	BC_Resources *resources = BC_WindowBase::get_resources();

// 	add_subwindow(new BC_Title(mwindow->theme->preferencestitle_x, 
// 		mwindow->theme->preferencestitle_y, 
// 		_("About"), 
// 		LARGEFONT, 
// 		resources->text_default));
	
	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y +
		get_text_height(LARGEFONT);

	char license1[BCTEXTLEN];
	sprintf(license1, "%s %s", PROGRAM_NAME, CINELERRA_VERSION);

	set_font(LARGEFONT);
	set_color(resources->text_default);
	draw_text(x, y, license1);

	y += get_text_height(LARGEFONT);
	char license2[BCTEXTLEN];
	sprintf(license2, 
		_("(C) %d Adam Williams\n\nheroinewarrior.com"), 
		COPYRIGHT_DATE);
	set_font(MEDIUMFONT);
	draw_text(x, y, license2);



	y += get_text_height(MEDIUMFONT) * 3;

// 	char versions[BCTEXTLEN];
// 	sprintf(versions, 
// _("Quicktime version %d.%d.%d\n"
// "Libmpeg3 version %d.%d.%d\n"),
// quicktime_major(),
// quicktime_minor(),
// quicktime_release(),
// mpeg3_major(),
// mpeg3_minor(),
// mpeg3_release());
// 	draw_text(x, y, versions);



	y += get_text_height(MEDIUMFONT) * 3;
	set_font(LARGEFONT);
	draw_text(x, y, "Contributors:");
	y += get_text_height(LARGEFONT);

	credits.append(new BC_ListBoxItem("Richard Baverstock"));
	credits.append(new BC_ListBoxItem("Karl Bielefeldt"));
	credits.append(new BC_ListBoxItem("Kevin Brosius"));
	credits.append(new BC_ListBoxItem("Jean-Luc Coulon"));
	credits.append(new BC_ListBoxItem("Jerome Cornet"));
	credits.append(new BC_ListBoxItem("Pierre Marc Dumuid"));
	credits.append(new BC_ListBoxItem("Nicola Ferralis"));
	credits.append(new BC_ListBoxItem("Alex Ferrer"));
	credits.append(new BC_ListBoxItem("Gustavo Iñiguez"));
	credits.append(new BC_ListBoxItem("Tefan de Konink"));
	credits.append(new BC_ListBoxItem("Nathan Kurz"));
	credits.append(new BC_ListBoxItem("Greg Mekkes"));
	credits.append(new BC_ListBoxItem("Jean-Michel Poure"));
	credits.append(new BC_ListBoxItem("Monty Montgomery"));
	credits.append(new BC_ListBoxItem("Bill Morrow"));
#ifdef X_HAVE_UTF8_STRING
	credits.append(new BC_ListBoxItem("Einar Rünkaru"));
#else
	credits.append(new BC_ListBoxItem("Einar R\374nkaru"));
#endif
	credits.append(new BC_ListBoxItem("Paolo Rampino"));
	credits.append(new BC_ListBoxItem("Petter Reinholdtsen"));
	credits.append(new BC_ListBoxItem("Eric Seigne"));
	credits.append(new BC_ListBoxItem("Johannes Sixt"));
	credits.append(new BC_ListBoxItem("Joe Stewart"));
	credits.append(new BC_ListBoxItem("Dan Streetman"));
	credits.append(new BC_ListBoxItem("Mark Taraba"));
	credits.append(new BC_ListBoxItem("Andraz Tori"));
	credits.append(new BC_ListBoxItem("Jonas Wulff"));




	BC_ListBox *listbox;
	add_subwindow(listbox = new BC_ListBox(x, 
		y,
		DP(200),
		DP(300),
		LISTBOX_TEXT,
		&credits,
		0,
		0,
		1));
	y += listbox->get_h() + get_text_height(LARGEFONT) + 10;

	set_font(LARGEFONT);
	set_color(resources->text_default);
	draw_text(x, y, "License:");
	y += get_text_height(LARGEFONT);

	set_font(MEDIUMFONT);

	char license3[BCTEXTLEN];
	sprintf(license3, _(
"This program is free software; you can redistribute it and/or modify it under the terms\n"
"of the GNU General Public License as published by the Free Software Foundation; either version\n"
"2 of the License, or (at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\n"
"without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR\n"
"PURPOSE.  See the GNU General Public License for more details.\n"
"\n"));
	draw_text(x, y, license3);

	x = get_w() - mwindow->theme->about_bg->get_w() - 10;
	y = mwindow->theme->preferencesoptions_y;
	BC_Pixmap *temp_pixmap = new BC_Pixmap(this, 
		mwindow->theme->about_bg,
		PIXMAP_ALPHA);
	draw_pixmap(temp_pixmap, 
		x, 
		y);

	delete temp_pixmap;


	x += mwindow->theme->about_bg->get_w() + 10;
	y += get_text_height(LARGEFONT) * 2;


	flash(1);
}


