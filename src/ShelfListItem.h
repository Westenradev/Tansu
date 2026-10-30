#ifndef SHELF_LIST_ITEM_H
#define SHELF_LIST_ITEM_H

#include <ListItem.h>

class ShelfItem;


// One row of the shelf: icon (or image thumbnail) and a one-line name.
class ShelfListItem : public BListItem
{
public:
								ShelfListItem(ShelfItem* item,
									int32 iconSize);
	virtual						~ShelfListItem();

			ShelfItem*			Item() const	{ return fItem; }
			BRect				IconFrame(BRect itemFrame) const;

	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete = false);
	virtual	void				Update(BView* owner, const BFont* font);

private:
			ShelfItem*			fItem;
			int32				fIconSize;
};

#endif // SHELF_LIST_ITEM_H
