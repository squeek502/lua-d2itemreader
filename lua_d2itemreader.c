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
#define set_boolean(L, k, v) set_field(boolean, L, k, v)
#define set_value(L, k, v) set_field(value, L, k, v)

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

static int push_d2err(lua_State *L, d2err err)
{
	return push_error(L, d2err_str(err));
}

static void ld2itemreader_checkfield(lua_State *L, int idx, const char* k, int type)
{
	lua_getfield(L, idx, k);
	if (lua_type(L, -1) != type)
	{
		const char *msg = lua_pushfstring(L, "expected the value of key '%s' to be of type %s, got %s", k, lua_typename(L, type), luaL_typename(L, idx));
		luaL_argerror(L, idx, msg);
	}
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
	set_boolean(L, "ethereal", item->ethereal);
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
	set_boolean(L, "runeword", item->isRuneword);
	return;
}

static int ld2itemreader_getitems(lua_State *L)
{
	d2err err;
	static d2char character;
	static d2personalstash pstash;
	static d2sharedstash sstash;
	static d2atmastash astash;
	static size_t bytesRead;
	static int key;

	const char* filepath = luaL_checkstring(L, 1);
	enum d2filetype type = d2filetype_of_file(filepath);
	if (type == D2FILETYPE_UNKNOWN)
		return push_ferror(L, "could not determine filetype of %s", filepath);

	lua_newtable(L);
	switch(type)
	{
	case D2FILETYPE_D2_CHARACTER:
		err = d2char_parse_file(filepath, &character, &bytesRead);
		if (err != D2ERR_OK)
		{
			goto err;
		}
		for (int i=0; i<character.items.count; i++)
		{
			d2item* item = &character.items.items[i];
			push_d2item(L, item);
			lua_rawseti(L, -2, i + 1);
		}
		for (int i = 0; i<character.itemsCorpse.count; i++)
		{
			d2item* item = &character.itemsCorpse.items[i];
			push_d2item(L, item);
			lua_rawseti(L, -2, i + 1);
		}
		for (int i = 0; i<character.itemsMerc.count; i++)
		{
			d2item* item = &character.itemsMerc.items[i];
			push_d2item(L, item);
			lua_rawseti(L, -2, i + 1);
		}
		d2char_destroy(&character);
		break;
	case D2FILETYPE_PLUGY_PERSONAL_STASH:
		err = d2personalstash_parse_file(filepath, &pstash, &bytesRead);
		if (err != D2ERR_OK)
		{
			goto err;
		}
		key=1;
		for (uint32_t i=0; i<pstash.numPages; i++)
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
		err = d2sharedstash_parse_file(filepath, &sstash, &bytesRead);
		if (err != D2ERR_OK)
		{
			goto err;
		}
		key=1;
		for (uint32_t i=0; i<sstash.numPages; i++)
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
	case D2FILETYPE_ATMA_STASH:
		err = d2atmastash_parse_file(filepath, &astash, &bytesRead);
		if (err != D2ERR_OK)
		{
			goto err;
		}
		for (int i = 0; i<astash.items.count; i++)
		{
			d2item* item = &astash.items.items[i];
			push_d2item(L, item);
			lua_rawseti(L, -2, i + 1);
		}
		d2atmastash_destroy(&astash);
		break;
	default:
		return push_ferror(L, "unhandled filetype of %s: %d", filepath, type);
		break;
	}
	return 1;

err:
	return push_ferror(L, "failed to parse %s: %s at byte 0x%X", filepath, d2err_str(err), bytesRead);
}

static int ld2itemreader_loadfiles(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	d2datafiles files;

	ld2itemreader_checkfield(L, 1, "armors", LUA_TSTRING);
	files.armorTxtFilepath = lua_tostring(L, -1);
	lua_pop(L, 1);

	ld2itemreader_checkfield(L, 1, "weapons", LUA_TSTRING);
	files.weaponsTxtFilepath = luaL_checkstring(L, -1);
	lua_pop(L, 1);

	ld2itemreader_checkfield(L, 1, "miscs", LUA_TSTRING);
	files.miscTxtFilepath = luaL_checkstring(L, -1);
	lua_pop(L, 1);

	ld2itemreader_checkfield(L, 1, "itemstats", LUA_TSTRING);
	files.itemStatCostTxtFilepath = luaL_checkstring(L, -1);
	lua_pop(L, 1);

	d2err err = d2itemreader_init_files(files);
	if (err != D2ERR_OK)
	{
		return push_d2err(L, err);
	}

	return 0;
}

static int ld2itemreader_loaddata(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	d2databufs bufs;

	ld2itemreader_checkfield(L, 1, "armors", LUA_TSTRING);
	bufs.armorTxt = (char*)lua_tolstring(L, -1, &bufs.armorTxtSize);
	lua_pop(L, 1);

	ld2itemreader_checkfield(L, 1, "weapons", LUA_TSTRING);
	bufs.weaponsTxt = (char*)lua_tolstring(L, -1, &bufs.weaponsTxtSize);
	lua_pop(L, 1);

	ld2itemreader_checkfield(L, 1, "miscs", LUA_TSTRING);
	bufs.miscTxt = (char*)lua_tolstring(L, -1, &bufs.miscTxtSize);
	lua_pop(L, 1);

	ld2itemreader_checkfield(L, 1, "itemstats", LUA_TSTRING);
	bufs.itemStatCostTxt = (char*)lua_tolstring(L, -1, &bufs.itemStatCostTxtSize);
	lua_pop(L, 1);

	d2err err = d2itemreader_init_bufs(bufs);
	if (err != D2ERR_OK)
	{
		return push_d2err(L, err);
	}

	return 0;
}

static const luaL_Reg d2itemreader_lualib[] = {
	{ "loadfiles", ld2itemreader_loadfiles },
	{ "loaddata", ld2itemreader_loaddata },
	{ "getitems", ld2itemreader_getitems },
	{ NULL, NULL }
};

static int ld2itemreader_close(lua_State *L)
{
	d2itemreader_destroy();
	return 0;
}

static const luaL_Reg d2itemreader_gc[] = {
	{ "__gc", ld2itemreader_close },
	{ NULL, NULL }
};


EXPORT int luaopen_d2itemreader(lua_State *L)
{
	d2err err = d2itemreader_init_default();
	if (err != D2ERR_OK)
	{
		luaL_error(L, "failed to initialize d2itemreader: %s", d2err_str(err));
	}
	luaL_newlib(L, d2itemreader_lualib);

	lua_newuserdata(L, 0);
	lua_newtable(L);
	luaL_setfuncs(L, d2itemreader_gc, 0);
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "__unload");

	return 1;
}
