import sys
from xide import *

# M-1270 - avoid problems when a user (re)defines 'sys' in the global namespace
from sys import __excepthook__ as _xide_m_1270_sys_dot_originalexcepthook
from sys import settrace as _xide_m_1270_sys_dot_settrace
from sys import gettrace as _xide_m_1270_sys_dot_gettrace
import sys as _xide_b_112320_sys     # some xap3emu out there still mess with sys

# Install a trace function to call app.processTraceEvents() periodically
# This keeps the UI alive and allow the user to break out of infinite loops with Ctrl-Z

# _xide_b_112320_logf_path = "/home/jt07/xide_python_logging.txt" Linux EXAMPLE
#_xide_b_112320_logf_path = "C:/xide_python_logging.txt" Windows EXAMPLE
# Set this path to enable logging
_xide_b_112320_logf_path = ""

_xide_b_112320_logf = None
xIDEPythonLoggingEnabled = False
xIDEPythonDetailedLoggingEnabled = False

if "" != _xide_b_112320_logf_path:
    import datetime
    _xide_b_112320_logf = open(_xide_b_112320_logf_path, "a")
    theTimeOfLogCommencement = datetime.datetime.now().isoformat() + "\n"
    _xide_b_112320_logf.write(theTimeOfLogCommencement)
    _xide_b_112320_logf.flush()
    xIDEPythonLoggingEnabled = True

def extensionCallSetFlag(value = True):
    if 'app' in globals():
        if hasattr(app.debugger, 'currentProcessor'):
            if hasattr(app.debugger.currentProcessor.extension, 'pyMain'):
                app.debugger.currentProcessor.extension.pyMain = value

def inPlugin():
    if 'app' in globals():
        if hasattr(app.debugger, 'currentProcessor'):
            if hasattr(app.debugger.currentProcessor.extension, 'pyInPlugin'):
                return app.debugger.currentProcessor.extension.pyInPlugin
    return False

# B-112320 note
# Both the profile and trace functions are needed as attempting to put all the functionality in just the profile falis
# because behaviour differs between Linux and Windows plus also the "exception" event only seems available in
# the trace not profile functionality.
def loggingtracefun1(frame, event, arg):
    global xIDEPythonLoggingEnabled
    global xIDEPythonDetailedLoggingEnabled
    if xIDEPythonLoggingEnabled:
        logo = "t1>> " + event +"\n"
        _xide_b_112320_logf.write(logo)
        _xide_b_112320_logf.flush()
    return loggingtracefun2

exceptionTriggered = False
def loggingtracefun2(frame, event, arg):
    global xIDEPythonLoggingEnabled
    global xIDEPythonDetailedLoggingEnabled
    global exceptionTriggered
    detailedLogging = xIDEPythonLoggingEnabled and xIDEPythonDetailedLoggingEnabled
    if detailedLogging:
        _xide_b_112320_logf.write("  t2>> " + event + "\n")

    if "exception" == event:
        if detailedLogging:
            logo = "  >> " + str(frame.f_lineno) + " " + frame.f_code.co_name + " " + str(frame.f_exc_type) +"\n"
            _xide_b_112320_logf.write(logo)
        exceptionTriggered = True

    if frame.f_back == None and exceptionTriggered:
        extensionCallSetFlag(False)
        _xide_b_112320_sys.setprofile(profilefun)
        exceptionTriggered = False

    if detailedLogging:
        _xide_b_112320_logf.flush()

    return loggingtracefun2

_xide_b_112320_sys.settrace(loggingtracefun1)

callCount = 0
def profilefun(frame, event, arg):
    global xIDEPythonLoggingEnabled
    global xIDEPythonDetailedLoggingEnabled
    if xIDEPythonLoggingEnabled:
        _xide_b_112320_logf.write(event+"\n")
        _xide_b_112320_logf.flush()

    notInPlugin = (0 == inPlugin())
    if event == "call" or "c_call" == event:
        extensionCallSetFlag()
        if xIDEPythonLoggingEnabled and xIDEPythonDetailedLoggingEnabled:
            logo = "p>"+frame.f_code.co_name + "\n"
            _xide_b_112320_logf.write(logo)
            _xide_b_112320_logf.flush()
        if notInPlugin:
            app.processTraceEvents()
        return None

    if(frame.f_back == None and event == "return"):
        extensionCallSetFlag(False)
        return None

    if(frame.f_back == None and event == "exception"):
        extensionCallSetFlag(False)

    return None


_xide_b_112320_sys.setprofile(profilefun)

# Display a message when users try to quit, quit(), exit or exit()
class XideQuitter:
    def __call__(self,code=None):
        print self.msg
    def __init__(self,msg):
        self.msg = msg
    def __repr__(self):
        return self.msg

quit = XideQuitter("You cannot quit the Command Line")
exit = XideQuitter("You cannot exit the Command Line")

# Reinstall the trace function when a KeyboardInterrupt is received
def customExceptionHook(type, value, tb):
    if type == KeyboardInterrupt:
        _xide_m_1270_sys_dot_settrace(loggingtracefun1)
    _xide_m_1270_sys_dot_originalexcepthook(type, value, tb)

sys.excepthook = customExceptionHook

class XideInput:
    @staticmethod
    def raw_input(prompt = None):
        if prompt is None:
            prompt = ""
        response = app.inputBox('xIDE Python', prompt, "")
        if response is None:
            raise KeyboardInterrupt, "Operation cancelled by user"
        return response

    @staticmethod
    def input(prompt = None):
        return eval(raw_input(prompt))

sys.modules['__builtin__'].raw_input = XideInput.raw_input
sys.modules['__builtin__'].input = XideInput.input



# ****************************************************
# Work around M-2004
def onDebuggerStartedHandler():
    """Workaround for xide bug M-2004
       The VM project needs to be master when start a debug session."""
    vmId = '{2b2c6868-a56e-4266-962a-cddf1c979343}'
    if app.currentProject.executionEnvironmentId != vmId:
        for i in app.projects:
            if i.executionEnvironmentId == vmId:
                print 'Switching active project to VM '
                app.currentProject = i
                break

def onWorkspaceLoaded():
    for i in app.projects:
        if i.executionEnvironmentId == '{2b2c6868-a56e-4266-962a-cddf1c979343}':
            app.debugger.onDebuggerStarted = onDebuggerStartedHandler
            print "Installing VM project auto-selector"
            break

app.onWorkspaceLoaded = onWorkspaceLoaded
# End of Work around M-2004
#*****************************************************



import macros

# Optionally include the sample macros if the env variable XIDE_EXTENSIONS=1
try:
    from os import environ
    if environ.get("XIDE_EXTENSIONS") == "1":
        import sample_macros
except:
    None

