/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

// python.h should always be included first before any other includes
#include <Python.h>
#include <osdefs.h>

#include "system.h"
#include "PythonInvoker.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/legacy/Addon.h"
#include "interfaces/python/LanguageHook.h"
#include "interfaces/python/PyContext.h"
#include "interfaces/python/pythreadstate.h"
#include "interfaces/python/swig.h"
#include "interfaces/python/XBPython.h"
#include "threads/SingleLock.h"
#if defined(TARGET_WINDOWS)
#include "utils/CharsetConverter.h"
#endif // defined(TARGET_WINDOWS)
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#ifdef TARGET_WINDOWS
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

#define GC_SCRIPT \
  "import gc\n" \
  "gc.collect(2)\n"

#define PY_PATH_SEP DELIM

// Time before ill-behaved scripts are terminated
#define PYTHON_SCRIPT_TIMEOUT 5000 // ms

using namespace std;
using namespace XFILE;

extern "C"
{
  int xbp_chdir(const char *dirname);
  char* dll_getenv(const char* szKey);
}

#define PythonModulesSize sizeof(PythonModules) / sizeof(PythonModule)

CCriticalSection CPythonInvoker::s_critical;

static const CStdString getListOfAddonClassesAsString(XBMCAddon::AddonClass::Ref<XBMCAddon::Python::LanguageHook>& languageHook)
{
  CStdString message;
  XBMCAddon::AddonClass::Synchronize l(*(languageHook.get()));
  std::set<XBMCAddon::AddonClass*>& acs = languageHook->GetRegisteredAddonClasses();
  bool firstTime = true;
  for (std::set<XBMCAddon::AddonClass*>::iterator iter = acs.begin(); iter != acs.end(); ++iter)
  {
    if (!firstTime)
      message += ",";
    else
      firstTime = false;
    message += (*iter)->GetClassname().c_str();
  }

  return message;
}

CPythonInvoker::CPythonInvoker(ILanguageInvocationHandler *invocationHandler)
  : ILanguageInvoker(invocationHandler),
    m_source(NULL), m_argc(0), m_argv(NULL),
    m_threadState(NULL), m_stop(false)
{ }

CPythonInvoker::~CPythonInvoker()
{
  // nothing to do for the default invoker used for registration with the
  // CScriptInvocationManager
  if (GetId() < 0)
    return;

  if (GetState() < InvokerStateDone)
    CLog::Log(LOGDEBUG, "CPythonInvoker(%d): waiting for python thread \"%s\" to stop",
      GetId(), (m_source != NULL ? m_source : "unknown script"));
  Stop(true);
  g_pythonParser.PulseGlobalEvent();

  delete [] m_source;
  if (m_argv != NULL)
  {
    for (unsigned int i = 0; i < m_argc; i++)
      delete [] m_argv[i];
    delete [] m_argv;
  }
  g_pythonParser.FinalizeScript();
}

bool CPythonInvoker::Execute(const std::string &script, const std::vector<std::string> &arguments /* = std::vector<std::string>() */)
{
  if (script.empty())
    return false;

  if (!CFile::Exists(script))
  {
    CLog::Log(LOGERROR, "CPythonInvoker(%d): python script \"%s\" does not exist", GetId(), CSpecialProtocol::TranslatePath(script).c_str());
    return false;
  }

  if (!g_pythonParser.InitializeEngine())
    return false;

  return ILanguageInvoker::Execute(script, arguments);
}

bool CPythonInvoker::execute(const std::string &script, const std::vector<std::string> &arguments)
{
  // copy the code/script into a local string buffer
#ifdef TARGET_WINDOWS
  CStdString strsrc = script;
  g_charsetConverter.utf8ToSystem(strsrc);
  m_source = new char[strsrc.length() + 1];
  strcpy(m_source, strsrc);
#else
  m_source = new char[script.length() + 1];
  strcpy(m_source, script.c_str());
#endif

  // copy the arguments into a local buffer
  m_argc = arguments.size();
  m_argv = new char*[m_argc];
  for (unsigned int i = 0; i < m_argc; i++)
  {
    m_argv[i] = new char[arguments.at(i).length() + 1];
    strcpy(m_argv[i], arguments.at(i).c_str());
  }

  CLog::Log(LOGDEBUG, "CPythonInvoker(%d, %s): start processing", GetId(), m_source);
  int m_Py_file_input = Py_file_input;

  // get the global lock
  PyEval_AcquireLock();
  PyThreadState* state = Py_NewInterpreter();
  if (state == NULL)
  {
    PyEval_ReleaseLock();
    CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): FAILED to get thread state!", GetId(), m_source);
    return false;
  }
  // swap in my thread state
  PyThreadState_Swap(state);

  XBMCAddon::AddonClass::Ref<XBMCAddon::Python::LanguageHook> languageHook(new XBMCAddon::Python::LanguageHook(state->interp));
  languageHook->RegisterMe();

  onInitialization();
  setState(InvokerStateInitialized);

  CLog::Log(LOGDEBUG, "CPythonInvoker(%d, %s): the source file to load is %s", GetId(), m_source, m_source);

  // get path from script file name and add python path's
  // this is used for python so it will search modules from script path first
  CStdString scriptDir = URIUtils::GetDirectory(CSpecialProtocol::TranslatePath(m_source));
  URIUtils::RemoveSlashAtEnd(scriptDir);
  addPath(scriptDir);

  // add on any addon modules the user has installed
  ADDON::VECADDONS addons;
  ADDON::CAddonMgr::Get().GetAddons(ADDON::ADDON_SCRIPT_MODULE, addons);
  for (unsigned int i = 0; i < addons.size(); ++i)
    addPath(CSpecialProtocol::TranslatePath(addons[i]->LibPath()));

  // we want to use sys.path so it includes site-packages
  // if this fails, default to using Py_GetPath
  PyObject *sysMod(PyImport_ImportModule((char*)"sys")); // must call Py_DECREF when finished
  PyObject *sysModDict(PyModule_GetDict(sysMod)); // borrowed ref, no need to delete
  PyObject *pathObj(PyDict_GetItemString(sysModDict, "path")); // borrowed ref, no need to delete

  if (pathObj != NULL && PyList_Check(pathObj))
  {
    for (int i = 0; i < PyList_Size(pathObj); i++)
    {
      PyObject *e = PyList_GetItem(pathObj, i); // borrowed ref, no need to delete
      if (e != NULL && PyString_Check(e))
        addPath(PyString_AsString(e)); // returns internal data, don't delete or modify
    }
  }
  else
    addPath(Py_GetPath());

  Py_DECREF(sysMod); // release ref to sysMod

  // set current directory and python's path.
  if (m_argv != NULL)
    PySys_SetArgv(m_argc, m_argv);

  CLog::Log(LOGDEBUG, "CPythonInvoker(%d, %s): setting the Python path to %s", GetId(), m_source, m_pythonPath.c_str());
  PySys_SetPath((char *)m_pythonPath.c_str());

  CLog::Log(LOGDEBUG, "CPythonInvoker(%d, %s): entering source directory %s", GetId(), m_source, scriptDir.c_str());
  PyObject* module = PyImport_AddModule((char*)"__main__");
  PyObject* moduleDict = PyModule_GetDict(module);

  // when we are done initing we store thread state so we can be aborted
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  // we need to check if we was asked to abort before we had inited
  bool stopping = false;
  { CSingleLock lock(m_critical);
    m_threadState = state;
    stopping = m_stop;
  }

  PyEval_AcquireLock();
  PyThreadState_Swap(state);

  bool failed = false;
  if (!stopping)
  {
    try
    {
      // run script from file
      // We need to have python open the file because on Windows the DLL that python
      //  is linked against may not be the DLL that xbmc is linked against so
      //  passing a FILE* to python from an fopen has the potential to crash.
      PyObject* file = PyFile_FromString((char *) CSpecialProtocol::TranslatePath(m_source).c_str(), (char*)"r");
      FILE *fp = PyFile_AsFile(file);

      if (fp != NULL)
      {
        PyObject *f = PyString_FromString(CSpecialProtocol::TranslatePath(m_source).c_str());
        PyDict_SetItemString(moduleDict, "__file__", f);

        onPythonModuleInitialization(moduleDict);

        Py_DECREF(f);
        setState(InvokerStateRunning);
        XBMCAddon::Python::PyContext pycontext; // this is a guard class that marks this callstack as being in a python context
        PyRun_FileExFlags(fp, CSpecialProtocol::TranslatePath(m_source).c_str(), m_Py_file_input, moduleDict, moduleDict,1,NULL);
      }
      else
        CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): %s not found!", GetId(), m_source, m_source);
    }
    catch (const XbmcCommons::Exception& e)
    {
      setState(InvokerStateFailed);
      e.LogThrowMessage();
      failed = true;
    }
    catch (...)
    {
      setState(InvokerStateFailed);
      CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): failure in script", GetId(), m_source);
      failed = true;
    }
  }

  bool systemExitThrown = false;
  if (!failed && !PyErr_Occurred())
  {
    CLog::Log(LOGINFO, "CPythonInvoker(%d, %s): script successfully run", GetId(), m_source);
    setState(InvokerStateDone);
    onSuccess();
  }
  else if (PyErr_ExceptionMatches(PyExc_SystemExit))
  {
    systemExitThrown = true;
    CLog::Log(LOGINFO, "CPythonInvoker(%d, %s): script aborted", GetId(), m_source);
    setState(InvokerStateFailed);
    onAbort();
  }
  else
  {
    setState(InvokerStateFailed);

    // if it failed with an exception we already logged the details
    if (!failed)
    {
      PythonBindings::PythonToCppException e;
      e.LogThrowMessage();
    }

    onError();
  }

  // no need to do anything else because the script has already stopped
  if (failed)
    return true;

  PyObject *m = PyImport_AddModule((char*)"xbmc");
  if (m == NULL || PyObject_SetAttrString(m, (char*)"abortRequested", PyBool_FromLong(1)))
    CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): failed to set abortRequested", GetId(), m_source);

  // make sure all sub threads have finished
  for (PyThreadState* s = state->interp->tstate_head, *old = NULL; s;)
  {
    if (s == state)
    {
      s = s->next;
      continue;
    }
    if (old != s)
    {
      CLog::Log(LOGINFO, "CPythonInvoker(%d, %s): waiting on thread %"PRIu64, GetId(), m_source, (uint64_t)s->thread_id);
      old = s;
    }

    CPyThreadState pyState;
    Sleep(100);
    pyState.Restore();

    s = state->interp->tstate_head;
  }

  // pending calls must be cleared out
  XBMCAddon::RetardedAsynchCallbackHandler::clearPendingCalls(state);

  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  // set stopped event - this allows ::stop to run and kill remaining threads
  // this event has to be fired without holding m_critical
  // also the GIL (PyEval_AcquireLock) must not be held
  // if not obeyed there is still no deadlock because ::stop waits with timeout (smart one!)
  m_stoppedEvent.Set();

  { CSingleLock lock(m_critical);
    m_threadState = NULL;
  }

  PyEval_AcquireLock();
  PyThreadState_Swap(state);

  onDeinitialization();

  // run the gc before finishing
  //
  // if the script exited by throwing a SystemExit excepton then going back
  // into the interpreter causes this python bug to get hit:
  //    http://bugs.python.org/issue10582
  // and that causes major failures. So we are not going to go back in
  // to run the GC if that's the case.
  if (!m_stop && languageHook->HasRegisteredAddonClasses() && !systemExitThrown &&
      PyRun_SimpleString(GC_SCRIPT) == -1)
    CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): failed to run the gc to clean up after running prior to shutting down the Interpreter", GetId(), m_source);

  Py_EndInterpreter(state);

  // If we still have objects left around, produce an error message detailing what's been left behind
  if (languageHook->HasRegisteredAddonClasses())
    CLog::Log(LOGWARNING, "CPythonInvoker(%d, %s): the python script \"%s\" has left several "
      "classes in memory that we couldn't clean up. The classes include: %s",
      GetId(), m_source, m_source, getListOfAddonClassesAsString(languageHook).c_str());

  // unregister the language hook
  languageHook->UnregisterMe();

  PyEval_ReleaseLock();

  return true;
}

bool CPythonInvoker::stop(bool abort)
{
  CSingleLock lock(m_critical);
  m_stop = true;

  if (!IsRunning())
    return false;

  setState(InvokerStateStopping);

  if (m_threadState != NULL)
  {
    PyEval_AcquireLock();
    PyThreadState* old = PyThreadState_Swap((PyThreadState*)m_threadState);

    //tell xbmc.Monitor to call onAbortRequested()
    if (m_addon != NULL)
      g_pythonParser.OnAbortRequested(m_addon->ID());

    PyObject *m;
    m = PyImport_AddModule((char*)"xbmc");
    if (m == NULL || PyObject_SetAttrString(m, (char*)"abortRequested", PyBool_FromLong(1)))
      CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): failed to set abortRequested", GetId(), m_source);

    PyThreadState_Swap(old);
    old = NULL;
    PyEval_ReleaseLock();

    XbmcThreads::EndTime timeout(PYTHON_SCRIPT_TIMEOUT);
    while (!m_stoppedEvent.WaitMSec(15))
    {
      if (timeout.IsTimePast())
      {
        CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): script didn't stop in %d seconds - let's kill it", GetId(), m_source, PYTHON_SCRIPT_TIMEOUT / 1000);
        break;
      }

      // We can't empty-spin in the main thread and expect scripts to be able to
      // dismantle themselves. Python dialogs aren't normal XBMC dialogs, they rely
      // on TMSG_GUI_PYTHON_DIALOG messages, so pump the message loop.
      if (g_application.IsCurrentThread())
      {
        CSingleExit ex(g_graphicsContext);
        CApplicationMessenger::Get().ProcessMessages();
      }
    }

    // Useful for add-on performance metrics
    if (!timeout.IsTimePast())
      CLog::Log(LOGDEBUG, "CPythonInvoker(%d, %s): script termination took %dms", GetId(), m_source, PYTHON_SCRIPT_TIMEOUT - timeout.MillisLeft());

    // everything which didn't exit by now gets killed
    {
      // grabbing the PyLock while holding the m_critical is asking for a deadlock
      CSingleExit ex2(m_critical);
      PyEval_AcquireLock();
    }

    // Since we released the m_critical it's possible that the state is cleaned up
    // so we need to recheck for m_threadState == NULL
    if (m_threadState != NULL)
    {
      old = PyThreadState_Swap((PyThreadState*)m_threadState);
      for (PyThreadState* state = ((PyThreadState*)m_threadState)->interp->tstate_head; state; state = state->next)
      {
        // Raise a SystemExit exception in python threads
        Py_XDECREF(state->async_exc);
        state->async_exc = PyExc_SystemExit;
        Py_XINCREF(state->async_exc);
      }

      // If a dialog entered its doModal(), we need to wake it to see the exception
      g_pythonParser.PulseGlobalEvent();
    }

    if (old != NULL)
      PyThreadState_Swap(old);

    lock.Leave();
    PyEval_ReleaseLock();
  }

  return true;
}

void CPythonInvoker::onExecutionFailed()
{
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  setState(InvokerStateFailed);
  CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): abnormally terminating python thread", GetId(), m_source);

  CSingleLock lock(m_critical);
  m_threadState = NULL;

  ILanguageInvoker::onExecutionFailed();
}

std::map<std::string, CPythonInvoker::PythonModuleInitialization> CPythonInvoker::getModules() const
{
  static std::map<std::string, PythonModuleInitialization> modules;
  return modules;
}

void CPythonInvoker::onInitialization()
{
  TRACE;
  {
    GilSafeSingleLock lock(s_critical);
    initializeModules(getModules());
  }

  // get a possible initialization script
  const char* runscript = getInitializationScript();
  if (runscript!= NULL && strlen(runscript) > 0)
  {
    // redirecting default output to debug console
    if (PyRun_SimpleString(runscript) == -1)
      CLog::Log(LOGFATAL, "CPythonInvoker(%d, %s): initialize error", GetId(), m_source);
  }
}

void CPythonInvoker::onPythonModuleInitialization(void* moduleDict)
{
  if (m_addon.get() == NULL || moduleDict == NULL)
    return;

  PyObject *moduleDictionary = (PyObject *)moduleDict;

  PyObject *pyaddonid = PyString_FromString(m_addon->ID().c_str());
  PyDict_SetItemString(moduleDictionary, "__xbmcaddonid__", pyaddonid);

  CStdString version = ADDON::GetXbmcApiVersionDependency(m_addon);
  PyObject *pyxbmcapiversion = PyString_FromString(version.c_str());
  PyDict_SetItemString(moduleDictionary, "__xbmcapiversion__", pyxbmcapiversion);

  CLog::Log(LOGDEBUG, "CPythonInvoker(%d, %s): instantiating addon using automatically obtained id of \"%s\" dependent on version %s of the xbmc.python api",
            GetId(), m_source, m_addon->ID().c_str(), version.c_str());
}

void CPythonInvoker::onDeinitialization()
{
  TRACE;
}

void CPythonInvoker::onError()
{
  CPyThreadState releaseGil;
  CSingleLock gc(g_graphicsContext);

  CGUIDialogKaiToast *pDlgToast = (CGUIDialogKaiToast*)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
  if (pDlgToast != NULL)
  {
    CStdString desc;
    CStdString script;
    if (m_addon.get() != NULL)
      script = m_addon->Name();
    else
    {
      CStdString path;
      URIUtils::Split(m_source, path, script);
      if (script.Equals("default.py"))
      {
        CStdString path2;
        URIUtils::RemoveSlashAtEnd(path);
        URIUtils::Split(path, path2, script);
      }
    }

    desc.Format(g_localizeStrings.Get(2100), script);
    pDlgToast->QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(257), desc);
  }
}

const char* CPythonInvoker::getInitializationScript() const
{
  return NULL;
}

void CPythonInvoker::initializeModules(const std::map<std::string, PythonModuleInitialization> &modules)
{
  for (std::map<std::string, PythonModuleInitialization>::const_iterator module = modules.begin(); module != modules.end(); ++module)
  {
    if (!initializeModule(module->second))
      CLog::Log(LOGWARNING, "CPythonInvoker(%d, %s): unable to initialize python module \"%s\"", GetId(), m_source, module->first.c_str());
  }
}

bool CPythonInvoker::initializeModule(PythonModuleInitialization module)
{
  if (module == NULL)
    return false;

  module();
  return true;
}

void CPythonInvoker::addPath(const std::string path)
{
  if (path.empty())
    return;

  if (!m_pythonPath.empty())
    m_pythonPath += PY_PATH_SEP;

#if defined(TARGET_WINDOWS)
  CStdString tmp(path);
  g_charsetConverter.utf8ToSystem(tmp);
  m_pythonPath += tmp;
#else
  m_pythonPath += path;
#endif // defined(TARGET_WINDOWS)
}
