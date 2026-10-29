#include "ShelfView.h"

#include <LayoutBuilder.h>
#include <StringView.h>

#include "ItemRowView.h"


ShelfView::ShelfView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fModel(new ShelfModel()),
	fEmptyView(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fEmptyView = new BStringView("empty", "Drop files or text here");
	fEmptyView->SetAlignment(B_ALIGN_CENTER);
	fEmptyView->SetFont(be_plain_font);
	fEmptyView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(4)
		.Add(fEmptyView);
}


ShelfView::~ShelfView()
{
	delete fModel;
}


void
ShelfView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_RebuildRows();
}


void
ShelfView::Draw(BRect updateRect)
{
	SetHighColor(ViewColor());
	FillRect(updateRect);
}


void
ShelfView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
ShelfView::MouseMoved(BPoint point, uint32 transit, const BMessage* dragMessage)
{
	if (dragMessage != NULL)
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_LIGHTEN_2_TINT));
	else
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	Invalidate();
	BView::MouseMoved(point, transit, dragMessage);
}


void
ShelfView::_RebuildRows()
{
	fEmptyView->SetHidden(fModel->CountItems() > 0);
}
