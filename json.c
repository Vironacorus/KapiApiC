#include "json.h"

//thanks Stackoverflow :)
int ipow(int base, int exp)
{
	int result = 1;
	for (;;)
	{
		if (exp & 1)
			result *= base;
		exp >>= 1;
		if (!exp)
			break;
		base *= base;
	}

	return result;
}


WCHAR* RawToUnicode(SLICE s)
{
	const SIZEPARAM allocation_size = (s.length + 1) * sizeof(wchar_t*);
	WCHAR* wc = malloc(allocation_size);
	memset(wc, 0, allocation_size);

	typedef enum MODE
	{
		FORMAT_MODE_ESC,
		FORMAT_MODE_NORMAL,
		FORMAT_MODE_UNICODE
	} FORMAT_MODE;

	FORMAT_MODE mode = FORMAT_MODE_NORMAL;

	//I didn't want to do fancy stuff, so I just use 3 shared variables
	SIZEPARAM counter = 0;
	WCHAR counter1 = 0;
	SIZEPARAM counter2 = 0;

	SIZEPARAM r = 0;
	SIZEPARAM w = 0;
	for (; r < s.length && w < s.length; (++r, ++w))
	{
		const char ch = s.ptr[r];
		const WCHAR converted = ch;

		switch (mode)
		{
		case FORMAT_MODE_NORMAL:
			
			if (ch == '\\')
			{
				mode = FORMAT_MODE_ESC;
			}
			else
			{
				wc[w] = converted;
			}
			break;
		case FORMAT_MODE_ESC:			
			if (ch == 'u')
			{
				mode = FORMAT_MODE_UNICODE;
				counter = 0;
				counter1 = 0;
			}
			else if (ch == 'n')
			{
				--w;
				wc[w] = '\n';
			}
			else if (ch == 'r')
			{
				--w;
				wc[w] = '\r';
			}
			else if (ch == 'b')
			{
				--w;
				wc[w] = '\b';
			}
			else
			{
				--w;
				wc[w] = converted;
			}

			mode = FORMAT_MODE_NORMAL;
			break;
		case FORMAT_MODE_UNICODE:
		{
			++counter;
			char val = '\0';
			if (ch <= 'z' && ch >= 'a')
			{
				val = (ch - 'a') + 10;
			}
			else if (ch <= 'Z' && ch >= 'A')
			{
				val = (ch - 'A') + 10;
			}
			else
			{
				val = (ch - '0');
			}

			counter1 += val * ipow(16, (4 - counter));
			if (counter >= 4)
			{
				w -= 5; //back to \ character
				wc[w] = counter1;
				mode = FORMAT_MODE_NORMAL;
			}
		}
			break;
		}
	}

	wc[w] = 0;

	return wc;
}

static JSONNODE* Add(JSONNODEARRAY* node_array, JSONNODE node)
{
	SIZEPARAM newsize = sizeof(node) + node_array->size;
	
	if (node_array->capacity < newsize)
	{
		//reallocate
		const SIZEPARAM newcapacity = node_array->capacity + newsize;
		JSONNODE* json_nodes = malloc(newcapacity);
		memcpy(json_nodes,node_array->nodes,node_array->size);
		free(node_array->nodes);
		node_array->nodes = json_nodes;
		node_array->capacity = newcapacity;
	}
	JSONNODE* added_ptr = ((JSONNODE*)&(((U8*)node_array->nodes)[node_array->size]));
	*added_ptr = node;
	node_array->size = newsize;
	return added_ptr;
}

static void Reserve(JSONNODEARRAY* node_array, SIZEPARAM size)
{
	const SIZEPARAM newcapacity = node_array->capacity + size;
	JSONNODE* json_nodes = malloc(newcapacity);
	memcpy(json_nodes, node_array->nodes, node_array->size);
	free(node_array->nodes);
	node_array->nodes = json_nodes;
	node_array->capacity = newcapacity;
}

static void Destroy(const JSONNODEARRAY* node_array)
{
	free(node_array->nodes);
}

static double ParseNumber(SLICE number_slice)
{
	double d = 0.0;
	I64 i = 0;
	for (; i < number_slice.length; ++i)
	{
		_CHAR c = number_slice.ptr[i];
		if (c == '.')
			break;
	}
	const I64 dot_pos = i;
	++i;
	double mul = 0.1;
	for (; i < number_slice.length; ++i)
	{
		_CHAR c = number_slice.ptr[i];
		d += mul * (c - '0');
		mul /= 10.0;
	}
	
	mul = 1.0;

	for (i = dot_pos-1; i >= 0; --i)
	{
		_CHAR c = number_slice.ptr[i];
		d += mul * (c - '0');
		mul *= 10.0;
	}

	return d;
}

JSONERROR JsonParse(_STRING str)
{
	SLICE special_values[] =
	{
		{"null",4},
		{"true",4},
		{"false",5}
	};

	PARSERSTATE state = PARSER_STATE_IDLE;
	JSONERROR result = {0}; //0 it all out
	JSONNODE* top_node = NULL;
	
	for (SIZEPARAM i = 0; str[i]; ++i)
	{
		_CHAR  c = str[i];
		const _CHAR* ch = &str[i];
	repeat:
		switch (state)
		{
		case PARSER_STATE_IDLE:
			if (c == '{')
			{
				JSONNODE* prev = top_node;
				top_node = &result.result.base_node;
				top_node->state = state;
				top_node->node_type = JSON_NODE_TYPE_MAP;
				top_node->previous = prev;
				state = PARSER_STATE_MAP;
			}
			else if (c == '[')
			{
				JSONNODE* prev = top_node;
				top_node = &result.result.base_node;
				top_node->state = state;
				top_node->node_type = JSON_NODE_TYPE_LIST;
				top_node->previous = prev;
				state = PARSER_STATE_LIST;
			}
			else
			{
				result.error_code = JSON_ERROR_UNEXPECTED_TOKEN;
				goto end;
			}
			break;
		case PARSER_STATE_MAP:
			if (c == '}')
			{
				state = top_node->state;
				top_node = top_node->previous;
			}
			else if (c == '"')
			{
				JSONNODE* prev = top_node;

				JSONNODE kv_pair;
				kv_pair.node_type = JSON_NODE_TYPE_KV_PAIR;
				top_node = Add(&top_node->map.array, kv_pair);
				top_node->state = state;
				top_node->previous = prev;
				state = PARSER_STATE_KEY;
				goto repeat;
			}
			else if (c == ',' || c == ' ' || c == '\n')
					;
			else
			{
				result.error_code = JSON_ERROR_UNEXPECTED_TOKEN;
				goto end;
			}
			break;
			case PARSER_STATE_KEY:
				if (c == ':')
				{
					state = PARSER_STATE_VALUE;
				}
				else if (c == '"')
				{
					JSONNODE* prev = top_node;
					top_node->key_value_pair.key = malloc(sizeof(JSONNODE));
					top_node->key_value_pair.key->node_type = JSON_NODE_TYPE_STRING;
					top_node = top_node->key_value_pair.key;
					top_node->state = state;
					top_node->previous = prev;
					top_node->string.string_slice = (SLICE){ ch+1,0 };
					state = PARSER_STATE_STRING;
				}
				else if (c == ',' || c == ' ' || c == '\n')
					;
				break;
			case PARSER_STATE_STRING:
				if (c == '"')
				{
					state = top_node->state;
					top_node = top_node->previous;
				}
				else if (c == '\\')
				{
					top_node->string.string_slice.length++;
					state = PARSER_STATE_ESCAPE_STRING;
				}
				else
					top_node->string.string_slice.length++;
				break;
			case PARSER_STATE_ESCAPE_STRING:
				top_node->string.string_slice.length++;
				state = PARSER_STATE_STRING;
				break;
			case PARSER_STATE_VALUE:
				if (c == '"')
				{
					JSONNODE* prev = top_node;
					top_node->key_value_pair.value = malloc(sizeof(JSONNODE));
					top_node->key_value_pair.value->node_type = JSON_NODE_TYPE_STRING;
					top_node = top_node->key_value_pair.value;
					top_node->state = state;
					top_node->previous = prev;
					top_node->string.string_slice = (SLICE){ ch + 1,0 };
					state = PARSER_STATE_STRING;
				}
				else if (c == '[')
				{
					JSONNODE* prev = top_node;
					top_node->key_value_pair.value = malloc(sizeof(JSONNODE));
					top_node->key_value_pair.value->node_type = JSON_NODE_TYPE_LIST;
					top_node = top_node->key_value_pair.value;
					top_node->state = state;
					top_node->list.array = (JSONNODEARRAY){ 0,0,0 };
					top_node->previous = prev;
					state = PARSER_STATE_LIST;
				}
				else if (c == '{')
				{
					JSONNODE* prev = top_node;
					top_node->key_value_pair.value = malloc(sizeof(JSONNODE));
					top_node->key_value_pair.value->node_type = JSON_NODE_TYPE_MAP;
					top_node = top_node->key_value_pair.value;
					top_node->state = state;
					top_node->map.array = (JSONNODEARRAY){ 0,0,0 };
					top_node->previous = prev;
					state = PARSER_STATE_MAP;
				}
				else if (isdigit(c))
				{

					JSONNODE* prev = top_node;
					top_node->key_value_pair.value = malloc(sizeof(JSONNODE));
					top_node->key_value_pair.value->node_type = JSON_NODE_TYPE_NUMBER;
					top_node = top_node->key_value_pair.value;
					top_node->state = state;
					top_node->number.value = 0;
					top_node->previous = prev;
					top_node->number.string_slice = (SLICE){ ch,0 };
					state = PARSER_STATE_NUMBER;
				}
				else if (c == ' ' || c == '\n')
					;
				else if(isalpha(c))
				{
					I8 ok = 0;
					SIZEPARAM j;
					SLICE s;
					s.ptr = ch;

					for (j = 0; j < sizeof(special_values) / sizeof(special_values[0]); ++j)
					{
						const SLICE value_name = special_values[j];
						const SIZEPARAM i_prev = i;
						ok = 1;
						for (; (i - i_prev) < value_name.length && str[i]; ++i)
						{
							c = str[i];
							ch = &str[i];
							const _CHAR  sc = special_values[j].ptr[i - i_prev];
							if (c != sc)
							{
								ok = 0;
								break;
							}
						}
						if (ok)
						{
							s.length = i - i_prev;
							goto foundv;
						}
						i = i_prev;
					}
					//not found
					result.error_code = JSON_ERROR_UNEXPECTED_TOKEN; //TODO: Add unrecognized value error
					goto end;
				foundv:
					{
						I64 value;
						switch (j)
						{
						case 0:
							value = 0;
							break;
						case 1:
							value = 1;
							break;
						case 2:
							value = 0;
							break;
						}
						--i;
						JSONNODE* val = malloc(sizeof(JSONNODE));
						val->node_type = JSON_NODE_TYPE_SPECIAL_VALUE;
						top_node->key_value_pair.value = val;
						val->state = state;
						val->previous = top_node;
						val->special_value.c_value = value;
						val->special_value.json_value = s;
					}
				}
				else
				{
					state = top_node->state;
					top_node = top_node->previous;
					goto repeat;
				}
				break;
			case PARSER_STATE_LIST:
				if (c == ']')
				{
					state = top_node->state;
					top_node = top_node->previous;
				}
				else if (c == '"')
				{
					JSONNODE* prev = top_node;
					JSONNODE nd;
					nd.node_type = JSON_NODE_TYPE_STRING;
					top_node = Add(&top_node->list.array, nd);
					top_node->state = state;
					top_node->previous = prev;
					top_node->string.string_slice = (SLICE){ ch + 1,0 };
					state = PARSER_STATE_STRING;
				}
				else if (c == '{')
				{
					JSONNODE* prev = top_node;
					JSONNODE nd;
					nd.node_type = JSON_NODE_TYPE_MAP;
					top_node = Add(&top_node->list.array, nd);
					top_node->state = state;
					top_node->previous = prev;
					top_node->map.array = (JSONNODEARRAY){ 0,0,0 };
					state = PARSER_STATE_MAP;
				}
				else if (c == '[')
				{
					JSONNODE* prev = top_node;
					JSONNODE nd;
					nd.node_type = JSON_NODE_TYPE_LIST;
					top_node = Add(&top_node->list.array, nd);
					top_node->state = state;
					top_node->previous = prev;
					top_node->list.array = (JSONNODEARRAY){ 0,0,0 };
					state = PARSER_STATE_LIST;
				}
				else if (isdigit(c))
				{

					JSONNODE* prev = top_node;
					JSONNODE nd;
					nd.node_type = JSON_NODE_TYPE_NUMBER;
					top_node = Add(&top_node->list.array, nd);
					top_node->state = state;
					top_node->previous = prev;
					top_node->number.value = 0.0;
					top_node->number.string_slice = (SLICE){ ch,0};
					state = PARSER_STATE_NUMBER;
				}
				else if (c == ' ' || c == '\n' || c == ',')
					;
				else
				{
					I8 ok = 0;
					SIZEPARAM j;
					SLICE s;
					s.ptr = ch;

					for (j = 0; j < sizeof(special_values) / sizeof(special_values[0]); ++j)
					{
						const SLICE value_name = special_values[j];
						const SIZEPARAM i_prev = i;
						ok = 1;
						for (; (i-i_prev) < value_name.length && str[i]; ++i)
						{
							c = str[i];
							ch = &str[i];
							const _CHAR  sc = special_values[j].ptr[i-i_prev];
							if (c != sc)
							{
								ok = 0;
								break;
							}
						}
						if (ok)
						{
							s.length = i - i_prev;
							goto found;
						}
						i = i_prev;
					}
					//not found
					result.error_code = JSON_ERROR_UNEXPECTED_TOKEN; //TODO: Add unrecognized value error
					goto end;
					found:
					{
						I64 value;
						switch (j)
						{
						case 0:
							value = 0;
							break;
						case 1:
							value = 1;
							break;
						case 2:
							value = 0;
							break;
						}
						JSONNODE nd;
						nd.node_type = JSON_NODE_TYPE_SPECIAL_VALUE;
						JSONNODE* val = Add(&top_node->list.array, nd);
						val->state = state;
						val->previous = top_node;
						val->special_value.c_value = value;
						val->special_value.json_value = s;
					}
				}
				break;
			case PARSER_STATE_NUMBER:
				if (isdigit(c) || c == '.')
				{
					++top_node->number.string_slice.length;
				}
				else
				{
					++top_node->number.string_slice.length;
					top_node->number.value = ParseNumber(top_node->number.string_slice);
					state = top_node->state;
					top_node = top_node->previous;
					goto repeat;
				}
				break;
		}
	}
	end:
	return result;
}

void PrintJsonTree(const JSONNODE* node, I32 depth)
{
	for (SIZEPARAM i = 0; i < depth; ++i)
		printf("   ");
	switch(node->node_type)
	{
	case JSON_NODE_TYPE_MAP:
		printf("%s\n", "MAP");
		for (SIZEPARAM i = 0; i < (node->map.array.size/sizeof(JSONNODE)); ++i)
		{
			PrintJsonTree(&node->map.array.nodes[i], depth+1);
		}
		break;
	case JSON_NODE_TYPE_KV_PAIR:
		printf("%s\n","KEY VALUE PAIR");
		PrintJsonTree(node->key_value_pair.key, depth + 1);
		PrintJsonTree(node->key_value_pair.value, depth + 1);
		break;
	case JSON_NODE_TYPE_STRING:	
	{
		_CHAR  str[2500] = {0};
		memcpy(str, node->string.string_slice.ptr, node->string.string_slice.length);
		printf("%s : %s\n", "STRING", str);
	}
		break;
	case JSON_NODE_TYPE_LIST:
		printf("%s\n", "LIST");
		for (SIZEPARAM i = 0; i < (node->map.array.size / sizeof(JSONNODE)); ++i)
		{
			PrintJsonTree(&node->map.array.nodes[i], depth + 1);
		}
		break;
	case JSON_NODE_TYPE_NUMBER:
		printf("%s : %f\n","NUMBER", node->number.value);
		break;
	case JSON_NODE_TYPE_SPECIAL_VALUE:
		printf("%s : %i\n", "SPECIAL VALUE",node->special_value.c_value);
	}
}

/*
int main()
{
	char jsondata[1024];
	FILE* file = fopen("test.json","r");
	fread(jsondata,1,1024,file);
	fclose(file);
	JSONERROR json_error = JsonParse(jsondata);

	if(!json_error.error_code)
		DebugJson(&json_error.result.base_node);
	else
	{
		printf("Error code: %i", json_error.error_code);
	}
}
*/