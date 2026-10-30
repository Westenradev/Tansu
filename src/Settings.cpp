#include "Settings.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Mime.h>
#include <Path.h>


static const char* kSettingsDirName = "Tansu";
static const char* kSettingsFileName = "settings";


status_t
Settings::GetPath(BPath& path, const char* leaf, bool createDirectory)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK)
		status = path.Append(kSettingsDirName);
	if (status == B_OK && createDirectory)
		status = create_directory(path.Path(), 0755);
	if (status == B_OK)
		status = path.Append(leaf);
	return status;
}


Settings::Settings()
	:
	fWindowFrame(),
	fIconSize(B_LARGE_ICON),
	fEdgeRight(true)
{
}


status_t
Settings::Load()
{
	BPath path;
	status_t status = GetPath(path, kSettingsFileName);
	if (status != B_OK)
		return status;

	BFile file(path.Path(), B_READ_ONLY);
	status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage message;
	status = message.Unflatten(&file);
	if (status != B_OK)
		return status;

	BRect frame;
	if (message.FindRect("windowFrame", &frame) == B_OK)
		fWindowFrame = frame;

	int32 iconSize;
	if (message.FindInt32("iconSize", &iconSize) == B_OK)
		fIconSize = iconSize;

	bool edgeRight;
	if (message.FindBool("edgeRight", &edgeRight) == B_OK)
		fEdgeRight = edgeRight;

	return B_OK;
}


status_t
Settings::Save() const
{
	BPath path;
	status_t status = GetPath(path, kSettingsFileName, true);
	if (status != B_OK)
		return status;

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage message;
	message.AddRect("windowFrame", fWindowFrame);
	message.AddInt32("iconSize", fIconSize);
	message.AddBool("edgeRight", fEdgeRight);

	return message.Flatten(&file);
}
