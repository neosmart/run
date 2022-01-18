#include <cassert>
#include <string>
#include <Windows.h>
#include "utf8.h"
#pragma comment(lib, "shell32.lib")

// Skip past a single, un-nested, quoted or unquoted argument
static const char *
skipargs(const char *args, int skipCount = 1)
{
	for (int i = 0; i <= skipCount; ++i) {
		if (args[0] == '\'') {
			++args;
			while (args[0] && (args++)[0] != '\'');
		}
		else if (args[0] == '"') {
			++args;
			while (args[0] && (args++)[0] != '"');
		}
		else {
			while (args[0] && (args++)[0] != ' ');
		}

		// Clear any trailing whitespace
		while (args[0] && args[0] == ' ' || args[0] == '\t') {
			++args;
		}
	}

	return args;
}

static const std::string
unquote(std::string input)
{
	if ((input[0] == '\'' || input[0] == '\"') &&
		input[0] == input[input.size() - 1])
	{
		input[input.size() - 1] = '\0';
		return input.c_str() + 1;
	}
	return input;
}

/// <summary>
/// Invisibly runs the command and parameters specified as arguments.
/// </summary>
/// <returns>
/// If the first argument is /wait, then
/// the process waits for the launched process to exit and returns the exit code. Otherwise, the process
/// launches the target and exits immediately, returning the PID of the launched target. <c>-1</c> is
/// returned in case of an error.
/// </returns>
int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	int argc = 0;
	auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	bool wait = false;
	int targetIndex = 1;
	if (lstrcmpW(argv[targetIndex], L"/wait") == 0) {
		wait = true;
		++targetIndex;
	}

	if (argc <= targetIndex)
	{
		// No application specified
		MessageBox(nullptr, "An application to start was not specified! Correct syntax:\r\n"
			"run  [/wait] \\path\\to\\app.exe [/param1 ...]", "Invalid command line parameters!", MB_OK);
		return -1;
	}

	std::string application = unquote(wstring_to_utf8(argv[targetIndex]));
	if (GetFileAttributes(application.c_str()) == INVALID_FILE_ATTRIBUTES) {
		// Application not found
		std::string message = "The requested application '" + application + "' was not found!";
		MessageBox(nullptr, message.c_str(), "Application not found!", MB_OK);
		return -1;
	}

	SHELLEXECUTEINFO sexi = { 0 };
	sexi.fMask = SEE_MASK_NOCLOSEPROCESS; // to retrieve hProcess of result
	sexi.cbSize = sizeof(sexi);
	sexi.lpFile = application.c_str();

	// Skip past the executable we are launching; executable paths (obviously) cannot contain ' or "
	const char* args = skipargs(GetCommandLine(), targetIndex);
	sexi.lpParameters = args;

	//sexi.nShow = SW_NORMAL;
	sexi.nShow = SW_HIDE;
	if (ShellExecuteEx(&sexi) == FALSE) {
		return -1;
	}

	assert(sexi.hProcess != 0);

	// If waiting, return the exit code. Otherwise, return the process id.
	DWORD result = -1;
	if (wait) {
		WaitForSingleObject(sexi.hProcess, INFINITE);
		if (!GetExitCodeProcess(sexi.hProcess, &result)) {
			result = -1;
		}
	}
	else {
		result = GetProcessId(sexi.hProcess);
	}

	CloseHandle(sexi.hProcess); // Good form!

	return result;
}
