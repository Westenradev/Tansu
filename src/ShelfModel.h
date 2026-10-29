#ifndef SHELF_MODEL_H
#define SHELF_MODEL_H

#include <Entry.h>
#include <List.h>
#include <String.h>
#include <Bitmap.h>


enum ShelfItemKind {
	kItemFile = 0,
	kItemText,
	kItemImage
};


class ShelfItem
{
public:
							ShelfItem(ShelfItemKind kind);
							~ShelfItem();

	ShelfItemKind			Kind() const				{ return fKind; }
	const entry_ref*		Ref() const					{ return &fRef; }
	const BString&			Text() const				{ return fText; }
	const BString&			Name() const				{ return fName; }
	BBitmap*				Thumbnail() const			{ return fThumbnail; }

	void					SetRef(const entry_ref& ref);
	void					SetText(const BString& text);
	void					SetThumbnail(BBitmap* bitmap);

private:
	// owns fThumbnail, so copying would double-delete it
							ShelfItem(const ShelfItem&);
			ShelfItem&		operator=(const ShelfItem&);

	ShelfItemKind			fKind;
	entry_ref				fRef;
	BString					fText;
	BString					fName;
	BBitmap*				fThumbnail;
};


// Not thread-safe: the model is owned by ShelfView and must only be used
// from its window's thread.
class ShelfModel
{
public:
							ShelfModel();
							~ShelfModel();

	bool					AddItem(ShelfItem* item);
	int32					CountItems() const;
	ShelfItem*				ItemAt(int32 index) const;
	bool					RemoveItem(int32 index);
	void					MakeEmpty();

private:
							ShelfModel(const ShelfModel&);
			ShelfModel&		operator=(const ShelfModel&);

	BList					fItems;
};

#endif // SHELF_MODEL_H
