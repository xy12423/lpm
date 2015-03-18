#include "stdafx.h"

#ifdef _LPM_GUI

#include "main.h"
#include "main_GUI.h"

wxBEGIN_EVENT_TABLE(mainFrame, wxFrame)
EVT_LISTBOX(ID_LISTSRC, mainFrame::listSrc_SelectedIndexChanged)
EVT_BUTTON(ID_BUTTONADDSRC, mainFrame::buttonAddSrc_Click)
EVT_BUTTON(ID_BUTTONDELSRC, mainFrame::buttonDelSrc_Click)
wxEND_EVENT_TABLE()

void getSrcNameList(wxArrayString &ret)
{
	srcListTp::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
	for (; pSrc != pSrcEnd; pSrc++)
		ret.push_back((*pSrc)->add);
}

void getPakNameList(source *src, wxArrayString &ret)
{
	pakListTp::const_iterator pPak = src->pkgList.cbegin(), pPakEnd = src->pkgList.cend();
	for (; pPak != pPakEnd; pPak++)
		ret.push_back((*pPak)->getName());
}

mainFrame::mainFrame(const wxString& title)
	: wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(640, 480))
{
	Centre();
	wxPanel *panel = new wxPanel(this);
	
	staticSrc = new wxStaticBox(panel, ID_STATICSRC,
		wxT("软件源"),
		wxPoint(12, 12),
		wxSize(224, 417)
		);

	wxArrayString srcList;
	getSrcNameList(srcList);
	listSrc = new wxListBox(staticSrc, ID_LISTSRC,
		wxPoint(6, 20),
		wxSize(212, 364),
		srcList
		);
	buttonAddSrc = new wxButton(staticSrc, ID_BUTTONADDSRC,
		wxT("添加软件源"),
		wxPoint(6, 387),
		wxSize(105, 24)
		);
	buttonDelSrc = new wxButton(staticSrc, ID_BUTTONDELSRC,
		wxT("删除软件源"),
		wxPoint(113, 387),
		wxSize(105, 24)
		);


}

void mainFrame::listSrc_SelectedIndexChanged(wxCommandEvent& event)
{

}

void mainFrame::buttonAddSrc_Click(wxCommandEvent& event)
{

}

void mainFrame::buttonDelSrc_Click(wxCommandEvent& event)
{

}

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	if (readConfig() == false)
	{
		MessageBox(NULL, _T("Failed to load config"), _T("ERROR"), MB_OK | MB_ICONERROR);
		return false;
	}
	checkPath();
	if (readSource() == false)
	{
		MessageBox(NULL, _T("Failed to load source info"), _T("ERROR"), MB_OK | MB_ICONERROR);
		return false;
	}

	mainFrame *frm = new mainFrame(wxT("Live Package Manager"));
	frm->Show();

	return true;
}


#endif
