String8 ed_g_cmd_kind_string_table[50] =
{
Str8LitComp(""),
Str8LitComp("move_right"),
Str8LitComp("move_left"),
Str8LitComp("move_right_select"),
Str8LitComp("move_left_select"),
Str8LitComp("move_right_word"),
Str8LitComp("move_left_word"),
Str8LitComp("move_right_word_select"),
Str8LitComp("move_left_word_select"),
Str8LitComp("move_to_line_start"),
Str8LitComp("move_to_line_end"),
Str8LitComp("move_to_line_start_select"),
Str8LitComp("move_to_line_end_select"),
Str8LitComp("move_to_visual_line_start"),
Str8LitComp("move_to_visual_line_end"),
Str8LitComp("move_to_visual_line_start_select"),
Str8LitComp("move_to_visual_line_end_select"),
Str8LitComp("move_up"),
Str8LitComp("move_down"),
Str8LitComp("move_up_select"),
Str8LitComp("move_down_select"),
Str8LitComp("move_up_visual"),
Str8LitComp("move_down_visual"),
Str8LitComp("move_up_visual_select"),
Str8LitComp("move_down_visual_select"),
Str8LitComp("move_up_to_blank"),
Str8LitComp("move_down_to_blank"),
Str8LitComp("move_up_to_blank_select"),
Str8LitComp("move_down_to_blank_select"),
Str8LitComp("toggle_scope_side"),
Str8LitComp("toggle_scope_side_select"),
Str8LitComp("delete_forward"),
Str8LitComp("delete_backward"),
Str8LitComp("delete_forward_word"),
Str8LitComp("delete_backward_word"),
Str8LitComp("copy"),
Str8LitComp("cut"),
Str8LitComp("paste"),
Str8LitComp("undo"),
Str8LitComp("redo"),
Str8LitComp("toggle_insert_mode"),
Str8LitComp("toggle_multi_cursor_mode"),
Str8LitComp("place_cursor"),
Str8LitComp("clear_extra_cursors"),
Str8LitComp("write_text"),
Str8LitComp("swap_line_up"),
Str8LitComp("swap_line_down"),
Str8LitComp("auto_indent_line"),
Str8LitComp("auto_indent_scope"),
Str8LitComp("auto_indent_selection"),
};

String8 ed_g_cmd_kind_desc_table[50] =
{
Str8LitComp(""),
Str8LitComp("Moves the cursor right by one character."),
Str8LitComp("Moves the cursor left by one character."),
Str8LitComp("Moves the cursor right by one character, while selecting."),
Str8LitComp("Moves the cursor left by one character, while selecting."),
Str8LitComp("Moves the cursor right by one word."),
Str8LitComp("Moves the cursor left by one word."),
Str8LitComp("Moves the cursor right by one word, while selecting."),
Str8LitComp("Moves the cursor left by one word, while selecting."),
Str8LitComp("Moves the cursor to the start of the line."),
Str8LitComp("Moves the cursor to the end of the line."),
Str8LitComp("Moves the cursor to the start of the line, while selecting."),
Str8LitComp("Moves the cursor to the end of the line, while selecting."),
Str8LitComp("Moves the cursor to the start of the visual line."),
Str8LitComp("Moves the cursor to the end of the visual line."),
Str8LitComp("Moves the cursor to the start of the visual line, while selecting."),
Str8LitComp("Moves the cursor to the end of the visual line, while selecting."),
Str8LitComp("Moves the cursor up one line."),
Str8LitComp("Moves the cursor down one line."),
Str8LitComp("Moves the cursor up one line, while selecting."),
Str8LitComp("Moves the cursor down one line, while selecting."),
Str8LitComp("Moves the cursor up one line."),
Str8LitComp("Moves the cursor down one line."),
Str8LitComp("Moves the cursor up one line, while selecting."),
Str8LitComp("Moves the cursor down one line, while selecting."),
Str8LitComp("Moves the cursor up to the previous blank line."),
Str8LitComp("Moves the cursor down to the next blank line."),
Str8LitComp("Moves the cursor up to the previous blank line, while selecting."),
Str8LitComp("Moves the cursor down to the next blank line, while selecting."),
Str8LitComp("Moves the cursor between both sides of the current enclosure."),
Str8LitComp("Moves the cursor between both sides of the current enclosure, while selecting."),
Str8LitComp("Deletes one character forward, or the selected range, if any."),
Str8LitComp("Deletes one character backward, or the selected range, if any."),
Str8LitComp("Deletes one word forward, or the selected range, if any."),
Str8LitComp("Deletes one word backward, or the selected range, if any."),
Str8LitComp("Copies the selected text to the clipboard."),
Str8LitComp("Copies the selected text to the clipboard, then deletes it."),
Str8LitComp("Pastes the contents of the clipboard."),
Str8LitComp("Reverses one change to the current file."),
Str8LitComp("Replays one change to the current file."),
Str8LitComp("Toggles insert mode."),
Str8LitComp("Toggles multi-cursor mode, which applies operations to all cursors."),
Str8LitComp("Places an additional cursor, at the location of the main cursor."),
Str8LitComp("Clears all extra cursors that have been placed."),
Str8LitComp("Writes the text that was used to cause this command."),
Str8LitComp("Swaps the current line with the previous line."),
Str8LitComp("Swaps the current line with the next line."),
Str8LitComp("Auto-indents the current line."),
Str8LitComp("Auto-indents the current scope."),
Str8LitComp("Auto-indents the current selection."),
};

#if ENGINE
String8 ed_g_token_kind_key_table[18] =
{
Str8LitComp(""),
Str8LitComp("keyword"),
Str8LitComp("identifier"),
Str8LitComp("numeric_literal"),
Str8LitComp("string_literal"),
Str8LitComp("char_literal"),
Str8LitComp("operator"),
Str8LitComp("delimiter"),
Str8LitComp("comment"),
Str8LitComp("preprocessor"),
Str8LitComp("scope_open"),
Str8LitComp("scope_close"),
Str8LitComp("paren_open"),
Str8LitComp("paren_close"),
Str8LitComp("block_comment_open"),
Str8LitComp("block_comment_close"),
Str8LitComp("multi_line_string_literal_open"),
Str8LitComp("multi_line_string_literal_close"),
};
#endif // ENGINE

#if ENGINE
B8 ed_g_token_kind_is_range_endpoint_table[18] =
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
1,
1,
1,
1,
1,
1,
1,
1,
};
#endif // ENGINE

#if ENGINE
ED_TokenKind ed_g_token_range_point_kind_start_token_kind_table[5] =
{
ED_TokenKind_Null,
ED_TokenKind_ScopeOpen,
ED_TokenKind_ParenOpen,
ED_TokenKind_BlockCommentOpen,
ED_TokenKind_MultiLineStringLiteralOpen,
};
#endif // ENGINE

#if ENGINE
ED_TokenKind ed_g_token_range_point_kind_end_token_kind_table[5] =
{
ED_TokenKind_Null,
ED_TokenKind_ScopeClose,
ED_TokenKind_ParenClose,
ED_TokenKind_BlockCommentClose,
ED_TokenKind_MultiLineStringLiteralClose,
};
#endif // ENGINE

#if ENGINE
ED_TokenKind ed_g_token_range_point_kind_content_token_kind_table[5] =
{
ED_TokenKind_Null,
ED_TokenKind_Null,
ED_TokenKind_Null,
ED_TokenKind_Comment,
ED_TokenKind_StringLiteral,
};
#endif // ENGINE

#if ENGINE
String8 ed_g_token_range_point_kind_string_table[5] =
{
Str8LitComp(""),
Str8LitComp("scope"),
Str8LitComp("paren"),
Str8LitComp("block_comment"),
Str8LitComp("multi_line_string_literal"),
};
#endif // ENGINE

#if ENGINE
String8 ed_g_line_ending_encoding_string_table[3] =
{
Str8LitComp("bin"),
Str8LitComp("lf"),
Str8LitComp("crlf"),
};
#endif // ENGINE

