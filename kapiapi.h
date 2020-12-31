#pragma once
#define KAPIAPI_NULLABLE


#ifdef _WIN32
#include <Windows.h>
#include <winhttp.h>
#else
#error Library currently only available on windows!
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef char CHAR;
typedef wchar_t WCHAR;
typedef const WCHAR* WSTRING;
typedef const CHAR*  STRING;
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;
typedef size_t SIZEPARAM;
#ifndef VOID //<Windows.h> defines VOID as void
typedef void VOID;
typedef U8 BOOL; //<Windows.h> uses BOOL as LONG, we can aswell use BOOL as U8
#endif
typedef VOID* PVOID;

#ifdef __cplusplus
#define KAPIAPI extern "C"
#else
#define KAPIAPI
#endif
typedef enum KapiapiErrors
{
	KAPIAPI_ERR_NONE,
	KAPIAPI_ERR_OTHER,
	KAPIAPI_ERR_FAILED_TO_ESTABLISH_CONNECTION,
	KAPIAPI_ERR_FAILED_TO_SEND_REQUEST,
	KAPIAPI_ERR_FAILED_TO_RECEIVE_DATA,
	KAPIAPI_ERR_INVALID_JSON,
	KAPIAPI_ERR_UNABLE_TO_READ_DATA
} KAPIAPIERR;

typedef struct Slice
{
	STRING ptr;
	SIZEPARAM length;
} KAPIAPI_SLICE;

typedef struct KapiapiMapData
{
	U8* bytes;
	SIZEPARAM size;
} KAPIAPI_MAPDATA, *KAPIAPI_PMAPDATA;

typedef struct KapiapiMapAuthor
{
	KAPIAPI_SLICE username;
	KAPIAPI_SLICE endpoint;
} KAPIAPI_MAPAUTHOR;

typedef struct KapiapiMapInfo
{
	KAPIAPI_SLICE title;
	KAPIAPI_SLICE description;
	KAPIAPI_SLICE download_url;
	KAPIAPI_SLICE created_at;
	KAPIAPI_SLICE accepted_at;
	KAPIAPI_SLICE updated_at;
} KAPIAPI_MAPINFO;

typedef struct KapiapiMapCategory
{
	U64 id;
	KAPIAPI_SLICE name;
} KAPIAPI_CATEGORY;


typedef struct KapiapiMapMinecraftVersion
{
	U64 id;
	KAPIAPI_SLICE name;
	BOOL is_snapshot;
	BOOL is_other;
	BOOL is_bedrock;
} KAPIAPI_MAPVERSION;

typedef struct KapiapiMapImages
{
	U64 count;
	KAPIAPI_SLICE images[3];
} KAPIAPI_MAPIMAGES;

typedef struct KapiapiMapStats
{
	U64 diamond_count;
	U64 download_count;
	U64 comment_count;
} KAPIAPI_MAPSTATS;

typedef struct KapiapiMapEndpoints
{
	KAPIAPI_SLICE map;
	KAPIAPI_SLICE comments;
} KAPIAPI_MAPENDPOINTS;

typedef struct KapiapiMap
{
	U64 id;
	KAPIAPI_SLICE code;
	KAPIAPI_SLICE code_old;
	KAPIAPI_SLICE url;
	KAPIAPI_MAPAUTHOR author;
	KAPIAPI_MAPINFO info;
	KAPIAPI_CATEGORY category;
	KAPIAPI_MAPVERSION minecraft_version;
	KAPIAPI_MAPIMAGES images;
	KAPIAPI_MAPSTATS stats;
	KAPIAPI_MAPENDPOINTS endpoints;
} KAPIAPI_MAP, *KAPIAPI_PMAP;

typedef struct KapiapiUserData
{
	U8* bytes;
	SIZEPARAM size;
} KAPIAPI_USERDATA, *KAPIAPI_PUSERDATA;

typedef struct WSlice
{
	WSTRING ptr;
	SIZEPARAM length;
} KAPIAPI_WSLICE;

typedef enum KapiapiRoles
{
	KAPIAPI_ROLE_USER,
	KAPIAPI_ROLE_MOD,
	KAPIAPI_ROLE_ADMIN,
	KAPIAPI_ROLE_OWNER
} KAPIAPI_ROLES;

typedef struct KapiapiRole
{
	KAPIAPI_SLICE role_string;
	KAPIAPI_ROLES role_type;
} KAPIAPI_ROLE;

typedef struct KapiapiBan
{
	BOOL has_ban;
	KAPIAPI_SLICE reason;
	BOOL is_permament;
	KAPIAPI_SLICE expires_at;
} KAPIAPI_BAN;

typedef struct KapiapiUserInfo
{
	KAPIAPI_SLICE username;
	KAPIAPI_SLICE description;
	KAPIAPI_SLICE last_logged_at;
	KAPIAPI_SLICE last_logged_relative;
	BOOL is_active;
	KAPIAPI_SLICE avatar_url;
	KAPIAPI_ROLE role;
} KAPIAPI_USERINFO;

typedef struct KapiapiUserStats
{
	U64 diamond_sum;
	U64 download_sum;
	U64 written_comments;
	U64 given_diamonds;
	U64 map_count;
	U64 follower_count;
} KAPIAPI_USERSTATS;

typedef struct KapiapiUserEndpoint
{
	KAPIAPI_SLICE maps;
} KAPIAPI_USERENDPOINT;

typedef struct KapiapiUser
{
	U64 id;
	KAPIAPI_SLICE url;
	KAPIAPI_USERINFO info;
	KAPIAPI_USERSTATS stats;
	KAPIAPI_BAN ban;
	KAPIAPI_USERENDPOINT endpoint;
} KAPIAPI_USER, *KAPIAPI_PUSER;


#ifdef _WIN32
#pragma comment(lib, "winhttp.lib")
#endif

//Initializes Kapiapi, make sure to call it before any other KAPIAPI functions, returns 0 if operation failed
KAPIAPI BOOL KapiapiInit(VOID);
//Gets last error, returns KAPIAPI_ERR_NONE if no errors occured
KAPIAPI KAPIAPIERR KapiapiGetLastError(VOID);
//Deinitializes Kapiapi, call it at the end of your main() function
KAPIAPI VOID KapiapiDeinit(VOID);
//Gets KAPIAPI_USERDATA from provided username, which is case sensitive, returns 0 if operation failed
KAPIAPI BOOL KapiapiGetUserData(WSTRING user_name, KAPIAPI_PUSERDATA user_data_ptr);
//Parses KAPIAPI_USER from provided KAPIAPI_USERDATA
KAPIAPI BOOL KapiapiParseUserData(KAPIAPI_PUSER user_ptr, const KAPIAPI_USERDATA* user_data);
//Gets KAPIAPI_MAPDATA from provided map id, returns 0 if operation failed
KAPIAPI BOOL KapiapiGetMapData(KAPIAPI_WSLICE map_id, KAPIAPI_PMAPDATA map_data_ptr);
//Gets KAPIAPI_WSLICE map id from user, causes no errors
KAPIAPI KAPIAPI_WSLICE KapiapiGetUserMapId(KAPIAPI_PUSER user_ptr, WCHAR** memory_ptr);
//
KAPIAPI BOOL KapiapiEnumerateMapData(SIZEPARAM* size_ptr, KAPIAPI_NULLABLE KAPIAPI_PMAP maps,const KAPIAPI_MAPDATA* map_data);