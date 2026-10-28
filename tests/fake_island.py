#!/usr/bin/python3
"""A stand-in for the status bar's Island (com.lingmo.Island) on a private test bus:
logs every call as one JSON line, so the e2e test can check the progress and the actions.
usage: fake_island.py LOGFILE"""
import json
import sys

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

IFACE = "com.lingmo.Island"


class Island(dbus.service.Object):
    def __init__(self, bus, log):
        super().__init__(bus, "/Island")
        self.log = open(log, "a", buffering=1)
        self.next = 1

    def write(self, **kw):
        self.log.write(json.dumps(kw, default=str) + "\n")

    @dbus.service.method(IFACE, in_signature="sssssdas", out_signature="u")
    def Start(self, app, kind, title, subtitle, icon, progress, actions):
        return self.StartWithHints(app, kind, title, subtitle, icon, progress, actions, {})

    @dbus.service.method(IFACE, in_signature="sssssdasa{sv}", out_signature="u")
    def StartWithHints(self, app, kind, title, subtitle, icon, progress, actions, hints):
        i = self.next
        self.next += 1
        self.write(call="Start", id=i, app=str(app), kind=str(kind), title=str(title), subtitle=str(subtitle),
                   progress=float(progress), actions=[str(a) for a in actions])
        return dbus.UInt32(i)

    @dbus.service.method(IFACE, in_signature="usssdas", out_signature="b")
    def Update(self, i, title, subtitle, icon, progress, actions):
        self.write(call="Update", id=int(i), title=str(title), subtitle=str(subtitle), progress=float(progress),
                   actions=[str(a) for a in actions])
        return True

    @dbus.service.method(IFACE, in_signature="ua{sv}", out_signature="b")
    def UpdateFields(self, i, fields):
        self.write(call="UpdateFields", id=int(i), fields={str(k): (list(map(str, v)) if isinstance(v, dbus.Array) else str(v))
                                                          for k, v in fields.items()})
        return True

    @dbus.service.method(IFACE, in_signature="us", out_signature="b")
    def Finish(self, i, message):
        self.write(call="Finish", id=int(i), message=str(message))
        return True

    @dbus.service.method(IFACE, in_signature="u", out_signature="b")
    def Dismiss(self, i):
        self.write(call="Dismiss", id=int(i))
        return True

    @dbus.service.method(IFACE, in_signature="", out_signature="av")
    def List(self):
        return dbus.Array([], signature="v")

    @dbus.service.signal(IFACE, signature="us")
    def ActionInvoked(self, i, action):
        pass

    # test hook: as if the user clicked an action of the activity
    @dbus.service.method("com.lingmo.IslandTest", in_signature="us", out_signature="")
    def Invoke(self, i, action):
        self.write(call="Invoke", id=int(i), action=str(action))
        self.ActionInvoked(i, action)

    @dbus.service.signal(IFACE, signature="u")
    def Closed(self, i):
        pass


DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
name = dbus.service.BusName("com.lingmo.Island", bus)
island = Island(bus, sys.argv[1])
GLib.MainLoop().run()
