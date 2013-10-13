#!/bin/sh

BIN="sdl"
FILES="bbox bsp cmd console game input main menu mm model obj ogl player render tex tga tx2"

CC="cc"
CFLAGS="-pipe -Wall -march=native"
LDFLAGS="-lm"
BDIR="build"
SDIR="src"
MF="Makefile"
STRIP="strip"
GZEXE="gzexe"

TMPFILE=`mktemp` || exit 1

OBJS=""
for FILE in ${FILES}; do
  OBJS="${OBJS}${BDIR}/${FILE}.o "
done

echo -n "Checking for SDL: "
VER=`sdl-config --version`
if [ -z "${VER}" ]; then exit 1; fi
echo "${VER}"
CFLAGS="${CFLAGS} `sdl-config --cflags`"
LDFLAGS="${LDFLAGS} `sdl-config --libs`"

echo -n "Checking for OpenGL: "
echo "#include <GL/gl.h>
int main() { glEnd(); return 0; }
" > ${TMPFILE}
${CC} -xc -lGL -o /dev/null ${TMPFILE} || exit 1
echo "installed"
LDFLAGS="${LDFLAGS} -lGL"

rm ${TMPFILE}

echo "generating ${MF}"
echo "# generated by configure.sh
CC=${CC}
CFLAGS=${CFLAGS}
LDFLAGS=${LDFLAGS}

all:	${BDIR} ${BDIR}/${BIN}

${BDIR}:
	@echo \"	MKD	${BDIR}\"
	@mkdir \$@

${BDIR}/${BIN}: ${OBJS}
	@echo \"	LN	${BIN}\"
	@\$(CC) \$(LDFLAGS) ${OBJS} -o ${BDIR}/${BIN}
	@strip --strip-all ${BDIR}/${BIN}
#	@gzexe ${BDIR}/${BIN}
	@sync
	@mv ${BDIR}/${BIN} ${BIN}
" > ${MF}

for FILE in ${FILES}; do
  INCLUDES=`grep "^\#include \"*\"" ${SDIR}/${FILE}.c | cut -d\" -f2`
  LINE=""
  for INCLUDE in ${INCLUDES}; do
    LINE="${LINE} ${SDIR}/${INCLUDE}"
  done
  LINE="${BDIR}/${FILE}.o: ${SDIR}/${FILE}.c ${LINE}"
  echo ${LINE} >> ${MF}
  echo "	@echo \"	CC	${SDIR}/${FILE}.c\"" >> ${MF}
  echo "	@\$(CC) \$(CFLAGS) -c ${SDIR}/${FILE}.c -o ${BDIR}/${FILE}.o" >> ${MF}
  echo >> ${MF}
done

echo "clean:
	@echo \"	CLEAN\"
	@rm -rf ${BDIR}" >> ${MF}
