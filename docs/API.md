# Wye API

Other programs on your computer can drive the other computer directly. There's
no hotkey to press, and your own keyboard and mouse keep working on your own
computer. Nothing is needed on the other computer: it only ever sees the same
USB keyboard and mouse as always.

Wye must be running; `wye --background` starts it without a window. Closing Wye
ends the program and the API with it. To stop a Wye without a window, open Wye
again and close the window, or run `kill` on it. The API
listens on your computer only and accepts only your own user account. Turn it
off under Settings → "Let other apps on this computer drive it".

## From a shell

```sh
wye status                  # version=1.0.0 target=ready absolute=1
wye type "hello world"
wye key ctrl+shift+t enter  # one or more chords, pressed in order
wye move 100 -20            # move the pointer by this much
wye moveto 16384 16384      # place the pointer: 0-32767 across the screen, this is the middle
wye click                   # left, right, middle, back or forward
wye down left               # hold a button, for dragging...
wye up left                 # ...and let go
wye scroll -3               # scroll notches, up or (negative) down; a second number scrolls sideways
wye release                 # let go of everything and drop text still being typed
```

Exit codes: `0` done, `1` refused (the message says why), `2` Wye isn't running.

**Key names:** a character (`a`, `/`, `A`), or `enter`, `esc`, `tab`, `space`,
`backspace`, `delete`, `insert`, `home`, `end`, `pageup`, `pagedown`, `up`,
`down`, `left`, `right`, `f1`-`f24`, `capslock`, `numlock`, `scrolllock`,
`printscreen`, `pause`, `menu`. Modifiers: `ctrl`, `shift`, `alt`, `altgr`,
`win` (also `super`, `cmd`), and `rctrl`, `rshift`, `rsuper`.

## From a program

Connect to the socket and send text lines. Each line gets one reply line.

| System | Where |
| --- | --- |
| Linux, macOS | Unix socket: `$XDG_RUNTIME_DIR/wye.sock`, else `$TMPDIR/wye-<uid>/wye.sock` (`/tmp` if `TMPDIR` is unset) |
| Windows | named pipe: `\\.\pipe\wye-<username>` |

Set `WYE_API_SOCKET` to use another path, for both Wye and the `wye` commands.

The commands are the same as above, with one difference: in `type`, write a
newline as `\n`, a tab as `\t` and a backslash as `\\`. Lines can be up to 8191
bytes; send longer text in several `type` commands.

```python
import os, socket

s = socket.socket(socket.AF_UNIX)
s.connect(os.path.join(os.environ["XDG_RUNTIME_DIR"], "wye.sock"))
f = s.makefile("rw", newline="\n")

def wye(command):
    f.write(command + "\n")
    f.flush()
    reply = f.readline().strip()
    if reply.startswith("err "):
        raise RuntimeError(reply)
    return reply

print(wye("status"))
wye("key win+r")
wye("type notepad\\n")
```

## Replies

`ok`, sometimes followed by details, or `err <code> <message>`:

| Code | Meaning |
| --- | --- |
| `not-ready` | the other computer isn't connected |
| `busy` | you are controlling it with the hotkey, or a routine is playing; try again later |
| `bad-request` | the command or its values are wrong |
| `unsupported` | the Picos' firmware can't do this |

`type` replies with `ok typed=<n> skipped=<n>` and `key` with `ok`, once all the
keys have gone out, so the next command can't overtake them. Typing runs at about
80 characters a second.

## Good to know

- Input goes wherever the other computer's focus is. Nothing reads its screen,
  so a program using the API can't see the result of what it sends.
- `type` assumes the other computer uses a US keyboard layout, and skips
  characters a US keyboard can't type. On other layouts some characters come out
  differently, because the Pico sends key positions, not letters.
- Routines being recorded also record what the API sends.
