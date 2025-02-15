
#pragma once
#include <obj.h>

#include "../dialog.h"

class UiDialog {
public:

	void Hide();

	bool IsActive();

	// this seems to be mostly internal for the python based picker
	DialogState *GetCurrentDialog();
	void ReShowDialog(DialogState *info, int line);
	void Unk();

	/*
		Show an ingame popover with the portrait of the speaker and the given text message.
		Can be used outside of dialog too. 
		The NPC also turns towards the target it is speaking to.
		The voice sample with the id given in speechId is played back if it is not -1.
	*/
	void ShowTextBubble(objHndl speaker, objHndl speakingTo, const string &text, int speechId = -1);

};

extern UiDialog uiDialog;
