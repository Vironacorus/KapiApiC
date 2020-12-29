# KapiApiC
C wrapper around minecraftmapy.pl api. Tutorial and code samples below:

#Initialization And Deinitialization

To initialize KapiApi call KapiapiInit() and KapiapiDeinit() respectively. Like such:

```
	if(!KapiapiInit())
		printf("%i",KapiapiGetLastError());
	KapiapiDeinit();
```

It's important to mention, that most of functions (excluding ones like KapiapiDeinit()) return a BOOL of value 1 if they succeeded or of value 0 if they failed. I reccomend exiting or printing error when it occurs, because program might crash in latter function and error might be hard to track.

#Getting User Data
