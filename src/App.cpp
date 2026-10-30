#include "App.h"

#include <Alert.h>
#include <Entry.h>
#include <Path.h>

#include "Constants.h"
#include "ShelfWindow.h"


ShelfApp::ShelfApp()
	:
	BApplication(kAppSignature),
	fShelfWindow(NULL)
{
	// Created here rather than in ReadyToRun(): files passed at launch
	// (RefsReceived(), ArgvReceived(), the Tracker add-on) arrive before
	// ReadyToRun() and are queued in the window until it is shown.
	fShelfWindow = new ShelfWindow();
}


ShelfApp::~ShelfApp()
{
}


void
ShelfApp::ReadyToRun()
{
	fShelfWindow->Show();
}


void
ShelfApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddRefs:
			fShelfWindow->PostMessage(message);
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
ShelfApp::RefsReceived(BMessage* message)
{
	fShelfWindow->PostMessage(message);
}


void
ShelfApp::ArgvReceived(int32 argc, char** argv)
{
	const char* cwd = NULL;
	if (CurrentMessage() != NULL)
		CurrentMessage()->FindString("cwd", &cwd);

	BMessage refs(kMsgAddRefs);
	for (int32 i = 1; i < argc; i++) {
		BPath path;
		if (argv[i][0] == '/' || cwd == NULL)
			path.SetTo(argv[i]);
		else
			path.SetTo(cwd, argv[i]);

		entry_ref ref;
		if (get_ref_for_path(path.Path(), &ref) == B_OK
			&& BEntry(&ref).Exists()) {
			refs.AddRef("refs", &ref);
		}
	}

	if (refs.HasRef("refs"))
		fShelfWindow->PostMessage(&refs);
}


void
ShelfApp::AboutRequested()
{
	BAlert* alert = new BAlert("About Tansu",
		"Tansu 0.2.0\n\n"
		"A shelf for parking files, text and images during drag and drop.\n\n"
		"Copyright (c) 2026 Westenra Softworks\n"
		"MIT License", "OK");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go(NULL);
}
