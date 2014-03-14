#!/usr/bin/python
# -*- coding: utf-8 -*-
#
#     Copyright (C) 2013 Team-XBMC
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program. If not, see <http://www.gnu.org/licenses/>.
#


import platform
import urllib
import re
import xbmc
import lib.common
from lib.common import log, dialog_yesno
from lib.common import upgrade_message as _upgrademessage

__addon__        = lib.common.__addon__
__addonversion__ = lib.common.__addonversion__
__addonname__    = lib.common.__addonname__
__addonpath__    = lib.common.__addonpath__
__icon__         = lib.common.__icon__
__localize__     = lib.common.__localize__


class Main:
    def __init__(self):
        linux = False
        packages = []
        if __addon__.getSetting("versioncheck_enable") == 'true' and not xbmc.getCondVisibility('System.HasAddon(os.openelec.tv)'):
            if not sys.argv[0]:
                xbmc.executebuiltin('XBMC.AlarmClock(CheckAtBoot,XBMC.RunScript(service.xbmc.versioncheck, started),00:00:30,silent)')
                xbmc.executebuiltin('XBMC.AlarmClock(CheckWhileRunning,XBMC.RunScript(service.xbmc.versioncheck, started),24:00:00,silent,loop)')
            elif sys.argv[0] and sys.argv[1] == 'started':
                if xbmc.getCondVisibility('System.Platform.Linux'):
                    packages = ['xbmc']
                    _versionchecklinux(packages)
                else:
                    oldversion, msg = _versioncheck()
                    if oldversion:
                        _upgrademessage(msg, False)
            else:
                pass

def _versioncheck():
    # initial vars
    from lib.jsoninterface import get_installedversion, get_versionfilelist
    from lib.versions import compare_version
    # retrieve versionlists from supplied version file
    versionlist = get_versionfilelist()
    # retrieve version installed
    version_installed = get_installedversion()
    # copmpare installed and available
    oldversion, msg = compare_version(version_installed, versionlist)
    return oldversion, msg

def _versionchecklinux(packages):
    if platform.dist()[0].lower() in ['ubuntu', 'debian', 'linuxmint']:
        handler = False
        result = False
        try:
            # try aptdeamon first
            from lib.aptdeamonhandler import AptdeamonHandler
            handler = AptdeamonHandler()
        except:
            # fallback to shell
            # since we need the user password, ask to check for new version first
            from lib.shellhandlerapt import ShellHandlerApt
            sudo = True
            handler = ShellHandlerApt(sudo)
            if dialog_yesno(32015):
                pass
            elif dialog_yesno(32009, 32010):
                log("disabling addon by user request")
                __addon__.setSetting("versioncheck_enable", 'false')
                return

        if handler:
            if handler.check_upgrade_available(packages[0]):
                if _upgrademessage(32012, True):
                    if __addon__.getSetting("upgrade_system") == "false":
                        result = handler.upgrade_package(packages[0])
                    else:
                        result = handler.upgrade_system()
                    if result:
                        from lib.common import message_upgrade_success, message_restart
                        message_upgrade_success()
                        message_restart()
                    else:
                        log("Error during upgrade")
        else:
            log("Error: no handler found")

    elif platform.dist(supported_dists=('boxeebox'))[0].lower() == "boxeebox":
        oldversion, msg = _versioncheckboxee()
        if oldversion:
            xbmc.executebuiltin("XBMC.Notification(%s, %s, %d, %s)" %(__addonname__,
                                                                      msg,
                                                                      15000,
                                                                      __icon__))

    else:
        log("Unsupported platform %s" %platform.dist()[0])
        sys.exit(0)

def _versioncheckboxee():
    oldversion = False
    msg = ''

    # get localversion (date, hash)
    local_version = list(re.findall("(\d{8})-(.+)$", xbmc.getInfoLabel("System.BuildVersion"))[0])

    # get remoteversion (date, hash)
    stream = urllib.urlopen("http://devil-strike.com/xbmc_boxeebox/changelog.txt")
    changelog = stream.read()
    stream.close()

    for line in changelog.split('\n'):
        line = line.lower().strip()
        if line.startswith('changelog:'):
            remote_version = list(re.findall("(\d{4}\.\d{2}\.\d{2}).*_(.{7,})$", line[11:].strip())[0])
            break

    remote_version[0] = remote_version[0].replace('.', '')
    if remote_version[0] > local_version[0] or remote_version[1] != local_version[1]:
        oldversion = True
        msg = "Check http://boxeed.in/forums/viewtopic.php?f=13&t=148 for newest version."

    log("Boxee recognized: %s vs %s" %(local_version, remote_version))
    return oldversion, msg

if (__name__ == "__main__"):
    log('Version %s started' % __addonversion__)
    Main()
