#!/usr/bin/env python
import sys
import os
import subprocess

def main():
    if len(sys.argv) < 2:
        print("Usage: script.py <word>")
        sys.exit(1)
        
    search_word = sys.argv[1].lower()
    start_dir = "."
    target_folder = None

    # Find the first folder whose name contains the search word.
    for root, dirs, files in os.walk(start_dir):
        if search_word in os.path.basename(root).lower():
            target_folder = root
            break

    if not target_folder:
        print(f"No folder containing '{search_word}' found.")
        sys.exit(1)

    # Look for a .txt file that has 'args', 'rebuild' or 'rebuilt' in its name.
    target_file = None
    for filename in os.listdir(target_folder):
        lower_name = filename.lower()
        if lower_name.endswith(".txt") and any(sub in lower_name for sub in ["args", "rebuild", "rebuilt"]):
            target_file = os.path.join(target_folder, filename)
            break

    if not target_file:
        print("No matching text file found in folder:", target_folder)
        sys.exit(1)
    
    # Read the entire file content.
    with open(target_file, "r") as infile:
        file_content = infile.read()
    
    # Append a line 'kill' to the file content.
    if not file_content.endswith("\n"):
        file_content += "\n"
    file_content += "kill\n"

    print(f"Running c2export_dl.exe in folder: {target_folder} with input from: {target_file}")
    # Run c2export_dl.exe, passing file_content as the input.
    result = subprocess.run(["../bin/c2export_dl.exe"], input=file_content, text=True)

    print("c2export_dl.exe finished", target_folder, target_file)

if __name__ == "__main__":
    main()
