#ifndef SHELF_VIEW_H
#define SHELF_VIEW_H

#include <View.h>

#include "ShelfModel.h"


class ShelfView : public BView
{
public:
							ShelfView(const char* name);
	virtual					~ShelfView();

	virtual void			AttachedToWindow();
	virtual void			Draw(BRect updateRect);
	virtual void			MessageReceived(BMessage* message);
	virtual void			MouseMoved(BPoint point, uint32 transit,
								const BMessage* dragMessage);

	ShelfModel*				Model() const		{ return fModel; }

private:
	void					_RebuildRows();
	void					_SetDropHighlight(bool highlight);

	ShelfModel*				fModel;
	bool					fDropHighlight;
};

#endif // SHELF_VIEW_H
