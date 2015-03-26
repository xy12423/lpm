#include "stdafx.h"

#ifdef _LPM_GUI

#include "main.h"
#include "main_GUI.h"
#include "unzip.h"

wxBEGIN_EVENT_TABLE(mainFrame, wxFrame)

EVT_CHECKLISTBOX(ID_LISTSRC, mainFrame::listSrc_ItemCheck)
EVT_BUTTON(ID_BUTTONADDSRC, mainFrame::buttonAddSrc_Click)
EVT_BUTTON(ID_BUTTONDELSRC, mainFrame::buttonDelSrc_Click)
EVT_BUTTON(ID_BUTTONUPDSRC, mainFrame::buttonUpdSrc_Click)

EVT_CHECKBOX(ID_CHECKUPD, mainFrame::checkUpd_CheckedChanged)
EVT_CHECKBOX(ID_CHECKINST, mainFrame::checkInst_CheckedChanged)
EVT_TEXT(ID_TEXTSEARCH, mainFrame::textSearch_TextChanged)

EVT_LISTBOX(ID_LISTPAK, mainFrame::listPak_SelectedIndexChanged)
EVT_BUTTON(ID_BUTTONADDPAK, mainFrame::buttonAddPak_Click)
EVT_BUTTON(ID_BUTTONDELPAK, mainFrame::buttonDelPak_Click)
EVT_BUTTON(ID_BUTTONUPGPAK, mainFrame::buttonUpgPak_Click)
EVT_BUTTON(ID_BUTTONUPGALL, mainFrame::buttonUpgAll_Click)

wxEND_EVENT_TABLE()

#ifdef __WXMSW__
#define _GUI_GAP 20
#define _GUI_SIZE_X 640
#define _GUI_SIZE_Y 640
#else
#define _GUI_GAP 0
#define _GUI_SIZE_X 600
#define _GUI_SIZE_Y 600
#endif

wxString guiStrDataDefault[guiStrCount] = {
	wxT("源"),
	wxT("添加"),
	wxT("删除"),
	wxT("更新"),
	wxT("包"),
	wxT("只显示可更新"),
	wxT("只显示已安装"),
	wxT("搜索"),
	wxT("添加"),
	wxT("删除"),
	wxT("更新"),
	wxT("全部更新"),
	wxT("信息"),
	wxT("输入源地址"),
	wxT("添加源"),
	wxT("ERROR"),
	wxT("Failed to load source info"),
	wxT("Live Package Manager"),
};
wxString guiStrData[guiStrCount];

pakListTp pakList;
mainFrame *form;
std::streambuf *coutBuf = std::cout.rdbuf();
int pakMask;

void loadDefaultGUILang()
{
	for (int i = 0; i < guiStrCount; i++)
		guiStrData[i] = guiStrDataDefault[i];
}

bool readGUILang()
{
	namespace fs = boost::filesystem;
	fs::path guiLangPath(langPath.string() + "-gui");
	if (!fs::exists(guiLangPath) || fs::is_directory(guiLangPath))
		return false;
	std::ifstream fin(guiLangPath.string());
	if (!fin.is_open())
		return false;
	std::string tmp;
	for (int i = 0; i < guiStrCount; i++)
	{
		if (fin.eof())
			return false;
		std::getline(fin, tmp);
		guiStrData[i] = tmp;
	}
	fin.close();
	return true;
}

void printInfo(package *pkg)
{
	if (pkg == NULL)
		return;
	std::stringstream sstream;
	sstream << "Name:" << pkg->extInfo.fname << std::endl;
	sstream << "Package:" << pkg->name << std::endl;
	sstream << "Description:" << pkg->extInfo.info << std::endl;
	sstream << "Author:" << pkg->extInfo.author << std::endl;
	sstream << "Version:" << pkg->ver.major << '.' << pkg->ver.minor << '.' << pkg->ver.revision << std::endl;
	sstream << "Required:";
	std::for_each(pkg->depList.begin(), pkg->depList.end(), [&sstream](std::string pkgName){
		sstream << pkgName << ';';
	});
	sstream << std::endl << "Conflict:";
	std::for_each(pkg->confList.begin(), pkg->confList.end(), [&sstream](std::string pkgName){
		sstream << pkgName << ';';
	});
	sstream << std::endl;
	sstream << "Is installed:";
	if (is_installed(pkg->name))
		sstream << "Y";
	else
		sstream << "N";
	sstream << std::endl;
	sstream << std::endl;
	form->labelInfo->SetLabelText(sstream.str());
}

int lastProgress = -1;
void reportProgress(double progress)
{
	if (static_cast<int>(progress) != lastProgress)
	{
		form->gaugeProgress->SetValue(progress);
		lastProgress = static_cast<int>(progress);
	}
	return ;
}

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
	: wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(_GUI_SIZE_X, _GUI_SIZE_Y))
{
	Centre();
	panel = new wxPanel(this);
	
	staticSrc = new wxStaticBox(panel, ID_STATICSRC,
		guiStrData[TEXT_STATICSRC],
		wxPoint(12, 12),
		wxSize(224, 417)
		);

	wxArrayString srcList;
	getSrcNameList(srcList);
	listSrc = new wxCheckListBox(staticSrc, ID_LISTSRC,
		wxPoint(6, _GUI_GAP),
		wxSize(212, 364),
		srcList
		);
	int count = sourceList.size();
	for (int i = 0; i < count; i++)
		listSrc->Check(i);
	buttonAddSrc = new wxButton(staticSrc, ID_BUTTONADDSRC,
		guiStrData[TEXT_BUTTONADDSRC],
		wxPoint(6, 367 + _GUI_GAP),
		wxSize(66, 24)
		);
	buttonDelSrc = new wxButton(staticSrc, ID_BUTTONDELSRC,
		guiStrData[TEXT_BUTTONDELSRC],
		wxPoint(78, 367 + _GUI_GAP),
		wxSize(66, 24)
		);
	buttonUpdSrc = new wxButton(staticSrc, ID_BUTTONUPDSRC,
		guiStrData[TEXT_BUTTONUPDSRC],
		wxPoint(150, 367 + _GUI_GAP),
		wxSize(66, 24)
		);

	staticPak = new wxStaticBox(panel, ID_STATICPAK,
		guiStrData[TEXT_STATICPAK],
		wxPoint(240, 12),
		wxSize(372, 417)
		);

	checkUpd = new wxCheckBox(staticPak, ID_CHECKUPD,
		guiStrData[TEXT_CHECKUPD],
		wxPoint(6, _GUI_GAP),
		wxSize(96, 16)
		);
	checkInst = new wxCheckBox(staticPak, ID_CHECKINST,
		guiStrData[TEXT_CHECKINST],
		wxPoint(108, _GUI_GAP),
		wxSize(96, 16)
		);
	labelSearch = new wxStaticText(staticPak, ID_LABELSEARCH,
		guiStrData[TEXT_LABELSEARCH],
		wxPoint(210, _GUI_GAP),
		wxSize(29, 16)
		);
	textSearch = new wxTextCtrl(staticPak, ID_TEXTSEARCH,
		wxEmptyString,
		wxPoint(245, _GUI_GAP - 2),
		wxSize(119, 21)
		);

	listPak = new wxCheckListBox(staticPak, ID_LISTPAK,
		wxPoint(6, _GUI_GAP + 22),
		wxSize(358, 196)
		);
	buttonAddPak = new wxButton(staticPak, ID_BUTTONADDPAK,
		guiStrData[TEXT_BUTTONADDPAK],
		wxPoint(6, _GUI_GAP + 229),
		wxSize(85, 24)
		);
	buttonRemPak = new wxButton(staticPak, ID_BUTTONDELPAK,
		guiStrData[TEXT_BUTTONDELPAK],
		wxPoint(97, _GUI_GAP + 229),
		wxSize(85, 24)
		);
	buttonUpgPak = new wxButton(staticPak, ID_BUTTONUPGPAK,
		guiStrData[TEXT_BUTTONUPGPAK],
		wxPoint(188, _GUI_GAP + 229),
		wxSize(85, 24)
		);
	buttonUpgAll = new wxButton(staticPak, ID_BUTTONUPGALL,
		guiStrData[TEXT_BUTTONUPGALL],
		wxPoint(279, _GUI_GAP + 229),
		wxSize(85, 24)
		);
	labelInfo = new wxStaticText(staticPak, ID_LABELINFO,
		wxEmptyString,
		wxPoint(6, _GUI_GAP + 259),
		wxSize(358, 132)
		);

	staticInfo = new wxStaticBox(panel, ID_STATICINFO,
		guiStrData[TEXT_STATICINFO],
		wxPoint(12, 435),
		wxSize(600, 154)
		);
	textInfo = new wxTextCtrl(staticInfo, ID_TEXTINFO,
		wxEmptyString,
		wxPoint(6, _GUI_GAP),
		wxSize(586, 105),
		wxTE_MULTILINE | wxTE_READONLY
		);
	gaugeProgress = new wxGauge(staticInfo, ID_GAUGEPROGRESS,
		100,
		wxPoint(6, _GUI_GAP + 111),
		wxSize(586, 17)
		);

	std::cout.rdbuf(textInfo);
	prCallbackP = reportProgress;
	refreshPakList();
}

void mainFrame::refreshPakList()
{
	wxArrayInt sel;
	listSrc->GetCheckedItems(sel);
	pakList.clear();
	wxArrayInt::iterator pItr, pEnd = sel.end();
	for (pItr = sel.begin(); pItr != pEnd; pItr++)
		getPakList(sourceList[*pItr], pakList);
	pakListTp::iterator itrPak = pakList.begin(), itrPakEnd = pakList.end();
	std::string maskName = textSearch->GetLineText(0);
	bool enableSearch = !maskName.empty();
	for (std::string name; itrPak != itrPakEnd;)
	{
		name = (*itrPak)->getName();
		if ((getState(name) & pakMask) != pakMask || (enableSearch && name.find(maskName) == std::string::npos))
		{
			itrPak = pakList.erase(itrPak);
			itrPakEnd = pakList.end();
		}
		else
			itrPak++;
	}
	itrPak = pakList.begin();
	listPak->Clear();
	for (; itrPak != itrPakEnd; itrPak++)
		listPak->AppendString((*itrPak)->getExtInfo().fname);
}

void mainFrame::listSrc_ItemCheck(wxCommandEvent& event)
{
	refreshPakList();
}

void mainFrame::buttonAddSrc_Click(wxCommandEvent& event)
{
	wxTextEntryDialog inputDlg(this, guiStrData[TEXT_INPUTSRC], guiStrData[TITLE_INPUTSRC]);
	inputDlg.ShowModal();
	wxString src = inputDlg.GetValue();
	if (src != wxEmptyString)
	{
		listSrc->AppendString(src);
		source *newSrc = new source(src.ToStdString());
		newSrc->loadRemote();
		sourceList.push_back(newSrc);
		listSrc->Check(listSrc->GetCount() - 1);
		refreshPakList();
	}
	writeSource();
}

void mainFrame::buttonDelSrc_Click(wxCommandEvent& event)
{
	int srcIndex = listSrc->GetSelection();
	srcListTp::iterator pItr = sourceList.begin();
	for (; srcIndex > 0; srcIndex--)
		pItr++;
	sourceList.erase(pItr);
	listSrc->Delete(srcIndex + 1);
	refreshPakList();
	writeSource();
}

void mainFrame::buttonUpdSrc_Click(wxCommandEvent& event)
{
	textInfo->Clear();
	update();
	refreshPakList();
	writeSource();
}

void mainFrame::checkUpd_CheckedChanged(wxCommandEvent& event)
{
	if (event.IsChecked())
		pakMask |= PAK_STATE_NEED_UPGRADE;
	else
		pakMask &= (~PAK_STATE_NEED_UPGRADE);
	refreshPakList();
}

void mainFrame::checkInst_CheckedChanged(wxCommandEvent& event)
{
	if (event.IsChecked())
		pakMask |= PAK_STATE_INSTALLED;
	else
		pakMask &= (~PAK_STATE_INSTALLED);
	refreshPakList();
}

void mainFrame::textSearch_TextChanged(wxCommandEvent& event)
{
	refreshPakList();
}

void mainFrame::listPak_SelectedIndexChanged(wxCommandEvent& event)
{
	printInfo(pakList[listPak->GetSelection()]);
}

void mainFrame::buttonAddPak_Click(wxCommandEvent& event)
{
	textInfo->Clear();
	wxArrayInt sel;
	listPak->GetCheckedItems(sel);
	wxArrayInt::iterator pItr, pEnd = sel.end();
	for (pItr = sel.begin(); pItr != pEnd; pItr++)
	{
		infoStream << msgData[MSGI_PAK_INSTALLING] << ':' << pakList[*pItr]->getName() << std::endl;
		errInfo err = pakList[*pItr]->instFull();
		if (err.err)
			infoStream << err.info << std::endl;
	}
	if (checkInst->GetValue())
		refreshPakList();
}

void mainFrame::buttonDelPak_Click(wxCommandEvent& event)
{
	textInfo->Clear();
	wxArrayInt sel;
	listPak->GetCheckedItems(sel);
	wxArrayInt::iterator pItr, pEnd = sel.end();
	std::string name;
	for (pItr = sel.begin(); pItr != pEnd; pItr++)
	{
		name = pakList[*pItr]->getName();
		infoStream << msgData[MSGI_PAK_REMOVING] << ':' << name << std::endl;
		errInfo err = uninstall(name);
		if (err.err)
			infoStream << err.info << std::endl;
	}
	if (checkInst->GetValue())
		refreshPakList();
}

void mainFrame::buttonUpgPak_Click(wxCommandEvent& event)
{
	textInfo->Clear();
	wxArrayInt sel;
	listPak->GetCheckedItems(sel);
	wxArrayInt::iterator pItr, pEnd = sel.end();
	for (pItr = sel.begin(); pItr != pEnd; pItr++)
	{
		if (pakList[*pItr]->needUpgrade())
		{
			infoStream << msgData[MSGI_PAK_UPGRADING] << ':' << pakList[*pItr]->getName() << std::endl;
			errInfo err = pakList[*pItr]->upgrade(true);
			if (err.err)
				infoStream << err.info << std::endl;
		}
	}
}

void mainFrame::buttonUpgAll_Click(wxCommandEvent& event)
{
	textInfo->Clear();
	pakListTp::const_iterator itrPak = pakList.cbegin(), itrPakEnd = pakList.cend();
	for (; itrPak != itrPakEnd; itrPak++)
	{
		if ((*itrPak)->needUpgrade())
		{
			infoStream << msgData[MSGI_PAK_UPGRADING] << (*itrPak)->getName() << std::endl;
			errInfo err = (*itrPak)->upgrade(true);
			if (err.err)
				infoStream << err.info << std::endl;
		}
	}
}

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	if (readConfig() == false)
		init();
	checkPath();
	if (readLang() == false)
		loadDefaultLang();
	if (readGUILang() == false)
		loadDefaultGUILang();
	if (readSource() == false)
	{
		wxMessageBox(guiStrData[TEXTE_LOADSRC], guiStrData[TEXT_ERROR], wxOK | wxICON_ERROR);
		return false;
	}

	form = new mainFrame(guiStrData[TITLE_LPM]);
	form->Show();

	return true;
}

int MyApp::OnExit()
{
	srcListTp::const_iterator itrSrc = sourceList.cbegin(), itrSrcEnd = sourceList.cend();
	for (; itrSrc != itrSrcEnd; itrSrc++)
		delete *itrSrc;
	std::cout.rdbuf(coutBuf);

	return EXIT_SUCCESS;
}

#endif
