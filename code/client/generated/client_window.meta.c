String8 cw_g_cmd_kind_string_table[14] =
{
Str8LitComp(""),
Str8LitComp("window"),
Str8LitComp("new_panel_right"),
Str8LitComp("new_panel_down"),
Str8LitComp("next_panel"),
Str8LitComp("prev_panel"),
Str8LitComp("close_panel"),
Str8LitComp("next_view"),
Str8LitComp("prev_view"),
Str8LitComp("close_view"),
Str8LitComp("edit_view"),
Str8LitComp("open_project"),
Str8LitComp("close_project"),
Str8LitComp("system_cmd"),
};

String8 cw_g_cmd_kind_desc_table[14] =
{
Str8LitComp(""),
Str8LitComp("Opens a new window."),
Str8LitComp("Makes a new panel to the right of the active panel."),
Str8LitComp("Makes a new panel at the bottom of the active panel."),
Str8LitComp("Cycles the active panel forward."),
Str8LitComp("Cycles the active panel backwards."),
Str8LitComp("Closes the currently active panel."),
Str8LitComp("Cycles the active view forward."),
Str8LitComp("Cycles the active view backwards."),
Str8LitComp("Closes the currently active view."),
Str8LitComp("Changes the currently active view's command."),
Str8LitComp("Opens a project file and sets it as the active project."),
Str8LitComp("Closes the active project, if there is one."),
Str8LitComp("Runs a given system command."),
};

CW_CmdFastPath cw_g_cmd_kind_fast_path_table[14] =
{
CW_CmdFastPath_Null,
CW_CmdFastPath_Run,
CW_CmdFastPath_Run,
CW_CmdFastPath_Run,
CW_CmdFastPath_Run,
CW_CmdFastPath_Run,
CW_CmdFastPath_Run,
CW_CmdFastPath_Run,
CW_CmdFastPath_Run,
CW_CmdFastPath_Run,
CW_CmdFastPath_PutNameViewCommand,
CW_CmdFastPath_PutNamePath,
CW_CmdFastPath_Run,
CW_CmdFastPath_PutNamePath,
};

CW_ViewKind cw_g_cmd_kind_view_kind_table[14] =
{
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_Null,
CW_ViewKind_FileSystem,
CW_ViewKind_Null,
CW_ViewKind_FileSystem,
};

B8 cw_g_cmd_kind_keep_input_table[14] =
{
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
};

B8 cw_g_view_kind_text_focus_table[2] =
{
0,
0,
};

CW_ViewFunction* cw_g_view_kind_function_table[2] =
{
CW_VIEW_FUNCTION_NAME(Null),
CW_VIEW_FUNCTION_NAME(FileSystem),
};

