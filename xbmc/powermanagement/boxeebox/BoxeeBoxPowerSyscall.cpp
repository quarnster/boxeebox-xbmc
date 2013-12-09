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
#if defined(TARGET_BOXEE)

#include "BoxeeBoxPowerSyscall.h"
#include "utils/log.h"

CBoxeeBoxPowerSyscall::CBoxeeBoxPowerSyscall()
{
  CLog::Log(LOGDEBUG, "Using BoxeeBoxPowerSyscall");
}

bool CBoxeeBoxPowerSyscall::Powerdown()
{
  CLog::Log(LOGDEBUG, "BoxeeBoxPowerSyscall.Powerdown");
  return system("poweroff") == 0;
}

bool CBoxeeBoxPowerSyscall::Reboot()
{
  CLog::Log(LOGDEBUG, "BoxeeBoxPowerSyscall.Reboot");
  return system("reboot") == 0;
}

#endif
