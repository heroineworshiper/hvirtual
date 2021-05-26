/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#include "bcclipboard.h"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctextbox.h"
#include "clip.h"
#include "colors.h"
#include <ctype.h>
#include "cursors.h"
#include "filesystem.h"
#include "keys.h"
#include "language.h"
#include <math.h>
#include "bctimer.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>

#define VERTICAL_MARGIN 2
#define VERTICAL_MARGIN_NOBORDER 0
#define HORIZONTAL_MARGIN 4
#define HORIZONTAL_MARGIN_NOBORDER 2
#define UNDOLEVELS 500

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	const char *text, 
	int has_border, 
	int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	reset_parameters(rows, has_border, font);
	this->text.assign(text);
}

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	string *text, 
	int has_border, 
	int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	reset_parameters(rows, has_border, font);
	this->text.assign(*text);
}

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	int64_t text, 
	int has_border, 
	int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	reset_parameters(rows, has_border, font);
	
	char temp[BCTEXTLEN];
	sprintf(temp, "%lld", (long long)text);
	this->text.assign(temp);
}

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	float text, 
	int has_border, 
	int font,
	int precision)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	reset_parameters(rows, has_border, font);
	this->precision = precision;

	char temp[BCTEXTLEN];
	sprintf(temp, "%0.*f", precision, text);
	this->text.assign(temp);
}

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	int text, 
	int has_border, 
	int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	reset_parameters(rows, has_border, font);
	char temp[BCTEXTLEN];
	sprintf(temp, "%d", text);
	this->text.assign(temp);
    
}

BC_TextBox::~BC_TextBox()
{
	if(skip_cursor) delete skip_cursor;
	suggestions->remove_all_objects();
	delete suggestions;
	delete suggestions_popup;
	delete menu;
}

int BC_TextBox::reset_parameters(int rows, int has_border, int font)
{
	suggestions = new ArrayList<BC_ListBoxItem*>;
	suggestions_popup = 0;
	suggestion_column = 0;

	this->rows = rows;
	this->has_border = has_border;
	this->font = font;
	text_start = 0;
	text_end = 0;
	highlight_letter1 = highlight_letter2 = 0;
	highlight_letter3 = highlight_letter4 = 0;
	ibeam_letter = 0;
	active = 0;
    read_only = 0;
	text_selected = 0;
	word_selected = 0;
	line_selected = 0;
	text_x = 0;
	enabled = 1;
	highlighted = 0;
	precision = 4;
	if (!skip_cursor)
		skip_cursor = new Timer;
	keypress_draw = 1;
	last_keypress = 0;
	separators = 0;
	yscroll = 0;
	menu = 0;
    undo_enabled = 0;
	return 0;
}

int BC_TextBox::initialize()
{
	if (!skip_cursor)
		skip_cursor = new Timer;
	skip_cursor->update();
// Get dimensions
	text_ascent = get_text_ascent(font) + 1;
	text_descent = get_text_descent(font) + 1;
	text_height = text_ascent + text_descent;
	ibeam_letter = text.length();
	if(has_border)
	{
		left_margin = right_margin = HORIZONTAL_MARGIN;
		top_margin = bottom_margin = VERTICAL_MARGIN;
	}
	else
	{
		left_margin = right_margin = HORIZONTAL_MARGIN_NOBORDER;
		top_margin = bottom_margin = VERTICAL_MARGIN_NOBORDER;
	}
	h = get_row_h(rows);
	text_x = left_margin;
	text_y = top_margin;
	find_ibeam(0, 1);

// Create the subwindow
	BC_SubWindow::initialize();

	BC_Resources *resources = get_resources();
	if(has_border)
	{
		back_color = resources->text_background;
		high_color = resources->text_background_hi;
	}
	else 
	{
		high_color = resources->text_background_noborder_hi;
		back_color = bg_color;
	}

	draw(0);
	set_cursor(IBEAM_CURSOR, 0, 0);
	show_window(0);

	add_subwindow(menu = new BC_TextMenu(this));
	menu->create_objects();
    update_undo();

	return 0;
}

int BC_TextBox::calculate_h(BC_WindowBase *gui, 
	int font, 
	int has_border,
	int rows)
{
	return rows * (gui->get_text_ascent(font) + 1 + 
		gui->get_text_descent(font) + 1) +
		2 * (has_border ? VERTICAL_MARGIN : VERTICAL_MARGIN_NOBORDER);
}

void BC_TextBox::enable_undo()
{
    undo_enabled = 1;
}

void BC_TextBox::set_precision(int precision)
{
	this->precision = precision;
}

// Compute suggestions for a path
int BC_TextBox::calculate_suggestions(ArrayList<BC_ListBoxItem*> *entries)
{
// Let user delete suggestion
	if(get_last_keypress() != BACKSPACE)
	{

// Compute suggestions
		FileSystem fs;
		ArrayList<char*> suggestions;
		const char *current_text = get_text();

// If directory, tabulate it
		if(current_text[0] == '/' ||
			current_text[0] == '~')
		{
//printf("BC_TextBox::calculate_suggestions %d\n", __LINE__);
			char string[BCTEXTLEN];
			char string2[BCTEXTLEN];
			strcpy(string, current_text);
			char *ptr = strrchr(string, '/');
			if(!ptr) ptr = strrchr(string, '~');

//printf("BC_TextBox::calculate_suggestions %d\n", __LINE__);
			*(ptr + 1) = 0;
			int suggestion_column = ptr + 1 - string;

			fs.set_filter(get_resources()->filebox_filter);
//			fs.set_sort_order(filebox->sort_order);
//			fs.set_sort_field(filebox->column_type[filebox->sort_column]);


//printf("BC_TextBox::calculate_suggestions %d %c %s\n", __LINE__, *ptr, string);
			if(current_text[0] == '~' && *ptr != '/')
			{
				fs.update("/home");
			}
			else
			{
				fs.parse_tildas(string);
				fs.update(string);
			}
//printf("BC_TextBox::calculate_suggestions %d %d\n", __LINE__, fs.total_files());


// Accept only entries with matching trailing characters
			ptr = strrchr((char*)current_text, '/');
			if(!ptr) ptr = strrchr((char*)current_text, '~');
			if(ptr) ptr++;
//printf("BC_TextBox::calculate_suggestions %d %s %p\n", __LINE__, current_text, ptr);


			if(ptr && strlen(ptr))
			{
				for(int i = 0; i < fs.total_files(); i++)
				{
					char *current_name = fs.get_entry(i)->name;
					if(!strncmp(ptr, current_name, strlen(ptr)))
					{
						suggestions.append(current_name);
	//printf("BC_TextBox::calculate_suggestions %d %s\n", __LINE__, current_name);
					}
				}
			}
			else
	// Accept all entries
			for(int i = 0; i < fs.total_files(); i++)
			{
	//printf("BC_TextBox::calculate_suggestions %d %s\n", __LINE__, fs.get_entry(i)->name);
				suggestions.append(fs.get_entry(i)->name);
			}
//printf("BC_TextBox::calculate_suggestions %d\n", __LINE__);

// Add 1 to column to keep /
			set_suggestions(&suggestions, suggestion_column);
//printf("BC_TextBox::calculate_suggestions %d\n", __LINE__);
		}
		else
// Get entries from current listbox with matching trailing characters
		if(entries)
		{
// printf("BC_TextBox::calculate_suggestions %d %d\n", 
// __LINE__, 
// entries->size());
			for(int i = 0; i < entries->size(); i++)
			{
				char *current_name = entries->get(i)->get_text();

//printf("BC_TextBox::calculate_suggestions %d %s %s\n", __LINE__, current_text, current_name);
				if(!strncmp(current_text, current_name, strlen(current_text)))
				{
					suggestions.append(current_name);
				}
			}

			set_suggestions(&suggestions, 0);
		}
	}

	return 1;
}

void BC_TextBox::set_suggestions(ArrayList<char*> *suggestions, int column)
{
// Copy the array
	this->suggestions->remove_all_objects();
	this->suggestion_column = column;
	
	if(suggestions)
	{
		for(int i = 0; i < suggestions->size(); i++)
		{
			this->suggestions->append(new BC_ListBoxItem(suggestions->get(i)));
		}

// Show the popup without taking focus
		if(suggestions->size() > 1)
		{
			if(!suggestions_popup)
			{
				
				get_parent()->add_subwindow(suggestions_popup = 
					new BC_TextBoxSuggestions(this, x, y));
				suggestions_popup->set_is_suggestions(1);
				suggestions_popup->activate(0);
			}
			else
			{
				suggestions_popup->update(this->suggestions,
					0,
					0,
					1);
				suggestions_popup->activate(0);
			}
		}
		else
// Show the highlighted text
		if(suggestions->size() == 1)
		{
			char *current_suggestion = suggestions->get(0);
			highlight_letter1 = text.length();
			text.append(current_suggestion + highlight_letter1 - suggestion_column);
			highlight_letter2 = text.length();
//printf("BC_TextBox::set_suggestions %d %d\n", __LINE__, suggestion_column);

			draw(1);

			delete suggestions_popup;
			suggestions_popup = 0;
		}
	}

// Clear the popup
	if(!suggestions || !this->suggestions->size())
	{
		delete suggestions_popup;
		suggestions_popup = 0;
	}
}

void BC_TextBox::set_selection(int char1, int char2, int ibeam)
{
	highlight_letter1 = char1;
	highlight_letter2 = char2;
	ibeam_letter = ibeam;
	draw(1);
}

int BC_TextBox::update(const char *text)
{
//printf("BC_TextBox::update 1 %d %s %s\n", strcmp(text, this->text), text, this->text);
	int text_len = strlen(text);
// Don't update if contents are the same
	if(!this->text.compare(text)) return 0;


	this->text.assign(text);
	if(highlight_letter1 > text_len) highlight_letter1 = text_len;
	if(highlight_letter2 > text_len) highlight_letter2 = text_len;
	if(ibeam_letter > text_len) ibeam_letter = text_len;
	draw(1);
	return 0;
}

int BC_TextBox::update(int64_t value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%ld", value);


	update(string);
	return 0;
}

int BC_TextBox::update(float value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%0.*f", precision, value);
//printf("BC_TextBox::update %d precision=%d text =%s\n", __LINE__, precision, string);

	update(string);
	return 0;
}

void BC_TextBox::disable()
{
	if(enabled)
	{
		enabled = 0;
		if(top_level)
		{
			if(active) top_level->deactivate();
			draw(1);
		}
	}
}

void BC_TextBox::enable()
{
	if(!enabled)
	{
		enabled = 1;
		if(top_level)
		{
			draw(1);
		}
	}
}

int BC_TextBox::get_enabled()
{
	return enabled;
}

void BC_TextBox::set_read_only(int value)
{
    this->read_only = value;
    
    if(value)
    {
        menu->remove_item(menu->cut);
        menu->remove_item(menu->paste);
        menu->cut = 0;
        menu->paste = 0;
    }
}

int BC_TextBox::pixels_to_rows(BC_WindowBase *window, int font, int pixels)
{
	return (pixels - 4) / 
		(window->get_text_ascent(font) + 1 + 
			window->get_text_descent(font) + 1);
}

int BC_TextBox::calculate_row_h(int rows, 
	BC_WindowBase *parent_window, 
	int has_border, 
	int font)
{
	return rows * 
		(parent_window->get_text_ascent(font) + 1 + 
		parent_window->get_text_descent(font) + 1) +
		(has_border ? 4 : 0);
}

const char* BC_TextBox::get_text()
{
	return text.c_str();
}

int BC_TextBox::get_text_rows()
{
	int text_len = text.length();
	int result = 1;
	for(int i = 0; i < text_len; i++)
	{
		if(text[i] == 0xa) result++;
	}
	return result;
}


int BC_TextBox::get_row_h(int rows)
{
	return rows * text_height + top_margin + bottom_margin;
}

int BC_TextBox::reposition_window(int x, int y, int w, int rows)
{
	int new_h = get_h();
	if(w < 0) w = get_w();
	if(rows != -1)
	{
		new_h = get_row_h(rows);
		this->rows = rows;
	}

	if(x != get_x() || 
		y != get_y() || 
		w != get_w() || 
		new_h != get_h())
	{
// printf("BC_TextBox::reposition_window 1 %d %d %d %d %d %d %d %d\n",
// x, get_x(), y, get_y(), w, get_w(), new_h, get_h());
		BC_WindowBase::reposition_window(x, y, w, new_h);
		draw(0);
	}
	return 0;
}

void BC_TextBox::draw_border()
{
	BC_Resources *resources = get_resources();
// Clear margins
	set_color(background_color);
	draw_box(0, 0, left_margin, get_h());
	draw_box(get_w() - right_margin, 0, right_margin, get_h());

	if(has_border)
	{
		if(highlighted)
			draw_3d_border(0, 0, w, h,
				resources->text_border1, 
				resources->text_border2_hi, 
				resources->text_border3_hi,
				resources->text_border4);
		else
			draw_3d_border(0, 0, w, h, 
				resources->text_border1, 
				resources->text_border2,
				resources->text_border3,
				resources->text_border4);
	}
}

void BC_TextBox::draw_cursor()
{
//	set_color(background_color);

	if(ibeam_x >= 0 &&
		ibeam_y >= 0)
	{
		set_color(WHITE);
		set_inverse();

		draw_box(ibeam_x + text_x, 
			ibeam_y + text_y, 
			BCCURSORW, 
			text_height);
		set_opaque();
	}
}


void BC_TextBox::draw(int flush)
{
	int i, j, k, text_len;
	int row_begin, row_end;
	int highlight_x1, highlight_x2;
	int need_ibeam = 1;
	string text_row;
	BC_Resources *resources = get_resources();

//printf("BC_TextBox::draw %d %s\n", __LINE__, text.c_str());
// Background
	if(has_border)
	{
		background_color = resources->text_background;
	}
	else
	{
		if(highlighted)
		{
			background_color = high_color;
		}
		else
		{
			background_color = back_color;
		}
	}

	set_color(background_color);
	draw_box(0, 0, w, h);

// Draw text with selection
	set_font(font);
	text_len = text.length();
//printf("BC_TextBox::draw 0 %s %d %d %d %d\n", text, text_y, text_len, get_w(), text_height);

	for(i = 0, k = text_y; i < text_len && k < get_h(); k += text_height)
	{
// Draw row of text
		if(text[i] == '\n') i++;
		row_begin = i;
		text_row.erase();
		for(j = 0; text[i] != '\n' && i < text_len; j++, i++)
		{
			text_row.push_back(text[i]);
		}
		row_end = i;

//printf("BC_TextBox::draw 1 %d %d %c\n", row_begin, row_end, text_row[j - 1]);

		if(k > -text_height + top_margin && k < get_h() - bottom_margin)
		{
// Draw highlighted region of row
			if(highlight_letter2 > highlight_letter1 &&
				highlight_letter2 > row_begin && 
				highlight_letter1 <= row_end)
			{
				if(active && enabled && get_has_focus())
					set_color(resources->text_highlight);
				else
					set_color(resources->text_inactive_highlight);

				if(highlight_letter1 >= row_begin && 
					highlight_letter1 <= row_end)
					highlight_x1 = get_text_width(font, 
						text_row.c_str(), 
						highlight_letter1 - row_begin);
				else
					highlight_x1 = 0;

				if(highlight_letter2 > row_begin && 
					highlight_letter2 <= row_end)
					highlight_x2 = get_text_width(font, 
						text_row.c_str(), 
						highlight_letter2 - row_begin);
				else
					highlight_x2 = get_w();

				draw_box(highlight_x1 + text_x, 
					k, 
					highlight_x2 - highlight_x1, 
					text_height);
			}

// Draw text over highlight
			if(enabled)
				set_color(resources->text_default);
			else
				set_color(MEGREY);

			draw_text(text_x, 
				k + text_ascent, 
				text_row.c_str());

// Get ibeam location
			if(ibeam_letter >= row_begin && ibeam_letter <= row_end)
			{
				need_ibeam = 0;
				ibeam_y = k - text_y;
				ibeam_x = get_text_width(font, 
					text_row.c_str(), 
					ibeam_letter - row_begin);
			}
		}
	}

//printf("BC_TextBox::draw 3 %d\n", ibeam_y);
	if(need_ibeam)
	{
		if(text_len == 0)
		{
			ibeam_x = 0;
			ibeam_y = 0;
		}
		else
		{
			ibeam_x = -1;
			ibeam_y = -1;
		}
	}

//printf("BC_TextBox::draw 4 %d\n", ibeam_y);
// Draw solid cursor
	if (active) 
		draw_cursor();

// Border
	draw_border();
	flash(flush);
}

int BC_TextBox::focus_in_event()
{
	draw(1);
	return 1;
}

int BC_TextBox::focus_out_event()
{
	draw(1);
	return 1;
}

int BC_TextBox::cursor_enter_event()
{
	if(top_level->event_win == win && enabled)
	{
		tooltip_done = 0;

		if(!highlighted)
		{
			highlighted = 1;
			draw_border();
			flash(1);
		}
	}
	return 0;
}

int BC_TextBox::cursor_leave_event()
{
	if(highlighted)
	{
		highlighted = 0;
		draw_border();
		hide_tooltip();
		flash(1);
	}
	return 0;
}

int BC_TextBox::button_press_event()
{
	const int debug = 0;
	
	if(!enabled) return 0;
// 	if(get_buttonpress() != WHEEL_UP &&
// 		get_buttonpress() != WHEEL_DOWN &&
// 		get_buttonpress() != LEFT_BUTTON &&
// 		get_buttonpress() != MIDDLE_BUTTON) return 0;



	if(debug) printf("BC_TextBox::button_press_event %d\n", __LINE__);

	int cursor_letter = 0;
	int text_len = text.length();
	int update_scroll = 0;


	if(top_level->event_win == win)
	{
		if(!active)
		{
			hide_tooltip();
			top_level->deactivate();
			activate();
		}


		if(get_buttonpress() == WHEEL_UP)
		{
			text_y += text_height;
			text_y = MIN(text_y, top_margin);
			update_scroll = 1;
		}
		else
		if(get_buttonpress() == WHEEL_DOWN)
		{
			int min_y = -(get_text_rows() * 
				text_height - 
				get_h() + 
				bottom_margin);
			text_y -= text_height;
			text_y = MAX(text_y, min_y);
			text_y = MIN(text_y, top_margin);
			update_scroll = 1;
		}
		else
		if(get_buttonpress() == RIGHT_BUTTON)
		{
			menu->activate_menu();
		}
		else
		{

			cursor_letter = get_cursor_letter(top_level->cursor_x, top_level->cursor_y);

	//printf("BC_TextBox::button_press_event %d %d\n", __LINE__, cursor_letter);


			if(get_triple_click())
			{
	//printf("BC_TextBox::button_press_event %d\n", __LINE__);
				line_selected = 1;
				select_line(highlight_letter1, highlight_letter2, cursor_letter);
				highlight_letter3 = highlight_letter1;
				highlight_letter4 = highlight_letter2;
				ibeam_letter = highlight_letter2;
				copy_selection(PRIMARY_SELECTION);
			}
			else
			if(get_double_click())
			{
				word_selected = 1;
				select_word(highlight_letter1, highlight_letter2, cursor_letter);
				highlight_letter3 = highlight_letter1;
				highlight_letter4 = highlight_letter2;
				ibeam_letter = highlight_letter2;
				copy_selection(PRIMARY_SELECTION);
			}
			else
			if(get_buttonpress() == MIDDLE_BUTTON)
			{
				highlight_letter3 = highlight_letter4 = 
					ibeam_letter = highlight_letter1 = 
					highlight_letter2 = cursor_letter;
				paste_selection(PRIMARY_SELECTION);
                text_len = text.length();
                update_undo();
			}
			else
			{
				text_selected = 1;
				highlight_letter3 = highlight_letter4 = 
					ibeam_letter = highlight_letter1 = 
					highlight_letter2 = cursor_letter;
			}


	// Handle scrolling by highlighting text
			if(text_selected || word_selected || line_selected)
			{
				set_repeat(top_level->get_resources()->scroll_repeat);
			}

			if(ibeam_letter < 0) ibeam_letter = 0;
			if(ibeam_letter > text_len) ibeam_letter = text_len;
		}
		
		draw(1);
		if(update_scroll && yscroll)
		{
			yscroll->update_length(get_text_rows(),
				get_text_row(),
				yscroll->get_handlelength(),
				1);
		}
		return 1;
	}
	else
	if(active && (!yscroll || !yscroll->is_event_win()))
	{
//printf("BC_TextBox::button_press_event %d\n", __LINE__);
// Suggestion popup is not active but must be deactivated.
		if(suggestions_popup)
		{
			return 0;
// printf("BC_TextBox::button_press_event %d\n", __LINE__);
// // Pass event to suggestions popup
// 			if(!suggestions_popup->button_press_event())
// 			{
// printf("BC_TextBox::button_press_event %d\n", __LINE__);
// 				top_level->deactivate();
// 			}
		}
		else
		{
			top_level->deactivate();
		}
	}


	return 0;
}

int BC_TextBox::button_release_event()
{
	if(active)
	{
		hide_tooltip();
		if(text_selected || word_selected || line_selected)
		{
			text_selected = 0;
			word_selected = 0;
			line_selected = 0;

// Stop scrolling by highlighting text
			unset_repeat(top_level->get_resources()->scroll_repeat);
		}
	}
	return 0;
}

int BC_TextBox::cursor_motion_event()
{
	int cursor_letter, text_len = text.length(), letter1, letter2;

	if(active)
	{
		if(text_selected || word_selected || line_selected)
		{
			cursor_letter = get_cursor_letter(top_level->cursor_x, 
				top_level->cursor_y);

//printf("BC_TextBox::cursor_motion_event %d cursor_letter=%d\n", __LINE__, cursor_letter);

			if(line_selected)
			{
				select_line(letter1, letter2, cursor_letter);
			}
			else
			if(word_selected)
			{
				select_word(letter1, letter2, cursor_letter);
			}
			else
			if(text_selected)
			{
				letter1 = letter2 = cursor_letter;
			}

			if(letter1 <= highlight_letter3)
			{
				highlight_letter1 = letter1;
				highlight_letter2 = highlight_letter4;
				ibeam_letter = letter1;
			}
			else
			if(letter2 >= highlight_letter4)
			{
				highlight_letter2 = letter2;
				highlight_letter1 = highlight_letter3;
				ibeam_letter = letter2;
			}

			copy_selection(PRIMARY_SELECTION);

			


			draw(1);
			return 1;
		}
	}

	return 0;
}

int BC_TextBox::activate()
{
	top_level->active_subwindow = this;
	active = 1;
	draw(1);
	top_level->set_repeat(top_level->get_resources()->blink_rate);
	return 0;
}

int BC_TextBox::deactivate()
{
//printf("BC_TextBox::deactivate %d suggestions_popup=%p\n", __LINE__, suggestions_popup);
	active = 0;
	top_level->unset_repeat(top_level->get_resources()->blink_rate);
	if(suggestions_popup)
	{
// Must deactivate instead of delete since this is called from BC_ListBox::button_press_event
//		suggestions_popup->deactivate();

		delete suggestions_popup;
		suggestions_popup = 0;
	}

	draw(1);
	return 0;
}

int BC_TextBox::repeat_event(int64_t duration)
{
	int result = 0;
	int cursor_y = get_cursor_y();
	int cursor_x = get_cursor_x();

	if(duration == top_level->get_resources()->tooltip_delay &&
		tooltip_text[0] != 0 &&
		highlighted)
	{
		show_tooltip();
		tooltip_done = 1;
		result = 1;
	}
		
	if(duration == top_level->get_resources()->blink_rate && 
		active &&
		get_has_focus())
	{
// don't flash if keypress
		if(skip_cursor->get_difference() < 500)
		{
// printf("BC_TextBox::repeat_event 1 %lld %lld\n", 
// skip_cursor->get_difference(), 
// duration);
			result = 1;
		}
		else
		{
			if(!(text_selected || word_selected || line_selected))
			{
				draw_cursor();
				flash(1);
			}
			result = 1;
		}
	}

	if(duration == top_level->get_resources()->scroll_repeat && 
		(text_selected || word_selected || line_selected))
	{
		if(get_cursor_y() < top_margin)
		{
			int difference = top_margin - get_cursor_y();
			
			text_y += difference;
// printf("BC_TextBox::repeat_event %d %d %d\n", 
// __LINE__,
// text_y,
// top_margin);
			text_y = MIN(text_y, top_margin);

// select text
			
			draw(1);
			motion_event();
			result = 1;
		}
		else
		if(get_cursor_y() > get_h() - bottom_margin)
		{
			int difference = get_cursor_y() - 
				(get_h() - bottom_margin);
			int min_y = -(get_text_rows() * 
				text_height - 
				get_h() + 
				bottom_margin);
			
			text_y -= difference;

// printf("BC_TextBox::repeat_event %d %d %d\n", 
// __LINE__,
// text_y,
// min_y);

			text_y = MAX(min_y, text_y);
			text_y = MIN(text_y, top_margin);

			draw(1);
			motion_event();
			result = 1;
		}

		if(get_cursor_x() < left_margin)
		{
			int difference = left_margin - get_cursor_x();
			
			text_x += difference;
			text_x = MIN(text_x, left_margin);
			draw(1);
			result = 1;
		}
		else
		if(get_cursor_x() > get_w() - right_margin)
		{
			int difference = get_cursor_x() - (get_w() - right_margin);
			int new_text_x = text_x - difference;

// Get width of current row
			int min_x = 0;
			int row_width = 0;
			int text_len = text.length();
			int row_begin = 0;
			int row_end = 0;
			for(int i = 0, k = text_y; i < text_len; k += text_height)
			{
				row_begin = i;
				while(text[i] != '\n' && i < text_len)
				{
					i++;
				}
				row_end = i;
				if(text[i] == '\n') i++;
				
				if(cursor_y >= k && cursor_y < k + text_height)
				{
					row_width = get_text_width(font,
						text.c_str() + row_begin,
						row_end - row_begin);

// printf("BC_TextBox::repeat_event %d %s %d\n", 
// __LINE__,
// text + row_begin,
// row_width);
					break;
				}
			}
			
			min_x = -row_width + get_w() - left_margin - BCCURSORW;
			new_text_x = MAX(new_text_x, min_x);
			new_text_x = MIN(new_text_x, left_margin);
			
			if(new_text_x < text_x) text_x = new_text_x;
			draw(1);
			result = 1;
		}
	}

	return result;
}

void BC_TextBox::default_keypress(int &dispatch_event, int &result)
{
    if(read_only)
    {
        return;
    }

	if((top_level->get_keypress() == RETURN) ||
        (top_level->get_keypress() > 30 && top_level->get_keypress() <= 255))
//		(top_level->get_keypress() > 30 && top_level->get_keypress() < 127))
	{
		if((top_level->get_keypress() == RETURN) || 
			(top_level->get_keypress() > 30))
		{
// Substitute UNIX linefeed
			if(top_level->get_keypress() == RETURN)
			{
				temp_string[0] = 0xa;
				temp_string[1] = 0;
			}
			else
			{ 
#ifdef X_HAVE_UTF8_STRING
				if (top_level->get_keypress_utf8() != 0)
				{
					memcpy(temp_string, top_level->get_keypress_utf8(), KEYPRESSLEN);
				} 
				else 
				{
					temp_string[0] = top_level->get_keypress();
					temp_string[1] = 0;
				}
#else
				temp_string[0] = top_level->get_keypress();
				temp_string[1] = 0;
#endif
			}
		}
		
		insert_text((char*)temp_string);
		find_ibeam(1);
        update_undo();
		draw(1);
		dispatch_event = 1;
		result = 1;
	}
}

int BC_TextBox::keypress_event()
{
// Result == 2 contents changed
// Result == 1 trapped keypress
// Result == 0 nothing
	int result = 0;
	int text_len;
	int dispatch_event = 0;

	if(!active || !enabled) return 0;

	text_len = text.length();
	last_keypress = get_keypress();
//printf("BC_TextBox::keypress_event %d %x\n", __LINE__, get_keypress());
	switch(get_keypress())
	{
		case ESC:
// Deactivate the suggestions
			if(suggestions && suggestions_popup)
			{
				delete suggestions_popup;
				suggestions_popup = 0;
				result = 1;
			}
			else
			{
				top_level->deactivate();
				result = 0;
			}
			break;





		case RETURN:
			if(rows == 1)
			{
				top_level->deactivate();
				dispatch_event = 1;
				result = 0;
			}
			else
			{
				default_keypress(dispatch_event, result);
			}
			break;




// Handle like a default keypress
		case TAB:
			top_level->cycle_textboxes(1);
			result = 1;
			break;



		case LEFTTAB:
			top_level->cycle_textboxes(-1);
			result = 1;
			break;

		case LEFT:
			if(ibeam_letter > 0)
			{
				int old_ibeam_letter = ibeam_letter;
// Single character
				if(!ctrl_down())
				{
#ifdef X_HAVE_UTF8_STRING
					int s = utf8seek(ibeam_letter, 1);
					ibeam_letter -= (1 + s);
#else
 					ibeam_letter--;
#endif
				}
				else
// Word
				{
					ibeam_letter--;
					while(ibeam_letter > 0 && isalnum(text[ibeam_letter - 1]))
						ibeam_letter--;
				}


// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = ibeam_letter;
						highlight_letter2 = old_ibeam_letter;
					}
					else
// Extend left highlight
					if(highlight_letter1 == old_ibeam_letter)
					{
						highlight_letter1 = ibeam_letter;
					}
					else
// Shrink right highlight
					if(highlight_letter2 == old_ibeam_letter)
					{
						highlight_letter2 = ibeam_letter;
					}
				}
				else
				{
					highlight_letter1 = highlight_letter2 = ibeam_letter;
				}


				find_ibeam(1);
				if(keypress_draw) draw(1);
			}
			result = 1;
			break;

		case RIGHT:
			if(ibeam_letter < text_len)
			{
				int old_ibeam_letter = ibeam_letter;
// Single character
				if(!ctrl_down())
				{
#ifdef X_HAVE_UTF8_STRING
					int s = utf8seek(ibeam_letter, 1);
					ibeam_letter += (1 + s);
#else
 					ibeam_letter++;
#endif
				}
				else
// Word
				{
					while(ibeam_letter < text_len && isalnum(text[ibeam_letter++]))
						;
				}



// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = old_ibeam_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Shrink left highlight
					if(highlight_letter1 == old_ibeam_letter)
					{
						highlight_letter1 = ibeam_letter;
					}
					else
// Expand right highlight
					if(highlight_letter2 == old_ibeam_letter)
					{
						highlight_letter2 = ibeam_letter;
					}
				}
				else
				{
					highlight_letter1 = highlight_letter2 = ibeam_letter;
				}

				find_ibeam(1);
				if(keypress_draw) draw(1);
			}
			result = 1;
			break;

		case UP:
			if(suggestions && suggestions_popup)
			{
// Pass to suggestions popup
//printf("BC_TextBox::keypress_event %d\n", __LINE__);
				suggestions_popup->activate(1);
				suggestions_popup->keypress_event();
				result = 1;
			}
			else
			if(ibeam_letter > 0)
			{
//printf("BC_TextBox::keypress_event 1 %d %d %d\n", ibeam_x, ibeam_y, ibeam_letter);
				int new_letter = get_cursor_letter2(ibeam_x + text_x, 
					ibeam_y + text_y - text_height);
//printf("BC_TextBox::keypress_event 2 %d %d %d\n", ibeam_x, ibeam_y, new_letter);

// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Expand left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1 = new_letter;
					}
					else
// Shrink right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2)
				{
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw(1);
			}
			result = 1;
			break;

		case PGUP:
			if(ibeam_letter > 0)
			{
				int new_letter = get_cursor_letter2(ibeam_x + text_x, 
					ibeam_y + text_y - get_h());

// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Expand left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1 = new_letter;
					}
					else
// Shrink right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2)
				{
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw(1);
			}
			result = 1;
			break;

		case DOWN:
// printf("BC_TextBox::keypress_event %d %p %p\n", 
// __LINE__,
// suggestions,
// suggestions_popup);
			if(suggestions && suggestions_popup)
			{
// Pass to suggestions popup
				suggestions_popup->activate(1);
				suggestions_popup->keypress_event();
				result = 1;
			}
			else
//			if(ibeam_letter > 0)
			{
// Extend selection
				int new_letter = get_cursor_letter2(ibeam_x + text_x, 
					ibeam_y + text_y + text_height);
//printf("BC_TextBox::keypress_event 10 %d\n", new_letter);

				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Shrink left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1 = new_letter;
					}
					else
// Expand right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2)
				{
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw(1);

//printf("BC_TextBox::keypress_event 20 %d\n", ibeam_letter);
			}
			result = 1;
			break;

		case PGDN:
			{
// Extend selection
				int new_letter = get_cursor_letter2(ibeam_x + text_x, 
					ibeam_y + text_y + get_h());
//printf("BC_TextBox::keypress_event 10 %d\n", new_letter);

				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Shrink left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1 = new_letter;
					}
					else
// Expand right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2)
				{
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw(1);

//printf("BC_TextBox::keypress_event 20 %d\n", ibeam_letter);
			}
			result = 1;
			break;
		
		case END:
		{
			delete suggestions_popup;
			suggestions_popup = 0;
		
			int old_ibeam_letter = ibeam_letter;

			while(ibeam_letter < text_len && text[ibeam_letter] != '\n')
				ibeam_letter++;

			if(top_level->shift_down())
			{
// Begin selection
				if(highlight_letter1 == highlight_letter2)
				{
					highlight_letter2 = ibeam_letter;
					highlight_letter1 = old_ibeam_letter;
				}
				else
// Shrink selection
				if(highlight_letter1 == old_ibeam_letter)
				{
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = ibeam_letter;
				}
				else
// Extend selection
				if(highlight_letter2 == old_ibeam_letter)
				{
					highlight_letter2 = ibeam_letter;
				}
			}
			else
				highlight_letter1 = highlight_letter2 = ibeam_letter;

			find_ibeam(1);
			if(keypress_draw) draw(1);
			result = 1;
			break;
		}

		case HOME:
		{
			delete suggestions_popup;
			suggestions_popup = 0;

			int old_ibeam_letter = ibeam_letter;

			while(ibeam_letter > 0 && text[ibeam_letter - 1] != '\n')
				ibeam_letter--;

			if(top_level->shift_down())
			{
// Begin selection
				if(highlight_letter1 == highlight_letter2)
				{
					highlight_letter2 = old_ibeam_letter;
					highlight_letter1 = ibeam_letter;
				}
				else
// Extend selection
				if(highlight_letter1 == old_ibeam_letter)
				{
					highlight_letter1 = ibeam_letter;
				}
				else
// Shrink selection
				if(highlight_letter2 == old_ibeam_letter)
				{
					highlight_letter2 = highlight_letter1;
					highlight_letter1 = ibeam_letter;
				}
			}
			else
				highlight_letter1 = highlight_letter2 = ibeam_letter;

			find_ibeam(1);
			if(keypress_draw) draw(1);
			result = 1;
			break;
		}

    	case BACKSPACE:
            if(read_only)
            {
                break;
            }

			if(suggestions_popup)
			{
				delete suggestions_popup;
				suggestions_popup = 0;
			}

			if(highlight_letter1 == highlight_letter2)
			{
				if(ibeam_letter > 0)
				{
#ifdef X_HAVE_UTF8_STRING					
			        int s = utf8seek(ibeam_letter, 1);
					delete_selection(ibeam_letter - (1 + s), ibeam_letter, text_len);
					ibeam_letter -= (1 + s);
#else
 					delete_selection(ibeam_letter - 1, ibeam_letter, text_len);
 					ibeam_letter--;
#endif
				}
			}
			else
			{
				delete_selection(highlight_letter1, highlight_letter2, text_len);
				highlight_letter2 = ibeam_letter = highlight_letter1;
			}

			find_ibeam(1);
			if(keypress_draw) draw(1);
			dispatch_event = 1;
			result = 1;
    		break;

		case DELETE:
            if(read_only)
            {
                break;
            }

//printf("BC_TextBox::keypress_event %d\n", __LINE__);
			if(highlight_letter1 == highlight_letter2)
			{
				if(ibeam_letter < text_len)
				{
#ifdef X_HAVE_UTF8_STRING
					int s = utf8seek(ibeam_letter, 1);
					delete_selection(ibeam_letter, ibeam_letter + (1 + s), text_len);
#else
 					delete_selection(ibeam_letter, ibeam_letter + 1, text_len);
#endif
				}
			}
			else
			{
				delete_selection(highlight_letter1, highlight_letter2, text_len);
				highlight_letter2 = ibeam_letter = highlight_letter1;
			}
			
			find_ibeam(1);
			if(keypress_draw) draw(1);
			dispatch_event = 1;
			result = 1;
			break;



		default:
			if(ctrl_down())
			{
                if(get_keypress() == 'a' || get_keypress() == 'A')
                {
                    select_all();
                    result = 1;
                }
                else
				if(get_keypress() == 'c' || get_keypress() == 'C')
				{
					result = copy(0);
				}
				else
				if(!read_only && (get_keypress() == 'v' || get_keypress() == 'V'))
				{
					result = paste(0);

					
					dispatch_event = 1;
				}
				else
				if(!read_only && (get_keypress() == 'x' || get_keypress() == 'X'))
				{
					result = cut(0);

					dispatch_event = 1;
				}
                else
                if(!read_only && (get_keypress() == 'z'))
                {
                    undo();
                    result = 1;
                }
                else
                if(!read_only && (get_keypress() == 'Z'))
                {
                    redo();
                    result = 1;
                }
			}
            else
            {
    			default_keypress(dispatch_event, result);
            }
			break;
	}

	if(result) skip_cursor->update();
	if(dispatch_event) handle_event();
	return result;
}

int BC_TextBox::cut(int do_housekeeping)
{
	int text_len = text.length();
	if(highlight_letter1 != highlight_letter2)
	{
		copy_selection(SECONDARY_SELECTION);
		delete_selection(highlight_letter1, highlight_letter2, text_len);
		highlight_letter2 = ibeam_letter = highlight_letter1;
	}

	find_ibeam(1);
	if(keypress_draw) draw(1);
    update_undo();
	
	if(do_housekeeping)
	{
		skip_cursor->update();
		handle_event();
	}
	return 1;
}

int BC_TextBox::copy(int do_housekeeping)
{
	int result = 0;
	if(highlight_letter1 != highlight_letter2)
	{
		copy_selection(SECONDARY_SELECTION);
		result = 1;
		if(do_housekeeping)
		{
			skip_cursor->update();
		}
	}
	return result;
}

int BC_TextBox::paste(int do_housekeeping)
{
	paste_selection(SECONDARY_SELECTION);
	find_ibeam(1);
	if(keypress_draw) draw(1);
    update_undo();
	if(do_housekeeping)
	{
		skip_cursor->update();
		handle_event();
	}
	return 1;
}


int BC_TextBox::uses_text()
{
	return 1;
}

#ifdef X_HAVE_UTF8_STRING
int BC_TextBox::utf8seek(int &seekpoint, int reverse)
{
      int utf8pos = 0;
      int i = seekpoint;
      unsigned char z;
        if(reverse & 1)
        {
        if((unsigned char)text[i-1] >= 0x80)
        	  {
		for (int x = 1; x < 6; x++)
			{ 
			z = (unsigned char)text[i-x];
			
			if ((z >= 0xfc)) 
			{ 
			utf8pos = 5;
			break;
	 		} else if ((z >= 0xf8)) 
	 		{ 
	 		utf8pos = 4;
	 		break;
	 		} else if ((z >= 0xf0)) 
	 		{ 
	 		utf8pos = 3;
	 		break;
	 		} else if ((z >= 0xe0)) 
	 		{ 
	 		utf8pos = 2;
	 		break;
	 		} else if ((z >= 0xc0)) 
	 		{ 
	 		utf8pos = 1;
	 		break;
	 		}
                	}
        	   }
         } else {
		if((unsigned char)text[i] >= 0x80)
        	  {
        	  for (int x = 0; x < 5; x++)
			{ 
			z = (unsigned char)text[i+x];
			if (!(z & 0x20)) 
			{ 
			utf8pos = 1;
			break;
	 		} else if (!(z & 0x10)) 
	 		{ 
	 		utf8pos = 2;
	 		break;
	 		} else if (!(z & 0x08)) 
	 		{ 
	 		utf8pos = 3;
	 		break;
	 		} else if (!(z & 0x04)) 
	 		{ 
	 		utf8pos = 4;
	 		break;
	 		} else if (!(z & 0x02)) 
	 		{ 
	 		utf8pos = 5;
	 		break;
	 		}
                	}
        	   }
                
          }
          return utf8pos;
        
}
#endif // X_HAVE_UTF8_STRING


void BC_TextBox::delete_selection(int letter1, int letter2, int text_len)
{
	CLAMP(letter1, 0, text.length() - 1);
	CLAMP(letter2, letter1, text.length());
// printf("BC_TextBox::delete_selection %d %d %d %d\n", 
// __LINE__,
// letter1,
// letter2,
// text_len);
	text.erase(letter1, letter2 - letter1);

	do_separators(1);
}

void BC_TextBox::insert_text(char *string)
{
	int i, j, text_len, string_len;

	string_len = strlen(string);
	text_len = text.length();
	if(highlight_letter1 < highlight_letter2)
	{
		delete_selection(highlight_letter1, highlight_letter2, text_len);
		highlight_letter2 = ibeam_letter = highlight_letter1;
	}

//printf("BC_TextBox::insert_text %d %s\n", __LINE__, string);
	text.insert(ibeam_letter, string);

	ibeam_letter += string_len;

	do_separators(0);
}


// used for time entry
void BC_TextBox::do_separators(int ibeam_left)
{
	if(separators)
	{
// Remove separators from text
		int text_len = text.length();
		int separator_len = strlen(separators);
		for(int i = 0; i < text_len; i++)
		{
			if(!isalnum(text[i]))
			{
				text.erase(i, 1);
				if(!ibeam_left && i < ibeam_letter) ibeam_letter--;
				text_len--;
				i--;
			}
		}






// Insert separators into text
		for(int i = 0; i < separator_len; i++)
		{
			if(i < text_len)
			{
// Insert a separator
				if(!isalnum(separators[i]))
				{
					text.insert(i, 1, separators[i]);
 					text_len++;
					if(!ibeam_left && i < ibeam_letter) ibeam_letter++;
				}
			}
			else
			if(i >= text_len)
			{
				text.insert(i, 1, separators[i]);
				text_len++;
			}
		}

// Truncate text
		text.erase(separator_len);
		if(ibeam_letter > separator_len) ibeam_letter = separator_len;
	}

}

void BC_TextBox::get_ibeam_position(int &x, int &y)
{
	int i, j, k, row_begin, row_end, text_len;
	string text_row;

	text_len = text.length();
	y = 0;
	x = 0;
	for(i = 0; i < text_len; )
	{
		row_begin = i;
		text_row.erase();
		for(j = 0; text[i] != '\n' && i < text_len; j++, i++)
		{
			text_row.push_back(text[i]);
		}

		row_end = i;

		if(ibeam_letter >= row_begin && ibeam_letter <= row_end)
		{
			x = get_text_width(font, 
				text_row.c_str(), 
				ibeam_letter - row_begin);
//printf("BC_TextBox::get_ibeam_position 9 %d %d\n", x, y);
			return;
		}

		if(text[i] == '\n')
		{
			i++;
			y += text_height;
		}
	}
//printf("BC_TextBox::get_ibeam_position 10 %d %d\n", x, y);

	x = 0;
	return;
}

void BC_TextBox::set_text_row(int row)
{
	text_y = -(row * text_height) + top_margin;
	draw(1);
}

int BC_TextBox::get_text_row()
{
	return -(text_y - top_margin) / text_height;
}

void BC_TextBox::find_ibeam(int dispatch_event,
    int init)
{
	int x, y;
	int old_x = text_x, old_y = text_y;
    int x_pad = get_w() / 4;
    int y_pad = get_h() / 2;

	get_ibeam_position(x, y);

//printf("BC_TextBox::find_ibeam %d x=%d y=%d\n", __LINE__, x, y);
	if(init)
    {
        if(has_border)
	    {
		    x_pad = HORIZONTAL_MARGIN * 2 + BCCURSORW;
	    }
	    else
	    {
		    x_pad = HORIZONTAL_MARGIN_NOBORDER * 2 + BCCURSORW;
	    }
    }
    

	if(left_margin + text_x + x >= get_w() - right_margin - BCCURSORW)
	{
		text_x = -(x - (get_w() - x_pad)) + left_margin;
		if(text_x > left_margin) text_x = left_margin;
	}
	else
	if(left_margin + text_x + x < left_margin)
	{
		text_x = -(x - x_pad) + left_margin;
		if(text_x > left_margin) text_x = left_margin;
	}

	while(y + text_y >= get_h() - text_height - bottom_margin)
	{
		text_y -= text_height;
	}

	while(y + text_y < top_margin)
	{
		text_y += text_height;
		if(text_y > top_margin) 
		{
			text_y = top_margin;
			break;
		}
	}

	if(dispatch_event && (old_x != text_x || old_y != text_y)) motion_event();
// printf("BC_TextBox::find_ibeam %d text_x=%d text_y=%d\n",
// __LINE__,
// text_x,
// text_y);
}

// New algorithm
int BC_TextBox::get_cursor_letter(int cursor_x, int cursor_y)
{
	int i, j, k, current_y, row_begin, row_end, text_len, result = 0, done = 0;
	int column1, column2;
	int got_visible_row = 0;
	string text_row;

// Select complete row if cursor above the window
//printf("BC_TextBox::get_cursor_letter %d %d\n", __LINE__, text_y);
	if(cursor_y < text_y - text_height)
	{
		result = 0;
		done = 1;
	}

	
	text_len = text.length();

	for(i = 0, current_y = text_y; 
		i < text_len && current_y < get_h() && !done; 
		current_y += text_height)
	{
// Simulate drawing of 1 row
// entire row is a newline
		row_begin = i;
		text_row.erase();
		if(text[i] == '\n') 
		{
//printf("BC_TextBox::get_cursor_letter %d %d\n", __LINE__, i);
			j = 0;
			row_end = i;
		}
		else
		{
			for(j = 0; text[i] != '\n' && i < text_len; j++, i++)
			{
				text_row.push_back(text[i]);
			}
			row_end = i;
		}
//printf("BC_TextBox::get_cursor_letter %d %d\n", __LINE__, strlen(text_row));

		int first_visible_row = 0;
		int last_visible_row = 0;
		if(current_y + text_height > top_margin && !got_visible_row) 
		{
			first_visible_row = 1;
			got_visible_row = 1;
		}
		
		if((current_y + text_height >= get_h() - bottom_margin || 
			(row_end >= text_len && 
				current_y < get_h() - bottom_margin && 
				current_y + text_height > 0)))
			last_visible_row = 1;

// Cursor is inside vertical range of row
		if((cursor_y >= top_margin && 
				cursor_y < get_h() - bottom_margin && 
				cursor_y >= current_y && 
				cursor_y < current_y + text_height) ||
// Cursor is above 1st row
			(cursor_y < current_y + text_height && first_visible_row) ||
// Cursor is below last row
			(cursor_y >= current_y && last_visible_row))
		{
			column1 = 0;
			column2 = 0;
//printf("BC_TextBox::get_cursor_letter %d %d\n", __LINE__, cursor_x);
			if(cursor_x < left_margin)
			{
				result = row_begin;
				done = 1;
			}
			
			for(j = 0; j <= row_end - row_begin && !done; j++)
			{
				column2 = get_text_width(font, text_row.c_str(), j) + text_x;
				if((column2 + column1) / 2 >= cursor_x)
				{
					result = row_begin + j - 1;
					done = 1;
// printf("BC_TextBox::get_cursor_letter %d %d %d %d\n", 
// __LINE__, 
// result,
// first_visible_row,
// last_visible_row);
				}
				column1 = column2;
			}

			if(!done)
			{
				result = row_end;
				done = 1;
			}
		}

		if(text[i] == '\n') i++;


// Select complete row if last visible & cursor is below window
 		if(last_visible_row && cursor_y > current_y + text_height * 2)
 			result = row_end;

		if(i >= text_len && !done)
		{
			result = text_len;
		}
	}


// printf("BC_TextBox::get_cursor_letter %d cursor_y=%d current_y=%d h=%d %d %d\n", 
// __LINE__,
// cursor_y,
// current_y,
// get_h(),
// first_visible_row,
// last_visible_row);
	if(result < 0) result = 0;
	if(result > text_len) 
	{
//printf("BC_TextBox::get_cursor_letter %d\n", __LINE__);
		result = text_len;
	}


	return result;
}

// Old algorithm
int BC_TextBox::get_cursor_letter2(int cursor_x, int cursor_y)
{
	int i, j, k, row_begin, row_end, text_len, result = 0, done = 0;
	int column1, column2;
	string text_row;
	text_len = text.length();

	if(cursor_y < text_y)
	{
		result = 0;
		done = 1;
	}

	for(i = 0, k = text_y; i < text_len && !done; k += text_height)
	{
		row_begin = i;
		text_row.erase();
		for(j = 0; text[i] != '\n' && i < text_len; j++, i++)
		{
			text_row.push_back(text[i]);
		}
		row_end = i;

		if(cursor_y >= k && cursor_y < k + text_height)
		{
			column1 = 0;
			column2 = 0;
			for(j = 0; j <= row_end - row_begin && !done; j++)
			{
				column2 = get_text_width(font, text_row.c_str(), j) + text_x;
				if((column2 + column1) / 2 >= cursor_x)
				{
					result = row_begin + j - 1;
					done = 1;
				}
				column1 = column2;
			}
			if(!done)
			{
				result = row_end;
				done = 1;
			}
		}
		if(text[i] == '\n') i++;
		
		if(i >= text_len && !done)
		{
			result = text_len;
		}
	}
	if(result < 0) result = 0;
	if(result > text_len) result = text_len;
	return result;
}

// character types
#define ALNUM 0
#define WHITESPACE 1
#define SPECIAL 2
#define REJECT 3
static int char_type(int c)
{
// must be tested in the right order since isalnum is dumb
    if(c == ' ' ||
        c == '\t')
    {
        return WHITESPACE;
    }
    else
    if(isalnum(c))
    {
        return ALNUM;
    }
    else
    if(c != '\n' &&
        c != '\r')
    {
        return SPECIAL;
    }
    else
    {
        return REJECT;
    }
}

void BC_TextBox::select_word(int &letter1, int &letter2, int ibeam_letter)
{
	int text_len = text.length();
	if(!text_len) return;

// determine the type of character under the ibeam
    int type;
    type = char_type(text[ibeam_letter]);

// don't select anything besides the ibeam
    if(type == REJECT)
    {
        letter1 = ibeam_letter;
        letter2 = ibeam_letter + 1;
    }
    else
    {
// select characters like the ibeam
//printf("BC_TextBox::select_word %d ibeam_letter=%d\n", __LINE__, ibeam_letter);
    // start of word
	    letter1 = letter2 = ibeam_letter;
	    do
	    {
		    if(char_type(text[letter1]) == type)
            {
                letter1--;
            }
	    }while(letter1 > 0 && char_type(text[letter1]) == type);
	    if(char_type(text[letter1]) != type) letter1++;

    // end of word
	    do
	    {
		    if(char_type(text[letter2]) == type)
            {
                letter2++;
            }
	    }while(letter2 < text_len && char_type(text[letter2]) == type);
//	if(letter2 < text_len && char_type(text[letter2]) == type) letter2++;
    }

// clamp it
	if(letter1 < 0) letter1 = 0;
	if(letter2 < 0) letter2 = 0;
	if(letter1 > text_len) letter1 = text_len;
	if(letter2 > text_len) letter2 = text_len;
}

void BC_TextBox::select_line(int &letter1, int &letter2, int ibeam_letter)
{
	int text_len = text.length();
	if(!text_len) return;

	letter1 = letter2 = ibeam_letter;

// Rewind to previous linefeed
	do
	{
		if(text[letter1] != '\n') letter1--;
	}while(letter1 > 0 && text[letter1] != '\n');
	if(text[letter1] == '\n') letter1++;

// Advance to next linefeed
	do
	{
		if(text[letter2] != '\n') letter2++;
	}while(letter2 < text_len && text[letter2] != '\n');
	if(letter2 < text_len && text[letter2] == '\n') letter2++;

	if(letter1 < 0) letter1 = 0;
	if(letter2 < 0) letter2 = 0;
	if(letter1 > text_len) letter1 = text_len;
	if(letter2 > text_len) letter2 = text_len;
}

void BC_TextBox::select_all()
{
    highlight_letter1 = highlight_letter3 = 0;
    highlight_letter2 = highlight_letter4 = text.length();
    copy_selection(PRIMARY_SELECTION);
    draw(1);
}

void BC_TextBox::copy_selection(int clipboard_num)
{
	int text_len = text.length();

	if(highlight_letter1 >= text_len ||
		highlight_letter2 > text_len ||
		highlight_letter1 < 0 ||
		highlight_letter2 < 0 ||
		highlight_letter2 - highlight_letter1 <= 0) return;

	get_clipboard()->to_clipboard(text.c_str() + highlight_letter1, 
		highlight_letter2 - highlight_letter1, 
		clipboard_num);
}

void BC_TextBox::paste_selection(int clipboard_num)
{
//printf("BC_TextBox::paste_selection %d\n", __LINE__);
	int len = get_clipboard()->clipboard_len(clipboard_num);
	if(len)
	{
		char *string = new char[len + 1];
		get_clipboard()->from_clipboard(string, len, clipboard_num);
		insert_text(string);
		delete [] string;
	}
}

void BC_TextBox::set_keypress_draw(int value)
{
	keypress_draw = value;
}

int BC_TextBox::get_last_keypress()
{
	return last_keypress;
}

int BC_TextBox::get_ibeam_letter()
{
	return ibeam_letter;
}

void BC_TextBox::set_ibeam_letter(int number, int redraw)
{
	this->ibeam_letter = number;
	if(redraw)
	{
		draw(1);
	}
}

void BC_TextBox::set_separators(const char *separators)
{
	this->separators = (char*)separators;
}

int BC_TextBox::get_rows()
{
	return rows;
}

void BC_TextBox::update_undo()
{
    if(undo_enabled)
    {
        BC_TextBoxUndo *item = undos.push();
        item->from_textbox(this);
    }
}

void BC_TextBox::reset_undo()
{
    undos.clear();
    update_undo();
}

void BC_TextBox::undo()
{
    undos.pull();
    BC_TextBoxUndo *item = undos.current;


    if(item)
    {
        item->to_textbox(this);
        draw(1);
        handle_event();
    }
}

void BC_TextBox::redo()
{
    BC_TextBoxUndo *item = undos.pull_next();
    if(item)
    {
        item->to_textbox(this);
        draw(1);
        handle_event();
    }
}








BC_TextBoxSuggestions::BC_TextBoxSuggestions(BC_TextBox *text_box, 
	int x, 
	int y)
 : BC_ListBox(x,
 	y,
	text_box->get_w(),
	200,
	LISTBOX_TEXT,
	text_box->suggestions,
	0,
	0,
	1,
	0,
	1)
{
	this->text_box = text_box;
	set_use_button(0);
	set_justify(LISTBOX_LEFT);
}

BC_TextBoxSuggestions::~BC_TextBoxSuggestions()
{
}

int BC_TextBoxSuggestions::selection_changed()
{
return 0;

#if 0
//printf("BC_TextBoxSuggestions::selection_changed %d\n", __LINE__);
	BC_ListBoxItem *item = get_selection(0, 0);
//printf("BC_TextBoxSuggestions::selection_changed %d\n", __LINE__);

	if(item)
	{
		char *current_suggestion = item->get_text();
//printf("BC_TextBoxSuggestions::selection_changed %d\n", __LINE__);
//printf("BC_TextBoxSuggestions::selection_changed %d\n", __LINE__);
		strcpy(text_box->text + text_box->suggestion_column, current_suggestion);
//printf("BC_TextBoxSuggestions::selection_changed %d\n", __LINE__);
		*(text_box->text + text_box->suggestion_column + strlen(current_suggestion)) = 0;

//printf("BC_TextBoxSuggestions::selection_changed %d\n", __LINE__);
		text_box->draw(1);
		text_box->handle_event();
	}

	return 1;
#endif

}

int BC_TextBoxSuggestions::handle_event()
{
	BC_ListBoxItem *item = get_selection(0, 0);
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
	if(item)
	{
		char *current_suggestion = item->get_text();
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
		
		
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
		text_box->text.erase(text_box->suggestion_column);
		text_box->text.append(current_suggestion);
//		strcpy(text_box->text + text_box->suggestion_column, current_suggestion);
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
//		*(text_box->text + text_box->suggestion_column + strlen(current_suggestion)) = 0;
	}


//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
	text_box->highlight_letter1 = 
		text_box->highlight_letter2 = 
		text_box->ibeam_letter = text_box->text.length();
	text_box->draw(1);
	text_box->handle_event();
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
	return 1;
}








BC_ScrollTextBox::BC_ScrollTextBox(BC_WindowBase *parent_window, 
	int x, 
	int y, 
	int w,
	int rows,
	const char *default_text)
{
	this->parent_window = parent_window;
	this->x = x;
	this->y = y;
	this->w = w;
	this->rows = rows;
	this->default_text = default_text;
    undo_enabled = 0;
}

BC_ScrollTextBox::~BC_ScrollTextBox()
{
	delete yscroll;
	if(text)
	{
		text->gui = 0;
		delete text;
	}
}

void BC_ScrollTextBox::enable_undo()
{
    undo_enabled = 1;
}

void BC_ScrollTextBox::create_objects()
{
// Must be created first
	parent_window->add_subwindow(text = new BC_ScrollTextBoxText(this));
	parent_window->add_subwindow(yscroll = new BC_ScrollTextBoxYScroll(this));
	text->yscroll = yscroll;
	yscroll->bound_to = text;
}

int BC_ScrollTextBox::handle_event()
{
	return 1;
}

int BC_ScrollTextBox::get_x()
{
	return x;
}

int BC_ScrollTextBox::get_y()
{
	return y;
}

int BC_ScrollTextBox::get_w()
{
	return w;
}

int BC_ScrollTextBox::get_rows()
{
	return rows;
}


const char* BC_ScrollTextBox::get_text()
{
	return text->get_text();
}

void BC_ScrollTextBox::update(const char *text)
{
	this->text->update(text);
	yscroll->update_length(this->text->get_text_rows(),
		this->text->get_text_row(),
		yscroll->get_handlelength(),
		1);
}

void BC_ScrollTextBox::reposition_window(int x, int y, int w, int rows)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->rows = rows;

	text->reposition_window(x, 
		y, 
		w - yscroll->get_span(), 
		rows);
	yscroll->reposition_window(x + w - yscroll->get_span(), 
		y, 
		BC_TextBox::calculate_row_h(rows, 
			parent_window));
	yscroll->update_length(text->get_text_rows(),
		text->get_text_row(),
		rows,
		0);
}








BC_ScrollTextBoxText::BC_ScrollTextBoxText(BC_ScrollTextBox *gui)
 : BC_TextBox(gui->x, 
 	gui->y, 
	gui->w - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(), 
	gui->rows,
	gui->default_text)
{
	this->gui = gui;
//printf("BC_ScrollTextBoxText::BC_ScrollTextBoxText %d\n", __LINE__);
    if(gui->undo_enabled)
    {
        enable_undo();
    }
}

BC_ScrollTextBoxText::~BC_ScrollTextBoxText()
{
	if(gui)
	{
		gui->text = 0;
		delete gui;
	}
}



int BC_ScrollTextBoxText::handle_event()
{
	gui->yscroll->update_length(get_text_rows(),
		get_text_row(),
		gui->yscroll->get_handlelength(),
		1);
	return gui->handle_event();
}

int BC_ScrollTextBoxText::motion_event()
{
	gui->yscroll->update_length(get_text_rows(),
		get_text_row(),
		gui->yscroll->get_handlelength(),
		1);
	return 1;
}


BC_ScrollTextBoxYScroll::BC_ScrollTextBoxYScroll(BC_ScrollTextBox *gui)
 : BC_ScrollBar(gui->x + 
 			gui->w - 
			get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(), 
		gui->y, 
		SCROLL_VERT, 
		BC_TextBox::calculate_row_h(gui->rows, 
			gui->parent_window), 
		gui->text->get_text_rows(), 
		0, 
		gui->rows)
{
	this->gui = gui;
}

BC_ScrollTextBoxYScroll::~BC_ScrollTextBoxYScroll()
{
}

int BC_ScrollTextBoxYScroll::handle_event()
{
	gui->text->set_text_row(get_position());
	return 1;
}










BC_PopupTextBoxText::BC_PopupTextBoxText(BC_PopupTextBox *popup, int x, int y)
 : BC_TextBox(x, y, popup->text_w, 1, popup->default_text)
{
	this->popup = popup;
}

BC_PopupTextBoxText::~BC_PopupTextBoxText()
{
	if(popup)
	{
		popup->textbox = 0;
		delete popup;
		popup = 0;
	}
}


int BC_PopupTextBoxText::handle_event()
{
	popup->handle_event();
	return 1;
}

BC_PopupTextBoxList::BC_PopupTextBoxList(BC_PopupTextBox *popup, int x, int y)
 : BC_ListBox(x,
 	y,
	popup->list_w,
	popup->list_h,
	popup->list_format,
	popup->list_items,
	0,
	0,
	1,
	0,
	1)
{
	this->popup = popup;
}
int BC_PopupTextBoxList::handle_event()
{
	BC_ListBoxItem *item = get_selection(0, 0);
	if(item)
	{
		popup->textbox->update(item->get_text());
		popup->handle_event();
	}
	return 1;
}




BC_PopupTextBox::BC_PopupTextBox(BC_WindowBase *parent_window, 
		ArrayList<BC_ListBoxItem*> *list_items,
		const char *default_text,
		int x, 
		int y, 
		int text_w,
		int list_h,
		int list_format)
{
	this->x = x;
	this->y = y;
	this->list_h = list_h;
	this->list_format = list_format;
	this->default_text = (char*)default_text;
	this->text_w = text_w;
    this->list_w = text_w + BC_WindowBase::get_resources()->listbox_button[0]->get_w();
	this->parent_window = parent_window;
	this->list_items = list_items;
}

BC_PopupTextBox::~BC_PopupTextBox()
{
	delete listbox;
	if(textbox) 
	{
		textbox->popup = 0;
		delete textbox;
	}
}

void BC_PopupTextBox::set_list_w(int value)
{
    this->list_w = value;
}

int BC_PopupTextBox::create_objects()
{
	int x = this->x, y = this->y;
	parent_window->add_subwindow(textbox = new BC_PopupTextBoxText(this, x, y));
	x += textbox->get_w();
	parent_window->add_subwindow(listbox = new BC_PopupTextBoxList(this, x, y));
	return 0;
}

void BC_PopupTextBox::update(const char *text)
{
	textbox->update(text);
}

void BC_PopupTextBox::update_list(ArrayList<BC_ListBoxItem*> *data)
{
	listbox->update(data, 
		0, 
		0,
		1);
}


const char* BC_PopupTextBox::get_text()
{
	return textbox->get_text();
}

int BC_PopupTextBox::get_number()
{
	return listbox->get_selection_number(0, 0);
}

int BC_PopupTextBox::get_x()
{
	return x;
}

int BC_PopupTextBox::get_y()
{
	return y;
}

int BC_PopupTextBox::get_w()
{
	return textbox->get_w() + listbox->get_w();
}

int BC_PopupTextBox::get_h()
{
	return textbox->get_h();
}

int BC_PopupTextBox::handle_event()
{
	return 1;
}

void BC_PopupTextBox::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	int x1 = x, y1 = y;
	textbox->reposition_window(x1, 
		y1, 
		textbox->get_w(), 
		textbox->get_rows());
	x1 += textbox->get_w();
	listbox->reposition_window(x1, 
		y1, 
		listbox->get_w(), 
		listbox->get_h(), 
		0);
//	if(flush) parent_window->flush();
}














BC_TumbleTextBoxText::BC_TumbleTextBoxText(BC_TumbleTextBox *popup, 
	int64_t default_value,
	int64_t min,
	int64_t max,
	int x, 
	int y)
 : BC_TextBox(x, 
 	y, 
	popup->text_w, 
	1, 
	default_value)
{
	this->popup = popup;
}

BC_TumbleTextBoxText::BC_TumbleTextBoxText(BC_TumbleTextBox *popup, 
	float default_value,
	float min,
	float max,
	int x, 
	int y)
 : BC_TextBox(x, 
 	y, 
	popup->text_w, 
	1, 
	default_value,
    1,
    MEDIUMFONT,
    popup->precision)
{
	this->popup = popup;
}

BC_TumbleTextBoxText::~BC_TumbleTextBoxText()
{
	if(popup)
	{
		popup->textbox = 0;
		delete popup;
		popup = 0;
	}
}



int BC_TumbleTextBoxText::handle_event()
{
	popup->handle_event();
	return 1;
}

int BC_TumbleTextBoxText::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();

		if(get_buttonpress() == 4)
		{
			popup->tumbler->handle_up_event();
		}
		else
		if(get_buttonpress() == 5)
		{
			popup->tumbler->handle_down_event();
		}
		return 1;
	}
	return 0;
}




BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window, 
		int64_t default_value,
		int64_t min,
		int64_t max,
		int x, 
		int y, 
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min = min;
	this->max = max;
	this->default_value = default_value;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 0;
	precision = 4;
	increment = 1;
}

BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window, 
		int default_value,
		int min,
		int max,
		int x, 
		int y, 
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min = min;
	this->max = max;
	this->default_value = default_value;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 0;
	precision = 4;
	increment = 1;
}

BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window, 
		float default_value_f,
		float min_f,
		float max_f,
		int x, 
		int y, 
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min_f = min_f;
	this->max_f = max_f;
	this->default_value_f = default_value_f;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 1;
	precision = 4;
	increment = 1;
}

BC_TumbleTextBox::~BC_TumbleTextBox()
{
// Recursive delete.  Normally ~BC_TumbleTextBox is never called but textbox
// is deleted anyway by the windowbase so textbox deletes this.
	if(tumbler) delete tumbler;
	tumbler = 0;
// Don't delete text here if we were called by ~BC_TumbleTextBoxText
	if(textbox)
	{
		textbox->popup = 0;
		delete textbox;
	}
	textbox = 0;
}

void BC_TumbleTextBox::reset()
{
	textbox = 0;
	tumbler = 0;
	increment = 1.0;
}

void BC_TumbleTextBox::set_precision(int precision)
{
	this->precision = precision;
}

void BC_TumbleTextBox::set_increment(float value)
{
	this->increment = value;
	if(tumbler) tumbler->set_increment(value);
}

int BC_TumbleTextBox::create_objects()
{
	int x = this->x, y = this->y;

	if(use_float)
	{
		parent_window->add_subwindow(textbox = new BC_TumbleTextBoxText(this, 
			default_value_f,
			min_f, 
			max_f, 
			x, 
			y));
		textbox->set_precision(precision);
	}
	else
    {
		parent_window->add_subwindow(textbox = new BC_TumbleTextBoxText(this, 
			default_value,
			min, 
			max, 
			x, 
			y));
    }

	x += textbox->get_w();

	if(use_float)
		parent_window->add_subwindow(tumbler = new BC_FTumbler(textbox, 
 			min_f,
			max_f,
			x, 
			y));
	else
		parent_window->add_subwindow(tumbler = new BC_ITumbler(textbox, 
 			min, 
			max, 
			x, 
			y));

	tumbler->set_increment(increment);
	return 0;
}

const char* BC_TumbleTextBox::get_text()
{
	return textbox->get_text();
}

int BC_TumbleTextBox::update(const char *value)
{
	textbox->update(value);
	return 0;
}

int BC_TumbleTextBox::update(int64_t value)
{
	textbox->update(value);
	return 0;
}

int BC_TumbleTextBox::update(float value)
{
	textbox->update(value);
	return 0;
}

void BC_TumbleTextBox::disable()
{
    if(textbox)
    {
        textbox->disable();
    }
    
    if(tumbler)
    {
        tumbler->disable();
    }
}

void BC_TumbleTextBox::enable()
{
    if(textbox)
    {
        textbox->enable();
    }
    
    if(tumbler)
    {
        tumbler->enable();
    }
}

int BC_TumbleTextBox::get_enabled()
{
    if(textbox)
    {
        return textbox->get_enabled();
    }
    
    return 0;
}

int BC_TumbleTextBox::get_x()
{
	return x;
}

int BC_TumbleTextBox::get_y()
{
	return y;
}

int BC_TumbleTextBox::get_w()
{
	return textbox->get_w() + tumbler->get_w();
}

int BC_TumbleTextBox::get_h()
{
	return textbox->get_h();
}

int BC_TumbleTextBox::handle_event()
{
	return 1;
}

void BC_TumbleTextBox::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	
	textbox->reposition_window(x, 
 		y, 
		text_w, 
		1);
	tumbler->reposition_window(x + textbox->get_w(),
		y);
//	if(flush) parent_window->flush();
}


void BC_TumbleTextBox::set_boundaries(int64_t min, int64_t max)
{
	tumbler->set_boundaries(min, max);
}

void BC_TumbleTextBox::set_boundaries(float min, float max)
{
	tumbler->set_boundaries(min, max);
}







BC_TextMenu::BC_TextMenu(BC_TextBox *textbox)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->textbox = textbox;
}

BC_TextMenu::~BC_TextMenu()
{
}

void BC_TextMenu::create_objects()
{
    if(textbox->undo_enabled)
    {
        add_item(undo = new BC_TextMenuUndo(this));
        add_item(redo = new BC_TextMenuRedo(this));
    }
	add_item(cut = new BC_TextMenuCut(this));
	add_item(new BC_TextMenuCopy(this));
	add_item(paste = new BC_TextMenuPaste(this));
	add_item(new BC_TextMenuSelect(this));
}


BC_TextMenuUndo::BC_TextMenuUndo(BC_TextMenu *menu) 
 : BC_MenuItem(_("Undo"), "Ctrl+Z")
{
	this->menu = menu;
}

int BC_TextMenuUndo::handle_event()
{
	menu->textbox->undo();
    menu->textbox->activate();
	return 0;
}

BC_TextMenuRedo::BC_TextMenuRedo(BC_TextMenu *menu) 
 : BC_MenuItem(_("Redo"), "Ctrl+Shift+Z")
{
	this->menu = menu;
}

int BC_TextMenuRedo::handle_event()
{
	menu->textbox->redo();
    menu->textbox->activate();
	return 0;
}


BC_TextMenuSelect::BC_TextMenuSelect(BC_TextMenu *menu) 
 : BC_MenuItem(_("Select All"), "Ctrl+A")
{
	this->menu = menu;
}

int BC_TextMenuSelect::handle_event()
{
	menu->textbox->select_all();
    menu->textbox->activate();
	return 0;
}


BC_TextMenuCut::BC_TextMenuCut(BC_TextMenu *menu) 
 : BC_MenuItem(_("Cut"), "Ctrl+X")
{
	this->menu = menu;
}

int BC_TextMenuCut::handle_event()
{
	menu->textbox->cut(1);
    menu->textbox->activate();
	return 0;
}


BC_TextMenuCopy::BC_TextMenuCopy(BC_TextMenu *menu) 
 : BC_MenuItem(_("Copy"), "Ctrl+C")
{
	this->menu = menu;
}

int BC_TextMenuCopy::handle_event()
{
	menu->textbox->copy(1);
    menu->textbox->activate();
	return 0;
}



BC_TextMenuPaste::BC_TextMenuPaste(BC_TextMenu *menu) 
 : BC_MenuItem(_("Paste"), "Ctrl+V")
{
	this->menu = menu;
}

int BC_TextMenuPaste::handle_event()
{
	menu->textbox->paste(1);
    menu->textbox->activate();
	return 0;
}




BC_TextBoxUndo::BC_TextBoxUndo()
 : ListItem<BC_TextBoxUndo>()
{
    highlight_letter1 = 0;
    highlight_letter2 = 0;
    ibeam_letter = 0;
    text_x = 0;
    text_y = 0;
}

BC_TextBoxUndo::~BC_TextBoxUndo()
{
}

    
void BC_TextBoxUndo::to_textbox(BC_TextBox *textbox)
{
    textbox->highlight_letter1 = highlight_letter1;
    textbox->highlight_letter2 = highlight_letter2;
    textbox->ibeam_letter = ibeam_letter;
    textbox->text_x = text_x;
    textbox->text_y = text_y;
    textbox->text = text;
}

void BC_TextBoxUndo::from_textbox(BC_TextBox *textbox)
{
    highlight_letter1 = textbox->highlight_letter1;
    highlight_letter2 = textbox->highlight_letter2;
    ibeam_letter = textbox->ibeam_letter;
    text_x = textbox->text_x;
    text_y = textbox->text_y;
    text = textbox->text;
// printf("BC_TextBoxUndo::from_textbox %d text_x=%d text_y=%d\n",
// __LINE__,
// text_x,
// text_y);
}


BC_TextBoxUndos::BC_TextBoxUndos()
 : List<BC_TextBoxUndo>()
{
    current = 0;
}

BC_TextBoxUndos::~BC_TextBoxUndos()
{
}

BC_TextBoxUndo* BC_TextBoxUndos::push()
{
// current is only 0 if before first undo
	if(current)
	{
    	current = insert_after(current);
	}
    else
	{
    	current = insert_before(first);
    }

// delete future undos if necessary
	if(current && current->next)
	{
		while(current->next) remove(last);
	}


// delete oldest if necessary
	if(total() > UNDOLEVELS)
	{
		remove(first);
	}
	
	return current;
}


void BC_TextBoxUndos::pull()
{
    if(current)
    {
        current = PREVIOUS;
    }
}

BC_TextBoxUndo* BC_TextBoxUndos::pull_next()
{
// use first entry if none
	if(!current)
	{
    	current = first;
	}
    else
// use next entry if there is a next entry
	if(current->next)
	{
    	current = NEXT;
    }
// don't change current if there is no next entry
	else
	{
    	return 0;
    }
		
	return current;
}

void BC_TextBoxUndos::dump()
{
}





