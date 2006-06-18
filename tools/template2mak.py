#! /usr/bin/python
r"""
-------------------------------------------------------------------------

  FILE
      tools/template2mak.py

  DESCRIPTION
      Translates a template MAK file into a real MAK file.
      Used for generating VC++ makefiles.

  Copyright (c) 2006, Bart Samwel <bart@samwel.tk> and
                      Jeroen T. Vermeulen <jtv@xs4all.nl>

  See COPYING for copyright license.  If you did not receive a file called
  COPYING with this source code, please notify the distributor of this mistake,
  or contact the author.

-------------------------------------------------------------------------
"""

import sys, glob, os


# Expand a foreach template with the given path, and write the results to
# outfile.
def expand_foreach_template(path,template,outfile):
    basepath = os.path.splitext(os.path.basename(path))[0]
    for line in template:
        line = line.replace("###FILENAME###", path)
        line = line.replace("###BASENAME###", basepath)
        outfile.write(line)



# Expand the given foreach template with the given list of file patterns, and
# write the results to outfile.
def foreach_file_in_patterns_expand_template(filepatterns,template,outfile):
   for pattern in filepatterns:
       files = glob.glob(pattern)
       files.sort()
       for path in files:
           expand_foreach_template(path,template,outfile)



# Transform the template in infile into a makfile, and write the results to
# outfile. The available template commands are:
#
# Expand a template section for each file in a list of file patterns:
#
#   ###MAKTEMPLATE:FOREACH my/path*/*.cxx,other*.cxx
#       ...
#   ###MAKTEMPLATE:ENDFOREACH
#
# In the template section, you can use ###BASENAME### to get the base name of
# the file being processed (e.g. "base" for "../base.cxx"), and you can use
# ###FILENAME### to get the full filename.
def template2mak(infile,outfile):
    lines = infile.readlines()
    foreachcmd = r"###MAKTEMPLATE:FOREACH "
    endforeachcmd = r"###MAKTEMPLATE:ENDFOREACH"
    while lines:
        line = lines[0]
        if line.strip().upper().startswith(foreachcmd):
            filepatterns = line.strip() [ len(foreachcmd): ].split(',')
            lines = lines[1:]
            l = 0
            for line in lines:
                if line.strip().upper().startswith(endforeachcmd):
                    break
                l = l + 1
            template = lines[:l]
            lines = lines[l+1:]
            foreach_file_in_patterns_expand_template(filepatterns,template,outfile)
        else:
            outfile.write(line)
            lines = lines[1:]

if __name__ == '__main__':
    if len(sys.argv) > 3:
        print "Too many arguments."
	sys.exit(1)

    me = os.path.basename(sys.argv[0])
    hr = "#"*80 + "\n"

    input = sys.stdin
    inarg = None
    output = sys.stdout
    outarg = None

    if len(sys.argv) >= 2:
        inarg = os.path.abspath(sys.argv[1])
        input = file(inarg)
	if len(sys.argv) >= 3:
	    outarg = sys.argv[2]
	    output = file(outarg, 'w')
        #os.chdir(os.path.dirname(inarg))

    output.write(hr)
    output.write("""# AUTOMATICALLY GENERATED FILE--DO NOT EDIT
#
# This file is generated automatically by libpqxx's %s script.
#
# If you modify this file, chances are your modifications will be lost because
# the file will be re-written from time to time.
#
# The %s script should be available in the tools directory of the
# libpqxx source archive.
""" % (me, me))

    if len(sys.argv) > 1:
        output.write("#\n")
        output.write("# Generated from template '%s'.\n" % inarg)

    output.write(hr)

    template2mak(input, output)

