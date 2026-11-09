# Installing OpenPuck

This page shows how to put OpenPuck on your Life Puck, and how to go back to
the original software.

> **An easy browser installer is planned but not available yet.** It will let
> you install from a web page with no extra software. Until then, the steps
> below use a small command-line tool. This page will be updated when the
> browser installer is ready.

## Read this first

- **Back up your puck before you change anything.** The backup is a file that
  holds everything on the puck, including the original software. It is the only
  way to go back. The install tool will not run without one.
- **Installing is at your own risk.** OpenPuck is an independent project and is
  not made or supported by the maker of the puck. Putting new software on a
  device can fail. If you have a backup, you can put the original software
  back (see [Going back](#going-back-to-the-original-software)). Installing
  OpenPuck may affect your warranty.
- **Keep your backup file private.** It contains the original software, so
  do not share it or post it online. Store it somewhere safe, like a cloud
  drive or a USB stick.

## What you need

- A computer (Windows, macOS or Linux).
- A **USB-C cable that carries data.** Many cables that come with lights or
  small gadgets only charge and will not work. If the puck is not found, try a
  different cable first.
- A charged puck.
- About 30 minutes the first time, mostly for setting up the tools.
- A little comfort typing commands into a terminal. If that is not for you,
  you may want to wait for the browser installer.

## Step 1: Set up the tools

The tool needs Espressif's ESP-IDF development kit version 6.1. It also gives
you everything the install tool relies on.

1. Install **ESP-IDF v6.1**. Espressif's installer (the "ESP-IDF Installation
   Manager"), or the VS Code ESP-IDF extension, will do it for you. Follow
   Espressif's instructions for your system.
2. Download OpenPuck from [GitHub](https://github.com/wavemoran/openpuck).
   Click **Code**, then **Download ZIP**, and unzip it. (If you know Git, you
   can clone it instead.)
3. Open a terminal in the OpenPuck folder and turn on ESP-IDF. The exact
   command depends on how you installed it. For example:

   ```sh
   source ~/.espressif/tools/activate_idf_v6.1.sh
   ```

## Step 2: Back up your puck

1. Connect the puck to your computer with the USB-C data cable.
2. Run:

   ```sh
   python tools/flash.py backup my-puck-backup.bin
   ```

3. **If the puck switches off while it connects, this is normal.** The tool
   will say **"Press and hold the power button"**. Turn the puck on and **keep
   holding the button.** When the tool says **"Release the button now"**, let
   go. On a Mac the tool also says these things out loud.
4. Wait about a minute while it saves. It prints "Saved" when it is done.
5. You now have two files: `my-puck-backup.bin` and `my-puck-backup.bin.sha256`.
   **Copy both somewhere safe.**

Want to check the connection first? `python tools/flash.py info` only reads
information from the puck and changes nothing.

## Step 3: Install OpenPuck

1. Build OpenPuck. This makes the files the tool will write:

   ```sh
   idf.py set-target esp32c3   # first time only
   idf.py build
   ```

2. Install it, pointing at your backup:

   ```sh
   python tools/flash.py flash --backup my-puck-backup.bin
   ```

   The tool checks that the backup looks complete before it writes anything.
   If you did the button hold in step 2, you may need to do it again here (see
   [First install and updates](#first-install-and-updates)).
3. Wait for it to write and check the files. **Do not unplug the puck while it
   works.** It says "Done" at the end.
4. If the puck stays dark, press the power button. You should see the OpenPuck
   logo, then the counter.

Next: the [user guide](user-guide.md).

## First install and updates

The puck only stays on if its software tells it to. When the install tool
connects, it restarts the puck.

- **While the original software is installed**, the restart switches the puck
  off. That is why the tool asks you to **hold the power button** until it
  says to release it. You need this for your first backup and your first
  install.
- **While OpenPuck is installed**, it keeps the puck on through the restart.
  **You do not need to hold the button.** Later updates are just
  `idf.py build` and `python tools/flash.py flash --backup my-puck-backup.bin`.
  Your first backup is still the right one to use, because it holds the
  original software.
- If you restore the original software, you will need the button hold again
  the next time.

## Going back to the original software

If you want the original software again, run:

```sh
python tools/flash.py restore my-puck-backup.bin
```

The tool checks the file, writes it, and checks the result. Press the power
button afterward if the puck stays off. Restoring puts the puck back the way it
was when you made the backup.

## If something goes wrong

- **"The puck never appeared on USB":** try another cable (a data cable, not
  charge-only) or another USB port.
- **The puck is off after a step:** press the power button.
- **Stuck or not sure:** see [troubleshooting](troubleshooting.md), or the
  technical details in [flashing.md](flashing.md).

If you have a backup, you can always run the restore command above.
