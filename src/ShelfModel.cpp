#include "ShelfModel.h"


static const int32 kMaxNameChars = 40;


ShelfItem::ShelfItem(ShelfItemKind kind)
	:
	fKind(kind),
	fThumbnail(NULL)
{
}


ShelfItem::~ShelfItem()
{
	delete fThumbnail;
}


void
ShelfItem::SetRef(const entry_ref& ref)
{
	fRef = ref;
	if (fRef.name != NULL)
		fName = fRef.name;
}


void
ShelfItem::SetText(const BString& text)
{
	fText = text;

	// first line only, cut on character (not byte) boundaries for UTF-8
	fName = text;
	int32 lineEnd = fName.FindFirst('\n');
	if (lineEnd >= 0)
		fName.Truncate(lineEnd);
	if (fName.CountChars() > kMaxNameChars)
		fName.TruncateChars(kMaxNameChars);
}


void
ShelfItem::SetThumbnail(BBitmap* bitmap)
{
	delete fThumbnail;
	fThumbnail = bitmap;
}


ShelfModel::ShelfModel()
{
}


ShelfModel::~ShelfModel()
{
	MakeEmpty();
}


bool
ShelfModel::AddItem(ShelfItem* item)
{
	if (item == NULL)
		return false;
	return fItems.AddItem(item);
}


int32
ShelfModel::CountItems() const
{
	return fItems.CountItems();
}


ShelfItem*
ShelfModel::ItemAt(int32 index) const
{
	return (ShelfItem*)fItems.ItemAt(index);
}


bool
ShelfModel::RemoveItem(int32 index)
{
	ShelfItem* item = (ShelfItem*)fItems.RemoveItem(index);
	if (item == NULL)
		return false;
	delete item;
	return true;
}


void
ShelfModel::MakeEmpty()
{
	for (int32 i = fItems.CountItems() - 1; i >= 0; i--)
		delete (ShelfItem*)fItems.ItemAt(i);
	fItems.MakeEmpty();
}
