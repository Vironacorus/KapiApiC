#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
//#define char if you want to use wchar_t like such

//#define char wchar_t
//#include <json.h>
//#undef char
typedef char _CHAR;
typedef const _CHAR* _STRING;
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;
typedef size_t SIZEPARAM;
typedef wchar_t WCHAR;
#ifndef VOID //<Windows.h> defines VOID as void
typedef void VOID;
#endif
typedef VOID* PVOID;

typedef struct JsonNode JSONNODE;

typedef struct StringSlice
{
	_STRING ptr;
	SIZEPARAM length;
} SLICE;

typedef enum JsonNodeType
{
	JSON_NODE_TYPE_MAP, //{"X":"Y","Z":"W","V":[0,1,2,3]} - a map
	JSON_NODE_TYPE_KV_PAIR, // "X" : "Y" - a key value pair
	JSON_NODE_TYPE_NUMBER, // 10 - a number
	JSON_NODE_TYPE_STRING, //"string" - a string
	JSON_NODE_TYPE_LIST, //["X","Y","Z"] - a list
	JSON_NODE_TYPE_SPECIAL_VALUE //false, null, true - special values
} JSONNODETYPE;

typedef struct JsonNodeSpecialValue
{
	SLICE json_value;
	I64 c_value; //0 - false, null 1 - true
} JSONNODESPECIALVALUE;

typedef struct JsonNodeArray
{
	JSONNODE* nodes;
	SIZEPARAM size;
	SIZEPARAM capacity;
} JSONNODEARRAY;

typedef struct JsonNodeKvp
{
	JSONNODE* key;
	JSONNODE* value;
} JSONNODEKVP;

typedef struct JsonNodeMap
{
	JSONNODEARRAY array; //It stores only KVP values!
} JSONNODEMAP;

typedef struct JsonNodeList
{
	JSONNODEARRAY array; //it stores both number and string values!
} JSONNODELIST;

typedef struct JsonNodeString
{
	SLICE string_slice;
} JSONNODESTRING;

typedef struct JsonNodeNumber
{
	double value;
	SLICE string_slice;
} JSONNODENUMBER;

typedef enum ParserState
{
	PARSER_STATE_IDLE,
	PARSER_STATE_MAP,
	PARSER_STATE_KEY,
	PARSER_STATE_VALUE,
	PARSER_STATE_STRING,
	PARSER_STATE_ESCAPE_STRING,
	PARSER_STATE_NUMBER,
	PARSER_STATE_LIST
} PARSERSTATE;

typedef struct JsonNode
{
	JSONNODETYPE node_type;
	JSONNODE* previous;
	PARSERSTATE state;
	union
	{
		JSONNODEMAP map;
		JSONNODEKVP key_value_pair;
		JSONNODESTRING string;
		JSONNODENUMBER number;
		JSONNODELIST list;
		JSONNODESPECIALVALUE special_value;
	};
} JSONNODE;


typedef enum JsonErrorCode
{
	JSON_ERROR_NONE = 0x00,
	JSON_ERROR_UNEXPECTED_TOKEN
} JSONERRORCODE;


typedef struct JsonResult
{
	JSONNODE base_node;
} JSONRESULT;

typedef struct JsonError
{
	JSONERRORCODE error_code;
	union
	{
		JSONRESULT result;
	};
} JSONERROR;

JSONERROR JsonParse(_STRING str);
void PrintJsonTree(const JSONNODE* node, I32 depth);
WCHAR* RawToUnicode(SLICE s);

#define DebugJson(N) PrintJsonTree(N,0)