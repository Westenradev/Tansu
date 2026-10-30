#include "ShelfListItem.h"

#include <algorithm>

#include <Bitmap.h>
#include <ControlLook.h>
#include <View.h>

#include "ShelfItem.h"


ShelfListItem::ShelfListItem(ShelfItem* item, int32 iconSize)
	:
	fItem(item),
	fIconSize(iconSize)
{
}


ShelfListItem::~ShelfListItem()
{
	delete fItem;
}


BRect
ShelfListItem::IconFrame(BRect itemFrame) const
{
	float inset = be_control_look->DefaultLabelSpacing();
	float top = itemFrame.top + floorf((itemFrame.Height() - fIconSize) / 2);
	return BRect(itemFrame.left + inset, top,
		itemFrame.left + inset + fIconSize - 1, top + fIconSize - 1);
}


void
ShelfListItem::DrawItem(BView* owner, BRect frame, bool /*complete*/)
{
	rgb_color background = ui_color(IsSelected()
		? B_LIST_SELECTED_BACKGROUND_COLOR : B_LIST_BACKGROUND_COLOR);
	owner->SetHighColor(background);
	owner->SetLowColor(background);
	owner->FillRect(frame);

	BRect iconFrame = IconFrame(frame);
	owner->SetDrawingMode(B_OP_ALPHA);
	owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	if (fItem->Kind() == kItemImage) {
		// thumbnail, scaled to fit the icon box and centred in it
		BBitmap* image = fItem->Image();
		BRect bounds = image->Bounds();
		float scale = std::min(iconFrame.Width() / bounds.Width(),
			iconFrame.Height() / bounds.Height());
		if (scale > 1)
			scale = 1;
		float width = bounds.Width() * scale;
		float height = bounds.Height() * scale;
		BRect target(0, 0, width, height);
		target.OffsetTo(
			iconFrame.left + floorf((iconFrame.Width() - width) / 2),
			iconFrame.top + floorf((iconFrame.Height() - height) / 2));
		owner->DrawBitmap(image, bounds, target, B_FILTER_BITMAP_BILINEAR);
		owner->SetDrawingMode(B_OP_COPY);
		owner->SetHighColor(tint_color(background, B_DARKEN_2_TINT));
		owner->StrokeRect(target);
	} else {
		const BBitmap* icon = fItem->Icon(fIconSize);
		if (icon != NULL)
			owner->DrawBitmap(icon, iconFrame.LeftTop());
	}
	owner->SetDrawingMode(B_OP_COPY);

	float inset = be_control_look->DefaultLabelSpacing();
	float x = iconFrame.right + inset * 2;

	BFont font;
	owner->GetFont(&font);
	BString name(fItem->Name());
	font.TruncateString(&name, fItem->Kind() == kItemFile
		? B_TRUNCATE_MIDDLE : B_TRUNCATE_END, frame.right - x - inset);

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float y = frame.top + floorf((frame.Height() - fontHeight.ascent
		- fontHeight.descent) / 2 + fontHeight.ascent);

	owner->SetHighColor(ui_color(IsSelected()
		? B_LIST_SELECTED_ITEM_TEXT_COLOR : B_LIST_ITEM_TEXT_COLOR));
	owner->DrawString(name.String(), BPoint(x, y));
}


void
ShelfListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner, font);

	font_height fontHeight;
	font->GetHeight(&fontHeight);
	float textHeight = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);
	float inset = be_control_look->DefaultLabelSpacing();
	SetHeight(std::max(textHeight, (float)fIconSize) + inset * 2);
}
