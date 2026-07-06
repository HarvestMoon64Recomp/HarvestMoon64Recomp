#ifndef __UI_FUNCS_INTERNAL_H__
#define __UI_FUNCS_INTERNAL_H__


#ifndef __cplusplus
#ifndef bool
#define RECOMPUI_TEMP_BOOL_MACRO
#define bool unsigned char
#endif
#endif

#include "patch_helpers.h"
#include "recompui_event_structs.h"

DECLARE_FUNC(void, recomp_run_ui_callbacks);

#endif
