#include "App.h"

#include "ShelfWindow.h"

static const char* kAppSignature = "application/x-vnd.WestenraSoftworks-Tansu";


ShelfApp::ShelfApp()
	:
	BApplication(kAppSignature),
	fShelfWindow(NULL)
{
}


ShelfApp::~ShelfApp()
{
}


void
ShelfApp::ReadyToRun()
{
	fShelfWindow = new ShelfWindow();
	fShelfWindow->Show();
}


void
ShelfApp::MessageReceived(BMessage* message)
{
	BApplication::MessageReceived(message);
}
