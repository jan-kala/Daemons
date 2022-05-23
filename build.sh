
PREV_PWD=$(pwd)
cd "$(dirname "$0")"

if [ "$USER" == "root" ]; then 
    echo "Don't run this as sudo! you'll be prompted for sudo password if needed."
    exit 1
fi
# =======================================================================================
echo "=== Preparing output folder ==="
if [ -d "./dist/" ] 
then 
    echo "Output folder '/dist' already exists! Please remove it."
    exit 1
fi

mkdir "dist"
mkdir "dist/bin"
mkdir "dist/log"
if [ ! -f "config.json" ]
then
    cp "config.template" "config.json"
fi
cp "config.json" "dist/config.json"

echo "=== Output folderd prepared ==="
echo ""
# =======================================================================================


FOLDERS=("ProtobufMessages" "InterfaceMonitor" "HttpDataResender" "Joiner")

for FOLDER in ${FOLDERS[@]} 
do
    echo "=== Building $FOLDER ==="
    if [ ! -d "$FOLDER/build" ] 
    then
        mkdir "$FOLDER/build"
    fi
    cd "$FOLDER/build" && cmake .. && make
    cd ../..
    echo "=== Done building $FOLDER ==="
    echo ""

done

echo "=== Copying binaries into dist/bin folder ==="
cp "InterfaceMonitor/build/InterfaceMonitor" "./dist/bin/"
cp "HttpDataResender/build/HttpDataReSender" "./dist/bin/"
cp "Joiner/build/Joiner" "./dist/bin/"
echo "=== Done Copying ==="
echo ""

echo "=== Cleanup ==="
cd "$PREV_PWD"

echo "=== Done Building ==="