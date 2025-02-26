#### Crash 2/3 export/import tool with the ability to change format of some entries to the other game's format (WIP)


##### Compile/run

- C
- MinGW/gcc
- pthreads
- Windows (?)

use **make** to compile


##### How to use

See program's **help** command, if there are issues or questions DM me at Averso#5633 or create an issue.


#### REBUILD / REBUILD_DL

Arguably the most useful commands in the tool, used to rebuild levels. \
Allows to generate load lists and draw lists given parameters and input files. \
Places entries into chunks and creates a playable NSF & NSD (although it is recommended to Patch NSD with CrashEdit) \
The parameter list is quite long, it is recommended to keep it in a separate file to copy-paste into c2export. \
Examples of input files can be found in the **rebuild_examples** folder.

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
- [REBUILD_DL] **draw distance 2D horizontal** - maximal x-distance between current camera point and tested entity
- [REBUILD_DL] **draw distance 2D vertical** - maximal y-distance between current camera point and tested entity
- [REBUILD_DL] **draw distance 3D (xz)** - maximal xz-distance between current camera point and tested entity
- [REBUILD_DL] **draw angle 3D (y-rot/yaw)** - max distance between current campoint's angle and angle to tested entity
- **SLST distance** - distance between current campoint and another camera up to which neighbouring SLSTS are loaded
- **neighbour dist** - distance between current campoint and another zone up to which neighbouring zones are loaded
- **draw list dist** - distance between current and another campoint up to which dependencies of drawn objects are loaded
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
  
