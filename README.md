# parsefat

make

./parsefat disk.img

Notes:

* does not handle cluster lists (in case file or directory is located on several clusters)
* have no disk i/o optimization (does not read full sector but performs byte access)
* sector size is hardcoded

