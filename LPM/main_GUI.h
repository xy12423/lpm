#pragma once

#ifndef _H_MAIN_GUI
#define _H_MAIN_GUI

class mainFrame : public wxFrame
{
public:
	mainFrame(const wxString& title);
	enum itemID{
		ID_STATICSRC, ID_LISTSRC, ID_BUTTONADDSRC, ID_BUTTONDELSRC, ID_BUTTONUPDSRC,
		ID_STATICPAK, ID_LISTPAK, ID_BUTTONADDPAK, ID_BUTTONDELPAK, ID_BUTTONUPGPAK, ID_BUTTONUPGALL,
		ID_STATICINFO, ID_TEXTINFO
	};

	wxStaticBox *staticSrc;

	wxCheckListBox *listSrc;
	wxButton *buttonAddSrc, *buttonDelSrc, *buttonUpdSrc;
	void listSrc_ItemCheck(wxCommandEvent& event);
	void buttonAddSrc_Click(wxCommandEvent& event);
	void buttonDelSrc_Click(wxCommandEvent& event);
	void buttonUpdSrc_Click(wxCommandEvent& event);

	wxStaticBox *staticPak;

	wxCheckListBox *listPak;
	wxButton *buttonAddPak, *buttonRemPak, *buttonUpgPak, *buttonUpgAll;
	void listPak_SelectedIndexChanged(wxCommandEvent& event);
	void buttonAddPak_Click(wxCommandEvent& event);
	void buttonDelPak_Click(wxCommandEvent& event);
	void buttonUpgPak_Click(wxCommandEvent& event);
	void buttonUpgAll_Click(wxCommandEvent& event);

	wxStaticBox *staticInfo;
	wxTextCtrl *textInfo;

	wxDECLARE_EVENT_TABLE();
};

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
};

#endif