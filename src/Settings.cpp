#include "Settings.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <StorageDefs.h>


static const char* kSettingsFileName = "Tansu/settings";


Settings::Settings()
	:
	fWindowFrame(BRect(0, 0, 159, 399)),
	fIconSize(B_MINI_ICON),
	fEdgeRight(true)
{
}


status_t
Settings::Load()
{
	char path[B_PATH_NAME_LENGTH];
	status_t status = find_directory(B_USER_SETTINGS_PATH, path, sizeof(path));
	if (status != B_OK)
		return status;

	BPath settingsPath(path);
	if (settingsPath.Append(kSettingsFileName) != B_OK)
		return B_ERROR;

	BFile file(settingsPath.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return B_NO_INIT;

	BMessage message;
	if (message.Unflatten(&file) != B_OK)
		return B_ERROR;

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
	char path[B_PATH_NAME_LENGTH];
	status_t status = find_directory(B_USER_SETTINGS_PATH, path, sizeof(path));
	if (status != B_OK)
		return status;

	BPath settingsPath(path);
	if (settingsPath.Append("Tansu") != B_OK)
		return B_ERROR;

	BDirectory directory(settingsPath.Path());
	if (directory.InitCheck() != B_OK) {
		status = create_directory(settingsPath.Path(), 0777);
		if (status != B_OK)
			return status;
	}
	if (settingsPath.Append("settings") != B_OK)
		return B_ERROR;

	BFile file(settingsPath.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK)
		return file.InitCheck();

	BMessage message;
	message.AddRect("windowFrame", fWindowFrame);
	message.AddInt32("iconSize", fIconSize);
	message.AddBool("edgeRight", fEdgeRight);

	return message.Flatten(&file);
}
