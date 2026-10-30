#ifndef SHELF_WINDOW_H
#define SHELF_WINDOW_H

#include <Window.h>

#include "Settings.h"

class ShelfView;

class ShelfWindow : public BWindow
{
public:
							ShelfWindow();
	virtual					~ShelfWindow();

	virtual void			MessageReceived(BMessage* message);
	virtual bool			QuitRequested();

private:
	void					_PlaceOnScreen();

	ShelfView*				fShelfView;
	Settings				fSettings;
};

#endif // SHELF_WINDOW_H
