// Arboretum.cpp : Définit le point d'entrée de l'application.
//
#define UNICODE
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <string>
#include "resource.h"
#include <shlobj.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

#pragma warning(disable:6328)
HINSTANCE   g_hInst = nullptr;
HWND g_hMainWnd = nullptr;
HWND g_hTreeView = nullptr;
HWND g_hListView = nullptr;
HIMAGELIST  g_hImgList = nullptr;
std::wstring g_CurrentFolder;
const UINT WM_APP_ADD_LVITEM = WM_APP + 1;
struct DonneesDuNoeud
{
	std::wstring path;
	bool isDir;
};
struct DonneesElement
{
	std::wstring path;
	bool isDir;
	ULONGLONG size;
	FILETIME ftWrite;
	int iconIndex;
};
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void GenerentElementsEnfants(HWND hParent);
void CreerControleArborescence(HWND hParent);
void CreerVueListe(HWND hParent);
void InitialiserListeImage(HWND hTree);
int  RetrouverIndexIcone(const std::wstring& path, bool isDir);
HTREEITEM AjouterElelementControleArborescence(HWND hTree, LPCWSTR text, HTREEITEM hParent, const std::wstring& fullPath, bool isDir);
void AjouterSousDossier(HWND hTree, const std::wstring& path, HTREEITEM hParent);
void TrierElementsEnfants(HWND hTree, HTREEITEM hParent);
int CALLBACK ProcedureTriageArborescence(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
DWORD WINAPI ProcessusEnumeration(LPVOID lpParam);
void InitialiserProcessusEnumeration(const std::wstring& folder);
void VidangerAffichageVue();
void InjecterElementListeVue(DonneesElement* pItem);
std::wstring FormatageTaille(ULONGLONG size);
std::wstring FormatageHorodatageFichier(const FILETIME& ft);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	g_hInst = hInstance;
	INITCOMMONCONTROLSEX icc = {};
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icc);
	const wchar_t CLASS_NAME[] = L"Arboretum";
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hIcon = LoadIcon(wc.hInstance, (LPCWSTR)0x81);
	RegisterClass(&wc);
	HWND hWnd = CreateWindowEx(WS_EX_DLGMODALFRAME, CLASS_NAME, L"Démo Lister Arboretum style Explorateur Windows", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)return 0;
	g_hMainWnd = hWnd;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
void GenerentElementsEnfants(HWND hParent)
{
	CreerControleArborescence(hParent);
	CreerVueListe(hParent);
}
void CreerControleArborescence(HWND hParent) { g_hTreeView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, nullptr, WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 0, 0, 0, 0, hParent, (HMENU)1, g_hInst, nullptr); }
void CreerVueListe(HWND hParent)
{
	g_hListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS, 0, 0, 0, 0, hParent, (HMENU)2, g_hInst, nullptr);
	ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	LVCOLUMN col = {};
	col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	col.pszText = const_cast<LPWSTR>(L"Nom");
	col.cx = 300;
	col.iSubItem = 0;
	ListView_InsertColumn(g_hListView, 0, &col);
	col.pszText = const_cast<LPWSTR>(L"Taille");
	col.cx = 100;
	col.iSubItem = 1;
	ListView_InsertColumn(g_hListView, 1, &col);
	col.pszText = const_cast<LPWSTR>(L"Type");
	col.cx = 150;
	col.iSubItem = 2;
	ListView_InsertColumn(g_hListView, 2, &col);
	col.pszText = const_cast<LPWSTR>(L"Modifié");
	col.cx = 180;
	col.iSubItem = 3;
	ListView_InsertColumn(g_hListView, 3, &col);
}
void InitialiserListeImage(HWND hTree)
{
	SHFILEINFO sfi = {};
	g_hImgList = (HIMAGELIST)SHGetFileInfo(L"C:\\", 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	if (g_hImgList)
	{
		TreeView_SetImageList(hTree, g_hImgList, TVSIL_NORMAL);
		ListView_SetImageList(g_hListView, g_hImgList, LVSIL_SMALL);
	}
}
int RetrouverIndexIcone(const std::wstring& path, bool isDir)
{
	SHFILEINFO sfi = {};
	DWORD attr = isDir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	SHGetFileInfo(path.c_str(), attr, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	return sfi.iIcon;
}
HTREEITEM AjouterElelementControleArborescence(HWND hTree, LPCWSTR text, HTREEITEM hParent, const std::wstring& fullPath, bool isDir)
{
	DonneesDuNoeud* pData = new DonneesDuNoeud{ fullPath, isDir };
	TVINSERTSTRUCT tvis = {};
	tvis.hParent = hParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.item.pszText = const_cast<LPWSTR>(text);
	tvis.item.lParam = (LPARAM)pData;
	int iconIndex = RetrouverIndexIcone(fullPath, isDir);
	tvis.item.iImage = iconIndex;
	tvis.item.iSelectedImage = iconIndex;
	return (HTREEITEM)SendMessage(hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
}
int CALLBACK ProcedureTriageArborescence(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	DonneesDuNoeud* a = (DonneesDuNoeud*)lParam1;
	DonneesDuNoeud* b = (DonneesDuNoeud*)lParam2;
	if (!a || !b)               return 0;
	if (a->isDir != b->isDir)   return a->isDir ? -1 : 1;
	LPCWSTR nameA = PathFindFileName(a->path.c_str());
	LPCWSTR nameB = PathFindFileName(b->path.c_str());
	return _wcsicmp(nameA, nameB);
}
void TrierElementsEnfants(HWND hTree, HTREEITEM hParent)
{
	TVSORTCB sort = {};
	sort.hParent = hParent;
	sort.lpfnCompare = ProcedureTriageArborescence;
	sort.lParam = 0;
	TreeView_SortChildrenCB(hTree, &sort, 0);
}
void AjouterSousDossier(HWND hTree, const std::wstring& path, HTREEITEM hParent)
{
	wchar_t searchPath[MAX_PATH];
	wsprintf(searchPath, L"%s\\*", path.c_str());
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(searchPath, &fd);
	if (hFind == INVALID_HANDLE_VALUE)		return;
	do
	{
		if (lstrcmp(fd.cFileName, L".") == 0 || lstrcmp(fd.cFileName, L"..") == 0)			continue;
		wchar_t fullPath[MAX_PATH];
		wsprintf(fullPath, L"%s\\%s", path.c_str(), fd.cFileName);
		bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		HTREEITEM hItem = AjouterElelementControleArborescence(hTree, fd.cFileName, hParent, fullPath, isDir);
		if (isDir) { AjouterElelementControleArborescence(hTree, L"", hItem, L"", false); }
	} while (FindNextFile(hFind, &fd));
	FindClose(hFind);
	TrierElementsEnfants(hTree, hParent);
}
std::wstring FormatageTaille(ULONGLONG size)
{
	wchar_t buf[64];
	if (size < 1024)                                wsprintf(buf, L"%lu", size);
	else if (size < 1024 * 1024)                    wsprintf(buf, L"%lu Ko", size / 1024);
	else if (size < 1024ull * 1024ull * 1024ull)    wsprintf(buf, L"%lu Mo", size / (1024ull * 1024ull));
	else { wsprintf(buf, L"%lu Go", size / (1024ull * 1024ull * 1024ull)); }
	return buf;
}
std::wstring FormatageHorodatageFichier(const FILETIME& ft)
{
	SYSTEMTIME stUTC, stLocal;
	FileTimeToSystemTime(&ft, &stUTC);
	SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLocal);
	wchar_t buf[64];
	wsprintf(buf, L"%02d/%02d/%04d %02d:%02d", stLocal.wDay, stLocal.wMonth, stLocal.wYear, stLocal.wHour, stLocal.wMinute);
	return buf;
}
void VidangerAffichageVue()
{
	int count = ListView_GetItemCount(g_hListView);
	LVITEM lvi = {};
	lvi.mask = LVIF_PARAM;
	for (int i = 0; i < count; ++i)
	{
		lvi.iItem = i;
		if (ListView_GetItem(g_hListView, &lvi)) {
			DonneesElement* p = (DonneesElement*)lvi.lParam;
			delete p;
		}
	}
	ListView_DeleteAllItems(g_hListView);
}

std::wstring SelectionDossierParent(HWND hOwner)
{
	BROWSEINFO bi = { 0 };
	bi.hwndOwner = hOwner;
	bi.lpszTitle = L"Sélectionnez un dossier";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
	if (!pidl)return L"";
	wchar_t path[MAX_PATH];
	if (SHGetPathFromIDList(pidl, path))
	{
		CoTaskMemFree(pidl);
		return path;
	}
	CoTaskMemFree(pidl);
	return L"";
}
void InjecterElementListeVue(DonneesElement* pItem)
{
	std::wstring folder = pItem->path;
	PathRemoveFileSpec(&folder[0]);
	folder.resize(wcslen(folder.c_str()));
	if (_wcsicmp(folder.c_str(), g_CurrentFolder.c_str()) != 0) { delete pItem; return; }
	LPCWSTR name = PathFindFileName(pItem->path.c_str());
	LVITEM lvi = {};
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = ListView_GetItemCount(g_hListView);
	lvi.pszText = const_cast<LPWSTR>(name);
	lvi.iImage = pItem->iconIndex;
	lvi.lParam = (LPARAM)pItem;
	int index = ListView_InsertItem(g_hListView, &lvi);
	std::wstring sizeStr = pItem->isDir ? L"" : FormatageTaille(pItem->size);
	ListView_SetItemText(g_hListView, index, 1, const_cast<LPWSTR>(sizeStr.c_str()));
	std::wstring typeStr = pItem->isDir ? L"Dossier" : L"Fichier";
	ListView_SetItemText(g_hListView, index, 2, const_cast<LPWSTR>(typeStr.c_str()));
	std::wstring dateStr = FormatageHorodatageFichier(pItem->ftWrite);
	ListView_SetItemText(g_hListView, index, 3, const_cast<LPWSTR>(dateStr.c_str()));
}
DWORD WINAPI ProcessusEnumeration(LPVOID lpParam)
{
	std::wstring* pFolder = (std::wstring*)lpParam;
	std::wstring folder = *pFolder;
	delete pFolder;
	wchar_t searchPath[MAX_PATH];
	wsprintf(searchPath, L"%s\\*", folder.c_str());
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(searchPath, &fd);
	if (hFind == INVALID_HANDLE_VALUE)        return 0;
	do
	{
		if (lstrcmp(fd.cFileName, L".") == 0 || lstrcmp(fd.cFileName, L"..") == 0)continue;
		wchar_t fullPath[MAX_PATH];
		wsprintf(fullPath, L"%s\\%s", folder.c_str(), fd.cFileName);
		bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		DonneesElement* pItem = new DonneesElement;
		pItem->path = fullPath;
		pItem->isDir = isDir;
		if (isDir) { pItem->size = 0; }
		else {
			ULARGE_INTEGER uli;
			uli.LowPart = fd.nFileSizeLow;
			uli.HighPart = fd.nFileSizeHigh;
			pItem->size = uli.QuadPart;
		}
		pItem->ftWrite = fd.ftLastWriteTime;
		pItem->iconIndex = RetrouverIndexIcone(fullPath, isDir);
		PostMessage(g_hMainWnd, WM_APP_ADD_LVITEM, 0, (LPARAM)pItem);
	} while (FindNextFile(hFind, &fd));
	FindClose(hFind);
	return 0;
}
void InitialiserProcessusEnumeration(const std::wstring& folder)
{
	g_CurrentFolder = folder;
	VidangerAffichageVue();
	std::wstring* pFolder = new std::wstring(folder);
	HANDLE hThread = CreateThread(nullptr, 0, ProcessusEnumeration, pFolder, 0, nullptr);
	if (hThread)CloseHandle(hThread);
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		GenerentElementsEnfants(hWnd);
		InitialiserListeImage(g_hTreeView);
		wchar_t exePath[MAX_PATH];
		GetModuleFileName(nullptr, exePath, MAX_PATH);
		PathRemoveFileSpec(exePath);
		std::wstring folder = SelectionDossierParent(hWnd);
		std::wstring rootPath = exePath;
		if (!folder.empty())
		{
			rootPath = folder;
			SetCurrentDirectory(folder.c_str());
			InitialiserProcessusEnumeration(folder);
			MessageBox(hWnd, folder.c_str(), L"Dossier choisi", MB_OK);
		}
		LPCWSTR lastPart = PathFindFileName(exePath);
		std::wstring rootLabel = lastPart && *lastPart ? lastPart : exePath;
		HTREEITEM hRoot = AjouterElelementControleArborescence(g_hTreeView, rootLabel.c_str(), TVI_ROOT, rootPath, true);
		AjouterElelementControleArborescence(g_hTreeView, L"", hRoot, L"", false);
		TreeView_Expand(g_hTreeView, hRoot, TVE_EXPAND);
		InitialiserProcessusEnumeration(rootPath);
	}
	return 0;
	case WM_SIZE:
	{
		RECT rc;
		GetClientRect(hWnd, &rc);
		int width = rc.right - rc.left;
		int height = rc.bottom - rc.top;
		int treeWidth = width / 3; // 1/3 TreeView, 2/3 ListView
		if (g_hTreeView) { MoveWindow(g_hTreeView, 0, 0, treeWidth, height, TRUE); }
		if (g_hListView) { MoveWindow(g_hListView, treeWidth, 0, width - treeWidth, height, TRUE); }
	}
	return 0;
	case WM_NOTIFY:
	{
		LPNMHDR hdr = (LPNMHDR)lParam;
		if (hdr->hwndFrom == g_hTreeView) {
			switch (hdr->code)
			{
			case TVN_ITEMEXPANDING:
			{
				LPNMTREEVIEW ptv = (LPNMTREEVIEW)lParam;
				HTREEITEM hItem = ptv->itemNew.hItem;
				HTREEITEM hChild = TreeView_GetChild(g_hTreeView, hItem);
				if (hChild)
				{
					TVITEM tvi = {};
					tvi.mask = TVIF_TEXT;
					tvi.hItem = hChild;
					wchar_t buf[2] = {};
					tvi.pszText = buf;
					tvi.cchTextMax = 2;
					TreeView_GetItem(g_hTreeView, &tvi);
					if (buf[0] == L'\0') // dummy
					{
						TreeView_DeleteItem(g_hTreeView, hChild);
						TVITEM tviData = {};
						tviData.mask = TVIF_PARAM;
						tviData.hItem = hItem;
						TreeView_GetItem(g_hTreeView, &tviData);
						DonneesDuNoeud* pData = (DonneesDuNoeud*)tviData.lParam;
						if (pData && pData->isDir)
						{
							AjouterSousDossier(g_hTreeView, pData->path, hItem);
						}
					}
				}
			}
			break;
			case TVN_SELCHANGED:
			{
				LPNMTREEVIEW ptv = (LPNMTREEVIEW)lParam;
				DonneesDuNoeud* pData = (DonneesDuNoeud*)ptv->itemNew.lParam;
				if (pData && pData->isDir)
				{
					InitialiserProcessusEnumeration(pData->path);
				}
			}
			break;
			case TVN_DELETEITEM:
			{
				LPNMTREEVIEW ptv = (LPNMTREEVIEW)lParam;
				DonneesDuNoeud* pData = (DonneesDuNoeud*)ptv->itemOld.lParam;
				delete pData;
			}
			break;
			}
		}
	}
	return 0;
	case WM_APP_ADD_LVITEM:
	{
		DonneesElement* pItem = (DonneesElement*)lParam;
		InjecterElementListeVue(pItem);
	}
	return 0;
	case WM_DESTROY:
		VidangerAffichageVue();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
