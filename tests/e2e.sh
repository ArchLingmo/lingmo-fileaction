#!/bin/bash
# End-to-end test of the drag-and-tap gesture, the wheel, the engine and the Island,
# in a throwaway session: own Xvfb display, scratch HOME (every XDG folder inside it),
# private D-Bus with no service directories (nothing installed gets activated), a fake
# Island that logs what it is told. Nothing touches the real session or the user's files.
#
#   tests/e2e.sh OUTDIR [light|dark]
#
# Env: BUILD (dir with the lingmo-fileaction binary), LINGMO_FILEACTION_ENGINE (engine command),
#      FA_DISPLAY (Xvfb display number, default 97).
# Writes OUTDIR/<theme>/: screenshots, daemon.log, island.log, results.txt (PASS/FAIL lines with timings).
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${1:?usage: e2e.sh OUTDIR [light|dark]}
THEME=${2:-light}
BUILD=${BUILD:-$HERE/../build}
DISP=${FA_DISPLAY:-97}

if [ -z "${FA_INNER:-}" ]; then
    S=$OUT/$THEME
    rm -rf "$S"
    mkdir -p "$S"/home/{.config/lingmoos,.cache,.local/share/lingmo/intents,.local/state,Pictures,Music,Videos} "$S"/empty "$S"/run
    chmod 700 "$S/run"
    BUSDIR=$(mktemp -d /tmp/fa-bus.XXXXXX)   # socket paths must stay short
    export HOME=$S/home XDG_CONFIG_HOME=$S/home/.config XDG_CACHE_HOME=$S/home/.cache \
           XDG_DATA_HOME=$S/home/.local/share XDG_STATE_HOME=$S/home/.local/state XDG_RUNTIME_DIR=$S/run \
           XDG_DATA_DIRS=/usr/local/share:/usr/share XDG_CONFIG_DIRS=/etc/xdg
    unset DBUS_SESSION_BUS_ADDRESS DISPLAY WAYLAND_DISPLAY XDG_SESSION_TYPE
    cat > "$S/bus.conf" <<EOF
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <listen>unix:dir=$BUSDIR</listen>
  <servicedir>$S/empty</servicedir>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
EOF
    Xvfb ":$DISP" -screen 0 1280x800x24 -nolisten tcp >"$S/xvfb.log" 2>&1 &
    XPID=$!
    trap 'kill $XPID 2>/dev/null; wait $XPID 2>/dev/null; rm -rf "$BUSDIR"' EXIT
    for _ in $(seq 50); do [ -e "/tmp/.X11-unix/X$DISP" ] && break; sleep 0.1; done
    FA_INNER=1 FA_S=$S DISPLAY=":$DISP" dbus-run-session --config-file="$S/bus.conf" -- "$0" "$@"
    exit $?
fi

# ---------------------------------------------------------------- inside the private session
S=$FA_S
R=$S/results.txt
: > "$R"
PIDS=()
cleanup() { for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; }
trap cleanup EXIT
now() { date +%s.%N; }
ms() { awk -v a="$1" -v b="$2" 'BEGIN{printf "%d", (b-a)*1000}'; }
pass() { echo "PASS $*" | tee -a "$R"; }
fail() { echo "FAIL $*" | tee -a "$R"; }

export LANG=pt_BR.UTF-8 LANGUAGE=pt_BR QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=lingmo QT_FORCE_STDERR_LOGGING=1 QT_LOGGING_RULES="lingmo.fileaction.*=true;qml.debug=true;js.debug=true;default.debug=true"
[ "$THEME" = dark ] && printf '[General]\nDarkMode=true\n' > "$XDG_CONFIG_HOME/lingmoos/theme.conf" \
                    || printf '[General]\nDarkMode=false\n' > "$XDG_CONFIG_HOME/lingmoos/theme.conf"
cp "$HERE/../data/intents/lingmo-fileaction.json" "$XDG_DATA_HOME/lingmo/intents/"

P=$HOME/Pictures M=$HOME/Music V=$HOME/Videos
ffmpeg -loglevel error -y -f lavfi -i "testsrc2=size=1280x720:rate=1" -frames:v 1 "$P/photo.png"
ffmpeg -loglevel error -y -f lavfi -i "sine=frequency=440:duration=5" "$M/tone.wav"
ffmpeg -loglevel error -y -f lavfi -i "testsrc2=size=640x360:rate=25" -t 3 -pix_fmt yuv420p "$V/clip.mp4"

/usr/bin/python3 "$HERE/fake_island.py" "$S/island.log" & PIDS+=($!)
"$BUILD/lingmo-fileaction" --daemon > "$S/daemon.log" 2>&1 & PIDS+=($!)
sleep 1.5

shot() { # name: the wheel window (or the whole screen)
    local id geo
    id=$(xdotool search --onlyvisible --class lingmo-fileaction 2>/dev/null | head -1)
    if [ -n "$id" ]; then
        eval "$(xdotool getwindowgeometry --shell "$id")"
        import -window root -crop "${WIDTH}x${HEIGHT}+${X}+${Y}" +repage "$S/$1.png" 2>/dev/null
    else
        import -window root "$S/$1.png" 2>/dev/null
    fi
}
wheel_visible() { xdotool search --onlyvisible --class lingmo-fileaction >/dev/null 2>&1; }
wheel_xy() {
    local id; id=$(xdotool search --onlyvisible --class lingmo-fileaction | head -1)
    eval "$(xdotool getwindowgeometry --shell "$id")"; echo "$X $Y"
}
wait_file() { # file timeout_s
    local t=0; while [ ! -s "$1" ] && [ "$t" -lt "$(( $2 * 10 ))" ]; do sleep 0.1; t=$((t + 1)); done; [ -s "$1" ]
}

# Start a real XDND drag of $1 from the source window and bring the pointer to (640, 400)
SRC_PID=
start_drag() {
    [ -n "$SRC_PID" ] && kill "$SRC_PID" 2>/dev/null
    /usr/lib/qt6/bin/qml "$HERE/dragsource.qml" -- "file://$1" > "$S/source.log" 2>&1 & SRC_PID=$!; PIDS+=($SRC_PID)
    sleep 1.2
    xdotool mousemove 110 90; sleep 0.2
    xdotool mousedown 1; sleep 0.1
    for p in "120 100" "140 120" "200 180" "320 260" "480 340" "640 400"; do xdotool mousemove $p; sleep 0.05; done
    sleep 0.3
}
move_to() { # x y: in a few steps, like a hand
    local x0 y0; eval "$(xdotool getmouselocation --shell)"; x0=$X; y0=$Y
    for i in 1 2 3 4 5; do xdotool mousemove $(( x0 + ($1 - x0) * i / 5 )) $(( y0 + ($2 - y0) * i / 5 )); sleep 0.04; done
    sleep 0.25
}
tap() { xdotool key "$1"; }

# 1. photo.png: drag, tap Shift -> Convert wheel, drop on WEBP
start_drag "$P/photo.png"
t0=$(now); tap shift
for _ in $(seq 30); do wheel_visible && break; sleep 0.05; done
if wheel_visible; then pass "tap Shift during a drag opens the wheel ($(ms "$t0" "$(now)") ms)"; else fail "wheel did not open"; fi
sleep 0.8
shot "wheel-convert-image-$THEME"
read -r WX WY < <(wheel_xy)
read -r SX SY < <(/usr/bin/python3 "$HERE/slicepos.py" "$WX" "$WY" convert image.to.webp "$P/photo.png")
move_to "$SX" "$SY"
shot "wheel-hover-webp-$THEME"
t0=$(now); xdotool mouseup 1
if wait_file "$P/photo.webp" 15; then pass "drop on WEBP -> photo.webp ($(ms "$t0" "$(now)") ms after the drop)"; else fail "photo.webp missing"; fi
sleep 0.5
wheel_visible && fail "wheel still open after the drop" || pass "wheel closed after the drop"

# 2. the same key again repeats the last action (WEBP) -> photo (2).webp, never overwriting
start_drag "$P/photo.png"
tap shift; sleep 0.9; tap shift; sleep 0.5
shot "wheel-repeat-$THEME"
read -r WX WY < <(wheel_xy)
read -r SX SY < <(/usr/bin/python3 "$HERE/slicepos.py" "$WX" "$WY" convert repeat)
move_to "$SX" "$SY"
t0=$(now); xdotool mouseup 1
if wait_file "$P/photo (2).webp" 15; then pass "Shift twice repeats WEBP -> 'photo (2).webp' ($(ms "$t0" "$(now)") ms)"; else fail "repeat did not produce photo (2).webp"; fi
sleep 0.5

# 3. drop in the middle cancels
before=$(ls "$P" | wc -l)
start_drag "$P/photo.png"
tap shift; sleep 0.9
read -r WX WY < <(wheel_xy)
move_to $((WX + 28 + 168)) $((WY + 28 + 168))
shot "wheel-cancel-$THEME"
xdotool mouseup 1; sleep 2
after=$(ls "$P" | wc -l)
[ "$before" = "$after" ] && pass "drop in the middle cancels (no new file)" || fail "drop in the middle created a file"

# 4. holding Shift during a drag (file managers: move) is not a tap
start_drag "$P/photo.png"
xdotool keydown shift; sleep 0.7; xdotool keyup shift; sleep 0.6
wheel_visible && fail "held Shift opened the wheel" || pass "held Shift (700 ms) does not open the wheel"
xdotool mousemove 900 700; sleep 0.2; xdotool mouseup 1; sleep 0.5
# a tap without a drag does nothing either
tap shift; sleep 0.6
wheel_visible && fail "Shift tap without a drag opened the wheel" || pass "Shift tap without a drag does nothing"

# 5. tone.wav: Shift, then Ctrl (Tools) and Alt (Actions) switch wheels, Shift back, drop on MP3
start_drag "$M/tone.wav"
tap shift; sleep 0.9
tap ctrl; sleep 0.5; shot "wheel-tools-audio-$THEME"
tap alt; sleep 0.5; shot "wheel-actions-$THEME"
tap shift; sleep 0.5; shot "wheel-convert-audio-$THEME"
read -r WX WY < <(wheel_xy)
read -r SX SY < <(/usr/bin/python3 "$HERE/slicepos.py" "$WX" "$WY" convert audio.to.mp3 "$M/tone.wav")
move_to "$SX" "$SY"
t0=$(now); xdotool mouseup 1
if wait_file "$M/tone.mp3" 20; then pass "Ctrl/Alt switch wheels; drop on MP3 -> tone.mp3 ($(ms "$t0" "$(now)") ms)"; else fail "tone.mp3 missing"; fi
sleep 0.5

# 6. clip.mp4 -> GIF, with the Island following the progress
start_drag "$V/clip.mp4"
tap shift; sleep 1.0
shot "wheel-convert-video-$THEME"
read -r WX WY < <(wheel_xy)
read -r SX SY < <(/usr/bin/python3 "$HERE/slicepos.py" "$WX" "$WY" convert video.to.gif "$V/clip.mp4")
move_to "$SX" "$SY"
t0=$(now); xdotool mouseup 1
if wait_file "$V/clip.gif" 30; then pass "drop on GIF -> clip.gif ($(ms "$t0" "$(now)") ms)"; else fail "clip.gif missing"; fi
kill "$SRC_PID" 2>/dev/null
sleep 1

# 7. keyboard: the wheel for a file (as from the file manager or Meta+Shift+A), arrows + Enter
"$BUILD/lingmo-fileaction" --wheel tools "$P/photo.png"; sleep 1.2
shot "wheel-keyboard-tools-$THEME"
# Tools for an image: 1 Compress, 2 Shrink 50 % -> press 2
t0=$(now); xdotool key 2
if wait_file "$P/photo (smaller).png" 15; then pass "keyboard: 2 on the Tools wheel -> 'photo (smaller).png' ($(ms "$t0" "$(now)") ms)"; else fail "keyboard shrink missing"; fi
"$BUILD/lingmo-fileaction" "$P/photo.png"; sleep 1.0
xdotool key Escape; sleep 0.5
wheel_visible && fail "Esc did not close the wheel" || pass "Esc closes the wheel"

# 8. the drop zone (no files yet): files dragged onto it
"$BUILD/lingmo-fileaction"; sleep 1.0
shot "wheel-dropzone-$THEME"
xdotool key Escape; sleep 0.3

# 9. app action files.convert through lingmo-intent (as the Assistant / Flows call it)
t0=$(now)
if lingmo-intent run files.convert --param file="$M/tone.wav" --param to=ogg --json > "$S/intent.json" 2>&1 \
   && [ -s "$M/tone.ogg" ]; then pass "lingmo-intent run files.convert wav->ogg ($(ms "$t0" "$(now)") ms): $(tr -d '\n' < "$S/intent.json")"
else fail "files.convert: $(cat "$S/intent.json")"; fi
lingmo-intent run files.actions.list --param file="$V/clip.mp4" --json > "$S/intent-list.json" 2>&1 \
   && pass "files.actions.list: $(tr -d '\n' < "$S/intent-list.json" | cut -c1-200)" || fail "files.actions.list: $(cat "$S/intent-list.json")"

# 10. Cancel from the Island: a long conversion, "Cancelar" clicked in the Island -> no result, no partial file
ffmpeg -loglevel error -y -f lavfi -i "testsrc2=size=1280x720:rate=30" -t 40 -pix_fmt yuv420p "$V/long.mp4"
"$BUILD/lingmo-fileaction" --wheel convert "$V/long.mp4"; sleep 1.2
read -r WX WY < <(wheel_xy)
read -r SX SY < <(/usr/bin/python3 "$HERE/slicepos.py" "$WX" "$WY" convert video.to.webm "$V/long.mp4")
xdotool mousemove "$SX" "$SY"; sleep 0.3; xdotool click 1
sleep 3
ID=$(grep '"call": "Start"' "$S/island.log" | tail -1 | /usr/bin/python3 -c 'import json,sys; print(json.loads(sys.stdin.read())["id"])')
t0=$(now)
gdbus call --session --dest com.lingmo.Island --object-path /Island --method com.lingmo.IslandTest.Invoke "$ID" cancel >/dev/null
for _ in $(seq 50); do grep -q "\"Finish\", \"id\": $ID" "$S/island.log" && break; sleep 0.1; done
sleep 0.5
if grep -q "\"Finish\", \"id\": $ID, \"message\": \"Cancelado\"" "$S/island.log" && [ ! -e "$V/long.webm" ] \
   && ! ls -a "$V" | grep -q '^\.fa-'; then
    pass "Island Cancelar stops the conversion in $(ms "$t0" "$(now)") ms, no result and no partial file left"
else
    fail "cancel from the Island (see island.log): $(ls -a "$V" | tr '\n' ' ')"
fi

# 11. Lingmo Files (when FM_BUILD points at a lingmo-filemanager build): the context menu entries and
#     Meta+Shift+A taking the files selected in the window
if [ -n "${FM_BUILD:-}" ]; then
    PATH="$BUILD:$PATH" LINGMO_FM_TRANSLATIONS="$FM_BUILD" "$FM_BUILD/lingmo-filemanager" "$P" > "$S/fm.log" 2>&1 & PIDS+=($!)
    sleep 3
    FM=$(xdotool search --onlyvisible --class lingmo-filemanager | head -1)
    if [ -n "$FM" ]; then
        xdotool windowsize "$FM" 900 600; xdotool windowmove "$FM" 40 40; sleep 0.8
        eval "$(xdotool getwindowgeometry --shell "$FM")"
        xdotool mousemove $((X + 450)) $((Y + 300)) click 1; sleep 0.3
        xdotool key ctrl+a; sleep 0.5
        SEL=$(gdbus call --session --dest com.lingmo.FileManager --object-path /FileManager --method com.lingmo.FileManager.selectedFiles 2>&1)
        echo "$SEL" | grep -q photo.png && pass "Lingmo Files D-Bus selectedFiles: $(echo "$SEL" | cut -c1-120)" || fail "selectedFiles: $SEL"
        # Meta+Shift+A (chotkeys runs lingmo-fileaction --keyboard): the window in front is Lingmo Files
        xprop -root -f _NET_ACTIVE_WINDOW 32x -set _NET_ACTIVE_WINDOW "$FM"
        "$BUILD/lingmo-fileaction" --keyboard; sleep 1.5
        shot "wheel-keyboard-selection-$THEME"
        xdotool key Escape; sleep 0.4
        # right-click on the selection: the menu with "Converter…" and "Ações de arquivo…"
        eval "$(xdotool getwindowgeometry --shell "$FM")"   # shot() above reused X/Y
        xdotool mousemove $((X + 450)) $((Y + 300)) click 1; sleep 0.2; xdotool key ctrl+a; sleep 0.3
        # on the first icon (top left of the grid view, which the window size above fixes)
        xdotool mousemove $((X + 306)) $((Y + 100)) click 3; sleep 1.0
        import -window root "$S/fm-menu-$THEME.png" 2>/dev/null
        xdotool key Escape; sleep 0.3
    else
        fail "lingmo-filemanager did not show a window (see fm.log)"
    fi
fi

# 12. the Island saw progress, "Ready" with Open / Show in folder
sleep 1
if grep -q 'WEBP' "$S/island.log" && grep -q '"open"' "$S/island.log"; then
    pass "Island: $(grep -c '"call": "Start"' "$S/island.log") activities, progress updates $(grep -c progress "$S/island.log"), open/folder actions offered"
else
    fail "Island log lacks the expected activity (see island.log)"
fi
ls -la "$P" "$M" "$V" >> "$R"
