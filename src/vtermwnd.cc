#define Uses_TFrame
#define Uses_TEvent
#define Uses_TStaticText
#include <tvision/tv.h>

#include <tvterm/vtermwnd.h>
#include <tvterm/vtermview.h>
#include <tvterm/vtermframe.h>
#include <tvterm/cmds.h>

TCommandSet TVTermWindow::focusedCmds = []()
{
    TCommandSet ts;
    ts += cmGrabInput;
    ts += cmReleaseInput;
    return ts;
}();

TFrame *TVTermWindow::initFrame(TRect bounds)
{
    return new TVTermFrame(bounds);
}

TVTermWindow::TVTermWindow(const TRect &bounds) :
    TWindowInit(&TVTermWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber)
{
    options |= ofTileable;
    setState(sfShadow, False);
    TVTermView *vt = nullptr;
    {
        TRect r = getExtent().grow(-1, -1);
        TView *v;
        try
        {
            v = vt = new TVTermView(r, *this);
        }
        catch (std::string err)
        {
            v = new TStaticText(r, err);
            v->growMode = gfGrowHiX | gfGrowHiY;
        }
        insert(v);
    }
    ((TVTermFrame *) frame)->setTerm(vt);
}

void TVTermWindow::setTitle(std::string_view text)
{
    std::string_view tail = (helpCtx == hcInputGrabbed)
                          ? " (Input Grab)"
                          : "";
    char *str = new char[text.size() + tail.size() + 1];
    memcpy(str, text.data(), text.size());
    memcpy(str + text.size(), tail.data(), tail.size());
    str[text.size() + tail.size()] = '\0';
    termTitle = {str, text.size()};
    delete[] title; // 'text' could point to 'title', so don't free too early.
    title = str;
    frame->drawView();
}

void TVTermWindow::handleEvent(TEvent &ev)
{
    bool handled = true;
    switch (ev.what)
    {
        case evCommand:
            switch (ev.message.command)
            {
                case cmGrabInput:
                    if (helpCtx != hcInputGrabbed)
                        execute();
                    break;
                case cmIsTerm:
                    ev.message.infoPtr = this;
                    break;
                default: handled = false; break;
            }
            break;
        default: handled = false; break;
    }
    if (handled)
        clearEvent(ev);
    else
        TWindow::handleEvent(ev);
}

void TVTermWindow::setState(ushort aState, Boolean enable)
{
    TWindow::setState(aState, enable);
    if (aState == sfActive)
    {
        if (enable)
            enableCommands(focusedCmds);
        else
            disableCommands(focusedCmds);
    }
}

ushort TVTermWindow::execute()
{
    // Update help context.
    ushort hc = helpCtx;
    helpCtx = hcInputGrabbed;
    // Update title.
    setTitle(termTitle);
    while (true)
    {
        TEvent ev;
        getEvent(ev);
        if ( (ev.what == evCommand && ev.message.command == cmClose) ||
             (ev.what == evMouseDown && !mouseInView(ev.mouse.where)) )
        {
            putEvent(ev);
            break;
        }
        else if (ev.what == evCommand && ev.message.command == cmReleaseInput)
            break;
        else
            handleEvent(ev);
    }
    // Restore help context.
    helpCtx = hc;
    // Restore title.
    setTitle(termTitle);
    return 0;
}
