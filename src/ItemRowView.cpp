#include "ItemRowView.h"

#include <LayoutBuilder.h>
#include <StringView.h>

#include "ShelfModel.h"


ItemRowView::ItemRowView(ShelfItem* item)
	:
	BView("itemRow", B_WILL_DRAW),
	fItem(item)
{
	BStringView* label = new BStringView("label",
		item != NULL ? item->Name().String() : "");

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 4)
		.SetInsets(4, 2, 4, 2)
		.Add(label, 1.0f);
}


ItemRowView::~ItemRowView()
{
}
