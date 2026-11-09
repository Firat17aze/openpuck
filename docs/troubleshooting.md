# Troubleshooting

Short fixes for common problems. If yours is not here, see
[SUPPORT.md](../SUPPORT.md).

## Using the puck

**The number changed when I wanted the menu.** The menu opens when you press and
hold the very center of the screen. Holding the top or bottom changes the
number by 5.

**The puck turned itself off.** Auto-off switches it off after it has been left
alone (10 minutes at first). Change or switch it off in
Settings, under Auto-off. It does not switch off while the timer is running.
See the [user guide](user-guide.md#auto-off).

**The timer is missing.** It was turned off. Hold the center, choose Timer,
then Timer on.

**The power button did not start the timer.** A short press only starts or
pauses the timer when the timer is on and no menu or settings screen is open.
A press on a menu goes back one step instead.

**The timer was paused after I turned the puck on.** That is expected. It comes
back paused at the time it had when it was saved.

**My last tap was lost.** Changes are saved a couple of seconds after you stop
tapping. If power is lost sooner, the last change may not be saved.

**The battery icon is missing.** It may be set to Off (Settings, then Battery).
In a one player game it also hides for a moment while a running change like
"+2" is showing.

## Installing

**"The puck never appeared on USB."** Try a different cable. Many cables only
charge. Try another USB port too.

**The puck stays off after a step.** Press the power button.

**The tool asks me to hold the power button.** That is normal while the
original software is on the puck. Hold the button until the tool says to
release it. See the [install guide](install.md#first-install-and-updates).

**I want the original software back.** Run
`python tools/flash.py restore my-puck-backup.bin` with the backup you made
before installing. See the [install guide](install.md#going-back-to-the-original-software).

More technical detail is in [flashing.md](flashing.md).
