#ifndef SHELF_ITEM_H
#define SHELF_ITEM_H

#include <DataIO.h>
#include <Entry.h>
#include <Node.h>
#include <String.h>

class BBitmap;
class BFile;
class BMessage;


enum shelf_item_kind {
	kItemFile = 0,
	kItemText,
	kItemImage
};


class ShelfItem
{
public:
	static	ShelfItem*		CreateFile(const entry_ref& ref);
	static	ShelfItem*		CreateText(const BString& text);
	static	ShelfItem*		CreateImage(const void* data, size_t size,
								const char* mimeType);
	static	ShelfItem*		CreateImage(BBitmap* bitmap);
	static	ShelfItem*		Instantiate(const BMessage* archive);

							~ShelfItem();

			status_t		Archive(BMessage* archive) const;

			uint32			ID() const				{ return fID; }
			shelf_item_kind	Kind() const			{ return fKind; }
			const BString&	Name() const			{ return fName; }

			// file items
			const entry_ref& Ref() const			{ return fRef; }
			const node_ref&	NodeRef() const			{ return fNodeRef; }
			void			SetRef(const entry_ref& ref);

			// text items
			const BString&	Text() const			{ return fText; }

			// image items
			BBitmap*		Image() const			{ return fImage; }
			const void*		ImageData() const;
			size_t			ImageDataSize() const;

			// text and image items
			const char*		MimeType() const;
			const char*		ClipName() const;
			status_t		WriteTo(BFile& file) const;

			const BBitmap*	Icon(int32 size);

private:
							ShelfItem(shelf_item_kind kind);
							ShelfItem(const ShelfItem&);
			ShelfItem&		operator=(const ShelfItem&);

	static	uint32			sNextID;

			uint32			fID;
			shelf_item_kind	fKind;
			BString			fName;

			entry_ref		fRef;
			node_ref		fNodeRef;

			BString			fText;

			BBitmap*		fImage;
			BMallocIO		fImageData;
			BString			fImageType;

			BBitmap*		fIcon;
			int32			fIconSize;
};

#endif // SHELF_ITEM_H
