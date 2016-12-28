#!/usr/bin/env bash

# --style=1tbs:
# setBracketFormatMode(LINUX_MODE);
# setAddBracketsMode(true); // brackets added to headers for single line statements
# setRemoveBracketsMode(false); // brackets NOT removed from headers for single line statements
# LINUX_MODE:
# break a namespace if NOT stroustrup or mozilla
# break a class or interface if NOT stroustrup
# break a struct if mozilla - an enum is processed as an array bracket
# break the first bracket if a function
# break the first bracket after these if a function

astyle \
--style=1tbs \
--indent=spaces=2 \
--indent-switches \
--indent-cases \
--indent-namespaces \
--min-conditional-indent=0 \
--max-instatement-indent=40 \
--align-pointer=type \
--align-reference=type \
--break-closing-brackets \
--add-one-line-brackets \
--convert-tabs \
--close-templates \
--unpad-paren \
--pad-header \
--pad-oper \
\
--recursive \
"./src/*.cpp" "./src/*.h"

#--attach-extern-c \
#--attach-namespaces \
#--attach-classes \
#--attach-inlines \
