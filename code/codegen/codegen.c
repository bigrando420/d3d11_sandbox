#include <inttypes.h>

#include "third_party/metadesk/md.h"
#include "codegen.h"
#include "third_party/metadesk/md.c"

#include "codegen_table.h"

#include "codegen_enum.c"
#include "codegen_embed.c"
#include "codegen_table.c"

////////////////////////////////
//~ rjf: Helpers

static CG_FilePair
CG_FilePairFromNode(MD_Node *node)
{
    CG_FilePair result = {0};
    MD_CodeLoc loc = MD_CodeLocFromNode(node);
    MD_String8 filename = loc.filename;
    MD_b32 found = 0;
    for(int i = 0; i < cg_file_pair_count; i += 1)
    {
        if(MD_S8Match(filename, cg_file_pairs[i].src_filename, 0))
        {
            result = cg_file_pairs[i];
            found = 1;
            break;
        }
    }
    if(found == 0)
    {
        MD_String8 folder = MD_PathChopLastSlash(filename);
        MD_String8 layer_name = MD_PathChopLastPeriod(MD_PathSkipLastSlash(loc.filename));
        MD_String8 gen_folder = MD_S8Fmt(cg_arena, "%.*s/generated", MD_S8VArg(folder));
        MD_String8 h_filename = MD_S8Fmt(cg_arena, "%.*s/%.*s.meta.h", MD_S8VArg(gen_folder), MD_S8VArg(layer_name));
        MD_String8 c_filename = MD_S8Fmt(cg_arena, "%.*s/%.*s.meta.c", MD_S8VArg(gen_folder), MD_S8VArg(layer_name));
        result.src_filename = filename;
        result.h = fopen((char *)h_filename.str, "w");
        result.c = fopen((char *)c_filename.str, "w");
        cg_file_pairs[cg_file_pair_count] = result;
        cg_file_pair_count += 1;
    }
    return result;
}

static void
CG_CloseAllFiles(void)
{
    for(int i = 0; i < cg_file_pair_count; i += 1)
    {
        fclose(cg_file_pairs[i].h);
        fclose(cg_file_pairs[i].c);
    }
}

static void
CG_GenerateMultilineStringAsCLiteral(FILE *file, MD_String8 string)
{
    fprintf(file, "\"\"\n\"");
    for(MD_u64 i = 0; i < string.size; i += 1)
    {
        if(string.str[i] == '\n')
        {
            fprintf(file, "\\n\"\n\"");
        }
        else if(string.str[i] == '\r')
        {
            continue;
        }
        else
        {
            fprintf(file, "%c", string.str[i]);
        }
    }
    fprintf(file, "\"\n");
}

static MD_String8
CG_EscapedFromString(MD_Arena *arena, MD_String8 string)
{
    MD_ArenaTemp scratch = MD_GetScratch(&arena, 1);
    MD_String8List strs = {0};
    MD_b32 escaped = 0;
    MD_u64 start = 0;
    for(MD_u64 idx = 0; idx <= string.size; idx += 1)
    {
        if(idx < string.size && escaped)
        {
            escaped = 0;
            start = idx+1;
            MD_u8 replace_char = 0;
            switch(string.str[idx])
            {
                default: break;
                case 'a':  replace_char = 0x07; break;
                case 'b':  replace_char = 0x08; break;
                case 'e':  replace_char = 0x1b; break;
                case 'f':  replace_char = 0x0c; break;
                case 'n':  replace_char = 0x0a; break;
                case 'r':  replace_char = 0x0d; break;
                case 't':  replace_char = 0x09; break;
                case 'v':  replace_char = 0x0b; break;
                case '\\': replace_char = 0x5c; break;
                case '\'': replace_char = 0x27; break;
                case '\"': replace_char = 0x22; break;
                case '\?': replace_char = 0x3f; break;
            }
            if(replace_char)
            {
                MD_String8 string = MD_S8Copy(scratch.arena, MD_S8(&replace_char, 1));
                MD_S8ListPush(scratch.arena, &strs, string);
            }
        }
        else if(idx == string.size || string.str[idx] == '\\')
        {
            escaped = (string.str[idx] == '\\');
            MD_String8 part = MD_S8Substring(string, start, idx);
            MD_S8ListPush(scratch.arena, &strs, part);
            start = idx;
        }
    }
    MD_String8 result = MD_S8ListJoin(arena, strs, 0);
    MD_ReleaseScratch(scratch);
    return result;
}

////////////////////////////////
//~ rjf: Entry Point

int main(int argument_count, char **arguments)
{
    cg_arena = MD_ArenaAlloc();
    
    //- rjf: parse command line
    MD_String8List options = MD_StringListFromArgCV(cg_arena, argument_count, arguments);
    MD_CmdLine cmdln = MD_MakeCmdLineFromOptions(cg_arena, options);
    
    //- rjf: parse all files
    MD_Node *file_list = MD_MakeList(cg_arena);
    for(MD_String8Node *n = cmdln.inputs.first; n != 0; n = n->next)
    {
        MD_String8 code_dir = n->string;
        printf("searching %.*s for metacode...\n", MD_S8VArg(code_dir));
        MD_FileIter it = {0};
        MD_FileIterBegin(&it, code_dir);
        for(MD_FileInfo info = {0};;)
        {
            info = MD_FileIterNext(cg_arena, &it);
            if(info.filename.size == 0)
            {
                break;
            }
            if(!(info.flags & MD_FileFlag_Directory) &&
               MD_S8Match(MD_PathSkipLastPeriod(info.filename), MD_S8Lit("mdesk"), MD_StringMatchFlag_CaseInsensitive))
            {
                printf("parsing %.*s...\n", MD_S8VArg(info.filename));
                MD_String8 path = MD_S8Fmt(cg_arena, "%.*s/%.*s", MD_S8VArg(code_dir), MD_S8VArg(info.filename));
                MD_ParseResult parse = MD_ParseWholeFile(cg_arena, path);
                MD_PushNewReference(cg_arena, file_list, parse.node);
            }
        }
        MD_FileIterEnd(&it);
    }
    
    //- rjf: send all parses to backends
    // CG_ENUM_Generate(file_list);
    CG_EMBED_Generate(file_list);
    CG_TBL_Generate(file_list);
    CG_CloseAllFiles();
    
    return 0;
}
