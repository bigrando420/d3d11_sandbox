#include "third_party/metadesk/md.h"
#include "third_party/metadesk/md.c"
#include "third_party/meow_hash_x64_aesni.h"

#include "webgen.h"

////////////////////////////////
//~ rjf: Globals

static MD_Arena *wg_arena = 0;
static MD_String8 wg_g_top_level_keys_table[] =
{
    MD_S8LitComp("title"),
    MD_S8LitComp("author"),
    MD_S8LitComp("date"),
    MD_S8LitComp("publish"),
    MD_S8LitComp("no_toc"),
    MD_S8LitComp("main"),
};

////////////////////////////////
//~ rjf: Assets

static WG_AssetHash
WG_AssetHashFromData(MD_String8 data)
{
    meow_u128 hash = MeowHash(MeowDefaultSeed, (meow_umm)data.size, (void *)data.str);
    WG_AssetHash result = {0};
    MemoryCopy(result.u64, &hash, sizeof(hash));
    return result;
}

static WG_AssetNode *
WG_AssetNodeFromPath(MD_String8 string)
{
    
}

static WG_AssetHash
WG_AssetHashFromPath(MD_String8 path)
{
    
}

////////////////////////////////
//~ rjf: Parsing

static WG_SiteInfo
WG_SiteInfoFromNode(MD_Node *node)
{
    WG_SiteInfo result = {0};
    result.title         = MD_ChildFromString(node, MD_S8Lit("title"), MD_StringMatchFlag_CaseInsensitive)->first_child->string;
    result.desc          = MD_ChildFromString(node, MD_S8Lit("desc"), MD_StringMatchFlag_CaseInsensitive)->first_child->string;
    result.canonical_url = MD_ChildFromString(node, MD_S8Lit("canonical_url"), MD_StringMatchFlag_CaseInsensitive)->first_child->string;
    result.link_dictionary = MD_MapMake(wg_arena);
    {
        MD_Node *link_dict = MD_ChildFromString(node, MD_S8Lit("link_dictionary"), MD_StringMatchFlag_CaseInsensitive);
        for(MD_EachNode(child, link_dict->first_child))
        {
            MD_Node *text = MD_ChildFromIndex(child, 0);
            MD_Node *link = MD_ChildFromIndex(child, 1);
            MD_MapInsert(wg_arena, &result.link_dictionary, MD_MapKeyStr(text->string), link);
        }
    }
    result.header = MD_ChildFromString(node, MD_S8Lit("header"), MD_StringMatchFlag_CaseInsensitive)->first_child->string;
    result.footer = MD_ChildFromString(node, MD_S8Lit("footer"), MD_StringMatchFlag_CaseInsensitive)->first_child->string;
    result.style  = MD_ChildFromString(node, MD_S8Lit("style"), MD_StringMatchFlag_CaseInsensitive)->first_child->string;
    return result;
}

static WG_Page *
WG_PageFromNode(MD_Node *node)
{
    //- rjf: allocate page
    WG_Page *page = MD_PushArrayZero(wg_arena, WG_Page, 1);
    
    //- rjf: fill basic key values
    page->name = MD_PathSkipLastSlash(MD_PathChopLastPeriod(node->string));
    page->title = MD_ChildFromString(node, MD_S8Lit("title"), MD_StringMatchFlag_CaseInsensitive)->first_child->string;
    page->author = MD_ChildFromString(node, MD_S8Lit("author"), MD_StringMatchFlag_CaseInsensitive)->first_child->string;
    
    //- rjf: parse out date
    MD_Node *date = MD_ChildFromString(node, MD_S8Lit("date"), MD_StringMatchFlag_CaseInsensitive);
    {
        MD_Node *year_node = MD_NilNode();
        MD_Node *month_node = MD_NilNode();
        MD_Node *day_node = MD_NilNode();
        for(MD_EachNode(child, date->first_child))
        {
            if(child->flags & MD_NodeFlag_Numeric)
            {
                if(MD_NodeIsNil(year_node))
                {
                    year_node = child;
                }
                else if(MD_NodeIsNil(month_node))
                {
                    month_node = child;
                }
                else if(MD_NodeIsNil(day_node))
                {
                    day_node = child;
                    break;
                }
            }
        }
        page->year = MD_U64FromString(year_node->string, 10);
        page->month = MD_U64FromString(month_node->string, 10);
        page->day = MD_U64FromString(day_node->string, 10);
    }
    
    //- rjf: build list of content nodes
    MD_Node *explicit_content_layout = MD_ChildFromString(node, MD_S8Lit("main"), MD_StringMatchFlag_CaseInsensitive);
    if(!MD_NodeIsNil(explicit_content_layout))
    {
        page->content_list = explicit_content_layout;
    }
    else
    {
        page->content_list = MD_MakeList(wg_arena);
        for(MD_EachNode(child, node->first_child))
        {
            // rjf: determine  if this node looks like a key-value pair.
            MD_b32 key_value_pair = 0;
            for(MD_u64 idx = 0; idx < MD_ArrayCount(wg_g_top_level_keys_table); idx += 1)
            {
                if(MD_S8Match(wg_g_top_level_keys_table[idx], child->string, MD_StringMatchFlag_CaseInsensitive))
                {
                    key_value_pair = 1;
                    break;
                }
            }
            
            // rjf: skip if this is a key-value pair.
            if(key_value_pair != 0)
            {
                continue;
            }
            
            // rjf: push reference to this content node otherwise.
            MD_PushNewReference(wg_arena, page->content_list, child);
        }
    }
    
    return page;
}

static void
WG_PageListPush(WG_PageList *list, WG_Page *page)
{
    MD_QueuePush(list->first, list->last, page);
    list->count += 1;
}

////////////////////////////////
//~ rjf: Generation

static void
WG_PushTextContentString_HTML(MD_String8 string, MD_String8List *out)
{
    MD_String8List strs = {0};
    MD_S8ListPush(wg_arena, &strs, string);
    MD_String8 strs_joined = MD_S8ListJoin(wg_arena, strs, 0);
    MD_S8ListPush(wg_arena, out, strs_joined);
}

static void
WG_PushNodeStrings_HTML(MD_Node *node, MD_String8List *out)
{
    //- rjf: determine the content kind.
    WG_ContentKind kind = WG_ContentKind_Text;
    if(MD_NodeHasTag(node, MD_S8Lit("title"), MD_StringMatchFlag_CaseInsensitive))
    {
        kind = WG_ContentKind_Title;
    }
    else if(MD_NodeHasTag(node, MD_S8Lit("subtitle"), MD_StringMatchFlag_CaseInsensitive))
    {
        kind = WG_ContentKind_Subtitle;
    }
    else if(MD_NodeHasTag(node, MD_S8Lit("code"), MD_StringMatchFlag_CaseInsensitive))
    {
        kind = WG_ContentKind_Code;
    }
    else if(MD_NodeHasTag(node, MD_S8Lit("link"), MD_StringMatchFlag_CaseInsensitive))
    {
        kind = WG_ContentKind_Link;
    }
    
    //- rjf: convert content kind => html info
    MD_String8 html_tag_str = MD_S8Lit("p");
    MD_String8 html_class_str = MD_S8Lit("paragraph");
    MD_String8 html_href_str = MD_S8Lit("");
    switch(kind)
    {
        default:break;
        case WG_ContentKind_Text:
        {
            html_tag_str = MD_S8Lit("p");
            html_class_str = MD_S8Lit("paragraph");
        }break;
        case WG_ContentKind_Title:
        {
            html_tag_str = MD_S8Lit("h1");
            html_class_str = MD_S8Lit("title");
        }break;
        case WG_ContentKind_Subtitle:
        {
            html_tag_str = MD_S8Lit("h2");
            html_class_str = MD_S8Lit("subtitle");
        }break;
        case WG_ContentKind_Code:
        {
            html_tag_str = MD_S8Lit("pre");
            html_class_str = MD_S8Lit("code");
        }break;
    }
    
    //- rjf: generate content
    if(kind != WG_ContentKind_Null)
    {
        MD_S8ListPushFmt(wg_arena, out, "<%S class=\"%S\" %s%S%s>\n",
                         html_tag_str,
                         html_class_str,
                         html_href_str.size != 0 ? "href=\"" : "",
                         html_href_str,
                         html_href_str.size != 0 ? "\"" : "");
        WG_PushTextContentString_HTML(node->string, out);
        MD_S8ListPushFmt(wg_arena, out, "</%S>\n", html_tag_str);
    }
}

static void
WG_PushPageStrings_HTML(WG_Page *page, MD_String8List *out)
{
    for(MD_EachNode(content_ref, page->content_list->first_child))
    {
        MD_Node *node = MD_ResolveNodeFromReference(content_ref);
        WG_PushNodeStrings_HTML(node, out);
    }
}

static MD_String8
WG_HTMLStringFromPage(WG_SiteInfo *site_info, WG_Page *page)
{
    MD_String8List strs = {0};
    
    // rjf: push starter
    MD_S8ListPushFmt(wg_arena, &strs, "<!DOCTYPE html>\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<html lang=\"en\">\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<head>\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<meta charset=\"utf-8\">\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><meta name=\"author\" content=\"%S\">\n",
                     page->author);
    MD_S8ListPushFmt(wg_arena, &strs, "<meta property=\"og:title\" content=\"%S\">\n",
                     page->title);
    MD_S8ListPushFmt(wg_arena, &strs, "<meta name=\"twitter:title\" content=\"%S\">\n",
                     page->title);
    MD_S8ListPushFmt(wg_arena, &strs, "<link rel=\"canonical\" href=\"%S\">\n",
                     site_info->canonical_url);
    MD_S8ListPushFmt(wg_arena, &strs, "<meta property=\"og:type\" content=\"website\">\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<meta property=\"og:url\" content=\"%S\">\n",
                     site_info->canonical_url);
    MD_S8ListPushFmt(wg_arena, &strs, "<meta property=\"og:site_name\" content=\"%S\">\n",
                     site_info->title);
    MD_S8ListPushFmt(wg_arena, &strs, "<meta name=\"twitter:card\" content=\"summary\">\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<meta name=\"twitter:site\" content=\"%S\">\n",
                     site_info->twitter_handle);
    MD_S8ListPushFmt(wg_arena, &strs, "<link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"favicon.ico\">\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<script src=\"site.js\"></script>\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<title>%S | %S</title>\n",
                     page->title, site_info->title);
    MD_S8ListPushFmt(wg_arena, &strs, "</head>\n");
    MD_S8ListPushFmt(wg_arena, &strs, "<body>\n");
    
    // rjf: push header
    MD_S8ListPush(wg_arena, &strs, site_info->header); 
    
    // rjf: push content
    WG_PushPageStrings_HTML(page, &strs);
    
    // rjf: push footer
    MD_S8ListPush(wg_arena, &strs, site_info->footer); 
    
    // rjf: push ender
    MD_S8ListPush(wg_arena, &strs, MD_S8Lit("</body>\n</html>\n"));
    
    MD_String8 result = MD_S8ListJoin(wg_arena, strs, 0);
    return result;
}

////////////////////////////////
//~ rjf: Entry Point

int main(int argc, char **argv)
{
    //- rjf: set up arena
    wg_arena = MD_ArenaAlloc();
    
    //- rjf: extract command line info
    MD_String8List options = MD_StringListFromArgCV(wg_arena, argc, argv);
    MD_CmdLine cmdln = MD_MakeCmdLineFromOptions(wg_arena, options);
    MD_String8List pages_dir_strs = MD_CmdLineValuesFromString(cmdln, MD_S8Lit("pages"));
    MD_String8List assets_dir_strs = MD_CmdLineValuesFromString(cmdln, MD_S8Lit("assets"));
    MD_String8List site_info_strs = MD_CmdLineValuesFromString(cmdln, MD_S8Lit("site_info"));
    MD_String8 pages_dir = MD_S8ListJoin(wg_arena, pages_dir_strs, 0);
    MD_String8 assets_dir = MD_S8ListJoin(wg_arena, assets_dir_strs, 0);
    MD_String8 site_info_path = MD_S8ListJoin(wg_arena, site_info_strs, 0);
    printf("Pages Directory:  %.*s\n", MD_S8VArg(pages_dir));
    printf("Assets Directory: %.*s\n", MD_S8VArg(assets_dir));
    printf("Site Info Path:   %.*s\n", MD_S8VArg(site_info_path));
    
    //- rjf: parse out top-level site info
    MD_Node *site_info_node = MD_ParseWholeFile(wg_arena, site_info_path).node;
    WG_SiteInfo site_info = WG_SiteInfoFromNode(site_info_node);
    
    //- rjf: parse all pages
    WG_PageList pages = {0};
    {
        MD_FileIter it = {0};
        MD_FileIterBegin(&it, pages_dir);
        for(MD_FileInfo info = {0}; (info = MD_FileIterNext(wg_arena, &it)).filename.size != 0;)
        {
            if(!MD_S8Match(info.filename, MD_S8Lit("."), 0) &&
               !MD_S8Match(info.filename, MD_S8Lit(".."), 0))
            {
                MD_String8 path = MD_S8Fmt(wg_arena, "%S/%S", pages_dir, info.filename);
                printf("Parsing page at %.*s...\n", MD_S8VArg(path));
                MD_ParseResult page_parse = MD_ParseWholeFile(wg_arena, path);
                MD_Node *page_node = page_parse.node;
                WG_Page *page = WG_PageFromNode(page_node);
                WG_PageListPush(&pages, page);
            }
        }
        MD_FileIterEnd(&it);
    }
    
    //- rjf: paste stylesheet
    {
        FILE *f = fopen("style.css", "wb");
        fprintf(f, "%.*s", MD_S8VArg(site_info.style));
        fclose(f);
    }
    
    //- rjf: generate HTML pages
    for(WG_Page *page = pages.first; page != 0; page = page->next)
    {
        MD_String8 path = MD_S8Fmt(wg_arena, "%S.html", page->name);
        MD_String8 html = WG_HTMLStringFromPage(&site_info, page);
        FILE *f = fopen((char *)path.str, "wb");
        fprintf(f, "%.*s", MD_S8VArg(html));
        fclose(f);
    }
    
    return 0;
}
