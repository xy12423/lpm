#pragma once

#ifndef _H_MAIN_GUI
#define _H_MAIN_GUI

class mainFrame : public wxFrame
{
public:
	mainFrame(const wxString& title);
	enum itemID{
		ID_STATICSRC, ID_LISTSRC, ID_BUTTONADDSRC, ID_BUTTONDELSRC, ID_BUTTONUPDSRC,
		ID_STATICPAK, ID_CHECKUPD, ID_CHECKINST, ID_LISTPAK, ID_BUTTONADDPAK, ID_BUTTONDELPAK, ID_BUTTONUPGPAK, ID_BUTTONUPGALL, ID_LABELINFO,
		ID_STATICINFO, ID_TEXTINFO
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
	wxCheckListBox *listPak;
	wxButton *buttonAddPak, *buttonRemPak, *buttonUpgPak, *buttonUpgAll;
	wxStaticText *labelInfo;
	void checkUpd_CheckedChanged(wxCommandEvent& event);
	void checkInst_CheckedChanged(wxCommandEvent& event);
	void listPak_SelectedIndexChanged(wxCommandEvent& event);
	void buttonAddPak_Click(wxCommandEvent& event);
	void buttonDelPak_Click(wxCommandEvent& event);
	void buttonUpgPak_Click(wxCommandEvent& event);
	void buttonUpgAll_Click(wxCommandEvent& event);

	wxStaticBox *staticInfo;
	wxTextCtrl *textInfo;

	friend std::ostream& myEndl(std::ostream& os);
	friend void printInfo(package *pkg);

	wxDECLARE_EVENT_TABLE();
};

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int OnExit();
};

#endif