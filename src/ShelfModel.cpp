#include "ShelfModel.h"


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
	BEntry entry(&ref);
	BString name;
	if (entry.InitCheck() == B_OK && entry.GetName(name) == B_OK)
		fName = name;
}


void
ShelfItem::SetText(const BString& text)
{
	fText = text;
	fName = text;
	fName.Truncate(40);
}


void
ShelfItem::SetThumbnail(BBitmap* bitmap)
{
	delete fThumbnail;
	fThumbnail = bitmap;
}


ShelfModel::ShelfModel()
	:
	fLocker("shelf model")
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
	BAutolock lock(fLocker);
	return fItems.AddItem(item);
}


int32
ShelfModel::CountItems() const
{
	BAutolock lock(fLocker);
	return fItems.CountItems();
}


ShelfItem*
ShelfModel::ItemAt(int32 index) const
{
	BAutolock lock(fLocker);
	return (ShelfItem*)fItems.ItemAt(index);
}


bool
ShelfModel::RemoveItem(int32 index)
{
	BAutolock lock(fLocker);
	ShelfItem* item = (ShelfItem*)fItems.RemoveItem(index);
	if (item == NULL)
		return false;
	delete item;
	return true;
}


void
ShelfModel::MakeEmpty()
{
	BAutolock lock(fLocker);
	for (int32 i = fItems.CountItems() - 1; i >= 0; i--)
		delete (ShelfItem*)fItems.ItemAt(i);
	fItems.MakeEmpty();
}
