#ifndef SHELF_WINDOW_H
#define SHELF_WINDOW_H

#include <Window.h>

class ShelfView;

class ShelfWindow : public BWindow
{
public:
							ShelfWindow();
	virtual					~ShelfWindow();

	virtual bool			QuitRequested();

private:
	ShelfView*				fShelfView;
};

#endif // SHELF_WINDOW_H
