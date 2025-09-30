
/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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
	char license1[BCTEXTLEN];
	sprintf(license1, "%s %s", PROGRAM_NAME, CINELERRA_VERSION);
	
	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;


    BC_Title *title1;
	add_subwindow(title1 = new BC_Title(x, y, license1, LARGEFONT, resources->text_default));

	y += title1->get_h();
	char license2[BCTEXTLEN];
	sprintf(license2, 
		_("(C) %d Adam Williams\nheroinewarrior.com"), 
		COPYRIGHT_DATE);
	add_subwindow(title1 = new BC_Title(x, y, license2, MEDIUMFONT, resources->text_default));

	y += title1->get_h() + DP(30);

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



//	y += get_text_height(MEDIUMFONT) * 3;
	add_subwindow(title1 = new BC_Title(x, y, "Contributors:", LARGEFONT, resources->text_default));
	y += title1->get_h();

    const char* names[] = {
	    "Richard Baverstock",
	    "Karl Bielefeldt",
	    "Kevin Brosius",
	    "Jean-Luc Coulon",
	    "Jerome Cornet",
	    "Pierre Marc Dumuid",
	    "Nicola Ferralis",
	    "Alex Ferrer",
	    "Gustavo Iñiguez",
	    "Stefan de Konink",
	    "Nathan Kurz",
	    "Greg Mekkes",
	    "Jean-Michel Poure",
	    "Monty Montgomery",
	    "Bill Morrow",
#ifdef X_HAVE_UTF8_STRING
	    "Einar Rünkaru",
#else
	    "Einar R\374nkaru",
#endif
	    "Paolo Rampino",
	    "Andrew Randrianasulu",
	    "Petter Reinholdtsen",
	    "Eric Seigne",
	    "Johannes Sixt",
	    "Joe Stewart",
	    "Dan Streetman",
	    "Mark Taraba",
	    "Andraz Tori",
	    "Jonas Wulff",
	    "David Martnez Moreno",
    };
#define TOTAL (sizeof(names) / sizeof(char*))

// create randomized list
    srand(time(NULL));
    int got_it[TOTAL] = { 0 };
    for(int i = 0; i < TOTAL; i++)
    {
        int count = rand() % (TOTAL + 1 - i);
        for(int j = 0; j < TOTAL; j++)
        {
            if(got_it[j]) continue;
            count--;
            if(count <= 0)
            {
                credits.append(new BC_ListBoxItem(names[j]));
                got_it[j] = 1;
//printf("%d ", j);
                break;
            }
        }
        if(count > 0) printf("AboutPrefs::create_objects %d: Bug\n", __LINE__);
    }
//printf("\n");



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
	y += listbox->get_h() + get_text_height(LARGEFONT) + DP(10);

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

//	x = get_w() - mwindow->theme->about_bg->get_w() - DP(10);
//	y = mwindow->theme->preferencesoptions_y;
// 	BC_Pixmap *temp_pixmap = new BC_Pixmap(this, 
// 		mwindow->theme->about_bg,
// 		PIXMAP_ALPHA);
// 	draw_pixmap(temp_pixmap, 
// 		x, 
// 		y);
// 
// 	delete temp_pixmap;


//	x += mwindow->theme->about_bg->get_w() + DP(10);
//	y += get_text_height(LARGEFONT) * 2;


	flash(1);
}


