#include "plugin.hpp"

Plugin *pluginInstance;

void init(Plugin *p)
{
    pluginInstance = p;

    // Add modules here
    // p->addModel(modelMyModule);
    p->addModel(modelMAR);
    p->addModel(modelFONT);
    p->addModel(modelALT);
    p->addModel(modelQUART);
    p->addModel(modelONA);
    p->addModel(modelSERRA); 
    p->addModel(modelCEQ);    
    p->addModel(modelSTMAR); 
    p->addModel(modelPerformanceMixer); 
    p->addModel(modelVCVRANDOM);    
    p->addModel(modelEXP4);    
    p->addModel(modelBLANK12Hp);
    p->addModel(modelBLANK8Hp);
    p->addModel(modelBLANK6Hp);
    p->addModel(modelBLANK4Hp);
    p->addModel(modelBLANK2Hp);

    // Any other plugin initialization may go here.
    // As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
