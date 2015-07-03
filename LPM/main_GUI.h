#pragma once

#if (!defined _H_MAIN_GUI) && (defined _LPM_GUI)
#define _H_MAIN_GUI

class mainFrame : public wxFrame
{
public:
	mainFrame(const wxString& title);
	enum itemID{
		ID_STATICPAK,
		ID_LISTPAK, ID_CHECKUPD, ID_CHECKINST, ID_TEXTSEARCH, ID_CHECKFORCE,
		ID_BUTTONADDPAK, ID_BUTTONDELPAK, ID_BUTTONUPGPAK, ID_BUTTONUPGALL,

		ID_STATICINFO,
		ID_TEXTFNAME, ID_TEXTNAME, ID_TEXTINFO, ID_TEXTAUTHOR, ID_TEXTVERSION, ID_TEXTDEP, ID_TEXTCONF,

		ID_STATICSRC,
		ID_LISTSRC, ID_BUTTONADDSRC, ID_BUTTONDELSRC, ID_BUTTONUPDSRC,

		ID_STATICOUTPUT, ID_TEXTOUTPUT, ID_GAUGEPROGRESS
	};
	void refreshPakList();

	wxPanel *panel;

	wxStaticBox *staticPak;

	wxCheckListBox *listPak;
	void listPak_SelectedIndexChanged(wxCommandEvent& event);

	wxCheckBox *checkUpd, *checkInst, *checkForce;
	wxTextCtrl *textSearch;
	void checkUpd_CheckedChanged(wxCommandEvent& event);
	void checkInst_CheckedChanged(wxCommandEvent& event);
	void textSearch_TextChanged(wxCommandEvent& event);

	wxButton *buttonAddPak, *buttonRemPak, *buttonUpgPak, *buttonUpgAll;
	void buttonAddPak_Click(wxCommandEvent& event);
	void buttonDelPak_Click(wxCommandEvent& event);
	void buttonUpgPak_Click(wxCommandEvent& event);
	void buttonUpgAll_Click(wxCommandEvent& event);

	wxStaticBox *staticInfo;

	wxTextCtrl *textFName, *textName, *textInfo, *textAuthor, *textVersion, *textDep, *textConf;

	wxStaticBox *staticSrc;

	wxCheckListBox *listSrc;
	wxButton *buttonAddSrc, *buttonDelSrc, *buttonUpdSrc;
	void listSrc_ItemCheck(wxCommandEvent& event);
	void buttonAddSrc_Click(wxCommandEvent& event);
	void buttonDelSrc_Click(wxCommandEvent& event);
	void buttonUpdSrc_Click(wxCommandEvent& event);

	wxStaticBox *staticOutput;
	wxTextCtrl *textOutput;
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
#ifdef NDEBUG
	virtual void OnUnhandledException();
#endif
};

enum guiStrID{
	TEXT_STATICPAK,

	TEXT_CHECKUPD,
	TEXT_CHECKINST,
	TEXT_CHECKFORCE,
	TEXT_LABELSEARCH,

	TEXT_BUTTONADDPAK,
	TEXT_BUTTONDELPAK,
	TEXT_BUTTONUPGPAK,
	TEXT_BUTTONUPGALL,

	TEXT_STATICINFO,

	TEXT_LABELFNAME,
	TEXT_LABELNAME,
	TEXT_LABELINFO,
	TEXT_LABELAUTHOR,
	TEXT_LABELVERSION,
	TEXT_LABELDEP,
	TEXT_LABELCONF,

	TEXT_STATICSRC,

	TEXT_BUTTONADDSRC,
	TEXT_BUTTONDELSRC,
	TEXT_BUTTONUPDSRC,

	TEXT_STATICOUTPUT,

	TEXT_INPUTSRC,
	TITLE_INPUTSRC,
	TEXT_ERROR,
	TEXT_INFO,
	TEXTE_LOADSRC,
	TITLE_LPM,

	guiStrCount
};

#endif
