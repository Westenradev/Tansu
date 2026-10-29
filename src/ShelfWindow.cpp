#include "ShelfWindow.h"

#include <LayoutBuilder.h>
#include <Screen.h>

#include "ShelfView.h"


ShelfWindow::ShelfWindow()
	:
	BWindow(BRect(0, 0, 159, 399), "Tansu",
		B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_AVOID_FOCUS | B_QUIT_ON_WINDOW_CLOSE),
	fShelfView(NULL)
{
	BScreen screen;
	BRect screenFrame = screen.Frame();
	MoveTo(screenFrame.right - Bounds().Width() - 8,
		(screenFrame.Height() - Bounds().Height()) / 2);

	fShelfView = new ShelfView("shelfView");

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fShelfView)
		.SetInsets(0, 0, 0, 0);

	SetSizeLimits(160, 160, 100, screenFrame.Height() - 40);
}


ShelfWindow::~ShelfWindow()
{
}


bool
ShelfWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
