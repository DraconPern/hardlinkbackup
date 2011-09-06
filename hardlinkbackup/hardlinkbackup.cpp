// hardlinkbackup.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int CopyDirWithHardLink(const CString &source, const CString &target)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	CString findpath = source + L"\\*";
	
	hFind = FindFirstFile(findpath, &FindFileData);

	if(hFind == INVALID_HANDLE_VALUE)
		return 1;

	// skip the first two entry since they are . and ..
	if(CString(FindFileData.cFileName) != ".")
		return 1;

	if(FindNextFile(hFind, &FindFileData) == 0)
		return 1;

	if(CString(FindFileData.cFileName) != "..")
		return 1;

	// now find the rest of the directory
	if(FindNextFile(hFind, &FindFileData) == 0)
		return 1;

	do
	{
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			CreateDirectory(target + L"\\" + FindFileData.cFileName, NULL);
			CopyDirWithHardLink(source + L"\\" + FindFileData.cFileName, target + L"\\" + FindFileData.cFileName);
		}
		else
		{
			CreateHardLink(target + L"\\" + FindFileData.cFileName, source + L"\\" + FindFileData.cFileName, NULL);		
		}
	} while(FindNextFile(hFind, &FindFileData) != 0);

	return ERROR_SUCCESS;
}

DWORD UpdateDirectoryModificationDate(const CString &dir)
{
	SetLastError(ERROR_SUCCESS);

	DWORD dwFileAttributes = GetFileAttributes(dir);
	DWORD dwErr = GetLastError();
	if(dwFileAttributes == INVALID_FILE_ATTRIBUTES && dwErr != ERROR_FILE_NOT_FOUND)
		return dwErr;

	bool bUpdateOrCreateDirectory = true;

	// If there is no directory, create it!
	if(bUpdateOrCreateDirectory && dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
		SetLastError(ERROR_SUCCESS);
		BOOL bResult = CreateDirectory(dir, NULL);
		
		if(!bResult) {
			DWORD dwErr = GetLastError();
			if(dwErr != ERROR_ALREADY_EXISTS)
				return dwErr;
		} else {
			// We're done, because we obviously created a directory!
			return ERROR_SUCCESS;
		}
	}

	SetLastError(ERROR_SUCCESS);

	HANDLE hFile = CreateFile(
		dir, 
		GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, 
		(bUpdateOrCreateDirectory) ? OPEN_EXISTING : OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL | (bUpdateOrCreateDirectory ? FILE_FLAG_BACKUP_SEMANTICS : 0), 
		0);

	DWORD dwRetVal = GetLastError();

	// Check for CreateFile() special cases
	if(hFile == INVALID_HANDLE_VALUE) {
		if(dwRetVal == ERROR_ALREADY_EXISTS)
			dwRetVal = ERROR_SUCCESS; // not an error according to MSDN docs

		return dwRetVal;
	}

	FILETIME ft;
    SYSTEMTIME st;

    GetSystemTime(&st);              // gets current time
    SystemTimeToFileTime(&st, &ft);  // converts to file time format
	BOOL bResult = SetFileTime(hFile, &ft, &ft, &ft);

	if(bResult)
		dwRetVal = ERROR_SUCCESS;
	else
		dwRetVal = GetLastError();
	

	CloseHandle(hFile);

	return dwRetVal;
}

int DeleteExtraFileAndDirectory(const CString &source, const CString &target);
int DeleteFileIncludingReadOnly(const CString &target);

int UpdateDirectory(const CString &source, const CString &target)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FILE_ATTRIBUTE_DATA FileData;

	CString findpath = source + L"\\*";
	
	hFind = FindFirstFile(findpath, &FindFileData);

	if(hFind == INVALID_HANDLE_VALUE)
		return 1;

	// skip the first two entry since they are . and ..
	if(CString(FindFileData.cFileName) != ".")
		return 1;

	if(FindNextFile(hFind, &FindFileData) == 0)
		return 1;

	if(CString(FindFileData.cFileName) != "..")
		return 1;

	// now find the rest of the entry
	if(FindNextFile(hFind, &FindFileData) == 0)
		return 1;

	do
	{	
		CString sourcefullpath;
		CString targetfullpath;

		sourcefullpath = source + L"\\" + FindFileData.cFileName;
		targetfullpath = target + L"\\" + FindFileData.cFileName;
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			CreateDirectory(targetfullpath, NULL);
			UpdateDirectory(sourcefullpath, targetfullpath);
		}
		else
		{
			if(GetFileAttributesEx(targetfullpath, GetFileExInfoStandard, &FileData) != 0)
			{
				if(CompareFileTime(&FindFileData.ftLastWriteTime, &FileData.ftLastWriteTime) != 0)
				{
					DeleteFileIncludingReadOnly(targetfullpath);
					wprintf(L"Copying \"%s\"\n", sourcefullpath);
					CopyFile(sourcefullpath, targetfullpath, FALSE);
				}
			}
			else
			{
				int a = GetLastError();
				wprintf(L"Copying \"%s\"\n", sourcefullpath);
				CopyFile(sourcefullpath, targetfullpath, FALSE);
			}
						
		}
	} while(FindNextFile(hFind, &FindFileData) != 0);
	FindClose(hFind);

	DeleteExtraFileAndDirectory(source, target);

	return ERROR_SUCCESS;

}

bool DirectoryExists(const CString &dirName)
{
	DWORD attribs = ::GetFileAttributes(dirName);
	if (attribs == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	return (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool FileExists(const CString &FileName)
{
	DWORD attribs = ::GetFileAttributes(FileName);
	if (attribs == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	
	return (attribs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int DeleteFileIncludingReadOnly(const CString &target)
{
	DWORD attrib = GetFileAttributes(target);
	if(attrib != INVALID_FILE_ATTRIBUTES)
	{
		if(attrib & FILE_ATTRIBUTE_READONLY)
			SetFileAttributes(target, attrib ^ FILE_ATTRIBUTE_READONLY);

		DeleteFile(target);
		return ERROR_SUCCESS;
	}

	return 1;
}

int DeleteDirectory(const CString &target)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	CString findpath = target + L"\\*";
	
	hFind = FindFirstFile(findpath, &FindFileData);

	if(hFind == INVALID_HANDLE_VALUE)
		return 1;

	// skip the first two entry since they are . and ..
	if(CString(FindFileData.cFileName) != ".")
		return 1;

	if(FindNextFile(hFind, &FindFileData) == 0)
		return 1;

	if(CString(FindFileData.cFileName) != "..")
		return 1;	

	while(FindNextFile(hFind, &FindFileData) != 0)
	{	
		CString targetfullpath;

		targetfullpath = target + L"\\" + FindFileData.cFileName;
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DeleteDirectory(targetfullpath);
		}
		else
		{
			DeleteFileIncludingReadOnly(targetfullpath);						
		}
	}

	FindClose(hFind);

	RemoveDirectory(target);
	return ERROR_SUCCESS;

}

int DeleteExtraFileAndDirectory(const CString &source, const CString &target)
{
	// assume source and target are the same except that target has extra files or directory
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	CString findpath = target + L"\\*";
	
	hFind = FindFirstFile(findpath, &FindFileData);

	if(hFind == INVALID_HANDLE_VALUE)
		return 1;

	// skip the first two entry since they are . and ..
	if(CString(FindFileData.cFileName) != ".")
		return 1;

	if(FindNextFile(hFind, &FindFileData) == 0)
		return 1;

	if(CString(FindFileData.cFileName) != "..")
		return 1;

	// now find the rest of the entry
	if(FindNextFile(hFind, &FindFileData) == 0)
		return 1;

	do
	{	
		CString sourcefullpath;
		CString targetfullpath;

		sourcefullpath = source + L"\\" + FindFileData.cFileName;
		targetfullpath = target + L"\\" + FindFileData.cFileName;		
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(!DirectoryExists(sourcefullpath))
			{
				wprintf(L"Deleting directory \"%s\"\n", targetfullpath);
				DeleteDirectory(targetfullpath);	
			}
		}
		else
		{
			if(!FileExists(sourcefullpath))
			{
				wprintf(L"Deleting file \"%s\"\n", targetfullpath);				
				DeleteFileIncludingReadOnly(targetfullpath);									
			}
		}
	} while(FindNextFile(hFind, &FindFileData) != 0);
	FindClose(hFind);
	
	return ERROR_SUCCESS;

}

CString MakeLongPath(const CString &path)
{
	CString longpath;
	// determine local vs unc, and prepend long path
	if(path.Find(L"\\\\") == 0)
	{	
		longpath = CString(L"\\\\?\\UNC") + &((LPCWSTR)path)[1];
	}
	else
	{
		longpath = L"\\\\?\\" + path;
	}

	return longpath;
}



CString RotateDirectory(int n, const CString &backupdir)
{
	WCHAR numstr1[1024];
	WCHAR numstr2[1024];

	swprintf(numstr1, 1023, L"%i", n + 1);
	wprintf(L" Moving \"%s\" for deletion.\n", backupdir + L"\\" + numstr1);
	srand( (unsigned)time( NULL ) );
	int ran = rand();
	swprintf(numstr2, 1023, L"%i", ran);
	CString deletedir = backupdir + L"\\delete_" + numstr2;
	MoveFile(backupdir + L"\\" + numstr1, deletedir);

	while(n > 0)
	{
		swprintf(numstr1, 1023, L"%i", n);
		swprintf(numstr2, 1023, L"%i", n + 1);
		wprintf(L"Moving \"%s\" to \"%s\"\n", backupdir + L"\\" + numstr1, backupdir + L"\\" + numstr2);
		MoveFile(backupdir + L"\\" + numstr1, backupdir + L"\\" + numstr2);
		n--;
	}

	return deletedir;
}

// Prints the usage/help and an optional error message
static void ShowUsage(LPCTSTR lpszErrorMessage = NULL) 
{
	if(lpszErrorMessage)
	_tprintf(_T("Error: %s\n"), lpszErrorMessage);
	_tprintf(_T("Usage: hardlinkbackup [-n N] [-S] <source dir> <backup dir>\n"));
	_tprintf(_T("   -n: Number of backups to keep, defaults to 8\n"));
	_tprintf(_T("   -S: Skip update, just rotate and link\n"));
	_tprintf(_T("   -h: This help text\n"));
	_tprintf(_T("\n"));
}

// Parses the command line arguments. This function expects
// that "argv[0]" is not included. (argv[0] = name of called executable)
static bool ParseOptions(int argc,  LPTSTR argv[], 
						 int &iFileArg, 
						 int &numbackups,
						 BOOL &skipupdate)
{
	numbackups = 8;	
	skipupdate = FALSE;

	if(!argc) {
		ShowUsage(_T(""));
		return false;
	} else if(argc == 1) {
		if(!_tcscmp(argv[0], _T("-h"))) {
			ShowUsage(_T(""));
			return false;
		} else if(argv[0][0] == _T('-')) {
			ShowUsage(_T("Not enough arguments"));
			return false;
		}
		iFileArg = 0;
	} else {
		int i;
		for(i = 0; i < argc; ++i) {
			LPTSTR curr_arg = argv[i];
			LPTSTR next_arg = (i + 1 <= (argc - 1) ? argv[i + 1] : NULL);

			if(curr_arg[0] == '-') {
				if(!_tcscmp(curr_arg, _T("-n"))) {
					numbackups = _wtoi(next_arg);				    
					++i;
				} else if(!_tcscmp(curr_arg, _T("-S"))) {
					skipupdate = TRUE;
				} else if(!_tcscmp(curr_arg, _T("-h"))) {
					ShowUsage(_T("Help Requested"));
					return false;
				} else {
					ShowUsage(_T("Unknown argument"));
					return false;
				}
			} else
				break;
		}

		iFileArg = i;
	}

	if(iFileArg >= argc) {
		ShowUsage(_T("No source dir specified!"));
		return false;
	}

	if(iFileArg >= argc - 1) {
		ShowUsage(_T("No backup dir specified!"));
		return false;
	}

	return true;
}

int wmain(int argc, wchar_t* argv[])
{

	int numbackups, iFileArg;
	BOOL skipupdate;

	if(!ParseOptions(argc - 1, argv + 1, iFileArg, numbackups, skipupdate))
		return -1;
	
	++iFileArg; // Adjust for executable
	
	CString source(argv[iFileArg++]);
	CString backupdir(argv[iFileArg++]);

	source = MakeLongPath(source);
	backupdir = MakeLongPath(backupdir);
	
	if(!DirectoryExists(source))
	{
		wprintf(L"Can't Access \"%s\"\n", source);
		return -1;
	}

	if(!DirectoryExists(backupdir))
	{
		wprintf(L"Can't Access \"%s\"\n", backupdir);
		return -1;
	}

	// to make the backup window as short as possible we assume that 
	// \nextstage and \1 are the same. We update \nextstage only, then after we are done, we 
	// rotate the 3->4, 2->3, 1->2, then nextstage->1.
	// we then copy 1 to nextstage for the next invocation
	wprintf(L"Looking to update \"%s\"\n", backupdir + L"\\nextstage");
	UpdateDirectoryModificationDate(backupdir + L"\\nextstage");

	if(!skipupdate)
	{
		wprintf(L"Updating \"%s\" to \"%s\"\n", source, backupdir + L"\\nextstage");
		UpdateDirectory(source, backupdir + L"\\nextstage");
	}	
	wprintf(L"Rotating backups in \"%s\"\n", backupdir);
	CString deletedir = RotateDirectory(numbackups, backupdir);
	wprintf(L"Moving \"%s\" to \"%s\"\n", backupdir + L"\\nextstage", backupdir + L"\\1");
	MoveFile(backupdir + L"\\nextstage", backupdir + L"\\1");
	wprintf(L"Hardlinking \"%s\" to \"%s\"\n", backupdir + L"\\1", backupdir + L"\\nextstage");
	CreateDirectory(backupdir + L"\\nextstage", NULL);
	CopyDirWithHardLink(backupdir + L"\\1", backupdir + L"\\nextstage");
	wprintf(L"Deleting directory \"%s\"\n", deletedir);
	DeleteDirectory(deletedir);
	return ERROR_SUCCESS;
}

