#include <lauxlib.h>
#include <stdarg.h>
#include "d2itemreader.h"

#ifdef _WIN32
	#define EXPORT __declspec(dllexport)
#else
	#define EXPORT
#endif

#if LUA_VERSION_NUM < 502
#ifndef luaL_newlib
#	define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif
#	define luaL_setfuncs(L,l,n) (luaL_register(L,NULL,l))
#endif

#define set_field(T, L, k, v) (lua_pushliteral(L, k), lua_push##T(L, v), lua_rawset(L, -3))
#define set_lfield(T, L, k, v, l) (lua_pushliteral(L, k), lua_push##T(L, v, l), lua_rawset(L, -3))

#define set_literal(L, k, v) set_field(literal, L, k, v)
#define set_string(L, k, v) set_field(string, L, k, v)
#define set_lstring(L, k, v, l) set_lfield(lstring, L, k, v, l)
#define set_integer(L, k, v) set_field(integer, L, k, v)
#define set_boolean(L, k, v) set_field(boolean, L, k ,v)
#define set_value(L, k, v) set_field(value, L, k ,v)

static int push_error(lua_State *L, const char* msg)
{
	lua_pushnil(L);
	lua_pushstring(L, msg);
	return 2;
}

static int push_ferror(lua_State *L, const char *fmt, ...)
{
	lua_pushnil(L);
	va_list args;
    va_start(args, fmt);
    lua_pushvfstring(L, fmt, args);
    va_end(args);
    return 2;
}

static void push_d2item_rarity_affixes(lua_State *L, uint16_t* affixes, uint8_t numAffixes)
{
	lua_createtable(L, numAffixes, 0);
	for (int i=0; i<numAffixes; i++)
	{
		lua_pushinteger(L, affixes[i]);
		lua_rawseti(L, -2, i+1);
	}
}

static void push_d2item_set_rarity(lua_State *L, d2item* item)
{
	switch(item->rarity)
	{
	case D2RARITY_UNIQUE:
		set_literal(L, "rarity", "unique");
		lua_createtable(L, 0, 1);
		set_integer(L, "id", item->uniqueID);
		lua_setfield(L, -2, "rarityData");
		break;
	case D2RARITY_SET:
		set_literal(L, "rarity", "set");
		lua_createtable(L, 0, 1);
		set_integer(L, "id", item->setID);
		lua_setfield(L, -2, "rarityData");
		break;
	case D2RARITY_NORMAL:
		set_literal(L, "rarity", "normal");
		break;
	case D2RARITY_LOW_QUALITY:
		set_literal(L, "rarity", "lowquality");
		lua_createtable(L, 0, 1);
		set_integer(L, "id", item->lowQualityID);
		lua_setfield(L, -2, "rarityData");
		break;
	case D2RARITY_HIGH_QUALITY:
		set_literal(L, "rarity", "superior");
		lua_createtable(L, 0, 1);
		set_integer(L, "id", item->superiorID);
		lua_setfield(L, -2, "rarityData");
		break;
	case D2RARITY_MAGIC:
		set_literal(L, "rarity", "magic");
		lua_createtable(L, 0, 2);
		set_integer(L, "prefix", item->magicPrefix);
		set_integer(L, "suffix", item->magicSuffix);
		lua_setfield(L, -2, "rarityData");
		break;
	case D2RARITY_CRAFTED:
	case D2RARITY_RARE:
		set_string(L, "rarity", (item->rarity == D2RARITY_CRAFTED ? "crafted" : "rare"));
		lua_createtable(L, 0, 4);
		set_integer(L, "name1", item->nameID1);
		set_integer(L, "name2", item->nameID2);
		push_d2item_rarity_affixes(L, item->rarePrefixes, item->numRarePrefixes);
		lua_setfield(L, -2, "prefixes");
		push_d2item_rarity_affixes(L, item->rareSuffixes, item->numRareSuffixes);
		lua_setfield(L, -2, "suffixes");
		lua_setfield(L, -2, "rarityData");
		break;
	}
}

static void push_d2item(lua_State *L, d2item* item)
{
	lua_newtable(L);
	set_boolean(L, "identified", item->identified);
	set_string(L, "code", item->code);
	if (!item->simpleItem)
	{
		if (item->id)
		{
			static char hexID[8+1];
			snprintf(hexID, 8+1, "%08X", item->id);
			set_string(L, "id", hexID);
		}
		set_integer(L, "level", item->level);
		push_d2item_set_rarity(L, item);
	}
	return;
}

static int ld2itemreader_get_items(lua_State *L)
{
	static d2char character;
	static d2personalstash pstash;
	static d2sharedstash sstash;
	static uint32_t bytesRead;
	static int key;

	const char* filepath = luaL_checkstring(L, 1);
	enum d2filetype type = d2filetype_of_file(filepath);
	if (type == D2FILETYPE_UNKNOWN)
		return push_ferror(L, "could not determine filetype of %s", filepath);

	lua_newtable(L);
	switch(type)
	{
	case D2FILETYPE_D2_CHARACTER:
		d2char_parse(filepath, &character, &bytesRead);
		for (int i=0; i<character.items.count; i++)
		{
			d2item* item = &character.items.items[i];
			push_d2item(L, item);
			lua_rawseti(L, -2, i+1);
		}
		d2char_destroy(&character);
		break;
	case D2FILETYPE_PLUGY_PERSONAL_STASH:
		d2personalstash_parse(filepath, &pstash, &bytesRead);
		key=1;
		for (int i=0; i<pstash.numPages; i++)
		{
			d2stashpage* page = &pstash.pages[i];
			for (int j=0; j<page->items.count; j++)
			{
				d2item* item = &page->items.items[j];
				push_d2item(L, item);
				lua_rawseti(L, -2, key);
				key++;
			}
		}
		d2personalstash_destroy(&pstash);
		break;
	case D2FILETYPE_PLUGY_SHARED_STASH:
		d2sharedstash_parse(filepath, &sstash, &bytesRead);
		key=1;
		for (int i=0; i<sstash.numPages; i++)
		{
			d2stashpage* page = &sstash.pages[i];
			for (int j=0; j<page->items.count; j++)
			{
				d2item* item = &page->items.items[j];
				push_d2item(L, item);
				lua_rawseti(L, -2, key);
				key++;
			}
		}
		d2sharedstash_destroy(&sstash);
		break;
	default:
		return push_ferror(L, "unhandled filetype of %s: %d", filepath, type);
		break;
	}
	return 1;
}

static int ld2itemreader_load_data(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	const char* filepath;

	lua_getfield(L, 1, "armors");
	filepath = luaL_checkstring(L, -1);
	d2data_load_armors_from_file(filepath, &g_d2data);
	lua_pop(L, 1);
	
	lua_getfield(L, 1, "weapons");
	filepath = luaL_checkstring(L, -1);
	d2data_load_weapons_from_file(filepath, &g_d2data);
	lua_pop(L, 1);
	
	lua_getfield(L, 1, "miscs");
	filepath = luaL_checkstring(L, -1);
	d2data_load_miscs_from_file(filepath, &g_d2data);
	lua_pop(L, 1);
	
	lua_getfield(L, 1, "itemstats");
	filepath = luaL_checkstring(L, -1);
	d2data_load_itemstats_from_file(filepath, &g_d2data);
	lua_pop(L, 1);

	return 0;
}

static int ld2itemreader_setup_data(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	size_t len;
	const char* data;

	lua_getfield(L, 1, "armors");
	data = luaL_checklstring(L, -1, &len);
	d2data_load_armors(data, len, &g_d2data);
	lua_pop(L, 1);
	
	lua_getfield(L, 1, "weapons");
	data = luaL_checklstring(L, -1, &len);
	d2data_load_weapons(data, len, &g_d2data);
	lua_pop(L, 1);
	
	lua_getfield(L, 1, "miscs");
	data = luaL_checklstring(L, -1, &len);
	d2data_load_miscs(data, len, &g_d2data);
	lua_pop(L, 1);
	
	lua_getfield(L, 1, "itemstats");
	data = luaL_checklstring(L, -1, &len);
	d2data_load_itemstats(data, len, &g_d2data);
	lua_pop(L, 1);

	return 0;
}

static const luaL_Reg d2itemreader_lualib[] = {
	{ "load_data", ld2itemreader_load_data },
	{ "setup_data", ld2itemreader_setup_data },
	{ "get_items", ld2itemreader_get_items },
	{ NULL, NULL }
};

EXPORT int luaopen_d2itemreader(lua_State *L)
{
	luaL_newlib(L, d2itemreader_lualib);
	return 1;
}
