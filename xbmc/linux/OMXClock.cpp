/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#elif defined(TARGET_WINDOWS)
#include "system.h"
#endif

#if defined(HAVE_OMXLIB)

#include "video/VideoReferenceClock.h"
#include "settings/Settings.h"

#include "OMXClock.h"
#include "utils/MathUtils.h"

#define OMX_PRE_ROLL 200
#define TP(speed) ((speed) < 0 || (speed) > 4*DVD_PLAYSPEED_NORMAL)

OMXClock::OMXClock()
{
  m_pause       = false;

  m_fps = 25.0f;
  m_omx_speed = DVD_PLAYSPEED_NORMAL;
  m_WaitMask = 0;
  m_eState = OMX_TIME_ClockStateStopped;
  m_eClock = OMX_TIME_RefClockNone;
  m_clock        = NULL;

  pthread_mutex_init(&m_lock, NULL);
}

OMXClock::~OMXClock()
{
  OMXDeinitialize();
  pthread_mutex_destroy(&m_lock);
}

void OMXClock::Lock()
{
  pthread_mutex_lock(&m_lock);
}

void OMXClock::UnLock()
{
  pthread_mutex_unlock(&m_lock);
}

void OMXClock::OMXSetClockPorts(OMX_TIME_CONFIG_CLOCKSTATETYPE *clock, bool has_video, bool has_audio)
{
  if(m_omx_clock.GetComponent() == NULL)
    return;

  if(!clock)
    return;

  clock->nWaitMask = 0;

  if(has_audio)
  {
    clock->nWaitMask |= OMX_CLOCKPORT0;
  }

  if(has_video)
  {
    clock->nWaitMask |= OMX_CLOCKPORT1;
  }
}

bool OMXClock::OMXSetReferenceClock(bool has_audio, bool lock /* = true */)
{
  if(lock)
    Lock();

  bool ret = true;
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE refClock;
  OMX_INIT_STRUCTURE(refClock);

  if(has_audio)
    refClock.eClock = OMX_TIME_RefClockAudio;
  else
    refClock.eClock = OMX_TIME_RefClockVideo;

  if (refClock.eClock != m_eClock)
  {
    CLog::Log(LOGNOTICE, "OMXClock using %s as reference", refClock.eClock == OMX_TIME_RefClockVideo ? "video" : "audio");

    omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeActiveRefClock, &refClock);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::OMXSetReferenceClock error setting OMX_IndexConfigTimeActiveRefClock");
      ret = false;
    }
    m_eClock = refClock.eClock;
  }
  if(lock)
    UnLock();

  return ret;
}

bool OMXClock::OMXInitialize(CDVDClock *clock)
{
  std::string componentName = "";

  m_pause       = false;

  m_clock = clock;

  componentName = "OMX.broadcom.clock";
  if(!m_omx_clock.Initialize((const std::string)componentName, OMX_IndexParamOtherInit))
    return false;

  m_omx_clock.DisableAllPorts();

  return true;
}

void OMXClock::OMXDeinitialize()
{
  if(m_omx_clock.GetComponent() == NULL)
    return;

  m_omx_clock.Deinitialize();

  m_omx_speed = DVD_PLAYSPEED_NORMAL;
}

bool OMXClock::OMXStateExecute(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(m_omx_clock.GetState() != OMX_StateExecuting)
  {

    OMXStateIdle(false);

    omx_err = m_omx_clock.SetStateForComponent(OMX_StateExecuting);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::StateExecute m_omx_clock.SetStateForComponent\n");
      if(lock)
        UnLock();
      return false;
    }
  }

  if(lock)
    UnLock();

  return true;
}

void OMXClock::OMXStateIdle(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return;

  if(lock)
    Lock();

  if(m_omx_clock.GetState() != OMX_StateIdle)
    m_omx_clock.SetStateForComponent(OMX_StateIdle);

  if(lock)
    UnLock();
}

COMXCoreComponent *OMXClock::GetOMXClock()
{
  return &m_omx_clock;
}

bool  OMXClock::OMXStop(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  CLog::Log(LOGDEBUG, "OMXClock::OMXStop\n");

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
  OMX_INIT_STRUCTURE(clock);

  clock.eState      = OMX_TIME_ClockStateStopped;
  clock.nOffset     = ToOMXTime(-1000LL * OMX_PRE_ROLL);

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::Stop error setting OMX_IndexConfigTimeClockState\n");
    if(lock)
      UnLock();
    return false;
  }
  m_eState = clock.eState;

  if(lock)
    UnLock();

  return true;
}

bool OMXClock::OMXStep(int steps /* = 1 */, bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_PARAM_U32TYPE param;
  OMX_INIT_STRUCTURE(param);

  param.nPortIndex = OMX_ALL;
  param.nU32 = steps;

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigSingleStep, &param);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::Error setting OMX_IndexConfigSingleStep\n");
    if(lock)
      UnLock();
    return false;
  }

  if(lock)
    UnLock();

  CLog::Log(LOGDEBUG, "OMXClock::Step (%d)", steps);
  return true;
}

bool OMXClock::OMXReset(bool has_video, bool has_audio, bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  if(!OMXSetReferenceClock(has_audio, false))
  {
    if(lock)
      UnLock();
    return false;
  }

  if (m_eState == OMX_TIME_ClockStateStopped)
  {
    OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
    OMX_INIT_STRUCTURE(clock);

    clock.eState    = OMX_TIME_ClockStateWaitingForStartTime;
    clock.nOffset   = ToOMXTime(-1000LL * OMX_PRE_ROLL);

    OMXSetClockPorts(&clock, has_video, has_audio);

    if(clock.nWaitMask)
    {
      OMX_ERRORTYPE omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
      if(omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "OMXClock::OMXReset error setting OMX_IndexConfigTimeClockState\n");
        if(lock)
          UnLock();
        return false;
      }
      CLog::Log(LOGDEBUG, "OMXClock::OMXReset audio / video : %d / %d wait mask %d->%d state : %d->%d\n",
          has_audio, has_video, m_WaitMask, clock.nWaitMask, m_eState, clock.eState);
      if (m_eState != OMX_TIME_ClockStateStopped)
        m_WaitMask = clock.nWaitMask;
      m_eState = clock.eState;
    }
  }

  if(lock)
    UnLock();

  return true;
}

double OMXClock::OMXMediaTime(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return 0;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  double pts = 0;

  OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
  OMX_INIT_STRUCTURE(timeStamp);
  timeStamp.nPortIndex = m_omx_clock.GetInputPort();

  omx_err = m_omx_clock.GetConfig(OMX_IndexConfigTimeCurrentMediaTime, &timeStamp);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::MediaTime error getting OMX_IndexConfigTimeCurrentMediaTime\n");
    if(lock)
      UnLock();
    return 0;
  }

  pts = FromOMXTime(timeStamp.nTimestamp);

  if(lock)
    UnLock();
  
  return pts;
}

double OMXClock::OMXClockAdjustment(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return 0;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  double pts = 0;

  OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
  OMX_INIT_STRUCTURE(timeStamp);
  timeStamp.nPortIndex = m_omx_clock.GetInputPort();

  omx_err = m_omx_clock.GetConfig(OMX_IndexConfigClockAdjustment, &timeStamp);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::MediaTime error getting OMX_IndexConfigClockAdjustment\n");
    if(lock)
      UnLock();
    return 0;
  }

  pts = (double)FromOMXTime(timeStamp.nTimestamp);
  //CLog::Log(LOGINFO, "OMXClock::ClockAdjustment %.0f %.0f\n", (double)FromOMXTime(timeStamp.nTimestamp), pts);
  if(lock)
    UnLock();

  return pts;
}


// Set the media time, so calls to get media time use the updated value,
// useful after a seek so mediatime is updated immediately (rather than waiting for first decoded packet)
bool OMXClock::OMXMediaTime(double pts, bool lock /* = true*/)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_INDEXTYPE index;
  OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
  OMX_INIT_STRUCTURE(timeStamp);
  timeStamp.nPortIndex = m_omx_clock.GetInputPort();

  if(m_eClock == OMX_TIME_RefClockAudio)
    index = OMX_IndexConfigTimeCurrentAudioReference;
  else
    index = OMX_IndexConfigTimeCurrentVideoReference;

  timeStamp.nTimestamp = ToOMXTime(pts);

  omx_err = m_omx_clock.SetConfig(index, &timeStamp);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::OMXMediaTime error setting %s", index == OMX_IndexConfigTimeCurrentAudioReference ?
       "OMX_IndexConfigTimeCurrentAudioReference":"OMX_IndexConfigTimeCurrentVideoReference");
    if(lock)
      UnLock();
    return false;
  }

  CLog::Log(LOGDEBUG, "OMXClock::OMXMediaTime set config %s = %.2f", index == OMX_IndexConfigTimeCurrentAudioReference ?
       "OMX_IndexConfigTimeCurrentAudioReference":"OMX_IndexConfigTimeCurrentVideoReference", pts);

  if(lock)
    UnLock();

  return true;
}

bool OMXClock::OMXPause(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(!m_pause)
  {
    if(lock)
      Lock();

    if (OMXSetSpeed(0, false, true))
      m_pause = true;

    if(lock)
      UnLock();
  }
  return m_pause == true;
}

bool OMXClock::OMXResume(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(m_pause)
  {
    if(lock)
      Lock();

    if (OMXSetSpeed(m_omx_speed, false, true))
      m_pause = false;

    if(lock)
      UnLock();
  }
  return m_pause == false;
}

bool OMXClock::OMXSetSpeed(int speed, bool lock /* = true */, bool pause_resume /* = false */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  CLog::Log(LOGDEBUG, "OMXClock::OMXSetSpeed(%.2f) pause_resume:%d", (float)speed / (float)DVD_PLAYSPEED_NORMAL, pause_resume);

  if (pause_resume)
  {
    OMX_ERRORTYPE omx_err = OMX_ErrorNone;
    OMX_TIME_CONFIG_SCALETYPE scaleType;
    OMX_INIT_STRUCTURE(scaleType);

    if (TP(speed))
      scaleType.xScale = 0; // for trickplay we just pause, and single step
    else
      scaleType.xScale = (speed << 16) / DVD_PLAYSPEED_NORMAL;
    omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeScale, &scaleType);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::OMXSetSpeed error setting OMX_IndexConfigTimeClockState\n");
      if(lock)
        UnLock();
      return false;
    }
  }
  if (!pause_resume)
    m_omx_speed = speed;

  if(lock)
    UnLock();

  return true;
}

bool OMXClock::HDMIClockSync(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
  OMX_INIT_STRUCTURE(latencyTarget);

  latencyTarget.nPortIndex = OMX_ALL;
  latencyTarget.bEnabled = OMX_TRUE;
  latencyTarget.nFilter = 10;
  latencyTarget.nTarget = 0;
  latencyTarget.nShift = 3;
  latencyTarget.nSpeedFactor = -200;
  latencyTarget.nInterFactor = 100;
  latencyTarget.nAdjCap = 100;

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigLatencyTarget, &latencyTarget);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::Speed error setting OMX_IndexConfigLatencyTarget\n");
    if(lock)
      UnLock();
    return false;
  }

  if(lock)
    UnLock();

  return true;
}

int64_t OMXClock::CurrentHostCounter(void)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return( ((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec );
}

int64_t OMXClock::CurrentHostFrequency(void)
{
  return( (int64_t)1000000000L );
}

int OMXClock::GetRefreshRate(double* interval)
{
  if(!interval)
    return false;

  *interval = m_fps;
  return true;
}

double OMXClock::NormalizeFrameduration(double frameduration)
{
  //if the duration is within 20 microseconds of a common duration, use that
  const double durations[] = {DVD_TIME_BASE * 1.001 / 24.0, DVD_TIME_BASE / 24.0, DVD_TIME_BASE / 25.0,
                              DVD_TIME_BASE * 1.001 / 30.0, DVD_TIME_BASE / 30.0, DVD_TIME_BASE / 50.0,
                              DVD_TIME_BASE * 1.001 / 60.0, DVD_TIME_BASE / 60.0};

  double lowestdiff = DVD_TIME_BASE;
  int    selected   = -1;
  for (size_t i = 0; i < sizeof(durations) / sizeof(durations[0]); i++)
  {
    double diff = fabs(frameduration - durations[i]);
    if (diff < DVD_MSEC_TO_TIME(0.02) && diff < lowestdiff)
    {
      selected = i;
      lowestdiff = diff;
    }
  }

  if (selected != -1)
    return durations[selected];
  else
    return frameduration;
}

#endif
