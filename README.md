# KapiApiC
C wrapper around minecraftmapy.pl api. Tutorial and code samples below:

# Initialization And Deinitialization

To initialize KapiApi call KapiapiInit() and KapiapiDeinit() respectively. Like such:

```c
	if(!KapiapiInit())
		printf("%i",KapiapiGetLastError());
	KapiapiDeinit();
```

It's important to mention, that most of functions (excluding ones like KapiapiDeinit()) return a BOOL of value 1 if they succeeded or of value 0 if they failed. I reccomend exiting or printing error when it occurs, because program might crash in latter function and error might be hard to track.

# Getting User Data
```c
	KAPIAPI_USERDATA user_data;
	KapiapiGetUserData(L"CzekotPL", &user_data);
	KAPIAPI_USER user;
	KapiapiParseUserData(&user,&user_data);
```

KAPIAPI_USERDATA contains raw JSON data fetched from the API. KAPIAPI_USER depends on data provided by user_data and DOES NOT copy it, but KAPIAPI_USERDATA can go out of scope, unless you want to clean it up (but as I just mentioned, it's used by KAPIAPI_USER, which means you probably want to keep it for entire program lifetime). KapiapiGetUserData() will get data from api and KapiapiParseUserData() will parse received JSON into KAPIAPI_USER struct. You can access data like such: `user.ban.has_ban`, `user.stats.diamond_sum` etc. Strings (KAPIAPI_SLICE) require additional formatting.

# Getting Map Data

```c
	WCHAR* mapid_mem;
	KAPIAPI_WSLICE map_id = KapiapiGetUserMapId(&user,&mapid_mem);
	KAPIAPI_MAPDATA map_data;
	KapiapiGetMapData(map_id,&map_data);
	free(mapid_mem);

	SIZEPARAM map_count;
	KAPIAPI_MAP* maps = NULL;
	KapiapiEnumerateMapData(&map_count,&maps, &map_data);
```

KapiapiGetUserMapId() function gets subpage directory of user's maps and returns it. It's not expected to fail, therefore it doesn't return a BOOL like other KAPIAPI functions. KAPIAPI_MAPDATA is similliar to KAPIAPI_USERDATA, but for maps. It stores raw JSON string fetched from the server. We get it using KapiapiGetMapData, just like we got KAPIAPI_USERDATA using KapiapiGetUserData. Big difference between KAPIAPI_USER and KAPIAPI_MAP lies in parsing functions. One user can have multiple maps, therefore we also need to get map count and instead of allocating KAPIAPI_MAP ourselves, on the stack, we pass pointer to a KAPIAPI_MAP* which will be then filled with contents ofmaps. As always, we pass data by const KAPIAPI_MAPDATA*.

You might have noticed that KapiapiGetUserMapId takes additional parameter (i.e. WCHAR**). That's because it allocates new memory, which has to be freed after is used (KapiapiGetMapData in most cases), as this is C and no automatic garbage collection or RAII is present.
