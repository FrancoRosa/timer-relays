#!/usr/bin/env python3
"""Local UI simulator for the relay scheduler.

Serves the exact same page embedded in relay-scheduler/src/webui.h and
fakes the firmware's HTTP API in memory (schedules, overrides, channel
names, a simulated clock) using only the Python standard library.

webui.h is re-read on every request to "/", so editing the page and
reloading the browser shows changes immediately -- no compile/flash
cycle. The API logic (schedule evaluation, validation) is a parallel
reimplementation of src/evaluator.cpp / src/schedule.cpp / src/webapi.cpp
in Python: good enough for UI work, but it can drift from the firmware
if the API contract changes there, so re-check against the real device
before relying on it for anything behavior-sensitive.

Usage:
    python3 scripts/simulate.py [port]      # default port 8000
"""
import http.server
import json
import os
import re
import sys
import time

ROOT = os.path.dirname(os.path.abspath(__file__))
WEBUI_PATH = os.path.join(ROOT, "..", "relay-scheduler", "src", "webui.h")

CHANNEL_COUNT = 8
MAX_SCHEDULES = 64
MAX_NAME_LEN = 24
SECONDS_PER_DAY = 86400
SECONDS_PER_WEEK = 604800


def load_index_html():
    with open(WEBUI_PATH, "r", encoding="utf-8") as f:
        text = f.read()
    start = text.index('R"WEBUI(') + len('R"WEBUI(')
    end = text.index(')WEBUI";', start)
    return text[start:end]


# ---- Evaluator (mirrors src/evaluator.cpp) ---------------------------------

def window_covers(window_start, dur, week_second):
    window_end = window_start + dur
    if window_end <= SECONDS_PER_WEEK:
        return window_start <= week_second < window_end
    wrapped_end = window_end - SECONDS_PER_WEEK
    return week_second >= window_start or week_second < wrapped_end


def evaluator_is_on(schedules, ch, week_second):
    for s in schedules:
        if not s["en"] or s["ch"] != ch:
            continue
        for d in range(7):
            if not (s["days"] & (1 << d)):
                continue
            window_start = (d * SECONDS_PER_DAY + s["start"]) % SECONDS_PER_WEEK
            if window_covers(window_start, s["dur"], week_second):
                return True
    return False


# ---- TZ (mirrors src/clock.cpp's use of a POSIX TZ string) -----------------

def parse_tz_offset_seconds(tz):
    """Good enough for the fixed-offset POSIX strings this project uses,
    e.g. "<-05>5" (Lima, UTC-5): the <...> is just a label, the trailing
    signed number is hours (POSIX sign convention: west of UTC is
    positive) west of UTC."""
    m = re.match(r"^(?:<[^>]*>|[A-Za-z]+)?\s*([+-]?\d{1,3})(?::(\d{2}))?(?::(\d{2}))?", tz or "")
    if not m:
        return 0
    hours = int(m.group(1))
    minutes = int(m.group(2) or 0)
    seconds = int(m.group(3) or 0)
    sign = -1 if hours < 0 else 1
    return sign * (abs(hours) * 3600 + minutes * 60 + seconds)


def local_time_fields(epoch, tz):
    local_epoch = epoch - parse_tz_offset_seconds(tz)
    t = time.gmtime(local_epoch)
    week_day = (t.tm_wday + 1) % 7  # Python Mon=0..Sun=6 -> our Sun=0..Sat=6
    week_second = week_day * SECONDS_PER_DAY + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec
    local_str = time.strftime("%Y-%m-%d %H:%M:%S", t)
    return local_str, week_second


# ---- In-memory state (mirrors ScheduleStore + SettingsStore + RelayDriver) -

class State:
    def __init__(self):
        self.schedules = []
        self.next_id = 1
        self.tz = "<-05>5"
        self.channel_names = [""] * CHANNEL_COUNT
        self.overrides = ["auto"] * CHANNEL_COUNT
        self.base_epoch = 0
        self.base_monotonic = time.monotonic()
        self.valid = False  # unverified until setEpoch() is called, like SoftClock

    def now_epoch(self):
        return int(self.base_epoch + (time.monotonic() - self.base_monotonic))

    def set_epoch(self, epoch):
        self.base_epoch = epoch
        self.base_monotonic = time.monotonic()
        self.valid = True

    def find_schedule(self, sid):
        return next((s for s in self.schedules if s["id"] == sid), None)

    def validate_schedule(self, s, is_create):
        if len(s.get("name") or "") > MAX_NAME_LEN:
            return "name too long (max 24 characters)"
        if not (0 <= s.get("ch", -1) < CHANNEL_COUNT):
            return "ch out of range (0-%d)" % (CHANNEL_COUNT - 1)
        days = s.get("days", 0)
        if days == 0 or days > 0x7F:
            return "days must have at least one of bits 0-6 set"
        if not (0 <= s.get("start", -1) <= 86399):
            return "start out of range (0-86399)"
        if not (1 <= s.get("dur", 0) <= 86400):
            return "dur out of range (1-86400)"
        if is_create and len(self.schedules) >= MAX_SCHEDULES:
            return "schedule limit reached (max 64)"
        return None

    def channel_is_on(self, ch, week_second):
        mode = self.overrides[ch]
        if mode == "on":
            return True
        if mode == "off":
            return False
        return self.valid and evaluator_is_on(self.schedules, ch, week_second)

    def state_json(self):
        epoch = self.now_epoch()
        local_str, week_second = local_time_fields(epoch, self.tz)
        return {
            "epoch": epoch,
            "valid": self.valid,
            "tz": self.tz,
            "local": local_str,
            "weekSecond": week_second,
            "channels": [
                {
                    "on": self.channel_is_on(ch, week_second),
                    "mode": self.overrides[ch],
                    "name": self.channel_names[ch],
                }
                for ch in range(CHANNEL_COUNT)
            ],
        }


state = State()


# ---- HTTP plumbing ----------------------------------------------------------

class Handler(http.server.BaseHTTPRequestHandler):
    server_version = "RelaySchedulerSim/1.0"

    def log_message(self, fmt, *args):
        pass  # keep the console quiet; errors still raise

    def _send_json(self, code, payload):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_html(self, code, html):
        body = html.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _error(self, code, reason):
        self._send_json(code, {"error": reason})

    def _read_json_body(self):
        length = int(self.headers.get("Content-Length") or 0)
        if length == 0:
            return None, "missing body"
        raw = self.rfile.read(length)
        try:
            return json.loads(raw), None
        except json.JSONDecodeError:
            return None, "invalid JSON"

    # -- routing --

    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            try:
                html = load_index_html()
            except (OSError, ValueError) as e:
                self._send_html(500, "<pre>Could not load webui.h: %s</pre>" % e)
                return
            self._send_html(200, html)
        elif self.path == "/api/state":
            self._send_json(200, state.state_json())
        elif self.path == "/api/schedules":
            self._send_json(200, state.schedules)
        else:
            self._error(404, "not found")

    def do_POST(self):
        if self.path == "/api/time":
            self._handle_post_time()
        elif self.path == "/api/schedules":
            self._handle_post_schedule()
        elif self.path.startswith("/api/relays/") and self.path.endswith("/mode"):
            self._handle_set_mode()
        elif self.path.startswith("/api/channels/") and self.path.endswith("/name"):
            self._handle_set_channel_name()
        else:
            self._error(404, "not found")

    def do_PUT(self):
        if self.path.startswith("/api/schedules/"):
            self._handle_put_schedule()
        else:
            self._error(404, "not found")

    def do_DELETE(self):
        if self.path.startswith("/api/schedules/"):
            self._handle_delete_schedule()
        else:
            self._error(404, "not found")

    # -- handlers --

    def _handle_post_time(self):
        body, err = self._read_json_body()
        if err:
            self._error(400, err)
            return
        if "epoch" not in body:
            self._error(400, "missing epoch")
            return
        state.set_epoch(int(body["epoch"]))
        if body.get("tz"):
            state.tz = body["tz"]
        self._send_json(200, state.state_json())

    def _handle_post_schedule(self):
        body, err = self._read_json_body()
        if err:
            self._error(400, err)
            return
        s = {
            "id": -1,
            "name": body.get("name", ""),
            "ch": body.get("ch", 0),
            "days": body.get("days", 0),
            "start": body.get("start", 0),
            "dur": body.get("dur", 0),
            "en": body.get("en", True),
        }
        reason = state.validate_schedule(s, is_create=True)
        if reason:
            self._error(400, reason)
            return
        s["id"] = state.next_id
        state.next_id += 1
        state.schedules.append(s)
        self._send_json(201, s)

    def _handle_put_schedule(self):
        sid = int(self.path.rsplit("/", 1)[-1])
        existing = state.find_schedule(sid)
        if existing is None:
            self._error(400, "schedule not found")
            return
        body, err = self._read_json_body()
        if err:
            self._error(400, err)
            return
        s = {
            "id": sid,
            "name": body.get("name", ""),
            "ch": body.get("ch", 0),
            "days": body.get("days", 0),
            "start": body.get("start", 0),
            "dur": body.get("dur", 0),
            "en": body.get("en", True),
        }
        reason = state.validate_schedule(s, is_create=False)
        if reason:
            self._error(400, reason)
            return
        existing.update(s)
        self._send_json(200, existing)

    def _handle_delete_schedule(self):
        sid = int(self.path.rsplit("/", 1)[-1])
        existing = state.find_schedule(sid)
        if existing is None:
            self._error(404, "schedule not found")
            return
        state.schedules.remove(existing)
        self.send_response(204)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def _handle_set_mode(self):
        ch = int(self.path.split("/")[3])
        if not (0 <= ch < CHANNEL_COUNT):
            self._error(404, "channel out of range")
            return
        body, err = self._read_json_body()
        if err:
            self._error(400, err)
            return
        mode = body.get("mode")
        if mode not in ("auto", "on", "off"):
            self._error(400, "mode must be auto, on or off")
            return
        state.overrides[ch] = mode
        self._send_json(200, state.state_json())

    def _handle_set_channel_name(self):
        ch = int(self.path.split("/")[3])
        if not (0 <= ch < CHANNEL_COUNT):
            self._error(404, "channel out of range")
            return
        body, err = self._read_json_body()
        if err:
            self._error(400, err)
            return
        name = body.get("name", "") or ""
        if len(name) > MAX_NAME_LEN:
            self._error(400, "name too long (max 24 characters)")
            return
        state.channel_names[ch] = name
        self._send_json(200, state.state_json())


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print(f"Relay scheduler UI simulator: http://127.0.0.1:{port}/")
    print(f"Serving {WEBUI_PATH} live -- edit it and reload the browser to see changes.")
    print("Ctrl+C to stop.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
