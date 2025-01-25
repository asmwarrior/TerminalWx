#ifndef TERMINALWXWIN_H_INCLUDED
#define TERMINALWXWIN_H_INCLUDED

#include <wx/wx.h>
#include <atomic>
#include <thread>
#include "src/TerminalWx.h"
#include "winpty.h"

struct TerminalWxWin : public TerminalWx
{
    TerminalWxWin(wxWindow* parent, wxWindowID id);
    ~TerminalWxWin();

    void pollData();
    void OnUserInput(wxString str) override;

private:
    winpty_t* m_winpty = nullptr;
    std::thread m_pollThread;
    std::atomic_bool m_quit = false;
    const wchar_t* m_master_fd = nullptr;
    const wchar_t* m_input_fd = nullptr;
    winpty_spawn_config_t* m_agent = nullptr;
};

#endif // TERMINALWXWIN_H_INCLUDED
