#include "ShelfView.h"

#include <ControlLook.h>


static const char* kEmptyText = "Drop files or text here";


ShelfView::ShelfView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fModel(new ShelfModel()),
	fDropHighlight(false)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
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
	BRect bounds = Bounds();

	if (fDropHighlight) {
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_LIGHTEN_2_TINT));
		FillRect(updateRect);
		SetHighUIColor(B_CONTROL_HIGHLIGHT_COLOR);
		StrokeRect(bounds);
	}

	if (fModel->CountItems() > 0)
		return;

	// The hint is drawn here rather than in a child view, so that the
	// shelf itself keeps receiving mouse and drag events over its whole area.
	BFont font;
	GetFont(&font);
	float inset = be_control_look->DefaultLabelSpacing();
	BString text(kEmptyText);
	font.TruncateString(&text, B_TRUNCATE_END, bounds.Width() - 2 * inset);

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float x = bounds.left
		+ (bounds.Width() - font.StringWidth(text.String())) / 2;
	float y = bounds.top + (bounds.Height() - fontHeight.ascent
		- fontHeight.descent) / 2 + fontHeight.ascent;

	SetHighUIColor(B_PANEL_TEXT_COLOR);
	SetLowColor(fDropHighlight
		? tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT)
		: ui_color(B_PANEL_BACKGROUND_COLOR));
	DrawString(text.String(), BPoint(x, y));
}


void
ShelfView::MessageReceived(BMessage* message)
{
	if (message->WasDropped())
		_SetDropHighlight(false);

	switch (message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
ShelfView::MouseMoved(BPoint point, uint32 transit, const BMessage* dragMessage)
{
	_SetDropHighlight(dragMessage != NULL && transit != B_EXITED_VIEW
		&& transit != B_OUTSIDE_VIEW);
	BView::MouseMoved(point, transit, dragMessage);
}


void
ShelfView::_RebuildRows()
{
	Invalidate();
}


void
ShelfView::_SetDropHighlight(bool highlight)
{
	if (highlight == fDropHighlight)
		return;
	fDropHighlight = highlight;
	Invalidate();
}
