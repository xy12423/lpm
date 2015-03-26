#pragma once

#ifndef _H_MAIN_GUI
#define _H_MAIN_GUI

class mainFrame : public wxFrame
{
public:
	mainFrame(const wxString& title);
	enum itemID{
		ID_STATICSRC, ID_LISTSRC, ID_BUTTONADDSRC, ID_BUTTONDELSRC, ID_BUTTONUPDSRC,
		ID_STATICPAK, ID_CHECKUPD, ID_CHECKINST, ID_LABELSEARCH, ID_TEXTSEARCH, ID_LISTPAK, ID_BUTTONADDPAK, ID_BUTTONDELPAK, ID_BUTTONUPGPAK, ID_BUTTONUPGALL, ID_LABELINFO,
		ID_STATICINFO, ID_TEXTINFO, ID_GAUGEPROGRESS
	};
	void refreshPakList();

	wxPanel *panel;

	wxStaticBox *staticSrc;

	wxCheckListBox *listSrc;
	wxButton *buttonAddSrc, *buttonDelSrc, *buttonUpdSrc; 
	void listSrc_ItemCheck(wxCommandEvent& event);
	void buttonAddSrc_Click(wxCommandEvent& event);
	void buttonDelSrc_Click(wxCommandEvent& event);
	void buttonUpdSrc_Click(wxCommandEvent& event);

	wxStaticBox *staticPak;

	wxCheckBox *checkUpd, *checkInst;
	wxStaticText *labelSearch;
	wxTextCtrl *textSearch;
	void checkUpd_CheckedChanged(wxCommandEvent& event);
	void checkInst_CheckedChanged(wxCommandEvent& event);
	void textSearch_TextChanged(wxCommandEvent& event);

	wxCheckListBox *listPak;
	wxButton *buttonAddPak, *buttonRemPak, *buttonUpgPak, *buttonUpgAll;
	wxStaticText *labelInfo;
	void listPak_SelectedIndexChanged(wxCommandEvent& event);
	void buttonAddPak_Click(wxCommandEvent& event);
	void buttonDelPak_Click(wxCommandEvent& event);
	void buttonUpgPak_Click(wxCommandEvent& event);
	void buttonUpgAll_Click(wxCommandEvent& event);

	wxStaticBox *staticInfo;
	wxTextCtrl *textInfo;
	wxGauge *gaugeProgress;

	friend void printInfo(package *pkg);
	friend void reportProgress(double progress);

	wxDECLARE_EVENT_TABLE();
};

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int OnExit();
};

enum guiStrID{
	TEXT_STATICSRC,
	TEXT_BUTTONADDSRC,
	TEXT_BUTTONDELSRC,
	TEXT_BUTTONUPDSRC,
	TEXT_STATICPAK,
	TEXT_CHECKUPD,
	TEXT_CHECKINST,
	TEXT_LABELSEARCH,
	TEXT_BUTTONADDPAK,
	TEXT_BUTTONDELPAK,
	TEXT_BUTTONUPGPAK,
	TEXT_BUTTONUPGALL,
	TEXT_STATICINFO,
	TEXT_INPUTSRC,
	TITLE_INPUTSRC,
	TEXT_ERROR,
	TEXTE_LOADSRC,
	TITLE_LPM,

	guiStrCount
};

#endif