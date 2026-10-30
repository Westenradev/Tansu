#include "ShelfView.h"

#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Window.h>

#include "Constants.h"
#include "Settings.h"
#include "ShelfItem.h"
#include "ShelfListItem.h"


static const char* kEmptyText = "Drop files or text here";
static const char* kShelfFileName = "shelf";
static const char* kPlainTextType = "text/plain";
static const char* kInternalDragField = "tansu:internal";
static const char* kItemIDField = "tansu:item";

// archived BBitmaps, as put on the clipboard or dragged by some apps
static const char* kBitmapTypes[] = {
	"image/x-be-bitmap",
	"image/bitmap",
	NULL
};


ShelfView::ShelfView(const char* name, int32 iconSize)
	:
	BListView(name, B_MULTIPLE_SELECTION_LIST,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fIconSize(iconSize),
	fDropHighlight(false),
	fDraggedImageID(0),
	fLoading(false)
{
}


ShelfView::~ShelfView()
{
	for (int32 i = CountItems() - 1; i >= 0; i--)
		delete ItemAt(i);
	MakeEmpty();
}


void
ShelfView::AttachedToWindow()
{
	BListView::AttachedToWindow();
	SetInvocationMessage(new BMessage(kMsgOpenItems));
	SetTarget(this);
	_LoadShelf();
}


void
ShelfView::DetachedFromWindow()
{
	stop_watching(this);
	BListView::DetachedFromWindow();
}


void
ShelfView::Draw(BRect updateRect)
{
	BListView::Draw(updateRect);

	BRect bounds = Bounds();
	if (CountItems() == 0) {
		if (fDropHighlight) {
			SetHighColor(tint_color(ui_color(B_LIST_BACKGROUND_COLOR),
				B_DARKEN_1_TINT));
			FillRect(bounds);
		}

		// The hint is drawn here rather than in a child view, so that the
		// shelf itself keeps receiving mouse and drag events.
		BFont font;
		GetFont(&font);
		float inset = be_control_look->DefaultLabelSpacing();
		BString text(kEmptyText);
		font.TruncateString(&text, B_TRUNCATE_END,
			bounds.Width() - 2 * inset);

		font_height fontHeight;
		font.GetHeight(&fontHeight);
		float x = bounds.left
			+ (bounds.Width() - font.StringWidth(text.String())) / 2;
		float y = bounds.top + (bounds.Height() - fontHeight.ascent
			- fontHeight.descent) / 2 + fontHeight.ascent;

		SetDrawingMode(B_OP_OVER);
		SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
		DrawString(text.String(), BPoint(x, y));
		SetDrawingMode(B_OP_COPY);
	}

	if (fDropHighlight) {
		SetHighUIColor(B_CONTROL_HIGHLIGHT_COLOR);
		StrokeRect(bounds);
		StrokeRect(bounds.InsetByCopy(1, 1));
	}
}


void
ShelfView::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		_SetDropHighlight(false);
		if (!message->HasBool(kInternalDragField))
			_HandleDrop(message);
		return;
	}

	switch (message->what) {
		case kMsgAddRefs:
		case B_REFS_RECEIVED:
			AddRefs(message);
			break;

		case B_MIME_DATA:
			// data sent in reply to our B_COPY_TARGET negotiation
			_AddFromData(message);
			break;

		case B_COPY_TARGET:
			// a drop target wants the image we dragged out
			_HandleCopyTarget(message);
			break;

		case B_NODE_MONITOR:
			_HandleNodeMonitor(message);
			break;

		case kMsgOpenItems:
			_OpenSelected();
			break;

		case kMsgShowInTracker:
			_ShowSelectedInTracker();
			break;

		case kMsgRemoveItems:
			_RemoveSelected();
			break;

		case kMsgClearShelf:
			_RemoveAll();
			break;

		case kMsgPaste:
			_Paste();
			break;

		default:
			BListView::MessageReceived(message);
			break;
	}
}


void
ShelfView::MouseDown(BPoint where)
{
	int32 buttons = 0;
	if (Window()->CurrentMessage() != NULL)
		Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		int32 index = IndexOf(where);
		if (index >= 0 && !ItemAt(index)->IsSelected())
			Select(index);
		_ShowContextMenu(where, index >= 0);
		return;
	}

	BListView::MouseDown(where);
}


void
ShelfView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	_SetDropHighlight(dragMessage != NULL
		&& !dragMessage->HasBool(kInternalDragField)
		&& transit != B_EXITED_VIEW && transit != B_OUTSIDE_VIEW);
	BListView::MouseMoved(where, transit, dragMessage);
}


void
ShelfView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1 && bytes[0] == B_DELETE) {
		_RemoveSelected();
		return;
	}
	BListView::KeyDown(bytes, numBytes);
}


bool
ShelfView::InitiateDrag(BPoint where, int32 index, bool /*wasSelected*/)
{
	if (!ItemAt(index)->IsSelected())
		Select(index);

	BMessage drag(B_SIMPLE_DATA);
	drag.AddBool(kInternalDragField, true);

	BString text;
	BString textName;
	int32 fileCount = 0;
	int32 textCount = 0;
	ShelfItem* image = NULL;
	for (int32 i = 0; ; i++) {
		int32 selected = CurrentSelection(i);
		if (selected < 0)
			break;

		ShelfItem* item = _ItemAt(selected);
		switch (item->Kind()) {
			case kItemFile:
				drag.AddRef("refs", &item->Ref());
				fileCount++;
				break;
			case kItemText:
				if (!text.IsEmpty())
					text << "\n";
				else
					textName = item->Name();
				text << item->Text();
				textCount++;
				break;
			case kItemImage:
				if (image == NULL)
					image = item;
				break;
		}
	}

	if (textCount > 0) {
		// text views and Tracker (text clipping) take plain text directly
		if (fileCount == 0)
			drag.what = B_MIME_DATA;
		drag.AddData(kPlainTextType, B_MIME_TYPE, text.String(),
			text.Length());
		// Tracker names the clipping file after this
		drag.AddString("be:clip_name", textName);
	} else if (fileCount == 0 && image != NULL) {
		// "promise" protocol: the target asks for a file or the data
		drag.AddInt32("be:actions", B_COPY_TARGET);
		drag.AddString("be:types", B_FILE_MIME_TYPE);
		drag.AddString("be:types", image->MimeType());
		drag.AddString("be:filetypes", image->MimeType());
		drag.AddString("be:type_descriptions", "Image");
		drag.AddString("be:clip_name", image->ClipName());
		BMessage origin;
		origin.AddInt32(kItemIDField, image->ID());
		drag.AddMessage("be:originator-data", &origin);
		fDraggedImageID = image->ID();
	}

	ShelfListItem* listItem = (ShelfListItem*)ItemAt(index);
	BRect frame = ItemFrame(index);
	const BBitmap* icon = listItem->Item()->Icon(fIconSize);
	if (icon != NULL) {
		BRect iconFrame = listItem->IconFrame(frame);
		DragMessage(&drag, new BBitmap(icon), B_OP_ALPHA,
			where - iconFrame.LeftTop(), this);
	} else
		DragMessage(&drag, frame, this);
	return true;
}


void
ShelfView::AddRefs(const BMessage* message)
{
	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		if (_HasFile(ref))
			continue;
		_AddItem(ShelfItem::CreateFile(ref));
	}
}


ShelfItem*
ShelfView::_ItemAt(int32 index) const
{
	ShelfListItem* item = (ShelfListItem*)ItemAt(index);
	return item != NULL ? item->Item() : NULL;
}


int32
ShelfView::_IndexOfID(uint32 id) const
{
	for (int32 i = 0; i < CountItems(); i++) {
		if (_ItemAt(i)->ID() == id)
			return i;
	}
	return -1;
}


int32
ShelfView::_IndexOfNode(const node_ref& nodeRef) const
{
	for (int32 i = 0; i < CountItems(); i++) {
		ShelfItem* item = _ItemAt(i);
		if (item->Kind() == kItemFile && item->NodeRef() == nodeRef)
			return i;
	}
	return -1;
}


bool
ShelfView::_HasFile(const entry_ref& ref) const
{
	node_ref nodeRef;
	BEntry entry(&ref);
	return entry.GetNodeRef(&nodeRef) == B_OK && _IndexOfNode(nodeRef) >= 0;
}


void
ShelfView::_AddItem(ShelfItem* item)
{
	if (item == NULL)
		return;

	AddItem(new ShelfListItem(item, fIconSize));
	if (item->Kind() == kItemFile)
		watch_node(&item->NodeRef(), B_WATCH_NAME, this);
	_SaveShelf();
}


void
ShelfView::_RemoveIndex(int32 index)
{
	ShelfListItem* listItem = (ShelfListItem*)RemoveItem(index);
	if (listItem == NULL)
		return;

	if (listItem->Item()->Kind() == kItemFile)
		watch_node(&listItem->Item()->NodeRef(), B_STOP_WATCHING, this);
	delete listItem;
}


void
ShelfView::_RemoveSelected()
{
	for (int32 i = CountItems() - 1; i >= 0; i--) {
		if (ItemAt(i)->IsSelected())
			_RemoveIndex(i);
	}
	_SaveShelf();
}


void
ShelfView::_RemoveAll()
{
	for (int32 i = CountItems() - 1; i >= 0; i--)
		_RemoveIndex(i);
	_SaveShelf();
}


void
ShelfView::_HandleDrop(BMessage* message)
{
	if (message->HasRef("refs")) {
		AddRefs(message);
		return;
	}

	if (_AddFromData(message))
		return;

	_NegotiateDrop(message);
}


bool
ShelfView::_AddFromData(const BMessage* message)
{
	const void* data;
	ssize_t size;
	if (message->FindData(kPlainTextType, B_MIME_TYPE, &data, &size)
			== B_OK) {
		_AddItem(ShelfItem::CreateText(BString((const char*)data, size)));
		return true;
	}

	char* name;
	type_code type;
	for (int32 i = 0; message->GetInfo(B_MIME_TYPE, i, &name, &type)
			== B_OK; i++) {
		if (strncmp(name, "image/", 6) != 0
			|| message->FindData(name, B_MIME_TYPE, &data, &size) != B_OK) {
			continue;
		}
		ShelfItem* item = ShelfItem::CreateImage(data, size, name);
		if (item != NULL) {
			_AddItem(item);
			return true;
		}
	}

	for (int32 i = 0; kBitmapTypes[i] != NULL; i++) {
		BMessage archive;
		if (message->FindMessage(kBitmapTypes[i], &archive) != B_OK)
			continue;
		BBitmap* bitmap = dynamic_cast<BBitmap*>(
			BBitmap::Instantiate(&archive));
		ShelfItem* item = ShelfItem::CreateImage(bitmap);
		if (item != NULL) {
			_AddItem(item);
			return true;
		}
	}

	return false;
}


bool
ShelfView::_NegotiateDrop(BMessage* message)
{
	// "promise" drag & drop: ask the source for text or an image
	bool canCopy = false;
	int32 action;
	for (int32 i = 0; message->FindInt32("be:actions", i, &action) == B_OK;
			i++) {
		if (action == B_COPY_TARGET)
			canCopy = true;
	}
	if (!canCopy)
		return false;

	// Prefer an image (PNG first) whenever one is offered: image sources
	// such as ShowImage also list "text/plain" (via a text translator).
	BString image;
	bool hasText = false;
	const char* type;
	for (int32 i = 0; message->FindString("be:types", i, &type) == B_OK;
			i++) {
		if (strcmp(type, "image/png") == 0
			|| (image.IsEmpty() && strncmp(type, "image/", 6) == 0)) {
			image = type;
		} else if (strcmp(type, kPlainTextType) == 0)
			hasText = true;
	}

	BString chosen = !image.IsEmpty() ? image
		: (hasText ? BString(kPlainTextType) : BString());
	if (chosen.IsEmpty())
		return false;

	BMessage reply(B_COPY_TARGET);
	reply.AddString("be:types", chosen);
	return message->SendReply(&reply, this) == B_OK;
}


void
ShelfView::_HandleCopyTarget(BMessage* message)
{
	int32 id = fDraggedImageID;
	BMessage origin;
	if (message->FindMessage("be:originator-data", &origin) == B_OK)
		origin.FindInt32(kItemIDField, &id);

	int32 index = _IndexOfID(id);
	if (index < 0)
		return;
	ShelfItem* item = _ItemAt(index);

	entry_ref directoryRef;
	const char* name;
	if (message->FindRef("directory", &directoryRef) == B_OK
		&& message->FindString("name", &name) == B_OK) {
		// Tracker created the file, we fill it in
		BDirectory directory(&directoryRef);
		BFile file(&directory, name,
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK)
			item->WriteTo(file);
		return;
	}

	const char* type;
	if (message->FindString("be:types", &type) == B_OK
		&& strcmp(type, item->MimeType()) == 0) {
		BMessage reply(B_MIME_DATA);
		reply.AddData(type, B_MIME_TYPE, item->ImageData(),
			item->ImageDataSize());
		message->SendReply(&reply);
	}
}


void
ShelfView::_HandleNodeMonitor(const BMessage* message)
{
	int32 opcode;
	node_ref nodeRef;
	if (message->FindInt32("opcode", &opcode) != B_OK
		|| message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK) {
		return;
	}

	int32 index = _IndexOfNode(nodeRef);
	if (index < 0)
		return;

	if (opcode == B_ENTRY_REMOVED) {
		_RemoveIndex(index);
		_SaveShelf();
	} else if (opcode == B_ENTRY_MOVED) {
		// keep following the file when it is renamed or moved
		entry_ref ref;
		const char* name;
		if (message->FindInt64("to directory", &ref.directory) != B_OK
			|| message->FindString("name", &name) != B_OK) {
			return;
		}
		ref.device = nodeRef.device;
		ref.set_name(name);
		_ItemAt(index)->SetRef(ref);
		InvalidateItem(index);
		_SaveShelf();
	}
}


void
ShelfView::_Paste()
{
	if (!be_clipboard->Lock())
		return;

	BMessage* clip = be_clipboard->Data();
	bool added = clip != NULL && _AddFromData(clip);
	be_clipboard->Unlock();

	if (!added) {
		BAlert* alert = new BAlert("Tansu",
			"The clipboard contains no text or image.", "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	}
}


void
ShelfView::_OpenSelected()
{
	for (int32 i = 0; ; i++) {
		int32 selected = CurrentSelection(i);
		if (selected < 0)
			break;

		ShelfItem* item = _ItemAt(selected);
		entry_ref ref;
		if (item->Kind() == kItemFile)
			ref = item->Ref();
		else {
			// text and images are opened from a temporary file
			BPath path;
			if (find_directory(B_SYSTEM_TEMP_DIRECTORY, &path) != B_OK)
				continue;
			BString name;
			name.SetToFormat("Tansu %s %" B_PRIu32, item->ClipName(),
				item->ID());
			path.Append(name.String());
			BFile file(path.Path(),
				B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
			if (file.InitCheck() != B_OK || item->WriteTo(file) != B_OK
				|| get_ref_for_path(path.Path(), &ref) != B_OK) {
				continue;
			}
		}
		be_roster->Launch(&ref);
	}
}


void
ShelfView::_ShowSelectedInTracker()
{
	for (int32 i = 0; ; i++) {
		int32 selected = CurrentSelection(i);
		if (selected < 0)
			break;

		ShelfItem* item = _ItemAt(selected);
		if (item->Kind() != kItemFile)
			continue;

		BEntry entry(&item->Ref());
		BEntry parent;
		entry_ref parentRef;
		if (entry.GetParent(&parent) != B_OK
			|| parent.GetRef(&parentRef) != B_OK) {
			continue;
		}

		// Tracker opens the folder and selects the node (see Tracker.cpp)
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &parentRef);
		message.AddData("nodeRefToSelect", B_RAW_TYPE, &item->NodeRef(),
			sizeof(node_ref));
		BMessenger(kTrackerSignature).SendMessage(&message);
	}
}


bool
ShelfView::_SelectionHasFiles() const
{
	for (int32 i = 0; ; i++) {
		int32 selected = CurrentSelection(i);
		if (selected < 0)
			return false;
		if (_ItemAt(selected)->Kind() == kItemFile)
			return true;
	}
}


void
ShelfView::_ShowContextMenu(BPoint where, bool onItem)
{
	BPopUpMenu* menu = new BPopUpMenu("context", false, false);

	if (onItem) {
		menu->AddItem(new BMenuItem("Open", new BMessage(kMsgOpenItems)));
		BMenuItem* show = new BMenuItem("Show in Tracker",
			new BMessage(kMsgShowInTracker));
		show->SetEnabled(_SelectionHasFiles());
		menu->AddItem(show);
		menu->AddItem(new BMenuItem("Remove",
			new BMessage(kMsgRemoveItems)));
		menu->AddSeparatorItem();
	}

	menu->AddItem(new BMenuItem("Paste from clipboard",
		new BMessage(kMsgPaste)));
	BMenuItem* clear = new BMenuItem("Clear shelf",
		new BMessage(kMsgClearShelf));
	clear->SetEnabled(CountItems() > 0);
	menu->AddItem(clear);
	menu->SetTargetForItems(this);

	menu->AddSeparatorItem();
	BMenuItem* about = new BMenuItem("About Tansu" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED));
	about->SetTarget(be_app);
	menu->AddItem(about);
	BMenuItem* quit = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED));
	quit->SetTarget(be_app);
	menu->AddItem(quit);

	menu->SetAsyncAutoDestruct(true);
	menu->Go(ConvertToScreen(where), true, false, true);
}


void
ShelfView::_SetDropHighlight(bool highlight)
{
	if (highlight == fDropHighlight)
		return;
	fDropHighlight = highlight;
	Invalidate();
}


void
ShelfView::_LoadShelf()
{
	BPath path;
	if (Settings::GetPath(path, kShelfFileName) != B_OK)
		return;

	BFile file(path.Path(), B_READ_ONLY);
	BMessage archive;
	if (file.InitCheck() != B_OK || archive.Unflatten(&file) != B_OK)
		return;

	fLoading = true;
	BMessage itemArchive;
	for (int32 i = 0; archive.FindMessage("item", i, &itemArchive) == B_OK;
			i++) {
		ShelfItem* item = ShelfItem::Instantiate(&itemArchive);
		if (item != NULL && item->Kind() == kItemFile
			&& _IndexOfNode(item->NodeRef()) >= 0) {
			delete item;
			continue;
		}
		_AddItem(item);
	}
	fLoading = false;
}


void
ShelfView::_SaveShelf() const
{
	if (fLoading)
		return;

	BMessage archive;
	for (int32 i = 0; i < CountItems(); i++) {
		BMessage itemArchive;
		if (_ItemAt(i)->Archive(&itemArchive) == B_OK)
			archive.AddMessage("item", &itemArchive);
	}

	BPath path;
	if (Settings::GetPath(path, kShelfFileName, true) != B_OK)
		return;

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK)
		archive.Flatten(&file);
}
