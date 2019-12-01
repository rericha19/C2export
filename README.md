#### Crash 2/3 export/import tool with the ability to change format of some entries to the other game's format (WIP)



##### How to run/compile

The code is in C++, as the file extension suggests, and can be compiled as it is with any regular C/C++ compiler.



##### How to use

Once you open it, you will be greeted with a terminal / command line window, with some intro text at the top. To show all the commands, you can type 'help'. It should also provide examples and explanations on the command format. There are 2 or 3 main commands - **'export', 'exportall' **and '**import**'. 

'**export**' and '**exportall**' work almost the same, but when using '**export**' you only export the contents of a single specific .NSF file, while '**exportall**' does this with every .NSF file it finds in the provided folder. Both make you pick what game the files were from and whether you want to change the format of some entries (not all yet). Then the files get exported one by one into CM_to_CN/curr_time/S00000XX, M being the game the files are from and N being the new format of some entries, e.g C2_to_C2, XX being the level ID.

'**import**' imports .nsentry files from a specified folder into a specified .NSF file. It does not overwrite the file, rather it creates a clone of it. For now it does not import sound, instrument and speech entries, as those are present in rather small amounts compared to other entry types, and would require additional restrictions in the algorithm due to the format. The way they are imported is rather crude for now and is not a good way to structure a level, as scenery, zones and SLSTs are not associated nor placed into the same chunks as it should be, rather they are added in alphabetical order, which can lead to situations when the game cannot load all the stuff that's to be loaded due to the entries being thrown around into independent chunks. 



##### Why to use

'**export**' and '**exportall**' provides some changes to the entry formats that would normally have to be done manually, such as neighbour count adjusting, entity coordinate rescaling, model size rescaling and some more depending on which way you want to port (2->3 or 3->2). The entries are exported in an organised way and even if you dont plan to use them all, it can be useful to pull the entries from a folder instead of exporting them manually. 

'**import**' for now it is rather clumsy, but even now it can save a lot of time that would have been spent adding the entries one by one. Upsides are the entire layout and all the normal entries are imported, downsides are that some entries, especially essential T11s and models and animations are imported as well are therefore present twice

If you change print to '**here**' (into the terminal) with "changeprint h", you get a little neat histogram with the entry counts.