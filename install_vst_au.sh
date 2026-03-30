
SYSTEM_PLUGINS_DIR="/Library/Audio/Plug-Ins"

echo 'DELETING "/Library/Audio/Plug-Ins/VST3/Periodic Loops.vst3"' 
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Periodic Loops.vst3"
echo 'DELETING "/Library/Audio/Plug-Ins/VST3/Periodic Loops.component"' 
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Periodic Loops.component"

echo "Installing VST3"
find ./build/src/PeriodicLoops_artefacts -name "*.vst3" | xargs -I {} sudo cp -r {} ${SYSTEM_PLUGINS_DIR}/VST3/
echo "Installing AU"
find ./build/src/PeriodicLoops_artefacts -name "*.component" | xargs -I {} sudo cp -r {} ${SYSTEM_PLUGINS_DIR}/Components/
