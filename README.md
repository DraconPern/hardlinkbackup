# hardlinkbackup

Quick and simple backup for Windows. Creates multiple snapshots of directories, while saving space for unchanged files by using Windows hard links. Great for use in batch files or with Windows Scheduler.

## Usage

    Usage: hardlinkbackup [-n N] [-S] <source dir> <backup dir>
       -n: Number of backups to keep, defaults to 8
       -S: Skip update, just rotate and link  
       -h: This help text

## Example usage
    hardlinkbackup \\Server1\share\ G:\snapshots\server1\
Will result in

    G:\snapshots\server1\current      (containing a copy of \\Server1\share\)
    G:\snapshots\server1\1            (containing a previous copy of \\Server1\share\) 
    G:\snapshots\server1\2            (containing a an even older copy of \\Server1\share\)
    G:\snapshots\server1\3
    etc...

## Compiling
- Tested with VS2012
- No other dependency required.   
