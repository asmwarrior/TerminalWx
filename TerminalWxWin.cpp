#include "TerminalWxWin.h"
#include <wx/log.h>
#include <windows.h>

TerminalWxWin::TerminalWxWin(wxWindow* parent, wxWindowID id)
    : TerminalWx(parent, id)
{
    // Initialize winpty
    winpty_error_ptr_t error = nullptr;
    winpty_config_t* cfg = winpty_config_new(0, &error);
    if (error) {
        wxLogError("Error initializing winpty: %s", winpty_error_msg(error));
        winpty_error_free(error);
        return;
    }

    m_winpty = winpty_open(cfg, &error);
    winpty_config_free(cfg);
    if (error) {
        wxLogError("Error opening winpty: %s", winpty_error_msg(error));
        winpty_error_free(error);
        return;
    }

    // Start the shell
    // const wchar_t* cmd = L"cmd.exe"; // Use L"powershell.exe" for PowerShell
    const wchar_t* cmd = L"C:\\Windows\\System32\\cmd.exe";
    HANDLE process_handle = nullptr;
    HANDLE thread_handle = nullptr;
    DWORD create_process_error = 0;

    winpty_spawn_config_t* spawn_config = winpty_spawn_config_new(
        WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN, // Flags
        cmd,                             // Command to execute
        nullptr,                         // Command arguments (optional)
        nullptr,                         // Environment variables (optional)
        nullptr,                         // Current directory (optional)
        &error                           // Error pointer
    );

    if (!spawn_config || error) {
        wxLogError("Error creating spawn config: %s", winpty_error_msg(error));
        if (error) winpty_error_free(error);
        return;
    }

    BOOL spawn_success = winpty_spawn(
        m_winpty,
        spawn_config,
        &process_handle,
        &thread_handle,
        &create_process_error,
        &error
    );

    winpty_spawn_config_free(spawn_config);

    if (!spawn_success || error) {
        wxLogError("Error spawning process: %s", winpty_error_msg(error));
        if (error) winpty_error_free(error);
        return;
    }

    m_master_fd = winpty_conout_name(m_winpty);
    m_input_fd = winpty_conin_name(m_winpty);

    // Non-blocking thread to poll for terminal data
    m_pollThread = std::thread(&TerminalWxWin::pollData, this);
}

TerminalWxWin::~TerminalWxWin()
{
    m_quit = true;
    if (m_pollThread.joinable())
        m_pollThread.join();
    if (m_winpty)
        winpty_free(m_winpty);
}

void TerminalWxWin::pollData()
{
    char buffer[1024];
    while (!m_quit) {
        FILE* fp = _wfopen(m_master_fd, L"r");
        if (!fp) break;

        while (fgets(buffer, sizeof(buffer), fp)) {
            wxString str = wxString::FromUTF8(buffer);
            DisplayChars(str);
        }
        fclose(fp);
    }
}

void TerminalWxWin::OnUserInput(wxString str)
{
    FILE* fp = _wfopen(m_input_fd, L"w");
    if (!fp) return;

    wxScopedCharBuffer buf = str.ToUTF8();
    fwrite(buf.data(), sizeof(char), buf.length(), fp);
    fclose(fp);
}
