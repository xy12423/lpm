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
#define _GUI_SIZE_X 626
#define _GUI_SIZE_Y 636
#else
#define _GUI_GAP 0
#define _GUI_SIZE_X 620
#define _GUI_SIZE_Y 600
#endif

wxString guiStrDataDefault[guiStrCount] = {
	wxT("软件包"),
	wxT("可更新的"),
	wxT("已安装的"),
	wxT("强制执行"),
	wxT("搜索"),
	wxT("安装"),
	wxT("移除"),
	wxT("升级"),
	wxT("升级全部"),
	wxT("信息"),
	wxT("名称"),
	wxT("包名"),
	wxT("信息"),
	wxT("作者"),
	wxT("版本"),
	wxT("依赖"),
	wxT("冲突"),
	wxT("软件源"),
	wxT("+"),
	wxT("-"),
	wxT("检查软件更新"),
	wxT("输出"),
	wxT("输入源地址"),
	wxT("添加源"),
	wxT("Info"),
	wxT("Error"),
	wxT("Failed to load source info"),
	wxT("Live Package Manager"),
};
wxString guiStrData[guiStrCount];

pakListTp pakList;
mainFrame *form;
std::streambuf *coutBuf = std::cout.rdbuf();
int pakMask;

std::string lcase(const std::string &str)
{
	std::string ret;
	std::for_each(str.begin(), str.end(), [&ret](char ch){
		ret.push_back(tolower(ch));
	});
	return ret;
}

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
	form->textFName->SetValue(pkg->extInfo.fname);
	form->textName->SetValue(pkg->name);
	form->textInfo->SetValue(pkg->extInfo.info);
	form->textAuthor->SetValue(pkg->extInfo.author);
	form->textVersion->SetValue(std::to_string(pkg->ver.major) + '.' + std::to_string(pkg->ver.minor) + '.' + std::to_string(pkg->ver.revision));
	form->textDep->Clear();
	std::for_each(pkg->depList.begin(), pkg->depList.end(), [&sstream](depInfo dpInf){
		form->textDep->AppendText(dpInf.fullStr());
	});
	form->textConf->Clear();
	std::for_each(pkg->confList.begin(), pkg->confList.end(), [&sstream](depInfo dpInf){
		form->textConf->AppendText(dpInf.fullStr());
	});
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
	pakListTp::const_iterator pPak = src->pakList.cbegin(), pPakEnd = src->pakList.cend();
	for (; pPak != pPakEnd; pPak++)
		ret.push_back(*pPak);
}

mainFrame::mainFrame(const wxString& title)
	: wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(_GUI_SIZE_X, _GUI_SIZE_Y))
{
	Center();
	panel = new wxPanel(this);

	wxStaticText *label;

	staticPak = new wxStaticBox(panel, ID_STATICPAK,
		guiStrData[TEXT_STATICPAK],
		wxPoint(12, 12),
		wxSize(588, 340)
		);

	listPak = new wxCheckListBox(staticPak, ID_LISTPAK,
		wxPoint(6, _GUI_GAP),
		wxSize(224, 292)
		);
	checkUpd = new wxCheckBox(staticPak, ID_CHECKUPD,
		guiStrData[TEXT_CHECKUPD],
		wxPoint(6, _GUI_GAP + 298),
		wxSize(96, 16)
		);
	checkInst = new wxCheckBox(staticPak, ID_CHECKINST,
		guiStrData[TEXT_CHECKINST],
		wxPoint(112, _GUI_GAP + 298),
		wxSize(96, 16)
		);
	label = new wxStaticText(staticPak, wxID_ANY,
		guiStrData[TEXT_LABELSEARCH],
		wxPoint(246, _GUI_GAP),
		wxSize(32, 16)
		);
	textSearch = new wxTextCtrl(staticPak, ID_TEXTSEARCH,
		wxEmptyString,
		wxPoint(281, _GUI_GAP - 2),
		wxSize(223, 21)
		);
	checkForce = new wxCheckBox(staticPak, ID_CHECKFORCE,
		guiStrData[TEXT_CHECKFORCE],
		wxPoint(510, _GUI_GAP + 2),
		wxSize(96, 16)
		);
	
	buttonAddPak = new wxButton(staticPak, ID_BUTTONADDPAK,
		guiStrData[TEXT_BUTTONADDPAK],
		wxPoint(248, _GUI_GAP + 290),
		wxSize(79, 24)
		);
	buttonRemPak = new wxButton(staticPak, ID_BUTTONDELPAK,
		guiStrData[TEXT_BUTTONDELPAK],
		wxPoint(333, _GUI_GAP + 290),
		wxSize(79, 24)
		);
	buttonUpgPak = new wxButton(staticPak, ID_BUTTONUPGPAK,
		guiStrData[TEXT_BUTTONUPGPAK],
		wxPoint(418, _GUI_GAP + 290),
		wxSize(79, 24)
		);
	buttonUpgAll = new wxButton(staticPak, ID_BUTTONUPGALL,
		guiStrData[TEXT_BUTTONUPGALL],
		wxPoint(503, _GUI_GAP + 290),
		wxSize(79, 24)
		);

	staticInfo = new wxStaticBox(staticPak, ID_STATICINFO,
		guiStrData[TEXT_STATICINFO],
		wxPoint(248, _GUI_GAP + 27),
		wxSize(334, 257)
		);

	label = new wxStaticText(staticInfo, wxID_ANY,
		guiStrData[TEXT_LABELFNAME],
		wxPoint(6, _GUI_GAP),
		wxSize(32, 16)
		);
	textFName = new wxTextCtrl(staticInfo, ID_TEXTFNAME,
		wxEmptyString,
		wxPoint(56, _GUI_GAP),
		wxSize(272, 21),
		wxTE_READONLY
		);
	label = new wxStaticText(staticInfo, wxID_ANY,
		guiStrData[TEXT_LABELNAME],
		wxPoint(6, _GUI_GAP + 27),
		wxSize(32, 16)
		);
	textName = new wxTextCtrl(staticInfo, ID_TEXTNAME,
		wxEmptyString,
		wxPoint(56, _GUI_GAP + 27),
		wxSize(272, 21),
		wxTE_READONLY
		);
	label = new wxStaticText(staticInfo, wxID_ANY,
		guiStrData[TEXT_LABELINFO],
		wxPoint(6, _GUI_GAP + 54),
		wxSize(32, 16)
		);
	textInfo = new wxTextCtrl(staticInfo, ID_TEXTINFO,
		wxEmptyString,
		wxPoint(56, _GUI_GAP + 54),
		wxSize(272, 96),
		wxTE_MULTILINE | wxTE_READONLY
		);
	label = new wxStaticText(staticInfo, wxID_ANY,
		guiStrData[TEXT_LABELAUTHOR],
		wxPoint(6, _GUI_GAP + 156),
		wxSize(32, 16)
		);
	textAuthor = new wxTextCtrl(staticInfo, ID_TEXTAUTHOR,
		wxEmptyString,
		wxPoint(56, _GUI_GAP + 156),
		wxSize(108, 21),
		wxTE_READONLY
		);
	label = new wxStaticText(staticInfo, wxID_ANY,
		guiStrData[TEXT_LABELVERSION],
		wxPoint(170, _GUI_GAP + 156),
		wxSize(32, 16)
		);
	textVersion = new wxTextCtrl(staticInfo, ID_TEXTVERSION,
		wxEmptyString,
		wxPoint(220, _GUI_GAP + 156),
		wxSize(108, 21),
		wxTE_READONLY
		);
	label = new wxStaticText(staticInfo, wxID_ANY,
		guiStrData[TEXT_LABELDEP],
		wxPoint(6, _GUI_GAP + 183),
		wxSize(32, 16)
		);
	textDep = new wxTextCtrl(staticInfo, ID_TEXTDEP,
		wxEmptyString,
		wxPoint(56, _GUI_GAP + 183),
		wxSize(272, 21),
		wxTE_READONLY
		);
	label = new wxStaticText(staticInfo, wxID_ANY,
		guiStrData[TEXT_LABELCONF],
		wxPoint(6, _GUI_GAP + 210),
		wxSize(32, 16)
		);
	textConf = new wxTextCtrl(staticInfo, ID_TEXTCONF,
		wxEmptyString,
		wxPoint(56, _GUI_GAP + 210),
		wxSize(272, 21),
		wxTE_READONLY
		);

	staticSrc = new wxStaticBox(panel, ID_STATICSRC,
		guiStrData[TEXT_STATICSRC],
		wxPoint(12, 358),
		wxSize(236, 204)
		);

	wxArrayString srcList;
	getSrcNameList(srcList);
	listSrc = new wxCheckListBox(staticSrc, ID_LISTSRC,
		wxPoint(6, _GUI_GAP),
		wxSize(224, 148),
		srcList
		);
	int count = sourceList.size();
	for (int i = 0; i < count; i++)
		listSrc->Check(i);
	buttonAddSrc = new wxButton(staticSrc, ID_BUTTONADDSRC,
		guiStrData[TEXT_BUTTONADDSRC],
		wxPoint(6, 154 + _GUI_GAP),
		wxSize(35, 24)
		);
	buttonDelSrc = new wxButton(staticSrc, ID_BUTTONDELSRC,
		guiStrData[TEXT_BUTTONDELSRC],
		wxPoint(195, 154 + _GUI_GAP),
		wxSize(35, 24)
		);
	buttonUpdSrc = new wxButton(staticSrc, ID_BUTTONUPDSRC,
		guiStrData[TEXT_BUTTONUPDSRC],
		wxPoint(47, 154 + _GUI_GAP),
		wxSize(142, 24)
		);

	staticOutput = new wxStaticBox(panel, ID_STATICOUTPUT,
		guiStrData[TEXT_STATICOUTPUT],
		wxPoint(254, 358),
		wxSize(346, 204)
		);
	textOutput = new wxTextCtrl(staticOutput, ID_TEXTOUTPUT,
		wxEmptyString,
		wxPoint(6, _GUI_GAP),
		wxSize(334, 178),
		wxTE_MULTILINE | wxTE_READONLY
		);

	gaugeProgress = new wxGauge(panel, ID_GAUGEPROGRESS,
		100,
		wxPoint(12, _GUI_GAP + 548),
		wxSize(588, 17)
		);

	std::cout.rdbuf(textOutput);
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
	for (; itrPak != itrPakEnd;)
	{
		if ((getState((*itrPak)->getName()) & pakMask) != pakMask || (enableSearch && lcase((*itrPak)->getExtInfo().fname).find(lcase(maskName)) == std::string::npos))
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
		listPak->AppendString((*itrPak)->getExtInfo().fname + '[' + (*itrPak)->getVer().toStr() + ']');
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

		pakListTp upgradeList;
		newSrc->checkUpgrade(upgradeList);
		if (upgradeList.empty())
		{
			infoStream << msgData[MSGI_NO_UPGRADE] << std::endl;
		}
		else
		{
			infoStream << msgData[MSGI_UPGRADE] << std::endl;
			std::for_each(upgradeList.begin(), upgradeList.end(), [](package *pak){
				infoStream << '\t' << pak->getName() << std::endl;
			});
		}
	}
	writeSource();
}

void mainFrame::buttonDelSrc_Click(wxCommandEvent& event)
{
	int srcIndex = listSrc->GetSelection();
	if (srcIndex == -1)
		return;
	srcListTp::iterator pItr = sourceList.begin();
	for (; srcIndex > 0; srcIndex--)
		pItr++;
	sourceList.erase(pItr);
	listSrc->Delete(srcIndex);
	refreshPakList();
	writeSource();
}

void mainFrame::buttonUpdSrc_Click(wxCommandEvent& event)
{
	textOutput->Clear();
	update();
	refreshPakList();
	writeSource();

	pakListTp upgradeList;
	checkUpgrade(upgradeList);
	if (upgradeList.empty())
	{
		infoStream << msgData[MSGI_NO_UPGRADE] << std::endl;
	}
	else
	{
		infoStream << msgData[MSGI_UPGRADE] << std::endl;
		std::for_each(upgradeList.begin(), upgradeList.end(), [](package *pak){
			infoStream << '\t' << pak->getName() << std::endl;
		});
	}
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
	textOutput->Clear();
	wxArrayInt sel;
	listPak->GetCheckedItems(sel);
	wxArrayInt::iterator pItr, pEnd = sel.end();
	for (pItr = sel.begin(); pItr != pEnd; pItr++)
	{
		if (is_installed(pakList[*pItr]->getName()))
			infoStream << msgData[MSGE_PAK_INSTALLED] << ':' << pakList[*pItr]->getName() << std::endl;
		else
		{
			infoStream << msgData[MSGI_PAK_INSTALLING] << ':' << pakList[*pItr]->getName() << std::endl;
			errInfo err = pakList[*pItr]->instFull(checkForce->GetValue());
			if (err.err)
				infoStream << err.info << std::endl;
		}
	}
	if (checkInst->GetValue())
		refreshPakList();
	checkForce->SetValue(false);
}

void mainFrame::buttonDelPak_Click(wxCommandEvent& event)
{
	textOutput->Clear();
	wxArrayInt sel;
	listPak->GetCheckedItems(sel);
	wxArrayInt::iterator pItr, pEnd = sel.end();
	std::string name;
	for (pItr = sel.begin(); pItr != pEnd; pItr++)
	{
		name = pakList[*pItr]->getName();
		errInfo err = uninstall(name, false, (checkForce->GetValue() ? REMOVE_RECURSIVE : REMOVE_NORMAL));
		if (err.err)
			infoStream << err.info << std::endl;
	}
	if (checkInst->GetValue())
		refreshPakList();

	checkForce->SetValue(false);
}

void mainFrame::buttonUpgPak_Click(wxCommandEvent& event)
{
	textOutput->Clear();
	wxArrayInt sel;
	listPak->GetCheckedItems(sel);
	wxArrayInt::iterator pItr, pEnd = sel.end();
	for (pItr = sel.begin(); pItr != pEnd; pItr++)
	{
		if (pakList[*pItr]->needUpgrade())
		{
			infoStream << msgData[MSGI_PAK_UPGRADING] << ':' << pakList[*pItr]->getName() << std::endl;
			errInfo err = pakList[*pItr]->upgrade();
			if (err.err)
				infoStream << err.info << std::endl;
		}
	}
}

void mainFrame::buttonUpgAll_Click(wxCommandEvent& event)
{
	textOutput->Clear();
	pakListTp::const_iterator itrPak = pakList.cbegin(), itrPakEnd = pakList.cend();
	for (; itrPak != itrPakEnd; itrPak++)
	{
		if ((*itrPak)->needUpgrade())
		{
			infoStream << msgData[MSGI_PAK_UPGRADING] << (*itrPak)->getName() << std::endl;
			errInfo err = (*itrPak)->upgrade();
			if (err.err)
				infoStream << err.info << std::endl;
		}
	}
}

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	try
	{
		int argp = 1;
		std::string cmd;
		std::string newPath, newLocal, newData;
		while (argp < argc)
		{
			cmd = argv[argp];
			if (cmd.front() != '-')
				break;
			cmd.erase(0, 1);
			switch (cmd.front())
			{
				case '-':
				{
					cmd.erase(0, 1);
					if (cmd.substr(0, 7) == "lpmdir=")
						newPath = cmd.substr(7);
					else if (cmd.substr(0, 6) == "local=")
						newLocal = cmd.substr(6);
					else if (cmd.substr(0, 5) == "data=")
						newData = cmd.substr(5);
					else
						throw(0);
					break;
				}
				default:
					throw(0);
			}
			argp++;
		}

		fs::path oldPath = fs::current_path();	//Save current path
		if (!newPath.empty())
			fs::current_path(newPath);	//Switch to new path to read config

		if (readConfig())
			checkPath();
		else
		{
			init();
			checkPath();
		}
		if (!newPath.empty())	//Switch back to old path to get absolute path of local path
			current_path(oldPath);

		if (!newLocal.empty())
			localPath = newLocal;
		localPath = system_complete(localPath);
		if (!newData.empty())
			dataPath = newData;
		if (!newPath.empty())
			fs::current_path(newPath);

		if (!readLang())
			loadDefaultLang();

		if (!newPath.empty())
			wxMessageBox(msgData[MSGI_USING_LPMDIR] + newPath, guiStrData[TEXT_INFO], wxOK | wxICON_INFORMATION);
		if (!newLocal.empty())
			wxMessageBox(msgData[MSGI_USING_LOCAL] + newLocal, guiStrData[TEXT_INFO], wxOK | wxICON_INFORMATION);

		if (readGUILang() == false)
			loadDefaultGUILang();
		if (readSource() == false)
		{
			wxMessageBox(guiStrData[TEXTE_LOADSRC], guiStrData[TEXT_ERROR], wxOK | wxICON_ERROR);
			throw(0);
		}
		readLocal();

		form = new mainFrame(guiStrData[TITLE_LPM]);
		form->Show();
	}
	catch (std::exception ex)
	{
		wxMessageBox(ex.what(), guiStrData[TEXT_ERROR], wxOK | wxICON_ERROR);
		return false;
	}
	catch (...)
	{
		return false;
	}

	return true;
}

int MyApp::OnExit()
{
	try
	{
		srcListTp::const_iterator itrSrc = sourceList.cbegin(), itrSrcEnd = sourceList.cend();
		for (; itrSrc != itrSrcEnd; itrSrc++)
			delete *itrSrc;
		std::cout.rdbuf(coutBuf);

		unlock();
	}
	catch (std::exception ex)
	{
		wxMessageBox(ex.what(), guiStrData[TEXT_ERROR], wxOK | wxICON_ERROR);
		return EXIT_FAILURE;
	}
	catch (...)
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#ifdef NDEBUG
void MyApp::OnUnhandledException()
{
	unlock();
	return wxApp::OnUnhandledException();
}
#endif

#endif
