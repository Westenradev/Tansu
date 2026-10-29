#ifndef ITEM_ROW_VIEW_H
#define ITEM_ROW_VIEW_H

#include <View.h>

class ShelfItem;

class ItemRowView : public BView
{
public:
							ItemRowView(ShelfItem* item);
	virtual					~ItemRowView();

	ShelfItem*				Item() const	{ return fItem; }

private:
	ShelfItem*				fItem;
};

#endif // ITEM_ROW_VIEW_H
