#include "File_loader.h"

std::string UTILS::Open_GLTF_Dialog(){
    NFD::UniquePath outPath;

    nfdfilteritem_t filterItem[2] = {
        { "glTF Binary (*.glb)",     "glb" },
        { "glTF Embedded (*.gltf)", "gltf" }
    };

    nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 2, nullptr);

    if (result == NFD_OKAY){
        return std::string(outPath.get());
    }
    return "";
}

std::string UTILS::Open_HDR_Dialog(){

};
