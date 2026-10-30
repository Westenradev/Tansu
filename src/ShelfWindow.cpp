#include "ShelfWindow.h"

#include <LayoutBuilder.h>
#include <Screen.h>
#include <ScrollView.h>

#include "Constants.h"
#include "ShelfView.h"


static const float kShelfWidth = 160;
static const float kDefaultHeight = 400;
static const float kMinHeight = 100;
static const float kEdgeGap = 8;


ShelfWindow::ShelfWindow()
	:
	BWindow(BRect(0, 0, kShelfWidth - 1, kDefaultHeight - 1), "Tansu",
		B_FLOATING_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_H_RESIZABLE | B_AVOID_FOCUS
			| B_QUIT_ON_WINDOW_CLOSE),
	fShelfView(NULL)
{
	fSettings.Load();

	fShelfView = new ShelfView("shelfView", fSettings.IconSize());
	BScrollView* scrollView = new BScrollView("scrollView", fShelfView, 0,
		false, true, B_NO_BORDER);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(scrollView)
		.SetInsets(0, 0, 0, 0);

	_PlaceOnScreen();
}


ShelfWindow::~ShelfWindow()
{
}


void
ShelfWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddRefs:
		case B_REFS_RECEIVED:
			fShelfView->AddRefs(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ShelfWindow::QuitRequested()
{
	// B_QUIT_ON_WINDOW_CLOSE quits the application for us.
	fSettings.SetWindowFrame(Frame());
	fSettings.Save();
	return true;
}


void
ShelfWindow::_PlaceOnScreen()
{
	BRect screenFrame = BScreen(this).Frame();

	SetSizeLimits(kShelfWidth - 1, kShelfWidth - 1, kMinHeight,
		screenFrame.Height() - 2 * kEdgeGap);

	BRect frame = fSettings.WindowFrame();
	if (frame.IsValid() && screenFrame.Contains(frame.LeftTop())) {
		ResizeTo(kShelfWidth - 1, frame.Height());
		MoveTo(frame.LeftTop());
		return;
	}

	float left = fSettings.EdgeRight()
		? screenFrame.right - Bounds().Width() - kEdgeGap
		: screenFrame.left + kEdgeGap;
	MoveTo(left, screenFrame.top
		+ (screenFrame.Height() - Bounds().Height()) / 2);
}
