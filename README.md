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
