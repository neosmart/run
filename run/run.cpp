//#include <cassert>
#include <string>
#include <Windows.h>
#include "utf8.h"
#pragma comment(lib, "shell32.lib")

//skip past a single, un-nested, quoted or unquoted argument
const char *skiparg(const char *args)
{
	if (args[0] == '\'')
	{
		++args;
		while (args[0] && (args++)[0] != '\'');
	}
	else if (args[0] == '"')
	{
		++args;
		while (args[0] && (args++)[0] != '"');
	}
	else
	{
		while (args[0] && (args++)[0] != ' ');
	}

	//clear any trailing whitespace
	while (args[0] && args[0] == ' ' || args[0] == '\t')
	{
		++args;
	}

	return args;
}

const std::string unquote(std::string input)
{
	if ((input[0] == input[input.size() - 1] && input[0] == '\'') ||
		(input[0] == input[input.size() - 1] && input[0] == '"'))
	{
		input[input.size() - 1] = '\0';
		return input.c_str() + 1;
	}
	return input;
}

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	int argc = 0;
	auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc <= 1)
	{
		//No application specified
		MessageBox(nullptr, "An application to start was not specified! Correct syntax:\r\n"
			"run \\path\\to\\app.exe [/param1 ...]", "Invalid command line parameters!", MB_OK);
		return 0;
	}

	std::string application = unquote(wstring_to_utf8(argv[1]));
	if (GetFileAttributes(application.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		//Application not found
		std::string message = "The requested application '";
		message += application;
		message += "' was not found!";
		MessageBox(nullptr, message.c_str(), "Application not found!", MB_OK);
		return 0;
	}

	SHELLEXECUTEINFO sexi = { 0 };
	sexi.fMask = SEE_MASK_NOCLOSEPROCESS; //to retrieve hProcess of result
	sexi.cbSize = sizeof(sexi);
	sexi.lpFile = application.c_str();

	//Fast-forward past our own executable to get at the command-line arguments
	const char *args = skiparg(GetCommandLine());
	//skip past the executable we are launching; executable paths (obviously) cannot contain ' or "
	args = skiparg(args);
	sexi.lpParameters = args;

	sexi.nShow = SW_NORMAL;
	BOOL result = ShellExecuteEx(&sexi);

	if (result == FALSE)
	{
		return 0;
	}

	auto pid = GetProcessId(sexi.hProcess);
	CloseHandle(sexi.hProcess); //good form!

	return pid;
}
