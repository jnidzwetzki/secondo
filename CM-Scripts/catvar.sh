# Shell script which displays the current 
# environment settings for SECONDO 

echo ""
echo "Environment variables used by SECONDO:"
echo "--------------------------------------"
echo "SECONDO_PLATFORM : " \"$SECONDO_PLATFORM\"
echo "SECONDO_BUILD_DIR: " \"$SECONDO_BUILD_DIR\"
echo "SECONDO_CONFIG   : " \"$SECONDO_CONFIG\"
echo "SECONDO_SDK      : " \"$SECONDO_SDK\"
echo "---"
echo "CVSROOT : " \"$CVSROOT\"
echo "---"
echo "BERKELEY_DB_DIR: " \"$BERKELEY_DB_DIR\"
echo "---"
echo "PATH: " \"$PATH\"
echo "---"
echo "LD_LIBRARY_PATH: " \"$LD_LIBRARY_PATH\"
echo "---"
echo "PL_INCLUDE_DIR: " \"$PL_INCLUDE_DIR\"
echo "PL_LIB_DIR    : " \"$PL_LIB_DIR\"
echo "SWI_HOME_DIR  : " \"$SWI_HOME_DIR\"
echo ""
