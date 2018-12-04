# Client for FTP

FTP client using Qt

Test environment:

- Ubuntu 16.04, 64bit.
- Qt 5.11.2 (GCC 5.3.1 20160406 (Red Hat 5.3.1-6), 64 bit)

## Build & Run

Built program is in `client/bin/`, together with its dependences. You can run by:
- `chmod +x client`
- `chmod +x client.sh`
- `./client.sh`

Or you can also use Qt Creator to build & run this project.

## Usage

#### Connect & disconnect

Execute `client`. input your `IP`, `port`, `username` and `password`, then press `connect`. Default address is the `ftp.gnu.org` for test.

After connected, you can easily `disconnect` and then reconnect again.

#### Text Browser -- commands & response show

You can know about the process in the Text Browser just below the connect-option part. Information will show in black, errors will show in red.

#### Main operation area -- local & remote, transfer pause & continue

In the left part, there exists the local path input, local files tree view. You can do:

- Input the folder path in the input box, press `Go` or press `enter` key to change local working directory.
- Input the target file name in the input box, press `Upload` to upload your file to the folder path shown in the right part.
- Click the item in the local files tree view to auto fill in the input box.
- Double click the items in the local files tree view, then:
  - For files, will upload;
  - For folders, will change local working directory.

Upload is enabled when:

- Connected to server, and there isn't a file transferring (you can decide using the `Upload` button).

Local working directory changing is forbidden when there is a file transferring.

In the right part, is the remote ones. Similarly, you can double-click or press `Go` or `Download` to do the specific things.

The process bar will show the uploading or downloading process when there is a file transferring. You can `Pause` or `Continue` before the transferring finished.

Note: after `Pause` a transferring process, you can either `Continue` it or start a new upload/download process.

Right click commands are supported:

- Right click on local / remote files tree view, you can upload, download, open folder, delete files / empty folder, create folder and refresh.

Files trees will update automatically after upload/download/delete, you can also refresh via right click menus.
