#### Crash 2/3 level rebuild & utility tool


##### Compile/run

- C
- MinGW/gcc
- pthreads
- Windows (?)

use **make** to compile

##### How to use

See program's **HELP** command (and HELP2), if there are issues or questions DM me at Averso#5633 or create an issue. \
**See github Actions tab for compiled executables (Artifacts).**

### REBUILD / REBUILD_DL (CrashEdit:Re integration)
Since CrashEdit:Re's v0.4.0.2 there is integration for rebuild and rebuild_dl commands, which allows you to run c2export from CrashEdit.
It has UI for creating and editing rebuild argument files, which is easier to use than raw cmd input.
\
CrashEdit:Re: \
https://github.com/airumu/CrashEdit \
Showcase: \
https://youtu.be/BTdX0XOQf4E \


#### REBUILD / REBUILD_DL (standalone / direct)

Arguably the most useful commands in the tool, used to rebuild levels. \
Allows to generate load lists and draw lists given parameters and input files. \
Places entries into chunks and creates a playable NSF & NSD (it is recommended to Patch NSD with CrashEdit) \
The parameter list is quite long, it is recommended to keep it in a separate file to copy-paste into c2export. \
Examples of input files can be found in the **rebuild_examples** folder.

An alternative to copy-pasting is a python script to launch c2export, \
with input being redirected from a file, example of which is **run_rebuild.py** in **rebuild_examples**.

This writeup assumes and recommends that you:
- use either **rebuild** or **rebuild_dl** (**build** command not included)
- want to rebuild load lists
- use chunk merge method 4 or 5 (the rest is worse or deprecated)
- do not use **omit entries**, do not use **backwards loading penalty** unless necessary


**Input files:**
- input NSF (NSD not necessary)
- permalist - text file with list of permaloaded entries (models are inferred from animations)
- dependency list - text file with a list of types/subtypes with their dependent entries
- [optional] coll dependencies - text file with a list of collision types (old format, eg 0005) with dependent entries
- [optional] music dependencies - text file with a list of music entries with entries to load in zones using the music

**Input parameters: (merge method 5)**
- **command** - rebuild or rebuild_dl
- **input NSF** - level to take as input, not necessarily with a valid name
- **output ID** - level ID to save the output as
- **secondary save** - extra folder to output NSF/NSD (optional)
- **remake load lists** - select whether to re-generate load lists (0/1)
- **merge method** - select normal chunk merge method - recommended 4 or 5
- **spawn** - spawn to set as the default (0th and 1st) from a list of spawns generated on 0-0 and 34-4 entities
- **permalist** - path to permalist file
- **subtype dependencies** - path to type/subtype dependency list
- **collision dependencies** - path to collision dependency list (optional)
- **music dependencies** - path to music entry dependency list (optional)
- [REBUILD_DL] **draw distance x dist cap** - maximal x-distance between current camera point and tested entity
- [REBUILD_DL] **draw distance y dist cap** - maximal y-distance between current camera point and tested entity
- [REBUILD_DL] **draw distance xz dist cap** - maximal xz-distance between current camera point and tested entity
- [REBUILD_DL] **draw angle 3D (y-rot/yaw)** - max dist between curr cam angle and angle to tested entity (3D only)
- **SLST distance** - dist between current campoint and other camera up to which neighbouring SLSTS are loaded
- **neighbour dist** - dist between current campoint and other zone up to which the zone's header deps are loaded
- **draw list dist** - dist between current and other campoint up to which dependencies of drawn objects are loaded
- **transition pre-loading** - select what to load on transitions (none, textures, normal entries, all)
- **backwards loading penalty** - penalty for loading things in the 'backwards' direction (0 recommended)
- **omit unused entries** - omit normal chunk entries that are never loaded (not recommended)
- **max payload limit** - maximum allowed normal chunk payload, stops if all zones fulfill it
- **iteration count** - maximum amount of attepts to merge chunks
- **randomness multiplier** - randomness factor during merging (1.5-3.0 recommended)
- **seed** - seed for chunk merging (0 for random)
- **thread count** - number of threads created for chunk merging

**Outputs:**
- NSF and NSD with the specified output ID (in the same folder as input NSF)
- [optional] NSF and NSD with specified ID in secondary output folder
  
**Special properties and attributes**
For load list generation, it is possible to specify extra entries to load in that zone.
It is in zone header (item 0), +0x1DC is special load list entry count, following data is special entries' EIDs.
CrashEdit:Re fork has UI support for them

**For draw list generation, for each entity it is possible to specify 2 extra properties:**
- draw list position override ID
  - instead of using current entity's position, use position of another entity (specify its ID)
  - the entity must be from the same zone as the replacement/override entity
  - uses property 0x3FE, CrashEdit:Re fork lets you edit them in UI
- draw list distance multiplier
  - instead of using the exact value of current distance, use a percentage of it
  - default is 100, can be >100
  - uses property 0x3FF, CrashEdit:Re fork lets you edit them in UI
  
### Other more useful commands

#### LL_ANALYZE
- runs a series of checks on the NSF, including draw list integrity, entry and entity usage, payload info and others

#### PAYLOAD_INFO
- prints a more detailed rundown of what is being loaded in all cameras of the level (chunks)

#### EID
- conversion from string version of an eid to game's hex representation

... for full list see in-program's HELP / HELP2 commands
