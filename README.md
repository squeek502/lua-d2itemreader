lua-d2itemreader
================

**work in progress**

Lua bindings to the [d2itemreader](https://github.com/squeek502/d2itemreader) library

## API Reference

### `d2itemreader.getfiletype(filepath)`
Returns the type of the file at `filepath`, or `nil, err` if the filetype cannot be determined. If the filetype can be determined, it will be returned as one of the following strings: `"character"`, `"item"`, `"atma"`, `"personal"`, `"shared"`

### `d2itemreader.getitems(filepath)`
Returns an array-like table of items, or `nil, err` if there was an error reading the file. See [Item Format](#item-format) for the format of each item.

### `d2itemreader.loadfiles(filepaths)`
Load custom Diablo II .txt data files from disk at the filepaths given. `filepaths` should be a table of the format:

```lua
{
  armors = <string>, -- path to Armor.txt
  weapons = <string>, -- path to Weapons.txt
  miscs = <string>, -- path to Misc.txt
  itemstats = <string>, -- path to ItemStatCost.txt
}
```

### `d2itemreader.loaddata(databufs)`
Load custom Diablo II .txt data files from memory with the contents given. `databufs` should be a table of the format:

```lua
{
  armors = <string>, -- contents of Armor.txt
  weapons = <string>, -- contents of Weapons.txt
  miscs = <string>, -- contents of Misc.txt
  itemstats = <string>, -- contents of ItemStatCost.txt
}
```

### Item Format

```lua
-- Item
{
  source = {
    -- filepath containing the item
    file = <string>,
    -- section only exists for filetype "character",
    -- will be one of "character", "corpse", "merc"
    section = <string>,
    -- page only exists for paginated filetypes
    -- ("personal" or "shared")
    page = <number>
  },

  identified = <boolean>,
  socketed = <boolean>,
  isNew = <boolean>,
  isEar = <boolean>,
  startItem = <boolean>,
  simpleItem = <boolean>,
  ethereal = <boolean>,
  personalized = <boolean>,
  isRuneword = <boolean>,
  version = <number>,
  locationID = <number>,
  equippedID = <number>,
  positionX = <number>,
  positionY = <number>,
  panelID = <number>,

  -- only exists if isEar is true
  ear = {
    classID = <number>,
    level = <number>,
    name = <string>
  },

  code = <string>,
  socketedItems = {...} -- array-like table of items

  -- every key below only exists if simpleItem is false
  id = <string>, -- unique ID of the item in hexadecimal
  level = <number>,
  -- rarity is one of:
  --   "unique", "set", "normal", "lowquality"
  --   "superior", "magic", "crafted", "rare"
  rarity = <string>,
  -- rarityData is different depending on rarity, and
  -- doesn't exist for "normal" rarity
  rarityData = {
    -- for "unique", "set", "lowquality", "superior" rarities
    id = <number>,
    -- for "magic" rarity
    prefix = <number>,
    suffix = <number>,
    -- for "rare" or "crafted" rarities
    name1 = <number>,
    name2 = <number>,
    prefixes = {...}, -- array-like table of prefix IDs (numbers)
    suffixes = {...} -- array-like table of suffix IDs (numbers)
  },
  multiplePictures = <boolean>,
  pictureID = <number>,
  classSpecific = <boolean>,
  automagicID = <number>,
  -- only exists if personalized is true
  personalizedName = <string>,
  timestamp = <boolean>,
  defenseRating = <number>,
  maxDurability = <number>,
  currentDurability = <number>,
  quantity = <number>,
  numSockets = <number>,
  magicProperties = {...}, -- array-like table of magic properties (see 'Magic Property' below)
  setBonuses = {...}, -- see 'Set Bonuses' below
  runewordProperties = {...} -- array-like table of magic properties (see 'Magic Property' below)
}

-- Magic Property
{
  id = <number>,
  params = {...} -- array-like table of parameter values (numbers)
}

-- Set Bonuses
--
-- A table with keys 1-5 where each key may have a nil value. This means that it is
-- not guaranteed to be an array-like table (so ipairs and the # operator will not
-- always work, use `for i=1,5 do` to iterate instead)
--
-- Note: These are the green per-item set bonuses, not the overall set bonuses
{
  -- Each non-nil value is an array-like table of magic properties (see 'Magic Property' above)
  [1] = {...} or nil,
  [2] = {...} or nil,
  [3] = {...} or nil,
  [4] = {...} or nil,
  [5] = {...} or nil,
  -- When the bonuses are active depends on the value of add_func in SetItems.txt
  -- for the setID of the item:
  --
  -- If add_func=2, then it uses the number of items of the set that are worn:
  --  - The property list at key 1 is active when >= 2 items of the set are worn.
  --  - The property list at key 2 is active when >= 3 items of the set are worn.
  --  - etc.
  --
  -- If add_func=1, then specific other items of the set need to be worn:
  --  - If the item's setID is the first of the set:
  --   + then the property list at key 1 is active when the second setID of the set is worn
  --   + and the property list at key 2 is active when the third setID of the set is worn
  --   + etc.
  --  - If the item's setID is the second of the set:
  --   + then the property list at key 1 is active when the first setID of the set is worn
  --   + and the property list at key 2 is active when the third setID of the set is worn
  --   + etc.
  --  - etc.
}
```