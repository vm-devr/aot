
bool Install()
{
	bool bResult = false;

	// Get module path
	TCHAR szFilePath[_MAX_PATH];
	::GetModuleFileName(nullptr, szFilePath, _MAX_PATH);

	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_PATH];
	_tsplitpath_s(szFilePath, szDrive, _MAX_DRIVE,
		szDir, _MAX_PATH, nullptr, 0, nullptr, 0);

	_tmakepath_s(szFilePath, _MAX_PATH, szDrive, szDir, _T("aot_manifest"), _T("json"));

	// Create json file, if not created
	DWORD dwBytesWritten = 0;
	HANDLE hJSONFile = CreateFile(szFilePath, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hJSONFile != INVALID_HANDLE_VALUE)
	{
		std::string strJSONData =
			R"({ "name": "always_on_top", "description": "Always on Top", "path": "AOT.exe", "type": "stdio", "allowed_extensions": [ "{E6C93316-271E-4b3d-8D7E-FE11B4350AEB}" ] })";
		WriteFile(hJSONFile, strJSONData.c_str(), strJSONData.length(), nullptr, nullptr);
		CloseHandle(hJSONFile);
	}
	bResult = GetLastError() != ERROR_FILE_EXISTS;

	// Fill registry
	HKEY hKeyConfig = nullptr;
	DWORD dwDispostion = REG_CREATED_NEW_KEY;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, _T(R"(Software\Mozilla\NativeMessagingHosts\always_on_top)"),
		0, nullptr, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &hKeyConfig, &dwDispostion) == 0)
	{
		RegSetValueEx(hKeyConfig, nullptr, 0, REG_SZ, (const BYTE*)szFilePath, lstrlen(szFilePath) * sizeof(TCHAR));
		RegCloseKey(hKeyConfig);
	}

	return bResult;
}

int APIENTRY wWinMain(_In_ HINSTANCE/* hInstance*/,
                     _In_opt_ HINSTANCE/* hPrevInstance*/,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	// std::wostringstream os0;
	// os0 << _T("AOT job started: ") << lpCmdLine;
	// OutputDebugString(os0.str().c_str());

	// Try to install the aot runtime app
	if (Install())
		return -1;

	// Run aot job
	std::wstring strCmdLine((lpCmdLine != nullptr) ? lpCmdLine : _T(""));
	bool bUseCoords = (strCmdLine != _T("-useoldmethod"));
	HWND hwndActive = ::GetForegroundWindow();
	if (bUseCoords)
	{
		POINT ptCur;
		GetCursorPos(&ptCur);

		hwndActive = WindowFromPoint(ptCur);
	}

	if (::IsWindow(hwndActive))
	{
		while (::IsWindow(::GetParent(hwndActive)))
			hwndActive = ::GetParent(hwndActive);
	}

	DWORD dwExStyle = ::GetWindowLong(hwndActive, GWL_EXSTYLE);
	BOOL bOnTop = ((dwExStyle & WS_EX_TOPMOST) != 0);

	TCHAR strName[MAX_PATH + 1];
	GetWindowText(hwndActive, strName, MAX_PATH);
	strName[MAX_PATH] = 0;

	std::wostringstream os1;
	os1 << _T("AOT job started for window: ") << strName << "(" << std::hex << hwndActive << _T(") which is ") << (bOnTop ? _T("") : _T("not")) << _T(" on top");
	if (bUseCoords)
		os1 << _T(" using coordinates");
	OutputDebugString(os1.str().c_str());

	DWORD dwErr = 0;
	if (!SetWindowPos(hwndActive, bOnTop ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE))
		dwErr = GetLastError();

	std::wostringstream os2;
	os2 << _T("AOT job finished with result: ") << dwErr;
	OutputDebugString(os2.str().c_str());

	return -1;
}
