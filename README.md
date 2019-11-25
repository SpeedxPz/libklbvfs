# libklbvfs

SQLite VFS that uses in LLSIF-AS to read encrypted sqlite database

# Prerequisite

To read the LLSIF-AS sqlite db you need SQ value from your game (It's unique value each device)
You can find this value at
/data/data/com.klab.lovelive.allstars/shared_prefs/com.klab.lovelive.allstars.v2.playerprefs.xml (Root required)

After that you need to do some logic to transform the SQ value into 3 uint key 

# How to use
Get your db from /sdcard/android/data/com.klab.lovelive.allstars/files/files/
Use file name in this format {Key1}.{Key2}.{Key3}_{PathName}
    #include "../src/sqlite3.h"
	#include "../src/klbvfs.h"
	int main()
	{
		sqlite3 *db;
		int flags;
		klbvfs_register();
		flags = SQLITE_OPEN_READONLY;
		sqlite3_open_v2("1111111.2222222.2222222_test/testdata/masterdata.db_60e49317aa15e662f5911fce8237264a8703122a4.db", &db, flags, NULL);
		//Do anything you want
		
	}

# Build
Just run 'make' with gcc
