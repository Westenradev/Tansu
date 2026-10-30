#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SupportDefs.h>


static const char* const kAppSignature = "application/x-vnd.WestenraSoftworks-Tansu";
static const char* const kTrackerSignature = "application/x-vnd.Be-TRAK";

// Sent by the Tracker add-on (addon/TrackerAddOn.cpp) with a "refs" list.
static const uint32 kMsgAddRefs = 'tREF';

static const uint32 kMsgOpenItems = 'tOPN';
static const uint32 kMsgShowInTracker = 'tSHW';
static const uint32 kMsgRemoveItems = 'tREM';
static const uint32 kMsgClearShelf = 'tCLR';
static const uint32 kMsgPaste = 'tPST';

#endif // CONSTANTS_H
