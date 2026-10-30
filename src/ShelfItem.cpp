#include "ShelfItem.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <File.h>
#include <Message.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>


static const int32 kMaxNameChars = 40;
static const char* kPlainTextType = "text/plain";
static const char* kPNGType = "image/png";


uint32 ShelfItem::sNextID = 1;


ShelfItem::ShelfItem(shelf_item_kind kind)
	:
	fID(sNextID++),
	fKind(kind),
	fImage(NULL),
	fIcon(NULL),
	fIconSize(0)
{
}


ShelfItem::~ShelfItem()
{
	delete fImage;
	delete fIcon;
}


ShelfItem*
ShelfItem::CreateFile(const entry_ref& ref)
{
	BEntry entry(&ref);
	if (entry.InitCheck() != B_OK || !entry.Exists())
		return NULL;

	ShelfItem* item = new ShelfItem(kItemFile);
	entry.GetNodeRef(&item->fNodeRef);
	item->SetRef(ref);
	return item;
}


ShelfItem*
ShelfItem::CreateText(const BString& text)
{
	if (text.IsEmpty())
		return NULL;

	ShelfItem* item = new ShelfItem(kItemText);
	item->fText = text;

	// first non-empty line, cut on character (not byte) boundaries for UTF-8
	BString name(text);
	name.Trim();
	int32 lineEnd = name.FindFirst('\n');
	if (lineEnd >= 0)
		name.Truncate(lineEnd);
	name.ReplaceAll('\t', ' ');
	if (name.CountChars() > kMaxNameChars)
		name.TruncateChars(kMaxNameChars);
	item->fName = name;
	return item;
}


ShelfItem*
ShelfItem::CreateImage(const void* data, size_t size, const char* mimeType)
{
	if (data == NULL || size == 0)
		return NULL;

	BMemoryIO stream(data, size);
	BBitmap* bitmap = BTranslationUtils::GetBitmap(&stream);
	if (bitmap == NULL)
		return NULL;

	ShelfItem* item = new ShelfItem(kItemImage);
	item->fImage = bitmap;
	item->fImageData.Write(data, size);
	item->fImageType = mimeType;
	item->fName.SetToFormat("Image %" B_PRId32 " x %" B_PRId32,
		bitmap->Bounds().IntegerWidth() + 1,
		bitmap->Bounds().IntegerHeight() + 1);
	return item;
}


ShelfItem*
ShelfItem::CreateImage(BBitmap* bitmap)
{
	if (bitmap == NULL)
		return NULL;

	// keep a PNG copy so the image can be saved and dragged out as a file
	BMallocIO png;
	BBitmapStream stream(bitmap);
	status_t status = BTranslatorRoster::Default()->Translate(&stream, NULL,
		NULL, &png, B_PNG_FORMAT);
	stream.DetachBitmap(&bitmap);
	if (status != B_OK) {
		delete bitmap;
		return NULL;
	}

	ShelfItem* item = CreateImage(png.Buffer(), png.BufferLength(), kPNGType);
	delete bitmap;
	return item;
}


ShelfItem*
ShelfItem::Instantiate(const BMessage* archive)
{
	int32 kind;
	if (archive->FindInt32("kind", &kind) != B_OK)
		return NULL;

	switch (kind) {
		case kItemFile:
		{
			const char* path;
			entry_ref ref;
			if (archive->FindString("path", &path) != B_OK
				|| get_ref_for_path(path, &ref) != B_OK) {
				return NULL;
			}
			return CreateFile(ref);
		}

		case kItemText:
		{
			BString text;
			if (archive->FindString("text", &text) != B_OK)
				return NULL;
			return CreateText(text);
		}

		case kItemImage:
		{
			const char* type;
			const void* data;
			ssize_t size;
			if (archive->FindString("type", &type) != B_OK
				|| archive->FindData("data", B_RAW_TYPE, &data, &size)
					!= B_OK) {
				return NULL;
			}
			return CreateImage(data, size, type);
		}
	}
	return NULL;
}


status_t
ShelfItem::Archive(BMessage* archive) const
{
	archive->AddInt32("kind", fKind);

	switch (fKind) {
		case kItemFile:
		{
			BPath path(&fRef);
			if (path.InitCheck() != B_OK)
				return path.InitCheck();
			return archive->AddString("path", path.Path());
		}

		case kItemText:
			return archive->AddString("text", fText);

		case kItemImage:
			archive->AddString("type", fImageType);
			return archive->AddData("data", B_RAW_TYPE, ImageData(),
				ImageDataSize());
	}
	return B_ERROR;
}


void
ShelfItem::SetRef(const entry_ref& ref)
{
	fRef = ref;
	if (fRef.name != NULL)
		fName = fRef.name;
}


const void*
ShelfItem::ImageData() const
{
	return fImageData.Buffer();
}


size_t
ShelfItem::ImageDataSize() const
{
	return fImageData.BufferLength();
}


const char*
ShelfItem::MimeType() const
{
	if (fKind == kItemText)
		return kPlainTextType;
	if (fKind == kItemImage)
		return fImageType.String();
	return NULL;
}


const char*
ShelfItem::ClipName() const
{
	return fKind == kItemImage ? "Image clipping" : "Text clipping";
}


status_t
ShelfItem::WriteTo(BFile& file) const
{
	const void* data;
	size_t size;
	if (fKind == kItemText) {
		data = fText.String();
		size = fText.Length();
	} else if (fKind == kItemImage) {
		data = ImageData();
		size = ImageDataSize();
	} else
		return B_BAD_VALUE;

	ssize_t written = file.WriteAt(0, data, size);
	if (written < 0)
		return written;
	if ((size_t)written != size)
		return B_IO_ERROR;
	file.SetSize(size);

	BNodeInfo info(&file);
	return info.SetType(MimeType());
}


const BBitmap*
ShelfItem::Icon(int32 size)
{
	if (fKind == kItemImage)
		return NULL;
	if (fIcon != NULL && fIconSize == size)
		return fIcon;

	delete fIcon;
	fIcon = new BBitmap(BRect(0, 0, size - 1, size - 1), B_RGBA32);
	fIconSize = size;

	status_t status;
	if (fKind == kItemFile)
		status = BNodeInfo::GetTrackerIcon(&fRef, fIcon, (icon_size)size);
	else {
		status = BMimeType(kPlainTextType).GetIcon(fIcon, (icon_size)size);
		if (status != B_OK)
			status = BMimeType("text").GetIcon(fIcon, (icon_size)size);
	}

	if (status != B_OK) {
		delete fIcon;
		fIcon = NULL;
	}
	return fIcon;
}
