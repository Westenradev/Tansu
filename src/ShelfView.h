#ifndef SHELF_VIEW_H
#define SHELF_VIEW_H

#include <ListView.h>
#include <Node.h>

class ShelfItem;


class ShelfView : public BListView
{
public:
							ShelfView(const char* name, int32 iconSize);
	virtual					~ShelfView();

	virtual void			AttachedToWindow();
	virtual void			DetachedFromWindow();
	virtual void			Draw(BRect updateRect);
	virtual void			MessageReceived(BMessage* message);
	virtual void			MouseDown(BPoint where);
	virtual void			MouseMoved(BPoint where, uint32 transit,
								const BMessage* dragMessage);
	virtual void			KeyDown(const char* bytes, int32 numBytes);
	virtual bool			InitiateDrag(BPoint where, int32 index,
								bool wasSelected);

			void			AddRefs(const BMessage* message);

private:
			ShelfItem*		_ItemAt(int32 index) const;
			int32			_IndexOfID(uint32 id) const;
			int32			_IndexOfNode(const node_ref& nodeRef) const;
			bool			_HasFile(const entry_ref& ref) const;

			void			_AddItem(ShelfItem* item);
			void			_RemoveIndex(int32 index);
			void			_RemoveSelected();
			void			_RemoveAll();

			void			_HandleDrop(BMessage* message);
			bool			_AddFromData(const BMessage* message);
			bool			_NegotiateDrop(BMessage* message);
			void			_HandleCopyTarget(BMessage* message);
			void			_HandleNodeMonitor(const BMessage* message);
			void			_Paste();

			void			_OpenSelected();
			void			_ShowSelectedInTracker();
			bool			_SelectionHasFiles() const;
			void			_ShowContextMenu(BPoint where, bool onItem);

			void			_SetDropHighlight(bool highlight);

			void			_LoadShelf();
			void			_SaveShelf() const;

			int32			fIconSize;
			bool			fDropHighlight;
			uint32			fDraggedImageID;
			bool			fLoading;
};

#endif // SHELF_VIEW_H
