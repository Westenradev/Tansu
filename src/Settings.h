#ifndef SETTINGS_H
#define SETTINGS_H

#include <GraphicsDefs.h>
#include <Message.h>
#include <Rect.h>


class Settings
{
public:
							Settings();

	status_t				Load();
	status_t				Save() const;

	BRect					WindowFrame() const		{ return fWindowFrame; }
	void					SetWindowFrame(BRect frame) { fWindowFrame = frame; }

	int32					IconSize() const		{ return fIconSize; }
	void					SetIconSize(int32 size)	{ fIconSize = size; }

	bool					EdgeRight() const		{ return fEdgeRight; }
	void					SetEdgeRight(bool right) { fEdgeRight = right; }

private:
	BRect					fWindowFrame;
	int32					fIconSize;
	bool					fEdgeRight;
};

#endif // SETTINGS_H
