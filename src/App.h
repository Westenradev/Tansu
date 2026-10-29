#ifndef APP_H
#define APP_H

#include <Application.h>

class ShelfWindow;

class ShelfApp : public BApplication
{
public:
							ShelfApp();
	virtual				~ShelfApp();

	virtual void		ReadyToRun();
	virtual void		MessageReceived(BMessage* message);

private:
	ShelfWindow*		fShelfWindow;
};

#endif // APP_H
