#include <Application.h>
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <TrackerAddOn.h>


static const char* kAppSignature = "application/x-vnd.WestenraSoftworks-Tansu";
static const uint32 kMsgAddRefs = 'tREF';


void
process_refs(entry_ref dir_ref, BMessage* message, void* /*reserved*/)
{
	if (message == NULL)
		return;

	BMessage addMessage(kMsgAddRefs);
	int32 count = 0;
	entry_ref ref;
	while (message->FindRef("refs", count, &ref) == B_OK) {
		addMessage.AddRef("refs", &ref);
		count++;
	}
	if (count == 0)
		return;

	BMessenger messenger(kAppSignature);
	if (messenger.IsValid()) {
		messenger.SendMessage(&addMessage);
		return;
	}

	team_id team = be_roster->Launch(kAppSignature, &addMessage);
	if (team < B_OK)
		be_roster->Launch(kAppSignature);
}
