// based on https://github.com/trapexit/3do-example-folio
#include "super.h"

static void _SWI_Super(void (*func)(void))
{
    func();
}

static FolioFunc FolioSWIFuncs[] = {
    (FolioFunc)_SWI_Super,
};

static TagArg g_FolioTags[] = {
    { TAG_ITEM_NAME,            (void*)"super" },
    { CREATEFOLIO_TAG_ITEM,     (void*)SUPER_FOLIO },
    { CREATEFOLIO_TAG_NSWIS,    (void*)1 },
    { CREATEFOLIO_TAG_SWIS,     (void*)FolioSWIFuncs },
    { TAG_END,                  (void*)0 },
};

int main(int argc_, char *argv_[])
{
    if (argc_ == DEMANDLOAD_MAIN_CREATE)
        return (int)CreateItem(MKNODEID(KERNELNODE,FOLIONODE), g_FolioTags);
    return 0;
}
