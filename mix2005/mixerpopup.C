#include "mixer.h"
#include "mixergui.h"
#include "mixerpopup.h"


MixerMenu::MixerMenu(Mixer *mixer)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mixer = mixer;
}

MixerMenu::~MixerMenu()
{
}

void MixerMenu::create_objects()
{
	add_item(new MixerMenuConfigure(mixer));
	add_item(lock_channels = new MixerMenuLockChannels(mixer));
	add_item(lock_elements = new MixerMenuLockElements(mixer));
	add_item(new MixerMenuRead(mixer));
	add_item(new MixerMenuSave(mixer));
//	add_item(new MixerMenuZero(mixer));
}





MixerMenuConfigure::MixerMenuConfigure(Mixer *mixer)
 : BC_MenuItem("Configure...")
{
	this->mixer = mixer;
}
int MixerMenuConfigure::handle_event()
{
	mixer->configure();
	return 1;
}





MixerMenuRead::MixerMenuRead(Mixer *mixer)
 : BC_MenuItem("Read hardware")
{
	this->mixer = mixer;
}
int MixerMenuRead::handle_event()
{
	mixer->read_hardware();
	return 1;
}





MixerMenuSave::MixerMenuSave(Mixer *mixer)
 : BC_MenuItem("Save settings")
{
	this->mixer = mixer;
}
int MixerMenuSave::handle_event()
{
	mixer->save_defaults();
	return 1;
}





MixerMenuZero::MixerMenuZero(Mixer *mixer)
 : BC_MenuItem("Zero settings")
{
	this->mixer = mixer;
}
int MixerMenuZero::handle_event()
{
	mixer->zero();
	return 1;
}






MixerMenuLockChannels::MixerMenuLockChannels(Mixer *mixer)
 : BC_MenuItem("Lock Channels")
{
	this->mixer = mixer;
	set_checked(mixer->lock_channels);
}
int MixerMenuLockChannels::handle_event()
{
	set_checked(!get_checked());
	mixer->lock_channels = get_checked();
	if(get_checked())
		mixer->lock_elements = 0;
	mixer->gui->menu->lock_elements->set_checked(mixer->lock_elements);
	mixer->save_defaults();
	return 1;
}







MixerMenuLockElements::MixerMenuLockElements(Mixer *mixer)
 : BC_MenuItem("Lock Elements")
{
	this->mixer = mixer;
	set_checked(mixer->lock_elements);
}

int MixerMenuLockElements::handle_event()
{
	set_checked(!get_checked());
	mixer->lock_elements = get_checked();
	if(get_checked())
		mixer->lock_channels = 0;
	mixer->gui->menu->lock_channels->set_checked(mixer->lock_channels);
	mixer->save_defaults();
	return 1;
}





