# hardlinkbackup

## Usage

    Usage: hardlinkbackup [-n N] [-S] <source dir> <backup dir>
       -n: Number of backups to keep, defaults to 8
       -S: Skip update, just rotate and link  
       -h: This help text

## Example usage
    hardlinkbackup \\Server1\share\ G:\snapshots\server1\

## Compiling
- Tested with VS2012
- No other dependency required.   
