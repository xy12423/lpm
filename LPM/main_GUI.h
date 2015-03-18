#pragma once

#ifndef _H_MAIN_GUI
#define _H_MAIN_GUI

class mainFrame : public wxFrame
{
public:
	mainFrame(const wxString& title);
	enum itemID{ ID_STATICSRC, ID_LISTSRC, ID_BUTTONADDSRC, ID_BUTTONDELSRC };

	wxStaticBox *staticSrc;

	wxListBox *listSrc;
	wxButton *buttonAddSrc, *buttonDelSrc;
	void listSrc_SelectedIndexChanged(wxCommandEvent& event);
	void buttonAddSrc_Click(wxCommandEvent& event);
	void buttonDelSrc_Click(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
};

#endif