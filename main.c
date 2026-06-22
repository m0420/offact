/* Copyright (C) 2024 John Törnblom

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "IME_dialog.h"
#include "SDL_listui.h"
#include "offact.h"
#include "notify.h"

#include "readme.h"


#define WINDOW_TITLE  "OffAct"
#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080


static SDL_ListUI *ui;


static void refreshListUI(void);


/**
 * Obtain a textual label for an account with the given number.
 **/
static int GetItemLabel(int account_numb, char* label, size_t size)
{
    char account_name[ACCOUNT_NAME_MAX];
    char account_type[ACCOUNT_TYPE_MAX];
    Uint64 account_id;
    int account_flags;

    if(OffAct_GetAccountName(account_numb, account_name)) {
	notify_err("GetItemLabel: GetAccountName failed for slot %d", account_numb);
	return -1;
    }
    if(!*account_name) {
	return -1;
    }
    if(OffAct_GetAccountId(account_numb, &account_id)) {
	notify_err("GetItemLabel: GetAccountId failed for slot %d", account_numb);
	return -1;
    }
    if(OffAct_GetAccountType(account_numb, account_type)) {
	notify_err("GetItemLabel: GetAccountType failed for slot %d", account_numb);
	return -1;
    }
    if(OffAct_GetAccountFlags(account_numb, &account_flags)) {
	notify_err("GetItemLabel: GetAccountFlags failed for slot %d", account_numb);
	return -1;
    }

    SDL_snprintf(label, size, "type: ""%2s  "		\
		 "flags: 0x%04x  id: 0x%016lx  name: %s",
		 account_type, account_flags, account_id, account_name);

    return 0;
}


static void OnDialogOutcome(void* ctx, IME_Dialog_Outcome outcome) {
    char account_type[ACCOUNT_TYPE_MAX] = "np";
    int account_numb = (int)(Uint64)ctx;
    int account_flags = 4098;
    Uint64 account_id;
    char buf[255];

    if(outcome != IME_DIALOG_COMPLETED) {
	if(outcome == IME_DIALOG_CANCELED)
	    notify_dbg("Dialog canceled by user (slot %d)", account_numb);
	else
	    notify_err("Dialog aborted by system (slot %d)", account_numb);
	return;
    }
    if(IME_Dialog_GetText(buf, sizeof(buf)) < 0) {
	notify_err("OnDialogOutcome: GetText failed (slot %d)", account_numb);
	return;
    }
    notify_dbg("Dialog input received: %s (slot %d)", buf, account_numb);
    if(sscanf(buf, "0x%lx", &account_id) != 1) {
	notify_err("OnDialogOutcome: invalid account ID format: %s", buf);
	return;
    }
    notify_dbg("Parsed account ID: 0x%lx for slot %d", account_id, account_numb);

    if(OffAct_SetAccountId(account_numb, account_id))
	notify_err("SetAccountId failed for slot %d", account_numb);
    else
	notify_dbg("SetAccountId OK: 0x%lx -> slot %d", account_id, account_numb);

    if(OffAct_SetAccountType(account_numb, account_type))
	notify_err("SetAccountType failed for slot %d", account_numb);
    else
	notify_dbg("SetAccountType OK: %s -> slot %d", account_type, account_numb);

    if(OffAct_SetAccountFlags(account_numb, account_flags))
	notify_err("SetAccountFlags failed for slot %d", account_numb);
    else
	notify_dbg("SetAccountFlags OK: 0x%04x -> slot %d", account_flags, account_numb);

    refreshListUI();
}


/**
 * Bring up the IME dialog for user input.
 **/
static void OnActivateItem(void *ctx, SDL_ListUI *listui, Uint64 item_id)
{
    int account_numb = (int)(Uint64)ctx;
    char account_name[ACCOUNT_NAME_MAX];
    Uint64 account_id;
    char buf[255];

    notify_dbg("OnActivateItem: slot %d selected", account_numb);
    if(OffAct_GetAccountName(account_numb, account_name)) {
	notify_err("OnActivateItem: GetAccountName failed for slot %d", account_numb);
	return;
    }
    notify_dbg("Account name: %s (slot %d)", account_name, account_numb);
    if(OffAct_GetAccountId(account_numb, &account_id)) {
	notify_err("OnActivateItem: GetAccountId failed for slot %d", account_numb);
	return;
    }
    if(!account_id) {
	account_id = OffAct_GenAccountId(account_name);
	notify_dbg("Generated account ID: 0x%lx for %s", account_id, account_name);
    } else {
	notify_dbg("Existing account ID: 0x%lx for %s", account_id, account_name);
    }
    sprintf(buf, "Enter account ID for %s", account_name);
    if(IME_Dialog_SetTitle(buf) < 0) {
	notify_err("OnActivateItem: IME_Dialog_SetTitle failed");
	return;
    }
    sprintf(buf, "0x%lx", account_id);
    if(IME_Dialog_SetText(buf) < 0) {
	notify_err("OnActivateItem: IME_Dialog_SetText failed");
	return;
    }
    notify_dbg("Opening IME dialog for slot %d", account_numb);
    IME_Dialog_OnOutcome(OnDialogOutcome, (void*)(Uint64)account_numb);
    if(IME_Dialog_Display(SCREEN_WIDTH/2, SCREEN_HEIGHT/2)) {
	notify_err("OnActivateItem: IME_Dialog_Display failed for slot %d", account_numb);
	return;
    }
}


static void refreshListUI(void) {
    Uint64 item_id;
    char buf[255];

    ListUI_Clear(ui);
    for (int n=1; n<=ACCOUNT_NUMB_MAX; n++) {
	*buf = 0;
	if(GetItemLabel(n, buf, sizeof(buf)) < 0) {
	    continue;
	}

	item_id = ListUI_AppendItem(ui, buf);
	ListUI_OnActivate(ui, item_id, OnActivateItem, (void*)(Uint64)n);
    }
}


int SDL_main(int argc, char* args[])
{
    SDL_Renderer* renderer;
    SDL_Window* window;
    SDL_Event event;
    TTF_Font* font;
    int quit = 0;

    printf("%s\n", README_md);
    printf("%s %s was compiled at %s %s\n",
           WINDOW_TITLE, VERSION_TAG, __DATE__, __TIME__);

    notify_dbg("%s %s starting (%s %s)", WINDOW_TITLE, VERSION_TAG, __DATE__, __TIME__);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL_Init: %s\n", SDL_GetError());
	notify_err("SDL_Init failed: %s", SDL_GetError());
	return -1;
    }
    notify_dbg("SDL_Init OK");

    if(!(window=SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED,
				 SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
				 SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN))) {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
	notify_err("SDL_CreateWindow failed: %s", SDL_GetError());
	return -1;
    }
    notify_dbg("SDL_CreateWindow OK (%dx%d)", SCREEN_WIDTH, SCREEN_HEIGHT);

    if(!(renderer=SDL_CreateRenderer(window, -1, (SDL_RENDERER_PRESENTVSYNC |
					  SDL_RENDERER_SOFTWARE)))) {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
	notify_err("SDL_CreateRenderer failed: %s", SDL_GetError());
	return -1;
    }
    notify_dbg("SDL_CreateRenderer OK");

    SDL_GameControllerOpen(0);
    notify_dbg("SDL_GameControllerOpen(0) called");

    if(TTF_Init()  < 0) {
        printf("TTF_Init: %s\n", TTF_GetError());
	notify_err("TTF_Init failed: %s", TTF_GetError());
	return -1;
    }
    notify_dbg("TTF_Init OK");
    if(!(font=TTF_OpenFont("/preinst/common/font/n023055ms.ttf", 44))) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
	notify_err("TTF_OpenFont failed: %s", TTF_GetError());
        return 1;
    }
    notify_dbg("TTF_OpenFont OK");

    ui = ListUI_Create("Offline account activation");
    if(!ui) {
	notify_err("ListUI_Create returned NULL");
	return -1;
    }
    ListUI_SetSelectedColor(ui, (SDL_Color){0x3b, 0x40, 0x47, 0xff});
    ListUI_SetTextColor(ui, (SDL_Color){0xb9, 0xbb, 0xbb, 0xff});
    ListUI_SetActivateTextColor(ui, (SDL_Color){0xff, 0xff, 0xff, 0xff});
    notify_dbg("ListUI created, loading accounts...");
    refreshListUI();
    notify_dbg("Account list loaded, entering main loop");

    while(!quit) {
	while(SDL_PollEvent(&event) != 0) {
	    if(event.type == SDL_CONTROLLERBUTTONDOWN) {
		switch(event.cbutton.button) {
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
		    ListUI_NavigateItemUp(ui, SDL_FALSE, SDL_TRUE);
		    break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		    ListUI_NavigateItemDown(ui, SDL_FALSE, SDL_TRUE);
		    break;
		case SDL_CONTROLLER_BUTTON_A:
		    ListUI_ActivateSelected(ui);
		    break;
                case SDL_CONTROLLER_BUTTON_B:
                    quit = 1;
                    break;
		}
	    }
	}

	SDL_SetRenderDrawColor(renderer, 0x05, 0x0d, 0x1c, 0xff);
	SDL_RenderClear(renderer);

	ListUI_Render(ui, renderer, font);
	SDL_RenderPresent(renderer);

	IME_Dialog_PullStatus();
    }

    ListUI_Destroy(ui);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}


/* Local Variables: */
/* tab-width: 8 */
/* c-basic-offset: 4 */
/* End: */
