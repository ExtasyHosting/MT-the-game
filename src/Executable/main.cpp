#include "stdafx.h"
#include "SystemClass.h"



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    try 
    {
        // TODO : 데이터드리븐으로 바꾸기
        const size_t defaultWidth = 800;
        const size_t defaultHeight = 600;
        const bool FullScreen = false;

        unique_ptr<SystemClass> System(new SystemClass(hInstance, defaultWidth, defaultHeight, FullScreen));
        System->Run(nCmdShow);
    }
    catch ( _com_error e )
    {
        ErrorReporter reporter(0, 0, 0, 0, ErrorCodeType::HR, e.Error(), e.ErrorMessage());
        reporter.Report(ErrorReportType::Error);
        ExitProcess(EXIT_FAILURE);
    }
    catch ( std::exception& e )
    {
        std::string name(typeid(e).name());
        std::string what(e.what());

        std::wstring message;
        message += L"Standard exception has occured. ( ";
        message.append(name.cbegin(), name.cend());
        message += L" )\n";
        message.append(what.cbegin(), what.cend());

        ErrorReporter reporter( message.c_str() );
        reporter.Report(ErrorReportType::Error);
        ExitProcess(EXIT_FAILURE);
    }
    catch ( ... )
    {
        ErrorReporter reporter(L"Unhandled exception has occured.");
        reporter.Report(ErrorReportType::Error);
        ExitProcess(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
