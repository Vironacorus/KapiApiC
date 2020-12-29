# KapiApiC
C wrapper around minecraftmapy.pl api. Tutorial and code samples below:

#Initialization And Deinitialization

To initialize KapiApi call KapiapiInit() and KapiapiDeinit() respectively. Like such:

	```c
	if(!KapiapiInit())
	printf("%i",KapiapiGetLastError());
	KapiapiDeinit();
```
