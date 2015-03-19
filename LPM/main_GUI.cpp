#include "stdafx.h"

#ifdef _LPM_GUI

#include "main.h"
#include "main_GUI.h"

wxBEGIN_EVENT_TABLE(mainFrame, wxFrame)
EVT_CHECKLISTBOX(ID_LISTSRC, mainFrame::listSrc_ItemCheck)
EVT_BUTTON(ID_BUTTONADDSRC, mainFrame::buttonAddSrc_Click)
EVT_BUTTON(ID_BUTTONDELSRC, mainFrame::buttonDelSrc_Click)
EVT_BUTTON(ID_BUTTONUPDSRC, mainFrame::buttonUpdSrc_Click)
wxEND_EVENT_TABLE()

std::stringstream outstream;
pakListTp pakList;

void getSrcNameList(wxArrayString &ret)
{
	srcListTp::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
	for (; pSrc != pSrcEnd; pSrc++)
		ret.push_back((*pSrc)->add);
}

void getPakList(source *src, pakListTp &ret)
{
	pakListTp::const_iterator pPak = src->pkgList.cbegin(), pPakEnd = src->pkgList.cend();
	for (; pPak != pPakEnd; pPak++)
		ret.push_back(*pPak);
}

mainFrame::mainFrame(const wxString& title)
	: wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(640, 640))
{
	Centre();
	wxPanel *panel = new wxPanel(this);
	
	staticSrc = new wxStaticBox(panel, ID_STATICSRC,
		wxT("源"),
		wxPoint(12, 12),
		wxSize(224, 417)
		);

	wxArrayString srcList;
	getSrcNameList(srcList);
	listSrc = new wxCheckListBox(staticSrc, ID_LISTSRC,
		wxPoint(6, 20),
		wxSize(212, 364),
		srcList
		);
	buttonAddSrc = new wxButton(staticSrc, ID_BUTTONADDSRC,
		wxT("添加"),
		wxPoint(6, 387),
		wxSize(66, 24)
		);
	buttonDelSrc = new wxButton(staticSrc, ID_BUTTONDELSRC,
		wxT("删除"),
		wxPoint(78, 387),
		wxSize(66, 24)
		);
	buttonUpdSrc = new wxButton(staticSrc, ID_BUTTONUPDSRC,
		wxT("更新"),
		wxPoint(150, 387),
		wxSize(66, 24)
		);

	staticPak = new wxStaticBox(panel, ID_STATICPAK,
		wxT("包列表"),
		wxPoint(240, 12),
		wxSize(372, 417)
		);

	std::cout.rdbuf(outstream.rdbuf());

	listPak = new wxCheckListBox(staticPak, ID_LISTPAK,
		wxPoint(6, 20),
		wxSize(358, 260)
		);
	buttonAddPak = new wxButton(staticPak, ID_BUTTONADDPAK,
		wxT("添加"),
		wxPoint(6, 286),
		wxSize(85, 24)
		);
	buttonRemPak = new wxButton(staticPak, ID_BUTTONDELPAK,
		wxT("删除"),
		wxPoint(97, 286),
		wxSize(85, 24)
		);
	buttonUpgPak = new wxButton(staticPak, ID_BUTTONUPGPAK,
		wxT("更新"),
		wxPoint(188, 286),
		wxSize(85, 24)
		);
	buttonUpgAll = new wxButton(staticPak, ID_BUTTONUPGALL,
		wxT("全部更新"),
		wxPoint(279, 286),
		wxSize(85, 24)
		);

	staticInfo = new wxStaticBox(panel, ID_STATICINFO,
		wxT("信息"),
		wxPoint(12, 435),
		wxSize(600, 154)
		);
	textInfo = new wxTextCtrl(staticInfo, ID_TEXTINFO,
		wxEmptyString,
		wxPoint(6, 20),
		wxSize(586, 128),
		wxTE_MULTILINE | wxTE_READONLY
		);
}

void mainFrame::listSrc_ItemCheck(wxCommandEvent& event)
{
	wxArrayInt sel;
	listSrc->GetCheckedItems(sel);
	pakList.clear();
	wxArrayInt::iterator pItr, pEnd = sel.end();
	for (pItr = sel.begin(); pItr != pEnd; pItr++)
		getPakList(sourceList[*pItr], pakList);
	pakListTp::const_iterator itrPak = pakList.cbegin(), itrPakEnd = pakList.cend();
	listPak->Clear();
	for (; itrPak != itrPakEnd; itrPak++)
		listPak->AppendString((*itrPak)->getName());
}

void mainFrame::buttonAddSrc_Click(wxCommandEvent& event)
{
	wxTextEntryDialog inputDlg(this, wxT("输入源地址"), wxT("添加源"));
	inputDlg.ShowModal();
	wxString src = inputDlg.GetValue();
	if (src != wxT(""))
	{
		listSrc->AppendString(src);
		sourceList.push_back(new source(src.ToStdString()));
	}
}

void mainFrame::buttonDelSrc_Click(wxCommandEvent& event)
{
	int srcIndex = listSrc->GetSelection();
	srcListTp::const_iterator pItr = sourceList.cbegin();
	for (; srcIndex > 0; srcIndex--)
		pItr++;
	sourceList.erase(pItr);
	listSrc->Delete(srcIndex + 1);
}

void mainFrame::buttonUpdSrc_Click(wxCommandEvent& event)
{
	update();
	std::string buf;
	while (!outstream.eof())
	{
		std::getline(outstream, buf);
		buf.push_back('\n');
		textInfo->AppendText(buf);
	}
	outstream.clear();
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
