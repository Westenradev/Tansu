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
	virtual void		RefsReceived(BMessage* message);
	virtual void		ArgvReceived(int32 argc, char** argv);
	virtual void		AboutRequested();

private:
	ShelfWindow*		fShelfWindow;
};

#endif // APP_H
