#include "kapiapi.h"
#include <stdio.h>
#define char char
#include "json.h"
#undef char

#define STRMATCH(STATIC_STR,DYNAMIC_STR) !memcmp(STATIC_STR,DYNAMIC_STR,sizeof(STATIC_STR)-1)


KAPIAPIERR last_error = KAPIAPI_ERR_NONE;

KAPIAPI KAPIAPIERR KapiapiGetLastError(VOID)
{
	KAPIAPIERR err = last_error;
	last_error = KAPIAPI_ERR_NONE;
	return err;
}

#ifdef _WIN32

HINTERNET winhttp_session;
HINTERNET winhttp_connection;

KAPIAPI BOOL KapiapiInit(VOID)
{
	winhttp_session = WinHttpOpen(L"KAPIAPI", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	winhttp_connection = WinHttpConnect(winhttp_session, L"www.minecraftmapy.pl", INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!winhttp_connection)
	{
		last_error = KAPIAPI_ERR_FAILED_TO_ESTABLISH_CONNECTION;
		return 0;
	}

	return 1;
}

KAPIAPI VOID KapiapiDeinit(VOID)
{
	WinHttpCloseHandle(winhttp_session);
}

KAPIAPI BOOL KapiapiGetUserData(WSTRING user_name, KAPIAPI_PUSERDATA user_data_ptr)
{
	const WCHAR basename[] = L"/api/user/";
	const SIZEPARAM user_name_size = (wcslen(user_name)+1) * sizeof(WCHAR);
	const SIZEPARAM basename_size = sizeof(basename) - sizeof(basename[0]);

	WCHAR* string = HeapAlloc(GetProcessHeap(),0,sizeof(basename) + user_name_size);
	memcpy(string,basename,basename_size); //subract \0 character
	memcpy((U8*)string+basename_size,user_name, user_name_size);

	HINTERNET winhttp_request = WinHttpOpenRequest(winhttp_connection,L"GET",string,0,0,0, WINHTTP_FLAG_SECURE);
	BOOL winhttp_send_result = WinHttpSendRequest(winhttp_request,0,0,0,0,0,0);

	if (!winhttp_send_result)
	{
		last_error = KAPIAPI_ERR_FAILED_TO_SEND_REQUEST;
		WinHttpCloseHandle(winhttp_request);
		return 0;
	}

	BOOL winhttp_receive_result = WinHttpReceiveResponse(winhttp_request,NULL);

	if (!winhttp_receive_result)
	{
		last_error = KAPIAPI_ERR_FAILED_TO_RECEIVE_DATA;
		WinHttpCloseHandle(winhttp_request);
		return 0;
	}

	DWORD size = 0;
	DWORD downloaded_size = 0;

	DWORD full_size = 0;

	U8* buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1);

	do
	{
		DWORD prev_size = full_size;

		if (!WinHttpQueryDataAvailable(winhttp_request, &size))
		{
			//TODO: Add proper error codes
			last_error = KAPIAPI_ERR_OTHER;
			WinHttpCloseHandle(winhttp_request);
			return 0;
		}

		full_size += size;

		CHAR* temp_buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, full_size+1);
		memcpy(temp_buffer,buffer, prev_size);
		HeapFree(GetProcessHeap(),0,buffer);
		buffer = temp_buffer;


		if (!WinHttpReadData(winhttp_request, &buffer[prev_size],
			size, &downloaded_size))
		{
			last_error = KAPIAPI_ERR_UNABLE_TO_READ_DATA;
			WinHttpCloseHandle(winhttp_request);
			return 0;
		}

	} while (size > 0);

	user_data_ptr->bytes = buffer;
	user_data_ptr->size = size;

	WinHttpCloseHandle(winhttp_request);

	return 1;
}

static void IterateStats(KAPIAPI_PUSER user_ptr,  const JSONNODE* stats)
{
#define PARSE(NAME) else if (STRMATCH( #NAME , json_node->key_value_pair.key->string.string_slice.ptr)) \
						user_ptr->stats.##NAME = (U64)json_node->key_value_pair.value->number.value;
	for (SIZEPARAM i = 0; i < (stats->map.array.size / sizeof(JSONNODE)); ++i)
	{
		JSONNODE* const json_node = &stats->map.array.nodes[i];
		if (0);
		PARSE(diamond_sum)
		PARSE(download_sum)
		PARSE(written_comments)
		PARSE(given_diamonds)
		PARSE(map_count)
		PARSE(follower_count)
	}
#undef PARSE
}

static void IterateInfo(KAPIAPI_PUSER user_ptr, const JSONNODE* info)
{
#define PARSE(NAME) else if (STRMATCH( #NAME , json_node->key_value_pair.key->string.string_slice.ptr)) \
						user_ptr->info.##NAME =  (KAPIAPI_SLICE){json_node->key_value_pair.value->string.string_slice.ptr,json_node->key_value_pair.value->string.string_slice.length};
	for (SIZEPARAM i = 0; i < (info->map.array.size / sizeof(JSONNODE)); ++i)
	{
		JSONNODE* const json_node = &info->map.array.nodes[i];
		if (0);
		PARSE(username)
			PARSE(description)
			PARSE(last_logged_at)
			PARSE(last_logged_relative)
			PARSE(avatar_url)
		else if (STRMATCH("is_active", json_node->key_value_pair.key->string.string_slice.ptr))
			user_ptr->info.is_active = (BOOL)json_node->key_value_pair.value->special_value.c_value;
		else if (STRMATCH("role", json_node->key_value_pair.key->string.string_slice.ptr))
		{
			user_ptr->info.role.role_string = (KAPIAPI_SLICE){ json_node->key_value_pair.value->string.string_slice.ptr,json_node->key_value_pair.value->string.string_slice.length };
			STRING str = json_node->key_value_pair.value->string.string_slice.ptr;
			if (STRMATCH("U\\u017cytkownik", str))
				user_ptr->info.role.role_type = KAPIAPI_ROLE_USER;
			else if (STRMATCH("Moderator", str))
				user_ptr->info.role.role_type = KAPIAPI_ROLE_MOD;
			else if (STRMATCH("Administrator", str))
				user_ptr->info.role.role_type = KAPIAPI_ROLE_ADMIN;
			else if (STRMATCH("W\\u0142a\\u015bciciel", str))
				user_ptr->info.role.role_type = KAPIAPI_ROLE_OWNER;
		}
	}
#undef PARSE
}

static void IterateBan(KAPIAPI_PUSER user_ptr, const JSONNODE* ban)
{
	if (ban->node_type == JSON_NODE_TYPE_SPECIAL_VALUE)
	{
		user_ptr->ban = (KAPIAPI_BAN){ 0 };
		user_ptr->ban.has_ban = 0;
	}
	else
	{
		user_ptr->ban.has_ban = 1;
		for (SIZEPARAM i = 0; i < (ban->map.array.size / sizeof(JSONNODE)); ++i)
		{
			JSONNODE* const json_node = &ban->map.array.nodes[i];
			STRING str = json_node->key_value_pair.key->string.string_slice.ptr;

			if (STRMATCH("expires_at", str))
			{
				if (json_node->key_value_pair.value->node_type == JSON_NODE_TYPE_SPECIAL_VALUE)
				{
					user_ptr->ban.expires_at = (KAPIAPI_SLICE){ 0,json_node->key_value_pair.value->special_value.c_value };
				}
				else
				{
					user_ptr->ban.expires_at = (KAPIAPI_SLICE){ json_node->key_value_pair.value->string.string_slice.length,json_node->key_value_pair.value->string.string_slice.ptr };
				}
			}
			else if (STRMATCH("is_permament", str))
			{
				user_ptr->ban.is_permament = json_node->key_value_pair.value->special_value.c_value;
			}
			else if (STRMATCH("reason", str))
			{
				user_ptr->ban.reason = (KAPIAPI_SLICE){ json_node->key_value_pair.value->string.string_slice.length,json_node->key_value_pair.value->string.string_slice.ptr };
			}
		}

	}
}

KAPIAPI BOOL KapiapiParseUserData(KAPIAPI_PUSER user_ptr, const KAPIAPI_USERDATA* user_data)
{
	JSONERROR err = JsonParse(user_data->bytes);
	if (err.error_code)
	{
		last_error = KAPIAPI_ERR_INVALID_JSON;
		return 0;
	}
	JSONNODE node = err.result.base_node;
	node = *node.map.array.nodes[0].key_value_pair.value;

	for (SIZEPARAM i = 0; i < (node.map.array.size / sizeof(JSONNODE)); ++i)
	{
		if (STRMATCH("stats", node.map.array.nodes[i].key_value_pair.key->string.string_slice.ptr))
		{
			IterateStats(user_ptr, node.map.array.nodes[i].key_value_pair.value);
		}
		else if (STRMATCH("info", node.map.array.nodes[i].key_value_pair.key->string.string_slice.ptr))
		{
			IterateInfo(user_ptr, node.map.array.nodes[i].key_value_pair.value);
		}
		else if (STRMATCH("ban", node.map.array.nodes[i].key_value_pair.key->string.string_slice.ptr))
		{
			IterateBan(user_ptr, node.map.array.nodes[i].key_value_pair.value);
		}
		else if (STRMATCH("url", node.map.array.nodes[i].key_value_pair.key->string.string_slice.ptr))
		{
			user_ptr->url = (KAPIAPI_SLICE){node.map.array.nodes[i].key_value_pair.value->string.string_slice.ptr,node.map.array.nodes[i].key_value_pair.value->string.string_slice.length};
		}
		else if (STRMATCH("id", node.map.array.nodes[i].key_value_pair.key->string.string_slice.ptr))
		{
			user_ptr->id = (U64)node.map.array.nodes[i].key_value_pair.value->number.value;
		}
	}

	return 1;
}

#endif


int main()
{
	if(!KapiapiInit())
		printf("%i",KapiapiGetLastError());

	KAPIAPI_USERDATA user_data;
	KapiapiGetUserData(L"CzekotPL", &user_data);
	KAPIAPI_USER user;
	KapiapiParseUserData(&user,&user_data);
	printf("%i", user.ban.has_ban);

	KapiapiDeinit();
}